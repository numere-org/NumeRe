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





#define CHAMELEON__CPP

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#ifdef _MSC_VER
    #include <crtdbg.h>
#else
    #define _ASSERT(expr) ((void)0)

    #define _ASSERTE(expr) ((void)0)
#endif

#include "NumeReWindow.h"
#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/string.h>
#include <wx/filedlg.h>
#include <wx/statusbr.h>
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wx/msw/wince/tbarwce.h>
#include <wx/msw/private.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/mimetype.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/artprov.h>
#include <fstream>
#include <wx/clipbrd.h>
#include <wx/display.h>
#include <array>


//need to include <wx/filename.h> and <wx/stdpaths.h>
#include <math.h>

#include "NumeReNotebook.h"
#include "numeredroptarget.hpp"
#include "DirTraverser.hpp"
#include "IconManager.h"
#include "wxProportionalSplitterWindow.h"
#include "documentationbrowser.hpp"
#include "graphviewer.hpp"
#include "textsplashscreen.hpp"

#include "compositions/viewerframe.hpp"
#include "compositions/imagepanel.hpp"
#include "compositions/helpviewer.hpp"
#include "compositions/tableviewer.hpp"
#include "compositions/tableeditpanel.hpp"
#include "compositions/wxTermContainer.h"
#include "compositions/debugviewer.hpp"
#include "compositions/customwindow.hpp"

#include "editor/editor.h"
#include "editor/history.hpp"
#include "editor/NumeRePrintout.h"

#include "dialogs/OptionsDialog.h"
#include "dialogs/AboutChameleonDialog.h"
#include "dialogs/textoutputdialog.hpp"
#include "dialogs/packagedialog.hpp"
#include "dialogs/dependencydialog.hpp"
#include "dialogs/revisiondialog.hpp"
#include "dialogs/pluginrepodialog.hpp"
#include "dialogs/newfiledialog.hpp"
#include "dialogs/listeditdialog.hpp"

#include "terminal/terminal.hpp"

#include "../kernel/core/version.h"
#include "../kernel/core/utils/tools.hpp"
#include "../kernel/core/procedure/dependency.hpp"
#include "../kernel/core/datamanagement/database.hpp"
#include "../kernel/core/documentation/docgen.hpp"
#include "../kernel/core/ui/error.hpp"
#include "../kernel/core/io/logger.hpp"
#include "../kernel/core/ui/calltipprovider.hpp"

#include "../common/recycler.hpp"
#include "../common/Options.h"
#include "../common/vcsmanager.hpp"
#include "../common/filerevisions.hpp"
#include "../common/ipc.hpp"

#include "../kernel/core/utils/http.h"
#include "../common/compareFiles.hpp"

#include "controls/treesearchctrl.hpp"
#include "controls/toolbarsearchctrl.hpp"

const std::string sVersion = toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + AutoVersion::STATUS + "\"";

// Forward declaration
std::string removeMaskedStrings(const std::string& sString);
std::string removeQuotationMarks(const std::string&);

std::string prepareStringsForDialog(const std::string& sString)
{
    return removeMaskedStrings(removeQuotationMarks(sString));
}
// Create the stack trace object here

Language _guilang;
FindReplaceDialog* g_findReplace;
double g_pixelScale = 1.0;

//! global print data, to remember settings during the session
wxPrintData* g_printData = (wxPrintData*) nullptr;
wxPageSetupData* g_pageSetupData = (wxPageSetupData*) nullptr;

BEGIN_EVENT_TABLE(NumeReWindow, wxFrame)
    EVT_MENU_RANGE                  (EVENTID_MENU_START, EVENTID_MENU_END-1, NumeReWindow::OnMenuEvent)
    EVT_MENU_RANGE                  (EVENTID_PLUGIN_MENU_START, EVENTID_PLUGIN_MENU_END-1, NumeReWindow::OnPluginMenuEvent)

    EVT_FIND						(-1, NumeReWindow::OnFindEvent)
    EVT_FIND_NEXT					(-1, NumeReWindow::OnFindEvent)
    EVT_FIND_REPLACE				(-1, NumeReWindow::OnFindEvent)
    EVT_FIND_REPLACE_ALL			(-1, NumeReWindow::OnFindEvent)
    EVT_FIND_CLOSE					(-1, NumeReWindow::OnFindEvent)

    EVT_CLOSE						(NumeReWindow::OnClose)

    //EVT_NOTEBOOK_PAGE_CHANGED		(ID_NOTEBOOK_ED, NumeReWindow::OnPageChange)
    EVT_AUINOTEBOOK_PAGE_CHANGED	(ID_NOTEBOOK_ED, NumeReWindow::OnPageChange)

    EVT_SPLITTER_DCLICK				(ID_SPLITPROJECTEDITOR, NumeReWindow::OnSplitterDoubleClick)
    EVT_SPLITTER_DCLICK				(ID_SPLITEDITOROUTPUT, NumeReWindow::OnSplitterDoubleClick)

    EVT_TREE_ITEM_RIGHT_CLICK		(ID_PROJECTTREE, NumeReWindow::OnTreeItemRightClick)
    EVT_TREE_ITEM_RIGHT_CLICK		(ID_FUNCTIONTREE, NumeReWindow::OnTreeItemRightClick)
    EVT_TREE_ITEM_ACTIVATED			(ID_PROJECTTREE, NumeReWindow::OnTreeItemActivated)
    EVT_TREE_ITEM_ACTIVATED			(ID_FUNCTIONTREE, NumeReWindow::OnTreeItemActivated)
    EVT_TREE_ITEM_GETTOOLTIP        (ID_PROJECTTREE, NumeReWindow::OnTreeItemToolTip)
    EVT_TREE_ITEM_GETTOOLTIP        (ID_FUNCTIONTREE, NumeReWindow::OnTreeItemToolTip)
    EVT_TREE_BEGIN_DRAG             (ID_PROJECTTREE, NumeReWindow::OnTreeDragDrop)
    EVT_TREE_BEGIN_DRAG             (ID_FUNCTIONTREE, NumeReWindow::OnTreeDragDrop)

    EVT_IDLE						(NumeReWindow::OnIdle)
    EVT_TIMER						(ID_STATUSTIMER, NumeReWindow::OnStatusTimer)
    EVT_TIMER						(ID_FILEEVENTTIMER, NumeReWindow::OnFileEventTimer)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)


/////////////////////////////////////////////////
/// \brief Static helper function to determine
/// the window size and position if necessary.
///
/// \param sWinSize std::string&
/// \return wxRect
///
/////////////////////////////////////////////////
static wxRect determineWindowSize(std::string& sWinSize)
{
    wxRect winRect;

    if (sWinSize == "{}")
        return winRect;

    EndlessVector<StringView> vArgs = getAllArguments(StringView(sWinSize, 1, sWinSize.length()-2));

    if (vArgs.size() < 4)
        sWinSize = "{}";
    else
    {
        winRect.x = StrToInt(vArgs[0].subview(2).to_string());
        winRect.y = StrToInt(vArgs[1].subview(2).to_string());
        winRect.width = StrToInt(vArgs[2].subview(2).to_string());
        winRect.height = StrToInt(vArgs[3].subview(2).to_string());

        int currDisplay = wxNOT_FOUND;
        wxRect dispdim;

        // Find the correct display
        for (size_t i = 0; i < wxDisplay::GetCount(); i++)
        {
            dispdim = wxDisplay(i).GetGeometry();

            if (dispdim.Contains(winRect.x+10, winRect.y+10))
            {
                currDisplay = i;
                break;
            }
        }

        g_logger.info("User-selected display: " + toString(currDisplay) + " @ {x=" + toString(winRect.x) + " y=" + toString(winRect.y)+"}");

        if (currDisplay == wxNOT_FOUND)
        {
            sWinSize = "{}";
            return winRect;
        }

        winRect.width = std::min(dispdim.width-winRect.x+dispdim.x, winRect.width);
        winRect.height = std::min(dispdim.height-winRect.y+dispdim.y, winRect.height);

        g_logger.info("Final window dimensions: w=" + toString(winRect.width) + " h=" + toString(winRect.height));

        if (winRect.width < 200 || winRect.height < 200)
            sWinSize = "{}";
    }

    return winRect;
}


//----------------------------------------------------------------------
/////////////////////////////////////////////////
/// \brief "Main program" equivalent: the program
/// execution "starts" here. If we detect an
/// already instance of NumeRe, we will send the
/// command line contents to the existing
/// instance and cancel the start up here.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MyApp::OnInit()
{
    #ifdef DO_LOG
    g_logger.setLoggingLevel(Logger::LVL_DEBUG);
    #endif // DO_LOG

    g_logger.push_info("NEW INSTANCE STARTUP");
    g_logger.push_info("NumeRe v " + sVersion + " (var. " + AutoVersion::UBUNTU_VERSION_STYLE + ")");
    g_logger.write_system_information();
    g_logger.info("Starting up.");

    wxString sInstanceLocation = wxStandardPaths::Get().GetDataDir();
    sInstanceLocation.Replace(":\\", "~");
    sInstanceLocation.Replace("\\", "~");

    g_logger.debug("Creating single instance checker.");
    m_singlinst = new wxSingleInstanceChecker("NumeRe::" + sInstanceLocation + "::" + wxGetUserId(), wxGetHomeDir());
    m_DDEServer = nullptr;

    // Avoid starting up a second instance
    if (m_singlinst->IsAnotherRunning())
    {
        // Create a new client
        wxLogNull ln; // own error checking implemented -> avoid debug warnings
        DDE::Client* client = new DDE::Client;
        DDE::Connection* connection = (DDE::Connection*)client->MakeConnection("localhost", DDE_SERVICE, DDE_TOPIC);

        if (connection)
        {
            // don't eval here just forward the whole command line to the other instance
            wxString cmdLine;

            for (int i = 1 ; i < argc; ++i)
                cmdLine += wxString(argv[i]) + ' ';

            if (!cmdLine.IsEmpty())
            {
                // escape openings and closings so it is easily possible to find the end on the rx side
                cmdLine.Replace(_T("("), _T("\\("));
                cmdLine.Replace(_T(")"), _T("\\)"));
                connection->Execute(_T("[CmdLine({") + cmdLine + _T("})]"));
            }

            connection->Disconnect();
            delete connection;
        }

        // free memory DDE-/IPC-clients
        delete client;
        delete m_singlinst;
        return false;
    }


    std::setlocale(LC_ALL, "C");
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());

    wxInitAllImageHandlers();
    wxBitmap splashImage;
    wxSplashScreen* splash = nullptr;
    g_logger.debug("Loading splash image.");

    if (splashImage.LoadFile(f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR)+"icons\\splash.png", wxBITMAP_TYPE_PNG))
        splash = new wxSplashScreen(splashImage, wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT, 0, nullptr, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);

    g_findReplace = nullptr;

    g_logger.info("Creating graphical user interface.");

    // Create and initialize the main frame. Will also
    // include loading the configuration, loading existing
    // caches and preparing the editor.
    NumeReWindow* NumeReMainFrame = new NumeReWindow("NumeRe (v" + sVersion + ")", wxDefaultPosition, wxDefaultSize);

    g_logger.debug("Starting DDE server instance.");
    // Create the DDE server for the first (main)
    // instance of the application
    m_DDEServer = new DDE::Server(NumeReMainFrame);
    m_DDEServer->Create(DDE_SERVICE);

    g_logger.debug("Determining window size.");

    if (NumeReMainFrame->getOptions()->getSetting(SETTING_B_SAVEWINDOWSIZE).active())
    {
        std::string sWinSize = NumeReMainFrame->getOptions()->getSetting(SETTING_S_WINDOWSIZE).stringval();
        g_logger.info("User-selected window size: " + sWinSize);
        wxRect winRect = determineWindowSize(sWinSize);

        if (sWinSize != "{}")
            NumeReMainFrame->SetSize(winRect);
        else
            NumeReMainFrame->Maximize();
    }
    else
        NumeReMainFrame->Maximize();

    g_logger.debug("Showing main frame.");
    NumeReMainFrame->Show(true);

    // Passed commandline items
    wxArrayString wxArgV;
    for (int i = 0; i < argc; i++)
    {
        wxArgV.Add(argv[i]);
    }

    // force the history window to perform a
    // page down to scroll to its actually
    // last position
    NumeReMainFrame->forceHistoryPageDown();
    NumeReMainFrame->EvaluateCommandLine(wxArgV);
    NumeReMainFrame->Ready();
    g_logger.info("NumeRe ready.");

    // Destroy the splash image once the window
    // is ready
    if (splash)
        splash->Destroy();

    // Tip of the day
    if (NumeReMainFrame->showTipAtStartup)
    {
        g_logger.debug("Showing tip of the day.");
        wxArrayString DialogText;
        DialogText.Add(_guilang.get("GUI_TIPOTD_HEAD"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_DYK"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_NEXT"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_STAS"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_CLOSE"));
        NumeReMainFrame->updateTipAtStartupSetting(ShowTip(NumeReMainFrame, NumeReMainFrame->tipProvider, DialogText));
    }

    delete NumeReMainFrame->tipProvider;
    NumeReMainFrame->tipProvider = nullptr;

    if (NumeReMainFrame->m_UnrecoverableFiles.length())
    {
        g_logger.warning("The files '" + NumeReMainFrame->m_UnrecoverableFiles + "' could not be restored.");
        NumeReMainFrame->Refresh();
        NumeReMainFrame->Update();
        wxMessageBox(_guilang.get("GUI_DLG_SESSION_RECREATIONERROR", NumeReMainFrame->m_UnrecoverableFiles), _guilang.get("GUI_DLG_SESSION_ERROR"), wxICON_ERROR);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Empty destructor.
/////////////////////////////////////////////////
MyApp::~MyApp()
{
}


/////////////////////////////////////////////////
/// \brief Called on application shut down. Will
/// free the memory of the IPC class instances.
///
/// \return int
///
/////////////////////////////////////////////////
int MyApp::OnExit()
{
    if (m_singlinst)
        delete m_singlinst;

    if (m_DDEServer)
        delete m_DDEServer;

    return 0;
}


/////////////////////////////////////////////////
/// \brief This handler should be called, if an
/// unhandled exception propagated through the
/// event loop. We'll try to recover from this
/// issue here. However, it might not be possible.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MyApp::OnExceptionInMainLoop()
{
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        g_logger.error(std::string("Unexpected exception: ") + e.what());
        wxMessageBox(std::string("An unexpected exception was caught. If it is reproducable, consider informing us about this issue. Message: ") + e.what(), "Exception caught");
    }
    catch (SyntaxError& e)
    {
        g_logger.error("Unexpected exception: " + toString((int)e.errorcode) + ", message: " + _guilang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3])) + ", expression: " + e.getExpr() + ", token: " + e.getToken());
        wxMessageBox("An unexpected exception was caught. If it is reproducable, consider informing us about this issue. Code: " + toString((int)e.errorcode) + ", message: " + _guilang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3])) + ", expression: " + e.getExpr() + ", token: " + e.getToken(), "Exception caught");
    }
    catch (mu::Parser::exception_type& e)
    {
        g_logger.error("Unexpected exception: " + e.GetMsg());
        wxMessageBox("An unexpected exception was caught. If it is reproducable, consider informing us about this issue. Message: " + e.GetMsg(), "Exception caught");
    }
    catch ( ... )
    {
        throw;
    }

    return true;
}

//----------------------------------------------------------------------




//////////////////////////////////////////////////////////////////////////////
///  public constructor NumeReWindow
///  Responsible for instantiating pretty much everything.  It's all initialized here.
///
///  @param  title const wxString & The main window title
///  @param  pos   const wxPoint &  Where on the screen to create the window
///  @param  size  const wxSize &   How big the window should be
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
NumeReWindow::NumeReWindow(const wxString& title, const wxPoint& pos, const wxSize& size)
       : wxFrame((wxFrame *)nullptr, -1, title, pos, size)
{
    // should be approximately 80x15 for the terminal
    SetMinSize(wxSize(1280, 768));

    m_debugViewer = nullptr;
    m_currentView = nullptr;
    m_statusBar = nullptr;

    m_copiedTreeItem = 0;
    m_multiRowState = false;
    m_sessionSaved = false;
    m_UnrecoverableFiles = "";
    m_loadingFilesDuringStartup = false;
    m_appStarting = true;

    g_logger.debug("Creating new file watcher.");
    m_watcher = new Filewatcher();
    m_watcher->SetOwner(this);

    g_logger.debug("Creating icon manager.");
    m_iconManager = new IconManager(getProgramFolder());

    // Show a log window for all debug messages
#ifdef _DEBUG
    logWindow = new wxLogWindow(this, "Debug messages");
    wxLog::SetActiveTarget(logWindow);
#endif

    wxString programPath = getProgramFolder();

    SetIcon(getStandardIcon());

    m_remoteMode = true;

    g_logger.debug("Creating window elements.");
    // Create the main frame splitters
    m_splitProjectEditor = new wxSplitterWindow(this, ID_SPLITPROJECTEDITOR, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
    m_splitEditorOutput = new ProportionalSplitterWindow(m_splitProjectEditor, ID_SPLITEDITOROUTPUT, 0.75, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH);
    m_splitCommandHistory = new ProportionalSplitterWindow(m_splitEditorOutput, wxID_ANY, 0.75, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH);

    // Create the different notebooks
    m_book = new EditorNotebook(m_splitEditorOutput, ID_NOTEBOOK_ED, m_iconManager);
    m_book->SetTopParent(this);
    m_noteTerm = new ViewerBook(m_splitCommandHistory, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    m_treeBook = new ViewerBook(m_splitProjectEditor, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    // Prepare the application settings
    m_options = new Options();

    // Create container and the contained terminal
    m_termContainer = new wxTermContainer(m_splitCommandHistory, ID_CONTAINER_TERM, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
    g_logger.debug("Creating terminal and starting kernel session.");
    m_terminal = new NumeReTerminal(m_termContainer, ID_TERMINAL, m_options, programPath, wxPoint(0, 0));
    m_terminal->set_mode_flag(GenericTerminal::CURSORINVISIBLE);
    m_termContainer->SetTerminal(m_terminal);
    m_terminal->SetParent(this);

    // Get the character height from the terminal
    m_splitEditorOutput->SetCharHeigth(m_terminal->getTextHeight());
    m_splitCommandHistory->SetCharHeigth(m_terminal->getTextHeight());

    g_logger.info("Initializing options.");
    // Fetch legacy settings, if available
    InitializeProgramOptions();

    _guilang.setTokens("<>="+getProgramFolder().ToStdString()+";");

    g_logger.info("Loading GUI language files.");
    if (m_options->useCustomLangFiles())
        _guilang.loadStrings(true);
    else
        _guilang.loadStrings(false);

    UpdateWindowTitle("");

    // Prepare the options dialog
    m_optionsDialog = new OptionsDialog(this, m_options, ID_OPTIONSDIALOG, _guilang.get("GUI_DLG_OPTIONS"));

    // Create the contents of file and symbols tree
    m_filePanel = new TreePanel(m_treeBook, wxID_ANY);
    m_fileTree = new FileTree(m_filePanel, ID_PROJECTTREE, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_HAS_BUTTONS | wxTR_NO_LINES | wxTR_FULL_ROW_HIGHLIGHT);
    TreeSearchCtrl* fileSearchCtrl = new TreeSearchCtrl(m_filePanel, wxID_ANY, _guilang.get("GUI_SEARCH_FILES"), _guilang.get("GUI_SEARCH_CALLTIP_TREE"), m_fileTree);
    m_filePanel->AddWindows(fileSearchCtrl, m_fileTree);
    m_treeBook->AddPage(m_filePanel, _guilang.get("GUI_FILETREE"));

    m_functionPanel = new TreePanel(m_treeBook, wxID_ANY);
    m_functionTree = new FileTree(m_functionPanel, ID_FUNCTIONTREE, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_HAS_BUTTONS | wxTR_NO_LINES | wxTR_FULL_ROW_HIGHLIGHT | wxTR_HIDE_ROOT);
    TreeSearchCtrl* functionSearchCtrl = new TreeSearchCtrl(m_functionPanel, wxID_ANY, _guilang.get("GUI_SEARCH_SYMBOLS"), _guilang.get("GUI_SEARCH_CALLTIP_TREE"), m_functionTree, true);
    m_functionPanel->AddWindows(functionSearchCtrl, m_functionTree);
    m_treeBook->AddPage(m_functionPanel, _guilang.get("GUI_FUNCTIONTREE"));
    m_treeBook->Hide();
    wxImageList* fileIcons = m_iconManager->GetImageList();

    // Sets the list, but doesn't take control of it away
    m_fileTree->SetImageList(fileIcons);
    m_functionTree->SetImageList(fileIcons);

    int idxFolderOpen = m_iconManager->GetIconIndex("FOLDEROPEN");

    wxTreeItemId rootNode = m_fileTree->AddRoot(_guilang.get("GUI_TREE_WORKSPACE"), m_iconManager->GetIconIndex("WORKPLACE"));
    m_projectFileFolders[0] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_DATAFILES"), idxFolderOpen);
    m_projectFileFolders[1] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_SAVEDFILES"), idxFolderOpen);
    m_projectFileFolders[2] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_SCRIPTS"), idxFolderOpen);
    m_projectFileFolders[3] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_PROCEDURES"), idxFolderOpen);
    m_projectFileFolders[4] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_PLOTS"), idxFolderOpen);

    g_logger.debug("Preparing function tree.");
    prepareFunctionTree();

#if wxUSE_DRAG_AND_DROP
    m_fileTree->SetDropTarget(new NumeReDropTarget(this, m_fileTree, NumeReDropTarget::FILETREE));
    m_terminal->SetDropTarget(new NumeReDropTarget(this, m_terminal, NumeReDropTarget::CONSOLE));
#endif //wxUSE_DRAG_AND_DROP

    m_currentPage = 0;
    m_fileNum = 0;

    m_appClosing = false;
    m_setSelection = false;

    m_statusBar = new NumeReStatusbar(this);
    SetStatusBar(m_statusBar);

    // Redirect the menu help strings to the
    // second status bar field
    SetStatusBarPane(1);

    SendSizeEvent();

    m_updateTimer = new wxTimer(this, ID_STATUSTIMER);
    m_fileEventTimer = new wxTimer(this, ID_FILEEVENTTIMER);

    m_splitEditorOutput->Initialize(m_book);
    m_splitProjectEditor->Initialize(m_splitEditorOutput);
    m_splitEditorOutput->Initialize(m_splitCommandHistory);
    m_splitProjectEditor->Show();
    m_splitEditorOutput->Show();
    m_splitCommandHistory->Show();
    m_book->Show();
    m_history = new NumeReHistory(this, m_options, m_noteTerm, -1, m_terminal->getSyntax(), m_terminal, wxDefaultPosition, wxDefaultSize);
    m_varViewer = new VariableViewer(m_noteTerm, this);
    m_procedureViewer = new ProcedureViewer(m_noteTerm);

    m_noteTerm->AddPage(m_history, _guilang.get("GUI_HISTORY"));
    m_noteTerm->AddPage(m_varViewer, _guilang.get("GUI_VARVIEWER"));
    m_noteTerm->AddPage(m_procedureViewer, _guilang.get("GUI_PROCEDUREVIEWER"));

    wxMilliSleep(250);

    UpdateMenuBar();
    UpdateTerminalNotebook();

    g_logger.info("Evaluating options.");
    EvaluateOptions();

    if (m_options->getSetting(SETTING_B_SAVESASHS).active())
    {
        m_splitProjectEditor->SetSashPosition(m_options->getSetting(SETTING_V_POS_SASH_V).value());
        m_splitCommandHistory->SetSashPositionFloat(m_options->getSetting(SETTING_V_POS_SASH_T).value() / 100.0);
        m_splitEditorOutput->SetSashPositionFloat(m_options->getSetting(SETTING_V_POS_SASH_H).value() / 100.0);
    }

    // Recreate the last session or
    // create a new empty file
    g_logger.info("Preparing session.");
    prepareSession();

    PageHasChanged(m_currentPage);

    // TO BE INVESTIGATED: For some reason we have to
    // set the procedure list twice to function it correctly
    m_procedureViewer->setCurrentEditor(m_book->getCurrentEditor());

    // bind the event after the loading of the files - FIX for Win10 1803
    Connect(wxEVT_FSWATCHER, wxFileSystemWatcherEventHandler(NumeReWindow::OnFileSystemEvent));

    m_filterNSCRFiles = _guilang.get("GUI_FILTER_SCRIPTS") + " (*.nscr)|*.nscr";
    m_filterNPRCFiles = _guilang.get("GUI_FILTER_PROCEDURES") + " (*.nprc)|*.nprc";
    m_filterNLYTFiles = _guilang.get("GUI_FILTER_LAYOUTS") + "(*.nlyt)|*.nlyt";
    m_filterExecutableFiles = _guilang.get("GUI_FILTER_EXECUTABLES") + " (*.nscr, *.nprc)|*.nscr;*.nprc";
    m_filterNumeReFiles = _guilang.get("GUI_FILTER_NUMEREFILES") + " (*.ndat, *.nscr, *.nprc, *.nlyt)|*.ndat;*.nscr;*.nprc;*.nlyt";
    m_filterDataFiles = _guilang.get("GUI_FILTER_DATAFILES");
    m_filterImageFiles = _guilang.get("GUI_FILTER_IMAGEFILES") + " (*.png, *.jpeg, *.eps, *.svg, *.gif, *.tiff)|*.png;*.jpg;*.jpeg;*.eps;*.svg;*.gif;*.tif;*.tiff";
    m_filterTeXSource = _guilang.get("GUI_FILTER_TEXSOURCE") + " (*.tex)|*.tex";
    m_filterNonsource = _guilang.get("GUI_FILTER_NONSOURCE");
    m_filterSupportedFiles = _guilang.get("GUI_FILTER_ALLSUPPORTEDFILES");

    // If this ever gets ported to Linux, we'd probably want to add
    // Linux library extensions here (.a, .so).  The other issue is that
    // the remote file dialog only looks in ~, which might need to be changed.
    m_filterAllFiles = _guilang.get("GUI_FILTER_ALLFILES") + " (*.*)|*.*";

    g_findReplace = nullptr;

    /// Interesting Bug: obviously it is necessary to declare the paper size first
    g_printData = this->setDefaultPrinterSettings();
    g_pageSetupData = new wxPageSetupDialogData(*g_printData);

    m_appStarting = false;

    //Msgbox
    NumeRe::DataBase tipDataBase;

    if (m_options->useCustomLangFiles() && fileExists(m_options->ValidFileName("<>/user/docs/hints.ndb", ".ndb")))
        tipDataBase.addData("<>/user/docs/hints.ndb");
    else
        tipDataBase.addData("<>/docs/hints.ndb");

    tipProvider = new MyTipProvider(tipDataBase.getColumn(0));
    showTipAtStartup = m_options->showHints();
    g_logger.info("Graphical user interface ready.");
}


//////////////////////////////////////////////////////////////////////////////
///  public destructor ~NumeReWindow
///  Responsible for cleaning up almost everything.
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
NumeReWindow::~NumeReWindow()
{
    delete g_printData;
    delete g_pageSetupData;

    if(g_findReplace != nullptr)
    {
        delete g_findReplace;
    }

    if (m_updateTimer)
    {
        delete m_updateTimer;
        m_updateTimer = nullptr;
    }

    if (m_fileEventTimer)
    {
        delete m_fileEventTimer;
        m_fileEventTimer = nullptr;
    }

    if (m_optionsDialog)
        delete m_optionsDialog;

    if (m_options)
        delete m_options;

    if (m_iconManager)
        delete m_iconManager;

    if (m_watcher)
        delete m_watcher;
}


/////////////////////////////////////////////////
/// \brief This function can be used to deactivate
/// the "Tip of the day" functionality directly
/// from the dialog.
///
/// \param bTipAtStartup bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::updateTipAtStartupSetting(bool bTipAtStartup)
{
    m_options->getSetting(SETTING_B_SHOWHINTS).active() = bTipAtStartup;
    m_terminal->setKernelSettings(*m_options);
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// application's root path.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReWindow::getProgramFolder()
{
    wxStandardPaths systemPaths = wxStandardPaths::Get();
    wxString appPath = systemPaths.GetExecutablePath();
    wxFileName fullProgramPath(appPath);
    return fullProgramPath.GetPath();
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper for the
/// corresponding function from the history
/// widget and stores the passed string in the
/// history file.
///
/// \param sCommand const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::AddToHistory(const wxString& sCommand)
{
    m_history->AddToHistory(sCommand);
}


/////////////////////////////////////////////////
/// \brief This member function returns the HTML
/// string containing the documentation for the
/// selected topic/doc id.
///
/// \param docid wxString
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReWindow::GetDocContent(wxString docid)
{
    return m_terminal->getDocumentation(docid.ToStdString());
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// documentation index as a vector.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReWindow::GetDocIndex()
{
    return m_terminal->getDocIndex();
}


/////////////////////////////////////////////////
/// \brief This member function is a simple helper
/// to force that the history displays the last
/// line at start-up.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::forceHistoryPageDown()
{
    m_history->PageDown();
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// standard path definitions as a vector.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReWindow::getPathDefs()
{
    return m_terminal->getPathSettings();
}


/////////////////////////////////////////////////
/// \brief This member function adds the passed
/// file name to the list of files, which shall
/// not be reloaded automatically, if a file is
/// changed from the outside.
///
/// \param sFilename const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::addToReloadBlackList(const wxString& sFilename)
{
    for (size_t i = 0; i < vReloadBlackList.size(); i++)
    {
        if (vReloadBlackList[i] == sFilename)
            return;
    }

    vReloadBlackList.push_back(sFilename);
}


/////////////////////////////////////////////////
/// \brief This member function removes the passed
/// file name from the list of files, which shall
/// not be reloaded automatically.
///
/// \param sFilename const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::removeFromReloadBlackList(const wxString& sFilename)
{
    for (size_t i = 0; i < vReloadBlackList.size(); i++)
    {
        if (vReloadBlackList[i] == sFilename)
        {
            vReloadBlackList.erase(vReloadBlackList.begin()+i);
            return;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns \c true,
/// if the passed file name is currently part of
/// the list of files, which shall not be reloaded
/// automatically.
///
/// \param sFilename wxString
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReWindow::isOnReloadBlackList(wxString sFilename)
{
    sFilename = replacePathSeparator(sFilename.ToStdString());

    for (size_t i = 0; i < vReloadBlackList.size(); i++)
    {
        if (vReloadBlackList[i] == sFilename)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function loads the
/// configuration file available for the graphical
/// user interface.
///
/// \return void
///
/// If no configuration file is available or if
/// the config file is of an older version,
/// default values are used.
/////////////////////////////////////////////////
void NumeReWindow::InitializeProgramOptions()
{
    // Open up the configuration file, assumed to be located
    // in the program's root directory
    wxString path = wxStandardPaths::Get().GetExecutablePath();

    // Construct a file name class from the path
    wxFileName configName(path.substr(0, path.rfind('\\')+1), "numeregui.ini");

    // Prepare the configuration file
    wxFileConfig m_config("numeregui", wxEmptyString, configName.GetFullPath());

    // Depending on whether the file exists, load the contents
    // or use the default values
    if (configName.FileExists())
    {
        bool printInColor = (m_config.Read("Miscellaneous/PrintInColor", "true") == "true");
        int printStyle = printInColor ? wxSTC_PRINT_COLOURONWHITE : wxSTC_PRINT_BLACKONWHITE;
        m_options->SetPrintStyle(printStyle);

        std::map<std::string, SettingsValue>& mSettings = m_options->getSettings();

        mSettings[SETTING_B_TOOLBARTEXT].active() = (m_config.Read("Interface/ShowToolbarText", "false") == "true");
        mSettings[SETTING_B_PATHSONTABS].active() = (m_config.Read("Interface/ShowPathsOnTabs", "false") == "true");
        mSettings[SETTING_B_PRINTLINENUMBERS].active() = (m_config.Read("Miscellaneous/PrintLineNumbers", "false") == "true");
        mSettings[SETTING_B_SAVESESSION].active() = (m_config.Read("Miscellaneous/SaveSession", "false") == "true");
        mSettings[SETTING_B_SAVEBOOKMARKS].active() = (m_config.Read("Miscellaneous/SaveBookmarksInSession", "false") == "true");
        mSettings[SETTING_B_FORMATBEFORESAVING].active() = (m_config.Read("Miscellaneous/FormatBeforeSaving", "false") == "true");
        mSettings[SETTING_B_USEREVISIONS].active() = (m_config.Read("Miscellaneous/KeepBackups", "true") == "true");
        mSettings[SETTING_B_FOLDLOADEDFILE].active() = (m_config.Read("Interface/FoldDuringLoading", "false") == "true");
        mSettings[SETTING_B_HIGHLIGHTLOCALS].active() = (m_config.Read("Styles/HighlightLocalVariables", "false") == "true");
        mSettings[SETTING_B_LINESINSTACK].active() = (m_config.Read("Debugger/ShowLineNumbersInStackTrace", "true") == "true");
        mSettings[SETTING_B_MODULESINSTACK].active() = (m_config.Read("Debugger/ShowModulesInStackTrace", "true") == "true");
        mSettings[SETTING_B_PROCEDUREARGS].active() = (m_config.Read("Debugger/ShowProcedureArguments", "true") == "true");
        mSettings[SETTING_B_GLOBALVARS].active() = (m_config.Read("Debugger/ShowGlobalVariables", "false") == "true");

        int debuggerFocusLine = 10;
        m_config.Read("Debugger/FocusLine", &debuggerFocusLine, 10);
        m_options->SetDebuggerFocusLine(debuggerFocusLine);

        wxFont font;
        wxString nativeInfo;
        m_config.Read("Styles/EditorFont", &nativeInfo, "Consolas 10");
        font.SetNativeFontInfoUserDesc(nativeInfo);
        m_options->SetEditorFont(font);

        wxString latexroot;
        m_config.Read("Miscellaneous/LaTeXRoot", &latexroot, "C:/Program Files");
        m_options->SetLaTeXRoot(latexroot);

        int terminalHistory = 300;
        m_config.Read("Miscellaneous/TerminalHistory", &terminalHistory, 300);
        if (terminalHistory >= 100 && terminalHistory <= 1000)
            m_options->SetTerminalHistorySize(terminalHistory);
        else
            m_options->SetTerminalHistorySize(300);

        int caretBlinkTime = 500;
        m_config.Read("Interface/CaretBlinkTime", &caretBlinkTime, 500);
        if (caretBlinkTime >= 100 && caretBlinkTime <= 1000)
            m_options->SetCaretBlinkTime(caretBlinkTime);
        else
            m_options->SetCaretBlinkTime(500);

        // Read the color codes from the configuration file,
        // if they exist
        m_options->readColoursFromConfig(&m_config);

        // Read the analyzer config from the configuration file,
        // if it exists
        m_options->readAnalyzerOptionsFromConfig(&m_config);

        // Inform the kernel about updated settings
        m_terminal->setKernelSettings(*m_options);

        m_config.DeleteAll();
    }
}


/////////////////////////////////////////////////
/// \brief This member function recreates the last
/// session by reading the session file or creates
/// a new empty session, if the corresponding
/// setting was set to \c false.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::prepareSession()
{
    // create the initial blank open file or recreate the last session
    if (wxFileExists(getProgramFolder()+"\\numere.session") && m_options->GetSaveSession())
    {
        g_logger.info("Reopening files from last session.");

        // Inform the application that the editor
        // settings do not have to be copied during
        // the session recovery
        m_loadingFilesDuringStartup = true;
        NewFile();
        std::ifstream if_session;
        std::vector<std::string> vSessionFile;
        std::string sLine;
        std::string sFileName;
        int activefile = 0;
        int nId = 0;
        int nLine = 0;
        int nSetting = 0;
        std::string sBookmarks;
        bool modifiedFile = false;
        if_session.open((getProgramFolder().ToStdString()+"\\numere.session").c_str());

        // Is the session file available and readable? Then
        // recreate the last session from this file
        if (if_session.is_open())
        {
            // Read the session file completely
            while (!if_session.eof())
            {
                getline(if_session, sLine);

                // Ignore comments
                if (!sLine.length() || sLine.front() == '#')
                    continue;

                // store the active file ID
                if (sLine.substr(0,12) == "ACTIVEFILEID")
                {
                    activefile = StrToInt(sLine.substr(sLine.find('\t')+1));
                    continue;
                }

                vSessionFile.push_back(sLine);
            }

            // Close the session file
            if_session.close();

            // Decode the session file
            for (size_t i = 0; i < vSessionFile.size(); i++)
            {
                // copy the current fileinfo
                sLine = vSessionFile[i];

                // check for the "modified" attribute
                if (sLine.front() == '*')
                {
                    sLine.erase(0,1);
                    modifiedFile = true;
                }
                else
                    modifiedFile = false;

                // create filename and current line
                sFileName = sLine.substr(0, sLine.find('\t'));

                // erase until id
                sLine.erase(0, sLine.find('\t')+1);
                nId = StrToInt(sLine.substr(0, sLine.find('\t')));

                // erase until position
                sLine.erase(0, sLine.find('\t')+1);
                nLine = StrToInt(sLine.substr(0, sLine.find('\t')));

                // Search for settings
                if (sLine.find('\t') != std::string::npos)
                {
                    // erase until setting
                    sLine.erase(0, sLine.find('\t')+1);
                    nSetting = StrToInt(sLine.substr(0, sLine.find('\t')));
                }
                else
                    nSetting = 0;

                // Search for bookmarks
                if (sLine.find('\t') != std::string::npos && m_options->GetSaveBookmarksInSession())
                    sBookmarks = sLine.substr(sLine.rfind('\t')+1);
                else
                    sBookmarks.clear();

                // create the files
                //
                // This is a new file
                if (sFileName == "<NEWFILE>")
                {
                    if (vSessionFile.size() != 1)
                    {
                        NewFile();
                        m_book->getCurrentEditor()->SetUnsaved();
                    }

                    m_book->getCurrentEditor()->GotoPos(nLine);
                    m_book->getCurrentEditor()->ToggleSettings(nSetting);
                    continue;
                }

                // Recreate the file if it exists
                if (wxFileExists(sFileName))
                {
                    OpenSourceFile(wxArrayString(1, sFileName));
                    NumeReEditor* currentEd = m_book->getCurrentEditor();
                    currentEd->GotoPos(nLine);
                    currentEd->ToggleSettings(nSetting);
                    currentEd->EnsureVisible(currentEd->LineFromPosition(nLine));
                    currentEd->setBookmarks(toIntVector(sBookmarks));
                }
                else
                {
                    // If it not exists, inform the user
                    // that we were not able to load it
                    if (!modifiedFile)
                    {
                        m_UnrecoverableFiles += sFileName + "\n";
                    }

                    int nUnloadableID = nId;

                    // Move the active file ID if necessary
                    if (nUnloadableID < activefile)
                        activefile--;
                }
            }

            // Select the active file
            if (activefile >= (int)m_book->GetPageCount())
                m_book->SetSelection(m_book->GetPageCount()-1);
            else
                m_book->SetSelection(activefile);
        }

        // Inform the application that we are finished
        // recreating the session
        m_loadingFilesDuringStartup = false;
    }
    else
    {
        // Simply create a new empty file
        NewFile();
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private OnClose
///  Called whenever the main frame is about to close.  Handles initial resource cleanup.
///
///  @param  event wxCloseEvent & The close event generated by wxWindows
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnClose(wxCloseEvent &event)
{
    // This event might be fired multiple times
    if (m_appClosing)
    {
        g_logger.info("Prevented additional call to NumeReWindow::OnClose()");
        return;
    }

    m_appClosing = true;
    g_logger.info("Closing NumeRe.");


    if (!CloseAllFiles())
    {
        if (event.CanVeto())
            event.Veto(true);
        return;
    }

    // double check in case something went wrong
    if (m_book->getCurrentEditor() && m_book->getCurrentEditor()->Modified())
    {
        if (event.CanVeto())
        {
            event.Veto (true);
        }

        return;
    }

    if (m_updateTimer)
        m_updateTimer->Stop();

    if (m_fileEventTimer)
        m_fileEventTimer->Stop();

    if (m_terminal->IsWorking())
    {
        m_options->copySettings(m_terminal->getKernelSettings());

        if (m_options->getSetting(SETTING_B_SAVESASHS).active())
        {
            m_options->getSetting(SETTING_V_POS_SASH_H).value() = 100 * m_splitEditorOutput->GetSplitPercentage();
            m_options->getSetting(SETTING_V_POS_SASH_V).value() = m_splitProjectEditor->GetSashPosition();
            m_options->getSetting(SETTING_V_POS_SASH_T).value() = 100 * m_splitCommandHistory->GetSplitPercentage();
        }
        else
        {
            m_options->getSetting(SETTING_V_POS_SASH_H).value() = 75;
            m_options->getSetting(SETTING_V_POS_SASH_V).value() = 200;
            m_options->getSetting(SETTING_V_POS_SASH_T).value() = 75;
        }

        if (m_options->getSetting(SETTING_B_SAVEWINDOWSIZE).active())
        {
            wxRect winRect = GetScreenRect();

            if (IsMaximized())
                m_options->getSetting(SETTING_S_WINDOWSIZE).stringval() = "{}";
            else
                m_options->getSetting(SETTING_S_WINDOWSIZE).stringval() = wxString::Format("{x=%d,y=%d,w=%d,h=%d}",
                                                                                           winRect.x, winRect.y,
                                                                                           winRect.width, winRect.height);
        }

        // Update the settings for the last time
        m_terminal->setKernelSettings(*m_options);
        wxMilliSleep(200);

        g_logger.info("Stopping Kernel.");
        m_terminal->EndKernelTask();
        wxMilliSleep(200);
    }

    if (m_debugViewer != nullptr)
        m_debugViewer->Destroy();


    if (m_history)
    {
        g_logger.info("Saving command line history.");
        m_history->saveHistory();
    }

    // Close all windows to avoid calls to the map afterwards
    closeWindows(WT_ALL);

    Destroy();
}


//////////////////////////////////////////////////////////////////////////////
///  private OnMenuEvent
///  Responsible for handling 95% of the menu and button-related events.
///
///  @param  event wxCommandEvent & The event generated by the button/menu click
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnMenuEvent(wxCommandEvent &event)
{
    int id = event.GetId();

    switch(id)
    {
        case ID_MENU_OPEN_FILE_FROM_TREE:
        case ID_MENU_OPEN_FILE_FROM_TREE_TO_TABLE:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_fileTree->GetItemData(m_clickedTreeItem));
            OnExecuteFile(data->filename.ToStdString(), id);
            break;
        }
        case ID_MENU_EDIT_FILE_FROM_TREE:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_fileTree->GetItemData(m_clickedTreeItem));

            wxArrayString fnames;
            fnames.Add(data->filename);
            OpenSourceFile(fnames);
            break;
        }
        case ID_MENU_INSERT_IN_EDITOR_FROM_TREE:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_functionTree->GetItemData(m_clickedTreeItem));
            wxString command;
            if (data->isCommand)
                command = (data->tooltip).substr(0, (data->tooltip).find(' ')) + " ";
            else if (data->isFunction)
                command = (data->tooltip).substr(0, (data->tooltip).find('(')+1);
            else if (data->isConstant)
                command = (data->tooltip).substr(0, (data->tooltip).find(' '));
            else if (data->isMethod)
            {
                size_t dot = (data->tooltip).find('.');
                size_t endPos = (data->tooltip).find_first_of("( ", dot);

                if ((data->tooltip)[endPos] == '(')
                    endPos++;

                command = (data->tooltip).substr(dot, endPos-dot);
            }

            NumeReEditor* edit = m_book->getFocusedEditor();
            edit->InsertText(edit->GetCurrentPos(), command);
            edit->GotoPos(edit->GetCurrentPos()+command.length());
            break;
        }
        case ID_MENU_INSERT_IN_CONSOLE_FROM_TREE:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_functionTree->GetItemData(m_clickedTreeItem));
            std::string command;
            if (data->isCommand)
                command = (data->tooltip).substr(0, (data->tooltip).find(' ')).ToStdString() + " ";
            else if (data->isFunction)
                command = (data->tooltip).substr(0, (data->tooltip).find('(')+1).ToStdString();
            else if (data->isConstant)
                command = (data->tooltip).substr(0, (data->tooltip).find(' ')).ToStdString();
            else if (data->isMethod)
            {
                size_t dot = (data->tooltip).find('.');
                size_t endPos = (data->tooltip).find_first_of("( ", dot);

                if ((data->tooltip)[endPos] == '(')
                    endPos++;

                command = (data->tooltip).substr(dot, endPos-dot);
            }

            showConsole();
            m_terminal->ProcessInput(command.length(), command);
            break;
        }
        case ID_MENU_HELP_ON_ITEM:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_functionTree->GetItemData(m_clickedTreeItem));
            std::string command = (data->tooltip).substr(0, (data->tooltip).find(' ')).ToStdString();
            //openHTML(m_terminal->getDocumentation(command));
            ShowHelp(command);
            break;
        }
        case ID_MENU_SHOW_DESCRIPTION:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_functionTree->GetItemData(m_clickedTreeItem));
            wxMessageBox(data->tooltip, "DESCRIPTION", wxICON_INFORMATION, this);
            break;
        }
        case ID_MENU_OPEN_IMAGE_FROM_TREE:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData*> (m_fileTree->GetItemData(m_clickedTreeItem));
            openImage(wxFileName(data->filename));
            break;
        }
        case ID_MENU_DELETE_FILE_FROM_TREE:
            deleteFile();
            break;
        case ID_MENU_COPY_FILE_FROM_TREE:
            m_copiedTreeItem = m_clickedTreeItem;
            break;
        case ID_MENU_INSERT_FILE_INTO_TREE:
            insertCopiedFile();
            break;
        case ID_MENU_RENAME_FILE_IN_TREE:
            renameFile();
            break;
        case ID_MENU_NEW_FOLDER_IN_TREE:
            OnCreateNewFolder();
            break;
        case ID_MENU_REMOVE_FOLDER_FROM_TREE:
            OnRemoveFolder();
            break;
        case ID_MENU_COPY_AS_PATH:
            OnCopyAsPath();
            break;
        case ID_MENU_OPEN_IN_EXPLORER:
            OnOpenInExplorer();
            break;
        case ID_MENU_SHOW_REVISIONS:
            OnShowRevisions();
            break;
        case ID_MENU_SHOW_REVISIONS_FROM_TAB:
            OnShowRevisionsFromTab();
            break;
        case ID_MENU_TAG_CURRENT_REVISION:
            OnTagCurrentRevision();
            break;
        case ID_MENU_AUTOINDENT:
        {
            m_book->getCurrentEditor()->ApplyAutoIndentation();
            break;
        }
        case ID_MENU_LINEWRAP:
        {
            wxToolBar* t = GetToolBar();
            m_book->getCurrentEditor()->ToggleSettings(NumeReEditor::SETTING_WRAPEOL);

            if (m_book->getCurrentEditor(true))
                m_book->getCurrentEditor(true)->ToggleSettings(NumeReEditor::SETTING_WRAPEOL);

            t->ToggleTool(ID_MENU_LINEWRAP, m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
            m_menuItems[ID_MENU_LINEWRAP]->Check(m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
            break;
        }
        case ID_MENU_DISPCTRLCHARS:
        {
            m_book->getCurrentEditor()->ToggleSettings(NumeReEditor::SETTING_DISPCTRLCHARS);

            if (m_book->getCurrentEditor(true))
                m_book->getCurrentEditor(true)->ToggleSettings(NumeReEditor::SETTING_DISPCTRLCHARS);

            break;
        }
        case ID_MENU_USETXTADV:
        {
            m_book->getCurrentEditor()->ToggleSettings(NumeReEditor::SETTING_USETXTADV);

            if (m_book->getCurrentEditor(true))
                m_book->getCurrentEditor(true)->ToggleSettings(NumeReEditor::SETTING_USETXTADV);

            break;
        }
        case ID_MENU_USEANALYZER:
        {
            wxToolBar* t = GetToolBar();
            m_book->getCurrentEditor()->ToggleSettings(NumeReEditor::SETTING_USEANALYZER);
            t->ToggleTool(ID_MENU_USEANALYZER, m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
            m_menuItems[ID_MENU_USEANALYZER]->Check(m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
            break;
        }
        case ID_MENU_INDENTONTYPE:
        {
            wxToolBar* t = GetToolBar();
            m_book->getCurrentEditor()->ToggleSettings(NumeReEditor::SETTING_INDENTONTYPE);

            if (m_book->getCurrentEditor(true))
                m_book->getCurrentEditor(true)->ToggleSettings(NumeReEditor::SETTING_INDENTONTYPE);

            t->ToggleTool(ID_MENU_INDENTONTYPE, m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
            m_menuItems[ID_MENU_INDENTONTYPE]->Check(m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
            break;
        }
        case ID_MENU_USESECTIONS:
        {
            m_book->getCurrentEditor()->ToggleSettings(NumeReEditor::SETTING_USESECTIONS);
            m_menuItems[ID_MENU_USESECTIONS]->Check(m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_USESECTIONS));
            break;
        }
        case ID_MENU_AUTOFORMAT:
        {
            m_book->getCurrentEditor()->ApplyAutoFormat();
            break;
        }
        case ID_MENU_TRANSPOSESELECTION:
        {
            m_book->getFocusedEditor()->OnTranspose();
            break;
        }
        case ID_MENU_GOTOLINE:
        {
            gotoLine();
            break;
        }
        case ID_MENU_TOGGLE_NOTEBOOK_MULTIROW:
        {
            m_book->ToggleWindowStyle(wxNB_MULTILINE);
            m_book->SendSizeEvent();
            m_book->Refresh();
            m_book->getCurrentEditor()->Refresh();

            if (m_book->getCurrentEditor(true))
                m_book->getCurrentEditor(true)->Refresh();

            m_multiRowState = !m_multiRowState;
            break;
        }
        case ID_MENU_TOGGLE_COMMENT_LINE:
        {
            m_book->getFocusedEditor()->ToggleCommentLine();
            break;
        }
        case ID_MENU_TOGGLE_COMMENT_SELECTION:
        {
            m_book->getFocusedEditor()->ToggleCommentSelection();
            break;
        }
        case ID_MENU_FOLD_ALL:
        {
            m_book->getFocusedEditor()->FoldAll();
            break;
        }
        case ID_MENU_UNFOLD_ALL:
        {
            m_book->getFocusedEditor()->UnfoldAll();
            break;
        }
        case ID_MENU_UNHIDE_ALL:
        {
            m_book->getFocusedEditor()->OnUnhideAllFromMenu();
            break;
        }
        case ID_MENU_SELECTION_UP:
        {
            m_book->getFocusedEditor()->MoveSelection(false);
            break;
        }
        case ID_MENU_SELECTION_DOWN:
        {
            m_book->getFocusedEditor()->MoveSelection();
            break;
        }
        case ID_MENU_SORT_SELECTION_ASC:
        {
            m_book->getFocusedEditor()->sortSelection();
            break;
        }
        case ID_MENU_SORT_SELECTION_DESC:
        {
            m_book->getFocusedEditor()->sortSelection(false);
            break;
        }
        case ID_MENU_BOOKMARK_TOGGLE:
        {
            m_book->getFocusedEditor()->toggleBookmark();
            break;
        }
        case ID_MENU_BOOKMARK_CLEARMENU:
        {
            m_book->getCurrentEditor()->clearBookmarks();
            break;
        }
        case ID_MENU_BOOKMARK_PREVIOUS:
        {
            m_book->getFocusedEditor()->JumpToBookmark(false);
            break;
        }
        case ID_MENU_BOOKMARK_NEXT:
        {
            m_book->getFocusedEditor()->JumpToBookmark(true);
            break;
        }
        case ID_MENU_FIND_DUPLICATES:
        {
            // only if the file type is matching
            m_book->getCurrentEditor()->InitDuplicateCode();
            break;
        }
        case ID_MENU_STRIP_SPACES_BOTH:
        {
            m_book->getFocusedEditor()->removeWhiteSpaces(RM_WS_BOTH);
            break;
        }
        case ID_MENU_STRIP_SPACES_FRONT:
        {
            m_book->getFocusedEditor()->removeWhiteSpaces(RM_WS_FRONT);
            break;
        }
        case ID_MENU_STRIP_SPACES_BACK:
        {
            m_book->getFocusedEditor()->removeWhiteSpaces(RM_WS_BACK);
            break;
        }
        case ID_MENU_CREATE_LATEX_FILE:
        {
            createLaTeXFile();
            break;
        }
        case ID_MENU_RUN_LATEX:
        {
            runLaTeX();
            break;
        }
        case ID_MENU_COMPILE_LATEX:
        {
            compileLaTeX();
            break;
        }
        case ID_MENU_NEW_ASK:
        {
            OnAskForNewFile();
            break;
        }
        case ID_MENU_NEW_EMPTY:
        {
            NewFile();
            break;
        }
        case ID_MENU_NEW_SCRIPT:
        {
            NewFile(FILE_NSCR);
            break;
        }
        case ID_MENU_NEW_PROCEDURE:
        {
            NewFile(FILE_NPRC);
            break;
        }
        case ID_MENU_NEW_PLUGIN:
        {
            NewFile(FILE_PLUGIN);
            break;
        }
        case ID_MENU_NEW_LAYOUT:
        {
            NewFile(FILE_NLYT);
            break;
        }

        case ID_MENU_OPEN_SOURCE_LOCAL:
        case ID_MENU_OPEN_SOURCE_REMOTE:
        {
            OnOpenSourceFile(id);
            break;
        }

        case ID_MENU_SAVE:
        {
            SaveCurrentFile(false);
            break;
        }

        case ID_MENU_SAVE_SOURCE_LOCAL:
        case ID_MENU_SAVE_SOURCE_REMOTE:
        {
            OnSaveSourceFile(id);
            break;
        }

        case ID_NEW_PROJECT:
        {
            SaveCurrentFile(true);
            break;
        }

        case ID_MENU_CLOSETAB:
        {
            CloseTab(false);
            break;
        }
        case ID_MENU_CLOSETABFORCE:
        {
            CloseTab(true);
            break;
        }
        case ID_MENU_RUN_FROM_TAB:
        {
            EvaluateTab();
            break;
        }

        case ID_MENU_CLOSEPAGE:
        {
            if (!m_book->getCurrentEditor())
            {
                return;
            }

            CloseFile();
            PageHasChanged();
            break;
        }

        case ID_MENU_CLOSEALL:
        {
            CloseAllFiles();
            break;
        }
        case ID_MENU_CLOSEOTHERS:
        {
            CloseOtherTabs();
            break;
        }
        case ID_MENU_OPEN_FOLDER:
        {
            OpenContainingFolder();
            break;
        }
        case ID_MENU_UNSPLIT_TAB:
        {
            int tab = GetIntVar(VN_CLICKEDTAB);
            m_book->unsplit(tab);
            break;
        }
        case ID_MENU_SPLIT_TAB_H:
        {
            int tab = GetIntVar(VN_CLICKEDTAB);
            m_book->split(tab, true);
            break;
        }
        case ID_MENU_SPLIT_TAB_V:
        {
            int tab = GetIntVar(VN_CLICKEDTAB);
            m_book->split(tab, false);
            break;
        }

        case ID_MENU_TOGGLE_CONSOLE:
        {
            toggleConsole();
            if (m_termContainer->IsShown())
                m_terminal->SetFocus();
            break;
        }

        case ID_MENU_TOGGLE_FILETREE:
        {
            toggleFiletree();
            break;
        }

        case ID_MENU_TOGGLE_HISTORY:
        {
            toggleHistory();
            break;
        }


        case ID_MENU_QUIT:
        {
            Close(true);
            break;
        }

        case ID_MENU_REDO:
        case ID_MENU_UNDO:
        {
            if(id == ID_MENU_REDO)
                m_book->getFocusedEditor()->Redo();
            else
                m_book->getFocusedEditor()->Undo();

            m_book->Refresh();
            break;
        }

        case ID_MENU_COPY:
        {
            if (m_book->getFocusedEditor()->HasFocus())
                m_book->getFocusedEditor()->Copy();
            else if (m_terminal->HasFocus())
                m_terminal->copyText();

            break;
        }

        case ID_MENU_CUT:
        {
            if (m_book->getFocusedEditor()->HasFocus())
                m_book->getFocusedEditor()->Cut();
            else if (m_terminal->HasFocus())
                m_terminal->cutText();

            break;
        }

        case ID_MENU_PASTE:
        {
            if (m_book->getFocusedEditor()->HasFocus())
                m_book->getFocusedEditor()->Paste();
            else if (m_terminal->HasFocus())
                m_terminal->pasteText();

            break;
        }

        case ID_MENU_FIND:
        case ID_MENU_REPLACE:
        {
            OnFindReplace(id);
            break;
        }
        case ID_MENU_FIND_PROCEDURE:
        {
            m_book->getFocusedEditor()->OnFindProcedureFromMenu();
            break;
        }
        case ID_MENU_FIND_INCLUDE:
        {
            m_book->getFocusedEditor()->OnFindIncludeFromMenu();
            break;
        }

        case ID_MENU_ABOUT:
        {
            OnAbout();
            break;
        }

        case ID_MENU_OPTIONS:
        {
            OnOptions();
            break;
        }
        case ID_MENU_TOGGLE_DEBUGGER:
        {
            Settings _option = m_terminal->getKernelSettings();
            _option.getSetting(SETTING_B_DEBUGGER).active() = !_option.useDebugger();
            m_options->getSetting(SETTING_B_DEBUGGER).active() = _option.useDebugger();
            m_terminal->setKernelSettings(_option);
            wxToolBar* tb = GetToolBar();
            tb->ToggleTool(ID_MENU_TOGGLE_DEBUGGER, _option.useDebugger());
            m_menuItems[ID_MENU_TOGGLE_DEBUGGER]->Check(_option.useDebugger());
            break;
        }
        case ID_MENU_RENAME_SYMBOL:
        {
            m_book->getFocusedEditor()->OnRenameSymbolsFromMenu();
            break;
        }
        case ID_MENU_ABSTRAHIZE_SECTION:
        {
            m_book->getFocusedEditor()->OnAbstrahizeSectionFromMenu();
            break;
        }
        case ID_MENU_SHOW_DEPENDENCY_REPORT:
        {
            OnCalculateDependencies();
            break;
        }
        case ID_MENU_COMPARE_FILES:
        {
            OnCompareFiles();
            break;
        }
        case ID_MENU_CREATE_PACKAGE:
        {
            OnCreatePackage("");
            break;
        }
        case ID_MENU_CREATE_DOCUMENTATION:
        {
            m_book->getFocusedEditor()->AddProcedureDocumentation();
            break;
        }
        case ID_MENU_EXPORT_AS_HTML:
        {
            m_book->getFocusedEditor()->OnExtractAsHTML();
            break;
        }
        case ID_MENU_EXPORT_FORMATTED:
        {
            m_book->getFocusedEditor()->OnExtractFormatted();
            break;
        }
        case ID_MENU_PLUGINBROWSER:
        {
            PackageRepoBrowser* repobrowser = new PackageRepoBrowser(this, m_terminal, m_iconManager);
            repobrowser->SetIcon(GetIcon());
            repobrowser->Show();
            break;
        }
        case ID_MENU_INSTALLPLUGINFROMFILE:
        {
            std::string packageFile = wxFileSelector(_guilang.get("GUI_SELECT_PACKAGE_FOR_INSTALL"),
                                                     m_terminal->getPathSettings()[SCRIPTPATH] + "/packages",
                                                     "", "*.nscr", "Packages (*.nscr)|*.nscr", wxFD_OPEN, this).ToStdString();

            if (packageFile.length())
                m_terminal->pass_command("install \"" + replacePathSeparator(packageFile) + "\"", false);

            break;
        }

        case ID_MENU_PRINT_PAGE:
        {
            OnPrintPage();
            break;
        }

        case ID_MENU_PRINT_PREVIEW:
        {
            OnPrintPreview();
            break;
        }

        case ID_MENU_PRINT_SETUP:
        {
            OnPrintSetup();
            break;
        }

        case ID_MENU_HELP:
        {
            OnHelp();
            break;
        }

        case ID_MENU_ADDEDITORBREAKPOINT:
        {
            m_book->getFocusedEditor()->OnAddBreakpoint(event);
            break;
        }
        case ID_MENU_REMOVEEDITORBREAKPOINT:
        {
            m_book->getFocusedEditor()->OnRemoveBreakpoint(event);
            break;
        }
        case ID_MENU_CLEAREDITORBREAKPOINTS:
        {
            m_book->getFocusedEditor()->OnClearBreakpoints(event);
            break;
        }

        case ID_MENU_EXECUTE:
        {
            if (!m_book->getCurrentEditor()->HasBeenSaved() || m_book->getCurrentEditor()->Modified())
            {
                int tabNum = m_book->GetSelection();
                int result = HandleModifiedFile(tabNum, MODIFIEDFILE_COMPILE);

                if (result == wxCANCEL)
                    return;

                if (m_book->getCurrentEditor()->getFileType() == FILE_NPRC
                    || m_book->getCurrentEditor()->getFileType() == FILE_NLYT
                    || m_book->getCurrentEditor()->getFileType() == FILE_NSCR)
                {
                    m_terminal->UpdateLibrary();
                }
            }

            if (m_book->getCurrentEditor()->getFileType() == FILE_TEXSOURCE)
                compileLaTeX();
            else
            {
                std::string command = replacePathSeparator((m_book->getCurrentEditor()->GetFileName()).GetFullPath().ToStdString());
                OnExecuteFile(command, id);
            }

            break;
        }
        case ID_MENU_STOP_EXECUTION:
            m_terminal->CancelCalculation();
            break;
    }
}


/////////////////////////////////////////////////
/// \brief Handles events, which originate from
/// package menu (i.e. graphical plugins).
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnPluginMenuEvent(wxCommandEvent& event)
{
    size_t id = event.GetId();

    auto iter = m_pluginMenuMap.find(id);

    if (iter != m_pluginMenuMap.end())
        m_terminal->pass_command("$" + iter->second + "()", true);
}


/////////////////////////////////////////////////
/// \brief This member function handles all
/// events, which result from changes in the file
/// system.
///
/// \param event wxFileSystemWatcherEvent&
/// \return void
///
/// Because it is likely that multiple of them are
/// fired, if whole folders are moved, we only
/// cache the paths and the event types here and
/// start a one shot timer. If this timer has ran
/// (it will be resetted for each incoming event),
/// the cached changes will be processed.
/////////////////////////////////////////////////
void NumeReWindow::OnFileSystemEvent(wxFileSystemWatcherEvent& event)
{
    if (!m_fileTree || m_appStarting)
        return;

    int type = event.GetChangeType();

    // Cache the event types and the event paths, if
    // the types match the selection and the path
    // does not contain the revisions folders
    if ((type == wxFSW_EVENT_CREATE
         || type == wxFSW_EVENT_DELETE
         || type == wxFSW_EVENT_RENAME
         || type == wxFSW_EVENT_MODIFY) && event.GetPath().GetFullPath().find(".revisions") == std::string::npos)
    {
        m_modifiedFiles.push_back(std::make_pair(type, event.GetPath().GetFullPath()));
        m_dragDropSourceItem = wxTreeItemId();
        m_fileEventTimer->StartOnce(500);
    }
}


/////////////////////////////////////////////////
/// \brief This member function finds every
/// procedure in the default search path and adds
/// them to the syntax autocompletion of the
/// terminal.
///
/// \param sProcedurePath const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::CreateProcedureTree(const std::string& sProcedurePath)
{
    std::vector<std::string> vFolderTree(1, sProcedurePath);
    std::vector<std::string> vProcedureTree;
    std::vector<std::string> vCurrentTree;
    std::string sPath = sProcedurePath;
    Settings _option = m_terminal->getKernelSettings();

    // Find every folder first
    do
    {
        sPath += "/*";
        vCurrentTree = getFolderList(sPath, _option, 1);

        if (vCurrentTree.size())
            vFolderTree.insert(vFolderTree.end(), vCurrentTree.begin(), vCurrentTree.end());
    }
    while (vCurrentTree.size());

    // Search through every folder for procedures
    for (size_t i = 0; i < vFolderTree.size(); i++)
    {
        if (vFolderTree[i].substr(vFolderTree[i].length()-3) == "/.." || vFolderTree[i].substr(vFolderTree[i].length()-2) == "/.")
            continue;

        vCurrentTree = getFileList(vFolderTree[i] + "/*.nprc", _option, 1);

        if (vCurrentTree.size())
            vProcedureTree.insert(vProcedureTree.end(), vCurrentTree.begin(), vCurrentTree.end());
    }

    // Remove the leading path part of the procedure path,
    // which is the procedure default path
    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        if (vProcedureTree[i].substr(0, sProcedurePath.length()) == sProcedurePath)
            vProcedureTree[i].erase(0, sProcedurePath.length());

        while (vProcedureTree[i].front() == '/' || vProcedureTree[i].front() == '\\')
            vProcedureTree[i].erase(0, 1);
    }

    m_terminal->getSyntax()->setProcedureTree(vProcedureTree);
}


/////////////////////////////////////////////////
/// \brief This member function opens the selected
/// image in the image viewer window.
///
/// \param filename wxFileName
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::openImage(wxFileName filename)
{
    wxString programPath = getProgramFolder();

    ViewerFrame* frame = new ViewerFrame(this, "NumeRe-ImageViewer: " + filename.GetName(),
                                         m_options->getSetting(SETTING_B_FLOATONPARENT).active() ? wxFRAME_FLOAT_ON_PARENT : 0);
    registerWindow(frame, WT_IMAGEVIEWER);
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    ImagePanel* _panel = new ImagePanel(frame);
    // Apply the settings for the image viewer
    sizer->Add(_panel, 1, wxEXPAND);
    frame->SetSizer(sizer);
    frame->SetIcon(getStandardIcon());
    m_currentView = frame;

    // Create the image panel
    if (filename.GetExt() == "png")
        _panel->showImage(filename.GetFullPath(), wxBITMAP_TYPE_PNG);
    else if (filename.GetExt() == "bmp")
        _panel->showImage(filename.GetFullPath(), wxBITMAP_TYPE_BMP);
    else if (filename.GetExt() == "gif")
        _panel->showImage(filename.GetFullPath(), wxBITMAP_TYPE_GIF);
    else if (filename.GetExt() == "jpg" || filename.GetExt() == "jpeg")
        _panel->showImage(filename.GetFullPath(), wxBITMAP_TYPE_JPEG);
    else if (filename.GetExt() == "tif" || filename.GetExt() == "tiff")
        _panel->showImage(filename.GetFullPath(), wxBITMAP_TYPE_TIF);
    else
    {
        frame->Destroy();
        return;
    }

    //_panel->SetSize(_panel->getRelation()*600,600);
    //frame->SetClientSize(_panel->GetSize());
    frame->Show();
    frame->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function opens a PDF
/// document using the windows shell.
///
/// \param filename wxFileName
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::openPDF(wxFileName filename)
{
    ShellExecuteA(NULL, "open", filename.GetFullPath().ToStdString().c_str(), "", "", SW_SHOW);
}


/////////////////////////////////////////////////
/// \brief This member function opens a HTML
/// document (a documentation article) in the
/// documentation viewer.
///
/// \param HTMLcontent wxString
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::openHTML(wxString HTMLcontent)
{
    //wxString programPath = getProgramFolder();
    //if (!HTMLcontent.length())
    //    return;
    //
    //ViewerFrame* frame = new ViewerFrame(this, "NumeRe-Hilfe:");
    //frame->CreateStatusBar();
    //HelpViewer* html = new HelpViewer(frame, this);
    //html->SetRelatedFrame(frame, _guilang.get("DOC_HELP_HEADLINE", "%s"));
    //html->SetRelatedStatusBar(0);
    //html->SetPage(HTMLcontent);
    //frame->SetSize(1000,600);
    //frame->SetIcon(getStandardIcon());
    //frame->Show();
    //frame->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// contents of the "string()" table or a cluster.
///
/// \param _stringTable NumeRe::Container<std::string>
/// \param tableDisplayName const std::string&
/// \param sIntName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::openTable(NumeRe::Container<std::string> _stringTable, const std::string& tableDisplayName, const std::string& sIntName)
{
    ViewerFrame* frame = new ViewerFrame(this, "NumeRe: " + tableDisplayName,
                                         m_options->getSetting(SETTING_B_FLOATONPARENT).active() ? wxFRAME_FLOAT_ON_PARENT : 0);
    registerWindow(frame, WT_TABLEVIEWER);
    frame->SetSize(800,600);
    TableViewer* grid = new TableViewer(frame, wxID_ANY, frame->CreateStatusBar(3), nullptr, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_STATIC);
    grid->SetData(_stringTable, tableDisplayName, sIntName);
    frame->SetSize(std::min(800u, grid->GetWidth()), std::max(std::min(600u, grid->GetHeight()+50), 300u));
    frame->SetIcon(getStandardIcon());
    frame->Show();
    frame->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// contents of a usual table.
///
/// \param _table NumeRe::Table
/// \param tableDisplayName const std::string&
/// \param sIntName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::openTable(NumeRe::Table _table, const std::string& tableDisplayName, const std::string& sIntName)
{
    ViewerFrame* frame = new ViewerFrame(this, "NumeRe: " + tableDisplayName,
                                         m_options->getSetting(SETTING_B_FLOATONPARENT).active() ? wxFRAME_FLOAT_ON_PARENT : 0);
    registerWindow(frame, WT_TABLEVIEWER);
    frame->SetSize(800,600);
    TablePanel* panel = new TablePanel(frame, wxID_ANY, frame->CreateStatusBar(3));
    panel->SetTerminal(m_terminal);
    panel->grid->SetData(_table, tableDisplayName, sIntName);
    frame->SetSize(std::min(800u, panel->grid->GetWidth()+200), std::max(std::min(600u, panel->grid->GetHeight()+50), 300u));
    frame->SetIcon(getStandardIcon());
    frame->Show();
    frame->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// contents of the "string()" table or a cluster
/// and enables editing its contents.
///
/// \param _stringTable NumeRe::Container<std::string>
/// \param tableDisplayName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::editTable(NumeRe::Container<std::string> _stringTable, const std::string& tableDisplayName)
{
    ViewerFrame* frame = new ViewerFrame(this, _guilang.get("GUI_TABLEEDITOR") + " " + tableDisplayName, wxFRAME_FLOAT_ON_PARENT);
    frame->SetSize(800,600);
    TableEditPanel* panel = new TableEditPanel(frame, wxID_ANY, frame->CreateStatusBar(3));
    panel->SetTerminal(m_terminal);
    panel->grid->SetData(_stringTable, tableDisplayName, "");
    frame->SetSize(std::min(800u, panel->grid->GetWidth()), std::max(std::min(600u, panel->grid->GetHeight()+50), 300u));
    frame->SetIcon(getStandardIcon());
    frame->Show();
    frame->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// contents of a usual table and enables editing
/// its contents.
///
/// \param _table NumeRe::Table
/// \param tableDisplayName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::editTable(NumeRe::Table _table, const std::string& tableDisplayName)
{
    ViewerFrame* frame = new ViewerFrame(this, _guilang.get("GUI_TABLEEDITOR") + " " + tableDisplayName, wxFRAME_FLOAT_ON_PARENT);
    frame->SetSize(800,600);
    TableEditPanel* panel = new TableEditPanel(frame, wxID_ANY, frame->CreateStatusBar(3));
    panel->SetTerminal(m_terminal);
    panel->grid->SetData(_table, tableDisplayName, "");
    frame->SetSize(std::min(800u, panel->grid->GetWidth()+200), std::max(std::min(600u, panel->grid->GetHeight()+50), 300u));
    frame->SetIcon(getStandardIcon());
    frame->Show();
    frame->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function is a wrapper for
/// an event handler of the variable viewer to
/// display the contents of the selected item.
///
/// \param tableName const wxString&
/// \param tableDisplayName const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showTable(const wxString& tableName, const wxString& tableDisplayName)
{
    if (tableDisplayName == "string()" || tableDisplayName.find("{}") != std::string::npos)
        openTable(m_terminal->getStringTable(tableName.ToStdString()), tableDisplayName.ToStdString(), tableName.ToStdString());
    else
        openTable(m_terminal->getTable(tableName.ToStdString()), tableDisplayName.ToStdString(), tableName.ToStdString());
}


/////////////////////////////////////////////////
/// \brief This public member function handles
/// the creation of windows requested by the
/// kernel.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showWindow(NumeRe::Window& window)
{
    if (window.getType() == NumeRe::WINDOW_GRAPH)
    {
        showGraph(window);
    }
    else if (window.getType() == NumeRe::WINDOW_MODAL)
    {
        if (window.getWindowSettings().nControls & NumeRe::CTRL_FILEDIALOG)
            showFileDialog(window);
        else if (window.getWindowSettings().nControls & NumeRe::CTRL_FOLDERDIALOG)
            showDirDialog(window);
        else if (window.getWindowSettings().nControls & NumeRe::CTRL_TEXTENTRY)
            showTextEntry(window);
        else if (window.getWindowSettings().nControls & NumeRe::CTRL_MESSAGEBOX)
            showMessageBox(window);
        else if (window.getWindowSettings().nControls & NumeRe::CTRL_LISTDIALOG)
            showListDialog(window);
        else if (window.getWindowSettings().nControls & NumeRe::CTRL_SELECTIONDIALOG)
            showSelectionDialog(window);
        else if (window.getWindowSettings().nControls & NumeRe::CTRL_LISTEDITDIALOG)
            showListEditDialog(window);
    }
    else if (window.getType() == NumeRe::WINDOW_CUSTOM)
    {
        CustomWindow* win = new CustomWindow(this, window,
                                             m_options->getSetting(SETTING_B_FLOATONPARENT).active() ? wxFRAME_FLOAT_ON_PARENT : 0);
        registerWindow(win, WT_CUSTOM);
        win->Show();
    }
}


/////////////////////////////////////////////////
/// \brief This private member function displays
/// a graph.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showGraph(NumeRe::Window& window)
{
    GraphViewer* viewer = new GraphViewer(this, "NumeRe: " + window.getGraph()->getTitle(), window.getGraph(), m_terminal);
    registerWindow(viewer, WT_GRAPH);

    viewer->SetIcon(getStandardIcon());
    viewer->Show();
    viewer->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This private member function displays
/// a file dialog.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showFileDialog(NumeRe::Window& window)
{
    std::string sExpression = window.getWindowSettings().sExpression;
    std::string sDir = prepareStringsForDialog(getNextArgument(sExpression, true));
    std::string sDefFile = prepareStringsForDialog(getNextArgument(sExpression, true));
    std::string sWildCard = prepareStringsForDialog(getNextArgument(sExpression, true));
    wxFileDialog dialog(this, window.getWindowSettings().sTitle, sDir, sDefFile, sWildCard);
    dialog.SetIcon(getStandardIcon());
    int ret = dialog.ShowModal();

    if (ret == wxID_CANCEL)
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    else
        window.updateWindowInformation(NumeRe::STATUS_OK, dialog.GetPath().ToStdString());
}


/////////////////////////////////////////////////
/// \brief This private member function displays
/// a directory dialog.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showDirDialog(NumeRe::Window& window)
{
    std::string sExpression = window.getWindowSettings().sExpression;
    wxDirDialog dialog(this, window.getWindowSettings().sTitle, prepareStringsForDialog(getNextArgument(sExpression, true)));
    dialog.SetIcon(getStandardIcon());
    int ret = dialog.ShowModal();

    if (ret == wxID_CANCEL)
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    else
        window.updateWindowInformation(NumeRe::STATUS_OK, dialog.GetPath().ToStdString());
}


/////////////////////////////////////////////////
/// \brief This private member function displays
/// a text entry dialog.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showTextEntry(NumeRe::Window& window)
{
    std::string sExpression = window.getWindowSettings().sExpression;
    wxTextEntryDialog dialog(this, prepareStringsForDialog(window.getWindowSettings().sMessage), window.getWindowSettings().sTitle, prepareStringsForDialog(getNextArgument(sExpression, true)));
    dialog.SetIcon(getStandardIcon());
    int ret = dialog.ShowModal();

    if (ret == wxID_CANCEL)
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    else
    {
        wxString value = dialog.GetValue();
        value.Replace("\"", "\\\"");
        window.updateWindowInformation(NumeRe::STATUS_OK, value.ToStdString());
    }
}


/////////////////////////////////////////////////
/// \brief This private member function displays
/// a message box.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showMessageBox(NumeRe::Window& window)
{
    long style = wxCENTRE;
    int nControls = window.getWindowSettings().nControls;

    if (nControls & NumeRe::CTRL_CANCELBUTTON)
        style |= wxCANCEL;

    if (nControls & NumeRe::CTRL_OKBUTTON)
        style |= wxOK;

    if (nControls & NumeRe::CTRL_YESNOBUTTON)
        style |= wxYES_NO;

    if (nControls & NumeRe::CTRL_ICONQUESTION)
        style |= wxICON_QUESTION;

    if (nControls & NumeRe::CTRL_ICONINFORMATION)
        style |= wxICON_INFORMATION;

    if (nControls & NumeRe::CTRL_ICONWARNING)
        style |= wxICON_WARNING;

    if (nControls & NumeRe::CTRL_ICONERROR)
        style |= wxICON_ERROR;

    int ret = wxMessageBox(prepareStringsForDialog(window.getWindowSettings().sMessage), window.getWindowSettings().sTitle, style, this);

    if (ret == wxOK)
        window.updateWindowInformation(NumeRe::STATUS_OK, "ok");
    else if (ret == wxCANCEL)
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "cancel");
    else if (ret == wxYES)
        window.updateWindowInformation(NumeRe::STATUS_OK, "yes");
    else
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "no");
}


/////////////////////////////////////////////////
/// \brief This private member function shows a
/// list dialog.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showListDialog(NumeRe::Window& window)
{
    std::string sExpression = window.getWindowSettings().sExpression;
    wxArrayString choices;

    while (sExpression.length())
    {
        choices.Add(prepareStringsForDialog(getNextArgument(sExpression, true)));
    }

    wxSingleChoiceDialog dialog(this, prepareStringsForDialog(window.getWindowSettings().sMessage), window.getWindowSettings().sTitle, choices);
    dialog.SetIcon(getStandardIcon());
    int ret = dialog.ShowModal();

    if (ret == wxID_CANCEL)
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    else
    {
        choices[dialog.GetSelection()].Replace("\"", "\\\"");
        window.updateWindowInformation(NumeRe::STATUS_OK, choices[dialog.GetSelection()].ToStdString());
    }
}


/////////////////////////////////////////////////
/// \brief This private member function shows a
/// selection dialog.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showSelectionDialog(NumeRe::Window& window)
{
    std::string sExpression = window.getWindowSettings().sExpression;
    wxArrayString choices;

    while (sExpression.length())
    {
        choices.Add(prepareStringsForDialog(getNextArgument(sExpression, true)));
    }

    wxMultiChoiceDialog dialog(this, prepareStringsForDialog(window.getWindowSettings().sMessage), window.getWindowSettings().sTitle, choices);
    dialog.SetIcon(getStandardIcon());
    int ret = dialog.ShowModal();

    if (ret == wxID_CANCEL)
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    else
    {
        wxArrayInt selections = dialog.GetSelections();

        sExpression.clear();

        for (size_t i = 0; i < selections.size(); i++)
        {
            choices[selections[i]].Replace("\"", "\\\"");
            sExpression += choices[selections[i]].ToStdString() + "\",\"";
        }

        if (sExpression.length())
            sExpression.erase(sExpression.length()-3);

        window.updateWindowInformation(NumeRe::STATUS_OK, sExpression);
    }
}


/////////////////////////////////////////////////
/// \brief This private member function shows a
/// list edit dialog.
///
/// \param window NumeRe::Window&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showListEditDialog(NumeRe::Window& window)
{
    std::string sExpression = window.getWindowSettings().sExpression;
    wxArrayString choices;

    while (sExpression.length())
    {
        choices.Add(prepareStringsForDialog(getNextArgument(sExpression, true)));
    }

    ListEditDialog dialog(this, choices, window.getWindowSettings().sTitle, prepareStringsForDialog(window.getWindowSettings().sMessage));
    dialog.SetIcon(getStandardIcon());
    int ret = dialog.ShowModal();

    if (ret == wxID_CANCEL)
        window.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    else
    {
        wxArrayString listEntries = dialog.getEntries();

        sExpression.clear();

        for (size_t i = 0; i < listEntries.size(); i++)
        {
            listEntries[i].Replace("\"", "\\\"");
            sExpression += listEntries[i].ToStdString() + "\",\"";
        }

        if (sExpression.length())
            sExpression.erase(sExpression.length()-3);

        window.updateWindowInformation(NumeRe::STATUS_OK, sExpression);
    }
}


/////////////////////////////////////////////////
/// \brief This member function is a wrapper for
/// the corresponding terminal function to pass
/// a command to the kernel.
///
/// \param command const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::pass_command(const wxString& command, bool isEvent)
{
    m_terminal->pass_command(command.ToStdString(), isEvent);
}


/////////////////////////////////////////////////
/// \brief This function will pass the obtained
/// debugging information to the debug viewer. If
/// this object does not yet exist, it will be
/// created on-the-fly.
///
/// \param vDebugInfo const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::evaluateDebugInfo(const std::vector<std::string>& vDebugInfo)
{
    // initialize the debugger, if necessary and pass the new contents
    std::string sTitle = vDebugInfo[0];
    std::vector<std::string> vStack;

    vStack.insert(vStack.begin(), vDebugInfo.begin()+1, vDebugInfo.end());

    // If the debug viewer does not yet exist, create a corresponding
    // instance here
    if (m_debugViewer == nullptr)
    {
        m_debugViewer = new DebugViewer(this, m_options, sTitle);
        m_debugViewer->SetSize(800, 700);
        m_debugViewer->SetIcon(getStandardIcon());
        m_debugViewer->setTerminal(m_terminal);
    }

    // If the debug viewer is not shown, show it
    if (!m_debugViewer->IsShown())
        m_debugViewer->Show();

    // Pass the obtained debugging information to the
    // debug viewer
    m_debugViewer->setDebugInfo(sTitle, vStack);
}


/////////////////////////////////////////////////
/// \brief This member function uses the parsed
/// contents from the current editor to create a
/// new LaTeX file from them.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::createLaTeXFile()
{
    std::string sFileName = m_book->getCurrentEditor()->GetFileNameAndPath().ToStdString();
    DocumentationGenerator docGen(m_terminal->getSyntax(), m_terminal->getPathSettings()[SAVEPATH] + "/docs");

    std::string sDocFile = docGen.createDocumentation(sFileName);

    if (sDocFile.length())
        wxMessageBox(_guilang.get("GUI_DLG_LATEX_SUCCESS_MESSAGE", sDocFile), _guilang.get("GUI_DLG_LATEX_SUCCESS"), wxCENTER | wxOK, this);
    else
        wxMessageBox(_guilang.get("GUI_DLG_LATEX_ERROR_MESSAGE", sFileName), _guilang.get("GUI_DLG_LATEX_ERROR"), wxCENTER | wxOK, this);
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// LaTeX documentation files and uses the Windows
/// shell to run the XeLaTeX compiler.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::runLaTeX()
{
    std::string sFileName = m_book->getCurrentEditor()->GetFileNameAndPath().ToStdString();
    DocumentationGenerator docGen(m_terminal->getSyntax(), m_terminal->getPathSettings()[SAVEPATH] + "/docs");
    std::string sMain = docGen.createFullDocumentation(sFileName);

    if (!sMain.length())
    {
        wxMessageBox(_guilang.get("GUI_DLG_LATEX_ERROR_MESSAGE", sFileName), _guilang.get("GUI_DLG_LATEX_ERROR"), wxCENTER | wxOK, this);
        return;
    }

    sMain += " -interaction=nonstopmode";

    if (fileExists((m_options->GetLaTeXRoot() + "/xelatex.exe").ToStdString()))
        ShellExecuteA(NULL,
                      "open",
                      (m_options->GetLaTeXRoot()+"/xelatex.exe").ToStdString().c_str(),
                      sMain.c_str(),
                      sMain.substr(0, sMain.rfind('/')).c_str(),
                      SW_SHOW);
    else
        wxMessageBox(_guilang.get("GUI_DLG_NOTEXBIN_ERROR", m_options->GetLaTeXRoot().ToStdString()),
                     _guilang.get("GUI_DLG_NOTEXBIN"), wxCENTER | wxOK | wxICON_ERROR, this);
}


/////////////////////////////////////////////////
/// \brief This function runs the XeLaTeX
/// compiler on the TeX source in the current
/// editor (if it is a TeX source).
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::compileLaTeX()
{
    FileFilterType fileType = m_book->getCurrentEditor()->getFileType();

    if (fileType == FILE_TEXSOURCE)
    {
        wxFileName filename = m_book->getCurrentEditor()->GetFileName();

        if (fileExists((m_options->GetLaTeXRoot() + "/xelatex.exe").ToStdString()))
            ShellExecuteA(NULL,
                          "open",
                          (m_options->GetLaTeXRoot()+"/xelatex.exe").ToStdString().c_str(),
                          (filename.GetName().ToStdString() + " -interaction=nonstopmode").c_str(),
                          filename.GetPath().ToStdString().c_str(),
                          SW_SHOW);
        else
            wxMessageBox(_guilang.get("GUI_DLG_NOTEXBIN_ERROR", m_options->GetLaTeXRoot().ToStdString()),
                         _guilang.get("GUI_DLG_NOTEXBIN"), wxCENTER | wxOK | wxICON_ERROR, this);
    }
}


/////////////////////////////////////////////////
/// \brief This member function moves the selected
/// file from the file tree directly to the
/// Windows trash bin, if the user confirms the
/// opened dialog.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::deleteFile()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));

    if (wxYES != wxMessageBox(_guilang.get("GUI_DLG_DELETE_QUESTION", data->filename.ToStdString()), _guilang.get("GUI_DLG_DELETE"), wxCENTRE | wxICON_QUESTION | wxYES_NO, this))
        return;

    if (m_clickedTreeItem == m_copiedTreeItem)
        m_copiedTreeItem = 0;

    Recycler _recycler;
    _recycler.recycle(data->filename.c_str());
}


/////////////////////////////////////////////////
/// \brief This member function copies the
/// selected file in the file tree to the target
/// location in the file tree.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::insertCopiedFile()
{
    FileNameTreeData* target_data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
    FileNameTreeData* source_data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_copiedTreeItem));
    wxFileName target_filename = target_data->filename;
    wxFileName source_filename = source_data->filename;

    target_filename.SetName(source_filename.GetName());
    target_filename.SetExt(source_filename.GetExt());

    if (wxFileExists(target_filename.GetFullPath()))
    {
        wxMessageBox(_guilang.get("GUI_DLG_COPY_ERROR"), _guilang.get("GUI_DLG_COPY"), wxCENTRE | wxICON_ERROR | wxOK, this);
        return;
    }

    wxCopyFile(source_filename.GetFullPath(), target_filename.GetFullPath());
    m_copiedTreeItem = 0;
}


/////////////////////////////////////////////////
/// \brief This member function renames the
/// selected file in the file tree with a new name
/// provided by the user in a text entry dialog.
///
/// \return void
///
/// If the file has internal revisions, the
/// version control system manager keeps track on
/// the renaming of the corresponding revisions
/// file.
/////////////////////////////////////////////////
void NumeReWindow::renameFile()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
    wxFileName target_filename = data->filename;
    wxFileName source_filename = target_filename;
    wxTextEntryDialog textentry(this, _guilang.get("GUI_DLG_RENAME_QUESTION"), _guilang.get("GUI_DLG_RENAME"), target_filename.GetName());
    int retval = textentry.ShowModal();

    if (retval == wxID_CANCEL)
        return;

    target_filename.SetName(textentry.GetValue());

    if (wxFileExists(target_filename.GetFullPath()))
    {
        wxMessageBox(_guilang.get("GUI_DLG_RENAME_ERROR"), _guilang.get("GUI_DLG_RENAME"), wxCENTRE | wxICON_ERROR | wxOK, this);
        return;
    }

    VersionControlSystemManager manager(this);

    if (manager.hasRevisions(source_filename.GetFullPath()))
    {
        std::unique_ptr<FileRevisions> revisions(manager.getRevisions(source_filename.GetFullPath()));

        if (revisions.get())
            revisions->renameFile(source_filename.GetFullName(), target_filename.GetFullName(), manager.getRevisionPath(target_filename.GetFullPath()));
    }

    wxRenameFile(source_filename.GetFullPath(), target_filename.GetFullPath());
    UpdateLocationIfOpen(source_filename, target_filename);
}


/////////////////////////////////////////////////
/// \brief The member function copies the path of
/// the selected tree item to the clipboard, so
/// that it can be inserted in scripts and
/// similar.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnCopyAsPath()
{
    std::string fileName = replacePathSeparator(getTreePath(m_clickedTreeItem).ToStdString());

    if (fileName.length() && wxTheClipboard->Open())
    {
        // Add the path to the clipboard
        wxTheClipboard->SetData(new wxTextDataObject("\"" + fileName + "\""));
        wxTheClipboard->Close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function uses the Windows
/// shell to open the selected folder in the
/// Windows explorer.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnOpenInExplorer()
{
    wxString fileName = getTreeFolderPath(m_clickedTreeItem);

    if (fileName.length())
        ShellExecute(nullptr, nullptr, fileName.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}


/////////////////////////////////////////////////
/// \brief This method displays the revision
/// dialog for the selected tree item.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnShowRevisions()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
    wxString filename = data->filename;
    VersionControlSystemManager manager(this);
    FileRevisions* revisions = manager.getRevisions(filename);

    // Only display the dialog, if the FileRevisions object exists
    if (revisions)
    {
        RevisionDialog* dialog = new RevisionDialog(this, revisions, filename);
        dialog->Show();
    }
}


/////////////////////////////////////////////////
/// \brief This method displays the revision
/// dialog for the selected tab item.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnShowRevisionsFromTab()
{
    NumeReEditor* edit = m_book->getEditor(GetIntVar(VN_CLICKEDTAB));
    wxString filename = edit->GetFileNameAndPath();
    VersionControlSystemManager manager(this);
    FileRevisions* revisions = manager.getRevisions(filename);

    // Only display the dialog, if the FileRevisions object exists
    if (revisions)
    {
        RevisionDialog* dialog = new RevisionDialog(this, revisions, edit->GetFileNameAndPath());
        dialog->Show();
    }
}


/////////////////////////////////////////////////
/// \brief This method allows the user to tag the
/// current active revision of a file.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnTagCurrentRevision()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
    wxString filename = data->filename;
    VersionControlSystemManager manager(this);
    std::unique_ptr<FileRevisions> revisions(manager.getRevisions(filename));

    // Only do something, if the FileRevisions object exists
    if (revisions.get())
    {
        // Display a text entry dialog to provide the user to
        // enter a comment for the new tag
        wxTextEntryDialog textdialog(this, _guilang.get("GUI_DLG_REVISIONDIALOG_PROVIDETAGCOMMENT"), _guilang.get("GUI_DLG_REVISIONDIALOG_PROVIDETAGCOMMENT_TITLE"), wxEmptyString, wxCENTER | wxOK | wxCANCEL);
        int ret = textdialog.ShowModal();

        if (ret == wxID_OK)
        {
            // Create the tag, if the user clicked on OK
            revisions->tagRevision(revisions->getCurrentRevision(), textdialog.GetValue());
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates a new
/// folder below the currently selected folder.
/// The name is supplied by the user via a text
/// entry dialog.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnCreateNewFolder()
{
    wxString fileName = getTreeFolderPath(m_clickedTreeItem);

    if (!fileName.length())
        return;

    wxTextEntryDialog textentry(this, _guilang.get("GUI_DLG_NEWFOLDER_QUESTION"), _guilang.get("GUI_DLG_NEWFOLDER"), _guilang.get("GUI_DLG_NEWFOLDER_DFLT"));
    int retval = textentry.ShowModal();

    if (retval == wxID_CANCEL)
        return;

    if (textentry.GetValue().length())
    {
        wxString foldername = fileName + "\\" + textentry.GetValue();
        wxMkdir(foldername);
    }
}


/////////////////////////////////////////////////
/// \brief This member function moves the selected
/// directory directly to the Windows trash bin,
/// if the user confirms the opened dialog.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnRemoveFolder()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));

    if (wxYES != wxMessageBox(_guilang.get("GUI_DLG_DELETE_QUESTION", data->filename.ToStdString()), _guilang.get("GUI_DLG_DELETE"), wxCENTRE | wxICON_QUESTION | wxYES_NO, this))
        return;

    if (m_clickedTreeItem == m_copiedTreeItem)
        m_copiedTreeItem = 0;

    Recycler _recycler;
    _recycler.recycle(data->filename.ToStdString().c_str());
    return;
}


/////////////////////////////////////////////////
/// \brief This member function evaluates the
/// command line passed to this application at
/// startup and evaluates, what to do with the
/// passed arguments.
///
/// \param wxArgV wxArrayString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::EvaluateCommandLine(wxArrayString& wxArgV)
{
    g_logger.info("Evaluating command line arguments.");
    wxArrayString filestoopen;
    wxString ext;

    for (size_t i = 1; i < wxArgV.size(); i++)
    {
        if (wxArgV[i].find('.') == std::string::npos)
            continue;

        if (wxArgV[i].find(".exe") != std::string::npos)
            continue;

        ext = toLowerCase(wxArgV[i].substr(wxArgV[i].rfind('.')).ToStdString());

        // Scripts: run or open?
        if (ext == ".nscr")
        {
            if (i+1 < wxArgV.size() && wxArgV[i+1] == "-e")
            {
                m_terminal->pass_command("start \"" + replacePathSeparator(wxArgV[i].ToStdString()) + "\"", false);
                i++;
            }
            else
                filestoopen.Add(wxArgV[i]);
        }

        // Window-Layouts: run or open?
        if (ext == ".nlyt")
        {
            if (i+1 < wxArgV.size() && wxArgV[i+1] == "-e")
            {
                m_terminal->pass_command("window \"" + replacePathSeparator(wxArgV[i].ToStdString()) + "\"", false);
                i++;
            }
            else
                filestoopen.Add(wxArgV[i]);
        }

        // Package creator projects
        if (ext == ".npkp")
            OnCreatePackage(wxArgV[i]);

        // Procedures: run or open?
        if (ext == ".nprc")
        {
            if (i+1 < wxArgV.size() && wxArgV[i+1] == "-e")
            {
                m_terminal->pass_command("$'" + replacePathSeparator(wxArgV[i].substr(0, wxArgV[i].rfind('.')).ToStdString()) + "'()", false);
                i++;
            }
            else
                filestoopen.Add(wxArgV[i]);
        }

        // Usual text files
        if (ext == ".dat"
            || ext == ".nhlp"
            || ext == ".xml"
            || ext == ".txt"
            || ext == ".tex")
            filestoopen.Add(wxArgV[i]);

        // Data files
        if (ext == ".ods"
            || ext == ".ibw"
            || ext == ".csv"
            || ext == ".jdx"
            || ext == ".jcm"
            || ext == ".dx"
            || ext == ".xls"
            || ext == ".xlsx"
            || ext == ".labx"
            || ext == ".ndat")
            m_terminal->pass_command("append \"" + replacePathSeparator(wxArgV[i].ToStdString()) + "\"", false);
    }

    if (filestoopen.size())
        OpenSourceFile(filestoopen);
}


//////////////////////////////////////////////////////////////////////////////
///  private NewFile
///  Creates a new empty editor and adds it to the editor notebook
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::NewFile(FileFilterType _filetype, const wxString& defaultfilename)
{
    if (!m_fileNum)
    {
        DefaultPage();
        return;
    }

    if (_filetype == FILE_NONSOURCE)
    {
        m_fileNum += 1;

        //wxString locationPrefix = "(?) ";

        wxString noname = _guilang.get("GUI_NEWFILE_UNTITLED") + " " + wxString::Format ("%d", m_fileNum);
        NumeReEditor* edit = m_book->createEditor(noname);
        //edit->SetSyntax(m_terminal->getSyntax());

    #if wxUSE_DRAG_AND_DROP
        edit->SetDropTarget(new NumeReDropTarget(this, edit, NumeReDropTarget::EDITOR));
    #endif

        edit->SetText("\r\n");
        int settings = CopyEditorSettings(_filetype);

        edit->EmptyUndoBuffer();
        m_currentPage = m_book->GetPageCount()-1;
        edit->ToggleSettings(settings);
        m_book->ChangeSelection(m_currentPage);
        m_book->SetSelection(m_currentPage);
    }
    else if (_filetype == FILE_DIFF)
    {
        wxString filename = defaultfilename;

        std::vector<std::string> vPaths = m_terminal->getPathSettings();

        m_fileNum += 1;

        // Create a new editor
        NumeReEditor* edit = m_book->createEditor("");
        edit->SetText("DIFF");
        int settings = CopyEditorSettings(_filetype);

        // Set the corresponding full file name
        edit->SetFilename(wxFileName(vPaths[SAVEPATH], filename), false);

        m_currentPage = m_book->GetPageCount()-1;
        edit->UpdateSyntaxHighlighting();

        // Add a new tab for the editor
        m_book->SetTabText(m_currentPage, edit->GetFileNameAndPath());
        edit->ToggleSettings(settings);
        m_book->ChangeSelection(m_currentPage);
        m_book->SetSelection(m_currentPage);
    }
    else
    {
        std::string filename;
        std::string folder;
        wxTextEntryDialog* textentry;
        bool isExternal = false;
        std::vector<std::string> vPaths = m_terminal->getPathSettings();

        // If no default file name was passed, ask
        // the user
        if (!defaultfilename.length())
        {
            if (_filetype == FILE_NSCR)
                textentry = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_NEWNSCR_QUESTION"), _guilang.get("GUI_DLG_NEWNSCR"), _guilang.get("GUI_DLG_NEWNSCR_DFLT"));
            else if (_filetype == FILE_NLYT)
                textentry = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_NEWNLYT_QUESTION"), _guilang.get("GUI_DLG_NEWNLYT"), _guilang.get("GUI_DLG_NEWNLYT_DFLT"));
            else if (_filetype == FILE_NPRC)
                textentry = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_NEWNPRC_QUESTION"), _guilang.get("GUI_DLG_NEWNPRC"), _guilang.get("GUI_DLG_NEWNPRC_DFLT"));
            else
                textentry = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_NEWPLUGIN_QUESTION"), _guilang.get("GUI_DLG_NEWPLUGIN"), _guilang.get("GUI_DLG_NEWPLUGIN_DFLT"));

            int retval = textentry->ShowModal();

            if (retval == wxID_CANCEL)
            {
                delete textentry;
                return;
            }

            // Get the file name, if the user didn't hit "Cancel"
            filename = textentry->GetValue();
            delete textentry;
        }
        else
        {
            filename = defaultfilename.ToStdString();
            isExternal = folder.substr(0, vPaths[PROCPATH].length()) != vPaths[PROCPATH];
        }

        // Remove the dollar sign, if there is one
        if (filename.find('$') != std::string::npos)
            filename.erase(filename.find('$'),1);

        // Replace all separators to UNIX separators
        replaceAll(filename, "~", "/");
        replaceAll(filename, "\\", "/");

        // Remove the path parts from the file name
        if (filename.find('/') != std::string::npos)
        {
            folder = filename.substr(0, filename.rfind('/')+1);
            filename.erase(0, filename.rfind('/')+1);
        }

        if (folder == "main/" && _filetype == FILE_NPRC)
            folder.clear();
        else if (!isExternal)
            folder.insert(0, "/");

        // Clean the file and folder names for procedures -
        // we only allow alphanumeric characters in non-external procedures
        if (_filetype == FILE_NPRC && !isExternal)
        {
            // Clean the folders after the root path
            for (size_t i = vPaths[PROCPATH].length(); i < folder.length(); i++)
            {
                if (!isalnum(folder[i]) && folder[i] != '_' && folder[i] != '/' && folder[i] != ':')
                    folder[i] = '_';
            }

            // Clean the file
            for (size_t i = 0; i < filename.length(); i++)
            {
                if (!isalnum(filename[i]) && filename[i] != '_')
                    filename[i] = '_';
            }
        }

        // Prepare the template file
        wxString template_file, dummy, timestamp;

        // Search the correct filename
        if (_filetype == FILE_NSCR)
            dummy = "tmpl_script.nlng";
        else if (_filetype == FILE_NLYT)
            dummy = "tmpl_layout.nlng";
        else if (_filetype == FILE_PLUGIN)
            dummy = "tmpl_plugin.nlng";
        else
            dummy = "tmpl_procedure.nlng";

        timestamp = getTimeStamp(false);

        // Get the template file contents
        if (m_options->useCustomLangFiles() && wxFileExists(getProgramFolder() + "\\user\\lang\\"+dummy))
            GetFileContents(getProgramFolder() + "\\user\\lang\\"+dummy, template_file, dummy);
        else
            GetFileContents(getProgramFolder() + "\\lang\\"+dummy, template_file, dummy);

        // Replace the tokens in the file
        template_file.Replace("%%1%%", filename);
        template_file.Replace("%%2%%", timestamp);

        // Determine the file extension
        if (_filetype == FILE_NSCR)
            filename += ".nscr";
        else if (_filetype == FILE_NLYT)
            filename += ".nlyt";
        else if (_filetype == FILE_PLUGIN)
            filename = "plgn_" + filename + ".nscr";
        else if (_filetype == FILE_NPRC)
            filename += ".nprc";

        m_fileNum += 1;

        // Create a new editor
        NumeReEditor* edit = m_book->createEditor("");
        edit->SetText(template_file);

        int settings = CopyEditorSettings(_filetype);

        // Set the corresponding full file name
        if (_filetype == FILE_NSCR || _filetype == FILE_PLUGIN || _filetype == FILE_NLYT)
            edit->SetFilename(wxFileName(vPaths[SCRIPTPATH] + folder, filename), false);
        else if (_filetype == FILE_NPRC)
            edit->SetFilename(isExternal ? wxFileName(folder, filename) : wxFileName(vPaths[PROCPATH] + folder, filename), false);
        else
            edit->SetFilename(wxFileName(vPaths[SAVEPATH] + folder, filename), false);

        m_currentPage = m_book->GetPageCount()-1;
        edit->UpdateSyntaxHighlighting();

        // Jump to the predefined template position
        edit->GotoPipe();

        edit->SetUnsaved();
        edit->EmptyUndoBuffer();

        // Add a new tab for the editor
        m_book->SetTabText(m_currentPage, edit->GetFileNameAndPath());
        edit->ToggleSettings(settings);
        m_book->ChangeSelection(m_currentPage);
        m_book->SetSelection(m_currentPage);
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates a new
/// editor page and copies the passed revision
/// contents to this page.
///
/// \param revisionName const wxString&
/// \param revisionContent const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::ShowRevision(const wxString& revisionName, const wxString& revisionContent)
{
    NewFile(FILE_DIFF, revisionName);
    NumeReEditor* edit = m_book->getCurrentEditor();
    edit->SetText(revisionContent);
    edit->EmptyUndoBuffer();
    edit->SetUnsaved();
    edit->UpdateSyntaxHighlighting(true);
}


/////////////////////////////////////////////////
/// \brief This member function returns the settings
/// from the current editor for the passed
/// FileFilterType.
///
/// \param _fileType FileFilterType
/// \return int
///
/////////////////////////////////////////////////
int NumeReWindow::CopyEditorSettings(FileFilterType _fileType)
{
    if (m_book->getCurrentEditor() && !m_loadingFilesDuringStartup)
    {
        int settings = m_book->getCurrentEditor()->getSettings();

        if (_fileType != FILE_NSCR && _fileType != FILE_NPRC && _fileType != FILE_MATLAB && _fileType != FILE_PLUGIN && _fileType != FILE_XML)
        {
            if (settings & NumeReEditor::SETTING_INDENTONTYPE)
                settings &= ~NumeReEditor::SETTING_INDENTONTYPE;

            if (settings & NumeReEditor::SETTING_USEANALYZER)
                settings &= ~NumeReEditor::SETTING_USEANALYZER;
        }
        else
        {
            if (settings & NumeReEditor::SETTING_USETXTADV)
                settings &= ~NumeReEditor::SETTING_USETXTADV;
        }

        return settings;
    }

    return 0;
}


/////////////////////////////////////////////////
/// \brief This member function creates a new
/// editor page and copies the contents of the
/// default page template to this page.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::DefaultPage()
{
    wxString template_file, dummy;
    dummy = "tmpl_defaultpage.nlng";

    if (m_options->useCustomLangFiles() && wxFileExists(getProgramFolder() + "\\user\\lang\\"+dummy))
        GetFileContents(getProgramFolder() + "\\user\\lang\\"+dummy, template_file, dummy);
    else
        GetFileContents(getProgramFolder() + "\\lang\\"+dummy, template_file, dummy);

    m_fileNum += 1;

    NumeReEditor* edit = m_book->createEditor(_guilang.get("GUI_EDITOR_TAB_WELCOMEPAGE"));

#if wxUSE_DRAG_AND_DROP
    edit->SetDropTarget(new NumeReDropTarget(this, edit, NumeReDropTarget::EDITOR));
#endif

    edit->LoadFileText(template_file);
    edit->defaultPage = true;
    edit->SetReadOnly(true);
    edit->ToggleSettings(NumeReEditor::SETTING_USETXTADV);
    m_currentPage = m_book->GetPageCount()-1;
    m_book->SetSelection(m_currentPage);
}


//////////////////////////////////////////////////////////////////////////////
///  private PageHasChanged
///  Called whenever the active tab has changed, updating the active editor pointer
///
///  @param  pageNr int  [=-1] The index of the newly selected page
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::PageHasChanged (int pageNr)
{
    // no pages - null out the current ed pointer
    // You know, this should really never happen with the current design
    // 3/11/04: Unless, of course, we're closing out the program...
    if (m_book->GetPageCount() == 0)
    {
        m_currentPage = -1;
        m_procedureViewer->setCurrentEditor(nullptr);
        return;
    }

    // no page passed in
    if (pageNr == -1)
    {
        pageNr = m_book->GetSelection();
    }

    if ((int)m_book->GetPageCount() <= pageNr)
    {
        pageNr = m_book->GetPageCount() - 1;
    }

    // activate the selected page
    if (pageNr >= 0)
    {
        NumeReEditor* edit = m_book->getCurrentEditor();

        if (edit->AutoCompActive())
            edit->AutoCompCancel();

        if (edit->CallTipActive())
            edit->AdvCallTipCancel();

        m_currentPage = pageNr;
        edit = m_book->getEditor(m_currentPage); // might need adaptions for the procedure viewer
        m_book->SetSelection(pageNr);

        //if (!m_book->GetMouseFocus())
            edit->SetFocus();
    }
    else
    {
        m_currentPage = -1;
    }

    // Set the current editor in the procedure viewer,
    // but avoid refreshing during closing the application
    if (m_appClosing || m_loadingFilesDuringStartup)
        m_procedureViewer->setCurrentEditor(nullptr);
    else
        m_procedureViewer->setCurrentEditor(m_book->getCurrentEditor());

    m_book->Refresh();

    if (m_book->getCurrentEditor() != nullptr)
    {
        NumeReEditor* edit = m_book->getCurrentEditor();

        m_menuItems[ID_MENU_LINEWRAP]->Check(edit->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
        m_menuItems[ID_MENU_DISPCTRLCHARS]->Check(edit->getEditorSetting(NumeReEditor::SETTING_DISPCTRLCHARS));
        m_menuItems[ID_MENU_USETXTADV]->Check(edit->getEditorSetting(NumeReEditor::SETTING_USETXTADV));
        m_menuItems[ID_MENU_USESECTIONS]->Check(edit->getEditorSetting(NumeReEditor::SETTING_USESECTIONS));
        m_menuItems[ID_MENU_USEANALYZER]->Check(edit->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
        m_menuItems[ID_MENU_INDENTONTYPE]->Check(edit->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));

        edit->Refresh();

        wxString tabText = m_book->GetPageText(m_currentPage);
        // set the title of the main window according the current opened file
        UpdateWindowTitle(tabText);

        // else assume unsaved file and don't change anything
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private CloseTab
///  Closes a tab after the user right-clicks it and selects "Close"
///
///  @param force bool
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::CloseTab(bool force)
{
    int tab = GetIntVar(VN_CLICKEDTAB);
    CloseFile(tab, !force);
    m_book->Refresh();
}


/////////////////////////////////////////////////
/// \brief This member function closes all other
/// editor tabs except of the current selected
/// one.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::CloseOtherTabs()
{
    int tab = GetIntVar(VN_CLICKEDTAB);

    // Close all pages left from the current tab
    while (tab)
    {
        CloseFile(0);
        tab--;
    }

    // Close all pages right from the current tab
    while (m_book->GetPageCount() > 1)
        CloseFile(1);

    m_book->Refresh();
}


/////////////////////////////////////////////////
/// \brief This member function uses the Windows
/// shell to open the containing folder of the
/// selected tab in the Windows Explorer.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OpenContainingFolder()
{
    int tab = GetIntVar(VN_CLICKEDTAB);
    NumeReEditor* edit = m_book->getEditor(tab);
    wxFileName filename = edit->GetFileName();
    wxExecute("explorer " + filename.GetPath(), wxEXEC_ASYNC, nullptr, nullptr);
}


/////////////////////////////////////////////////
/// \brief This member function executes the
/// contents of the editor page connected to the
/// selected tab.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::EvaluateTab()
{
    int tab = GetIntVar(VN_CLICKEDTAB);
    NumeReEditor* edit = m_book->getEditor(tab);

    if (!edit->HasBeenSaved() || edit->Modified())
    {
        int result = HandleModifiedFile(tab, MODIFIEDFILE_COMPILE);

        if (result == wxCANCEL)
            return;

        if (m_book->getCurrentEditor()->getFileType() == FILE_NPRC
            || m_book->getCurrentEditor()->getFileType() == FILE_NLYT
            || m_book->getCurrentEditor()->getFileType() == FILE_NSCR)
        {
            m_terminal->UpdateLibrary();
        }
    }

    std::string command = replacePathSeparator((edit->GetFileName()).GetFullPath().ToStdString());
    OnExecuteFile(command, 0);
}


/////////////////////////////////////////////////
/// \brief Closes a given editor, based on its
/// index in the editor notebook.
///
/// \param pageNr int
/// \param askforsave bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::CloseFile(int pageNr, bool askforsave)
{
    if (pageNr == -1)
    {
        pageNr = m_book->GetSelection();
    }

    if (askforsave)
    {
        // gives the user a chance to save if the file has been modified
        int modifiedFileResult = HandleModifiedFile(pageNr, MODIFIEDFILE_CLOSE);

        // a wxYES result is taken care of inside HandleModifiedFile, and a
        // wxNO is handled implicitly by the fact that the file isn't saved.
        if(modifiedFileResult == wxCANCEL)
        {
            return;
        }
    }

    if (m_book->GetPageCount() > 0)
    {
        NumeReEditor* edit = m_book->getCurrentEditor();

        g_logger.info("Closing file '" + edit->GetFileNameAndPath().ToStdString() + "'.");
        wxFileName currentFileName;
        m_terminal->clearBreakpoints(edit->GetFileNameAndPath().ToStdString());

        if ((m_book->GetPageCount() > 1) || m_appClosing)
        {
            currentFileName = edit->GetFileName();
            //NumeReEditor* pEdit = static_cast <NumeReEditor* >(m_book->GetPage(pageNr));
            m_book->DeletePage (pageNr);
            m_watcher->Remove(currentFileName);
        }
        // closing out the last buffer, reset it to act as a new one
        else
        {
            m_fileNum = 1;
            m_watcher->Remove(edit->GetFileName());
            //wxString locationPrefix = "(?) ";
            wxString noname = _guilang.get("GUI_NEWFILE_UNTITLED") + " " + wxString::Format ("%d", m_fileNum);
            m_book->SetPageText (pageNr, noname);
            edit->ResetEditor();
            edit->SetText("\r\n");
            edit->EmptyUndoBuffer();
        }

        if(m_book->GetPageCount() > 0)
        {
            if(currentFileName.IsOk())
            {
                int newSelectedPageNum = GetPageNum(currentFileName);
                PageHasChanged(newSelectedPageNum);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private CloseAllFiles
///  Closes all open files
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReWindow::CloseAllFiles()
{
    int cnt = m_book->GetPageCount();
    std::ofstream of_session;
    NumeReEditor* edit;
    std::string sSession = "# numere.session: Session save file. Do not edit!\n# ACTIVEFILE\tFILEID\nACTIVEFILEID\t" + toString(m_book->GetSelection()) + "\n# FILENAME\t\tFILEID\t\tCHARPOSITION\t\tSETTING\t\tBOOKMARKS\n";

    for (int i = 0; i < cnt; i++)
    {
        edit = m_book->getEditor(i);

        if (edit->defaultPage)
            continue;

        // gives the user a chance to save if the file has been modified
        int nReturn = HandleModifiedFile(i, MODIFIEDFILE_CLOSE);

        if (nReturn == wxCANCEL)
            return false;

        if (edit->Modified())
            sSession += "*";

        if (edit->GetFileNameAndPath().length())
        {
            g_logger.info("Adding file '" + edit->GetFileNameAndPath().ToStdString() + "' to the session backup.");
            sSession += edit->GetFileNameAndPath().ToStdString()
                        + "\t" + toString(i)
                        + "\t" + toString(edit->GetCurrentPos())
                        + "\t" + toString(edit->getSettings())
                        + (m_options->GetSaveBookmarksInSession() ? "\t" + toString(edit->getBookmarks()) + "\n" : "\n");
        }
        else
        {
            sSession += "<NEWFILE>\t" +toString(i) + "\t" + toString(edit->GetCurrentPos()) + "\n";
        }
    }

    g_logger.debug("Closing files.");

    for (int i = 0; i < cnt; i++)
    {
        CloseFile(-1, false);
    }

    if (m_appClosing && !m_sessionSaved && m_options->GetSaveSession())
    {
        g_logger.debug("Writing session file.");
        of_session.open((getProgramFolder().ToStdString()+"/numere.session").c_str(), std::ios_base::out | std::ios_base::trunc);

        if (of_session.is_open())
        {
            of_session << sSession;
        }

        m_sessionSaved = true;
    }

    PageHasChanged();

    return true;
}


//////////////////////////////////////////////////////////////////////////////
///  private GetPageNum
///  Searches through the open editors to find an editor with the given name
///
///  @param  fn               wxFileName  The editor's filename to find
///  @param  compareWholePath bool        [=true] Compare the entire filename + path, or just the name?
///  @param  startingTab      int         [=0] The index of the first tab to search
///
///  @return int              The index of the located editor, or -1 if not found
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int NumeReWindow::GetPageNum(wxFileName fn,  bool compareWholePath, int startingTab)
{
    NumeReEditor *edit;
    int numPages = m_book->GetPageCount();
    wxString filename = fn.GetFullName();

    for (int pageNum = startingTab; pageNum < numPages; pageNum++)
    {
        edit = m_book->getEditor(pageNum);

        bool fileMatches = false;

        if(compareWholePath)
        {
            fileMatches = (edit->GetFileName() == fn);
        }
        else
        {
            fileMatches = (edit->GetFilenameString() == filename);
        }
        if (fileMatches)
        {
            return pageNum;
        }
    }

    return -1;
}


//////////////////////////////////////////////////////////////////////////////
///  private OnPageChange
///  Event handler called when the user clicks a different tab
///
///  @param  event wxNotebookEvent & The notebook event (not used)
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnPageChange (wxAuiNotebookEvent& event)
{
    if (!m_setSelection)
    {
        ToolbarStatusUpdate();
        PageHasChanged();
    }
}


/////////////////////////////////////////////////
/// \brief Handle user interaction when closing
/// or reloading an editor.
///
/// \param pageNr int
/// \param fileAction ModifiedFileAction
/// \return int What the user chose to do. The
/// only real meaningful return value is wxCANCEL.
///
/////////////////////////////////////////////////
int NumeReWindow::HandleModifiedFile(int pageNr, ModifiedFileAction fileAction)
{
    NumeReEditor *edit = m_book->getEditor(pageNr);

    if (!edit)
    {
        return wxCANCEL;
    }

    if (edit->Modified())
    {
        wxString saveMessage = "The file ";
        wxString fileName = edit->GetFileNameAndPath();

        // the file hasn't been saved yet, grab the "<untitled> #" bit from the tab
        if(fileName == wxEmptyString)
        {
            int selectedTab = m_book->GetSelection();
            wxString tabText = m_book->GetPageText(selectedTab);

            int idx = tabText.Index('*');

            if(idx != wxNOT_FOUND)
            {
                tabText.Remove(idx, 1);
            }

            // remove the (R), (L), or (?) from the tab
            wxRegEx reLocationPrefix("\\((R|L|\\?)\\) ");
            reLocationPrefix.Replace(&tabText, wxEmptyString);

            fileName = tabText;
        }
        //saveMessage += fileName;

        //saveMessage << " has unsaved changes. ";
        saveMessage = _guilang.get("GUI_UNSAVEDFILE", fileName.ToStdString());
        /*
        if(closingFile)
        {
            saveMessage += "closed?";
        }
        else
        {
            saveMessage += "reloaded?";
        }
        */

        switch(fileAction)
        {
            case MODIFIEDFILE_CLOSE:
            {
                saveMessage += " " +_guilang.get("GUI_UNSAVEDFILE_CLOSE");//"Do you want to save them before the file is closed?";
                break;
            }
            case MODIFIEDFILE_RELOAD:
            {
                saveMessage += "Do you want to save them before the file is reloaded?";
                break;
            }
            case MODIFIEDFILE_COMPILE:
            {
                saveMessage += " " + _guilang.get("GUI_UNSAVEDFILE_EXECUTE");
                break;
            }
        }

        int options = wxYES_NO | wxICON_QUESTION | wxCANCEL;

        /*if(fileAction != MODIFIEDFILE_COMPILE)
        {
            options |= wxCANCEL;
        }*/

        int result = m_options->getSetting(SETTING_B_AUTOSAVEEXECUTION).active() ? wxYES : wxNO;

        if (fileAction != MODIFIEDFILE_COMPILE || !m_options->getSetting(SETTING_B_AUTOSAVEEXECUTION).active())
            result = wxMessageBox (saveMessage, _guilang.get("GUI_SAVE_QUESTION"), options);//wxYES_NO | wxCANCEL | wxICON_QUESTION);

        if (result == wxYES)
        {
            size_t selection = m_book->GetSelection();
            m_book->ChangeSelection(pageNr);
            // only do a Save As if necessary
            SaveCurrentFile(false);
            m_book->ChangeSelection(selection);
            m_book->getCurrentEditor()->SetFocus();

            if (edit->Modified())
            {
                wxString errorMessage = fileName + " could not be saved!";
                wxMessageBox (errorMessage, "File not closed",
                    wxOK | wxICON_EXCLAMATION);
                m_book->getCurrentEditor()->Refresh();
            }
        }

        return result;
    }
    // if I'm here, doesn't matter if I return wxNO or wxYES, just as long as it's not wxCANCEL
    return wxNO;
}


//////////////////////////////////////////////////////////////////////////////
///  private OpenFile
///  Shows a file dialog and returns a list of files to open.  Abstracts out local/remote file dialogs.
///
///  @param  filterType    FileFilterType  The type of files to show in the file dialog
///
///  @return wxArrayString The filenames to open
///
///  @remarks Currently, only one filename can be opened at a time (mostly due to the
///  @remarks fact that the RemoteFileDialog has that limitation).  OpenSourceFile
///  @remarks has the logic to handle multiple filenames, so this could be added
///  @remarks without too much difficulty.
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxArrayString NumeReWindow::OpenFile(FileFilterType filterType)
{
    wxArrayString fnames;

    if (!m_book->getCurrentEditor())
        return fnames;

    wxString filterString = ConstructFilterString(filterType);

    wxFileDialog dlg (this, _(_guilang.get("GUI_DLG_OPEN")), "", "", filterString, wxFD_OPEN | wxFD_FILE_MUST_EXIST  | wxFD_CHANGE_DIR);

    if (dlg.ShowModal() != wxID_OK)
        return fnames;

    m_book->getCurrentEditor()->SetFocus();
    dlg.GetPaths (fnames);

    return fnames;
}


/////////////////////////////////////////////////
/// \brief This member function opens the file
/// with the passed name in the corresponding
/// widget (either editor, ImageViewer or
/// externally).
///
/// \param filename const wxFileName&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OpenFileByType(const wxFileName& filename)
{
    if (filename.GetExt() == "nscr"
        || filename.GetExt() == "nprc"
        || filename.GetExt() == "nhlp"
        || filename.GetExt() == "nlng"
        || filename.GetExt() == "nlyt"
        || filename.GetExt() == "txt"
        || filename.GetExt() == "dat"
        || filename.GetExt() == "log"
        || filename.GetExt() == "m"
        || filename.GetExt() == "cpp"
        || filename.GetExt() == "cxx"
        || filename.GetExt() == "c"
        || filename.GetExt() == "hpp"
        || filename.GetExt() == "hxx"
        || filename.GetExt() == "h"
        || filename.GetExt() == "xml"
        || filename.GetExt() == "tex"
        || filename.GetExt() == "diff")
    {
        wxArrayString filesToOpen;
        filesToOpen.Add(filename.GetFullPath());
        OpenSourceFile(filesToOpen);
        CallAfter(&NumeReWindow::setEditorFocus);
    }
    else if (filename.GetExt() == "png"
        || filename.GetExt() == "jpeg"
        || filename.GetExt() == "jpg"
        || filename.GetExt() == "bmp"
        || filename.GetExt() == "gif"
        || filename.GetExt() == "tif"
        || filename.GetExt() == "tiff")
    {
        openImage(filename);
        CallAfter(&NumeReWindow::setViewerFocus);
    }
    else if (filename.GetExt() == "pdf")
        openPDF(filename);
    else if (filename.GetExt() == "npkp")
        OnCreatePackage(filename.GetFullPath());
    else
    {
        wxString path = "load \"" + replacePathSeparator(filename.GetFullPath().ToStdString()) + "\" -app -ignore";
        showConsole();
        m_terminal->pass_command(path.ToStdString(), false);
    }
}


/////////////////////////////////////////////////
/// \brief This member function opens a list of
/// files depending on their type in the correct
/// widget.
///
/// \param filenameslist const wxArrayString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OpenFilesFromList(const wxArrayString& filenameslist)
{
    for (size_t i = 0; i < filenameslist.size(); i++)
    {
        OpenFileByType(wxFileName(filenameslist[i]));
    }
}


/////////////////////////////////////////////////
/// \brief Opens the given list of source files
/// in the editor.
///
/// \param fnames wxArrayString
/// \param nLine unsigned int The line to jump to
/// \param nOpenFileFlag int
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OpenSourceFile(wxArrayString fnames, unsigned int nLine, int nOpenFileFlag)
{
    int firstPageNr = -1;
    wxString fileContents = wxEmptyString;
    wxString fileNameNoPath;

    for (size_t n = 0; n < fnames.GetCount(); n++)
    {
        fileNameNoPath = wxEmptyString;
        wxFileName newFileName(fnames[n]);
        int pageNr = GetPageNum(newFileName);

        if (!GetFileContents(fnames[n], fileContents, fileNameNoPath))
            return;

        if (nOpenFileFlag & OPENFILE_BLACKLIST_ADD)
            addToReloadBlackList(fnames[n]);

        if (nOpenFileFlag & OPENFILE_BLACKLIST_REMOVE)
            removeFromReloadBlackList(fnames[n]);

        // filename is already open
        if (pageNr >= 0)
        {
            if (nLine)
            {
                PageHasChanged(pageNr);
                m_book->getFocusedEditor()->FocusOnLine(nLine, true);
            }
            else
            {
                // Reload if required (or simply do nothing)
                if (nOpenFileFlag & (OPENFILE_FORCE | OPENFILE_BLACKLIST_REMOVE))
                {
                    m_setSelection = true;
                    m_book->SetSelection (pageNr);
                    m_setSelection = false;
                    m_currentPage = pageNr;

                    NumeReEditor* edit = m_book->getEditor(m_currentPage); // Needs work for the syntax highlighting
                    edit->LoadFileText(fileContents);
                    edit->UpdateSyntaxHighlighting();

                    if (m_book->getCurrentEditor(true))
                        m_book->getCurrentEditor(true)->UpdateSyntaxHighlighting();
                }
                else
                    PageHasChanged(pageNr);
            }
        }
        else
        {
            g_logger.info("Loading file '" + fnames[n].ToStdString() + "'.");
            FileFilterType _fileType;

            if (fnames[n].rfind(".nscr") != std::string::npos)
                _fileType = FILE_NSCR;
            else if (fnames[n].rfind(".nprc") != std::string::npos)
                _fileType = FILE_NPRC;
            else if (fnames[n].rfind(".xml") != std::string::npos || fnames[n].rfind(".nhlp") != std::string::npos)
                _fileType = FILE_XML;
            else
                _fileType = FILE_NOTYPE;

            NumeReEditor* edit = m_book->getCurrentEditor();

            // current buffer is empty and untouched, so load the file into it
            if ((!edit->Modified()
                && !edit->HasBeenSaved()
                && (edit->GetText().IsEmpty() || edit->GetText() == "\r\n")) || edit->defaultPage )
            {
                edit->LoadFileText(fileContents);
                edit->SetFilename(newFileName, m_remoteMode);
                edit->UpdateSyntaxHighlighting(true);
                m_book->SetTabText(m_currentPage, edit->GetFileNameAndPath());
            }
            // need to create a new buffer for the file
            else
            {
                NumeReEditor *edit = m_book->createEditor("");
#if wxUSE_DRAG_AND_DROP
                edit->SetDropTarget(new NumeReDropTarget(this, edit, NumeReDropTarget::EDITOR));
#endif
                edit->LoadFileText(fileContents);
                int settings = CopyEditorSettings(_fileType);
                m_currentPage = m_book->GetPageCount()-1;
                edit->SetFilename(newFileName, m_remoteMode);
                edit->UpdateSyntaxHighlighting(true);
                edit->ToggleSettings(settings);
                m_book->SetTabText(m_currentPage, edit->GetFileNameAndPath());
                m_book->SetSelection(m_currentPage);
            }

            if (m_options->GetFoldDuringLoading())
            {
                m_book->getCurrentEditor()->FoldAll();

                if (m_book->getCurrentEditor(true))
                    m_book->getCurrentEditor(true)->FoldAll();
            }

            m_watcher->Add(newFileName);

            if (nLine)
            {
                m_book->getFocusedEditor()->GotoLine(nLine);
                m_book->getFocusedEditor()->EnsureVisible(nLine);
            }
        }

        if (firstPageNr < 0)
            firstPageNr = m_currentPage;

    }

    // show the active tab, new or otherwise
    if (firstPageNr >= 0)
        PageHasChanged(firstPageNr);

    m_book->getCurrentEditor()->SetFocus();
}


//////////////////////////////////////////////////////////////////////////////
///  private GetFileContents
///  Gets the text of a source file.  Abstracts out opening local / remote files.
///
///  @param  fileToLoad   wxString   The name of the file to open
///  @param  fileContents wxString & Gets the contents of the opened file
///  @param  fileName     wxString & Gets the name of the file (no path)
///
///  @return bool         True if the open succeeded, false if it failed
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReWindow::GetFileContents(wxString fileToLoad, wxString &fileContents, wxString &fileName)
{
    wxFileName fn(fileToLoad);
	wxFile file(fileToLoad);

	if (!file.IsOpened())
	{
		return false;
	}

	long lng = file.Length();

	if (lng > 0)
	{
		file.ReadAll(&fileContents, wxConvAuto(wxFONTENCODING_CP1252));
	}

    fileName = fn.GetFullName();
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function gets the drag-drop
/// source item, if the source was the file tree.
///
/// \return wxTreeItemId
///
/////////////////////////////////////////////////
wxTreeItemId NumeReWindow::getDragDropSourceItem()
{
    wxTreeItemId retVal = m_dragDropSourceItem;
    m_dragDropSourceItem = wxTreeItemId();
    return retVal;
}


/////////////////////////////////////////////////
/// \brief This member function returns the paths
/// connected to a specific file or directory in
/// the file tree.
///
/// \param itemId const wxTreeItemId&
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReWindow::getTreePath(const wxTreeItemId& itemId)
{
    if (!itemId.IsOk())
        return wxString();

    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(itemId));
    wxString pathName;

    if (!data)
    {
        std::vector<std::string> vPaths = m_terminal->getPathSettings();

        for (size_t i = 0; i <= PLOTPATH-2; i++)
        {
            if (itemId == m_projectFileFolders[i])
            {
                pathName = vPaths[i+2];
                break;
            }
        }
    }
    else
        pathName = data->filename;

    return pathName;
}

/////////////////////////////////////////////////
/// \brief This member function returns the paths
/// connected to a specific directory in the file
/// tree.
///
/// \param itemId const wxTreeItemId&
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReWindow::getTreeFolderPath(const wxTreeItemId& itemId)
{
    if (!itemId.IsOk())
        return wxString();

    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(itemId));
    wxString pathName;

    if (!data)
    {
        std::vector<std::string> vPaths = m_terminal->getPathSettings();

        for (size_t i = 0; i <= PLOTPATH-2; i++)
        {
            if (itemId == m_projectFileFolders[i])
            {
                pathName = vPaths[i+2];
                break;
            }
        }
    }
    else if (data->isDir)
        pathName = data->filename;

    return pathName;
}


/////////////////////////////////////////////////
/// \brief This member function tells NumeRe that
/// it shall display the "ready" state to the user.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::Ready()
{
    if (m_statusBar)
        m_statusBar->Ready();

    if (m_debugViewer)
    {
        m_debugViewer->OnExecutionFinished();

        for (size_t i = 0; i < m_book->GetPageCount(); i++)
        {
            NumeReEditor* edit = m_book->getEditor(i);
            edit->MarkerDeleteAll(MARKER_FOCUSEDLINE);
        }
    }

    wxToolBar* tb = GetToolBar();
    tb->EnableTool(ID_MENU_EXECUTE, true);
    tb->EnableTool(ID_MENU_STOP_EXECUTION, false);

    CallAfter(NumeReWindow::UpdateVarViewer);
}


/////////////////////////////////////////////////
/// \brief This member function tells NumeRe that
/// it shall display the "busy" state to the user.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::Busy()
{
    if (m_statusBar)
        m_statusBar->Busy();

    wxToolBar* tb = GetToolBar();
    tb->EnableTool(ID_MENU_EXECUTE, false);
    tb->EnableTool(ID_MENU_STOP_EXECUTION, true);
}


/////////////////////////////////////////////////
/// \brief Saves the current opened file.
///
/// \param saveas True if this is explicitly a
/// "Save-As" command and a file dialog must be
/// shown
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReWindow::SaveCurrentFile(bool saveas)
{
    m_remoteMode = false;
    NumeReEditor* edit = m_book->getCurrentEditor();
    bool doSaveAs = saveas || !edit->HasBeenSaved();

    if (doSaveAs)
    {
        wxString filename;
        wxString filterString = ConstructFilterString(edit->getFileType());

        // the last item in a filter's list will be the default extension if none is given
        // ie, right now, .cpp is the default extension for C++ files
        wxString title = _guilang.get("GUI_DLG_SAVEAS");
        std::vector<std::string> vPaths = m_terminal->getPathSettings();
        int i = 0;

        if (edit->getFileType() == FILE_NSCR)
            i = SCRIPTPATH;
        else if (edit->getFileType() == FILE_NPRC)
            i = PROCPATH;
        else if (edit->getFileType() == FILE_DATAFILES
            || edit->getFileType() == FILE_TEXSOURCE
            || edit->getFileType() == FILE_NONSOURCE)
            i = SAVEPATH;

        // Create the file dialog
        wxFileDialog dlg (this, title, vPaths[i], edit->GetFileName().GetName(), filterString,
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);

        // Select the correct filter string depending on the
        // file extensions
        wxString sExt = edit->GetFileName().GetExt();

        if (edit->getFileType() == FILE_NSCR && sExt == "nlyt")
            dlg.SetFilterIndex(1);
        else if (edit->getFileType() == FILE_DATAFILES)
        {
            if (sExt == "dat")
                dlg.SetFilterIndex(0);
            else if (sExt == "txt")
                dlg.SetFilterIndex(1);
            else if (sExt == "csv")
                dlg.SetFilterIndex(2);
            else if (sExt == "jdx"
                || sExt == "dx"
                || sExt == "jcm")
                dlg.SetFilterIndex(3);
        }
        else if (edit->getFileType() == FILE_NONSOURCE)
        {
            if (sExt == "txt")
                dlg.SetFilterIndex(0);
            else if (sExt == "log")
                dlg.SetFilterIndex(1);
        }

        // ie, user clicked cancel
        if (dlg.ShowModal() != wxID_OK)
            return false;

        if (edit->GetFileName().IsOk())
            m_watcher->Remove(edit->GetFileName());

        filename = dlg.GetPath();
        m_watcher->Add(wxFileName(filename));

        // Append the correct file extension
        if (filename.find('.') == std::string::npos || filename.find('.', filename.rfind('\\')) == std::string::npos)
        {
            if (edit->getFileType() == FILE_NSCR)
                filename += ".nscr";
            else if (edit->getFileType() == FILE_NPRC)
                filename += ".nprc";
            else if (edit->getFileType() == FILE_DATAFILES)
            {
                if (edit->GetFilenameString().find('.') != std::string::npos)
                    filename += edit->GetFilenameString().substr(edit->GetFilenameString().rfind('.'));
                else
                    filename += ".dat";
            }
            else if (edit->getFileType() == FILE_TEXSOURCE)
                filename += ".tex";
            else if (edit->getFileType() == FILE_NONSOURCE)
            {
                if (edit->GetFilenameString().find('.') != std::string::npos)
                    filename += edit->GetFilenameString().substr(edit->GetFilenameString().rfind('.'));
                else
                    filename += ".txt";
            }
        }

        wxFileName fn(filename);
        edit->SetFilename(fn, false);

        if (m_book->getCurrentEditor(true))
            m_book->getCurrentEditor(true)->SetFilename(fn, false);
    }

    if (SaveTab(m_book->GetSelection()))
        UpdateWindowTitle(m_book->GetPageText(m_book->GetSelection()));

    return true;
}


/////////////////////////////////////////////////
/// \brief Saves the file in the selected editor
/// tab.
///
/// \param tab int
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReWindow::SaveTab(int tab)
{
    NumeReEditor* edit = m_book->getEditor(tab); // Might need adaption for the filename
    wxString filename = edit->GetFileNameAndPath();

    // Make the folder, if it doesn't exist
    std::string sPath = filename.ToStdString();
    sPath = replacePathSeparator(sPath);
    sPath.erase(sPath.rfind('/'));
    FileSystem _fSys;
    _fSys.setPath(sPath, true, getProgramFolder().ToStdString());

    m_filesLastSaveTime[filename] = time(0);

    if (!edit->SaveFile(filename))
    {
        wxMessageBox(_guilang.get("GUI_DLG_SAVE_ERROR"), _guilang.get("GUI_DLG_SAVE"), wxCENTRE | wxOK | wxICON_ERROR, this);
        return false;
    }

    edit->SetSavePoint();
    edit->UpdateSyntaxHighlighting();

    m_book->SetTabText(tab, filename);
    m_book->Refresh();

    return true;
}


/////////////////////////////////////////////////
/// \brief Saves all currently opened files to
/// the harddisk.
///
/// \param refreshLibrary bool
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReWindow::SaveAll(bool refreshLibrary)
{
    int currentTab = m_book->GetSelection();

    for (size_t i = 0; i < m_book->GetPageCount(); i++)
    {
        NumeReEditor* edit = m_book->getEditor(i);

        if (edit->Modified())
        {
            if (edit->HasBeenSaved())
            {
                if (SaveTab(i) && (int)i == currentTab)
                    UpdateWindowTitle(m_book->GetPageText(i));

                continue;
            }

            // Change with event
            m_book->SetSelection(i);
            SaveCurrentFile(false);
        }
    }

    // Change with event
    if (currentTab != m_book->GetSelection())
        m_book->SetSelection(currentTab);

    if (refreshLibrary)
    {
        std::vector<std::string> vPaths = m_terminal->getPathSettings();
        // Refresh lib
        CreateProcedureTree(vPaths[PROCPATH]);
        m_terminal->UpdateLibrary();
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function obtains the
/// contents of a file and
/// transforms it to be used by an installer
/// script.
///
/// \param sFileName const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReWindow::getFileForInstaller(const std::string& sFileName)
{
    std::ifstream file(sFileName.c_str());
    std::string sTargetFileName = sFileName;

    std::vector<std::string> vPaths = m_terminal->getPathSettings();
    std::vector<std::string> vContents;

    for (size_t i = LOADPATH; i < PATH_LAST; i++)
    {
        if (sTargetFileName.substr(0, vPaths[i].length()) == vPaths[i])
        {
            switch (i)
            {
            case LOADPATH:
                sTargetFileName = "<loadpath>" + sTargetFileName.substr(vPaths[i].length());
                break;
            case SAVEPATH:
                sTargetFileName = "<savepath>" + sTargetFileName.substr(vPaths[i].length());
                break;
            case SCRIPTPATH:
                sTargetFileName = "<scriptpath>" + sTargetFileName.substr(vPaths[i].length());
                break;
            case PROCPATH:
                sTargetFileName = "<procpath>" + sTargetFileName.substr(vPaths[i].length());
                break;
            case PLOTPATH:
                sTargetFileName = "<plotpath>" + sTargetFileName.substr(vPaths[i].length());
                break;
            }

            break;
        }
    }

    if (!file.good())
        return vContents;

    vContents.push_back("<file name=\"" + sTargetFileName + "\">");

    std::string currLine;

    while (!file.eof())
    {
        std::getline(file, currLine);
        vContents.push_back(currLine);
    }

    vContents.push_back("<endfile>");

    return vContents;
}


//////////////////////////////////////////////////////////////////////////////
///  private ConstructFilterString
///  Puts together the filter that defines what files are shown in a file dialog
///
///  @param  filterType FileFilterType  The type of filter to construct
///
///  @return wxString   The constructed filter string
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString NumeReWindow::ConstructFilterString(FileFilterType filterType)
{
    wxString filterString;
    switch(filterType)
    {
    case FILE_ALLSOURCETYPES:
        filterString = m_filterExecutableFiles;
        filterString += "|";
        filterString += m_filterNSCRFiles;
        filterString += "|";
        filterString += m_filterNPRCFiles;
        filterString += "|";
        filterString += m_filterNLYTFiles;
        break;
    case FILE_NSCR:
        filterString = m_filterNSCRFiles;
        filterString += "|" + m_filterNLYTFiles;
        break;
    case FILE_NPRC:
        filterString = m_filterNPRCFiles;
        break;
    case FILE_NUMERE:
        filterString = m_filterNumeReFiles;
        break;
    case FILE_IMAGEFILES:
        filterString = m_filterImageFiles;
        break;
    case FILE_DATAFILES:
        filterString = m_filterDataFiles;
        break;
    case FILE_TEXSOURCE:
        filterString = m_filterTeXSource;
        break;
    case FILE_NONSOURCE:
        filterString = m_filterNonsource;
        break;
    case FILE_SUPPORTEDFILES:
        filterString = m_filterSupportedFiles + "|" + m_filterExecutableFiles + "|" + m_filterImageFiles;
        break;
    case FILE_ALLFILES:
    default:
        break;
    }
    if (filterString.length())
        filterString += "|";
    filterString += m_filterAllFiles;
    return filterString;
}


// my "I need to try something out, I'll stick it in here" function
void NumeReWindow::Test(wxCommandEvent& WXUNUSED(event))
{
}


//////////////////////////////////////////////////////////////////////////////
///  private OnIdle
///  Initiates the UI update timer as needed
///
///  @param  event wxIdleEvent & The generated program idle event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnIdle(wxIdleEvent &event)
{
    if (m_updateTimer && !m_updateTimer->IsRunning ())
    {
        m_updateTimer->Start (250, wxTIMER_ONE_SHOT);

    }
    event.Skip();
}


//////////////////////////////////////////////////////////////////////////////
///  private OnStatusTimer
///  Initiates UI updates based on the internal timer
///
///  @param  event wxTimerEvent & The generated timer event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnStatusTimer(wxTimerEvent &WXUNUSED(event))
{
    if (m_updateTimer)
    {
        m_updateTimer->Stop();
        UpdateStatusBar();
        OnUpdateSaveUI();
        ToolbarStatusUpdate();
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the events from the file event timer.
///
/// \param event wxTimerEvent&
/// \return void
///
/// This timer is started by
/// the file system event handler to catch a list
/// of events before processing them (which happens here)
/////////////////////////////////////////////////
void NumeReWindow::OnFileEventTimer(wxTimerEvent& event)
{
    // store current selection
    int selection = m_book->GetSelection();
    NumeReEditor* focusedEd = m_book->getFocusedEditor();

    // Copy data and clear the cache
    std::vector<std::pair<int,wxString> > modifiedFiles = m_modifiedFiles;
    m_modifiedFiles.clear();

    // Create the relevant objects
    const FileFilterType fileType[] = {FILE_DATAFILES, FILE_DATAFILES, FILE_NSCR, FILE_NPRC, FILE_IMAGEFILES};
    VersionControlSystemManager manager(this);
    bool refreshProcedureLibrary = false;
    std::vector<std::string> vPaths = m_terminal->getPathSettings();
    std::array<bool, PATH_LAST-LOADPATH> pathsToRefresh;

    // Fill the refresh indicator with false values
    pathsToRefresh.fill(false);

    g_logger.info("File event handler: catched " + toString(modifiedFiles.size()) + " file events.");

    // Go through all cached events
    for (size_t i = 0; i < modifiedFiles.size(); i++)
    {
        wxFileName filename(modifiedFiles[i].second);

        if (modifiedFiles[i].first == wxFSW_EVENT_DELETE
            || modifiedFiles[i].first == wxFSW_EVENT_CREATE
            || modifiedFiles[i].first == wxFSW_EVENT_RENAME)
        {
            // These event types require refreshing of
            // the created file trees and the procedure
            // library, if necessary
            std::string sEventpath = replacePathSeparator(modifiedFiles[i].second.ToStdString());

            for (size_t j = LOADPATH; j < vPaths.size(); j++)
            {
                if (sEventpath.find(replacePathSeparator(vPaths[j])) != std::string::npos)
                {
                    pathsToRefresh[j-LOADPATH] = true;
                    break;
                }
            }

            // Mark the procedure library as to be
            // refreshed
            if (sEventpath.substr(sEventpath.length()-5) == ".nprc"
                || sEventpath.substr(sEventpath.length()-5) == ".nscr")
               refreshProcedureLibrary = true;

        }
        else if (modifiedFiles[i].first == wxFSW_EVENT_MODIFY)
        {
            // Ignore modified directories
            if (!filename.GetExt().length() && wxFileName::DirExists(modifiedFiles[i].second))
                continue;

            // This event type indicate, that files might have
            // to be reloaded and that the procedure library
            // should be refreshed as well.
            //
            // Mark the procedure library as to be
            // refreshed
            if (modifiedFiles[i].second.substr(modifiedFiles[i].second.length()-5) == ".nprc"
                || modifiedFiles[i].second.substr(modifiedFiles[i].second.length()-5) == ".nscr")
                refreshProcedureLibrary = true;

            // Ignore files, which have been saved by NumeRe
            // currently and therefore are result of a modify
            // event
            auto iter = m_filesLastSaveTime.find(modifiedFiles[i].second);

            if (iter != m_filesLastSaveTime.end()
                && (iter->second == 0 || time(0) - iter->second < 5))
            {
                g_logger.info("Ignored '" + modifiedFiles[i].second.ToStdString()
                              + "' due to last modification time difference: " + toString((int)(time(0) - iter->second)));
                continue;
            }
            else if (iter != m_filesLastSaveTime.end())
                g_logger.info("Reloading '" + modifiedFiles[i].second.ToStdString()
                              + "' due to last modification time difference: " + toString((int)(time(0) - iter->second)));

            // Ignore also files, whose modification time differs
            // more than two seconds from the current time. Older
            // modifications are likely to be metadata updates
            // done by the OS and do not require any refresh
            //
            // GetModificationTime() fails if the file is not readable
            // therefore we check this first
            {
                wxLogNull logNull;

                if (filename.IsDir()
                    || !filename.IsFileReadable()
                    || !filename.GetModificationTime().IsValid()
                    || (wxDateTime::Now() - filename.GetModificationTime()).GetSeconds() > 2)
                    continue;
            }

            // Add a new revision in the list of revisions that
            // the file was modified from the outside. Files with
            // a revision list, will therefore never lose their
            // changes, even if the user disagrees with the reloading
            if (manager.hasRevisions(modifiedFiles[i].second) && m_options->GetKeepBackupFile())
            {
                std::unique_ptr<FileRevisions> revisions(manager.getRevisions(modifiedFiles[i].second));
                g_logger.info("Adding external revision to '" + modifiedFiles[i].second.ToStdString() + "'.");

                if (revisions.get())
                    revisions->addExternalRevision(modifiedFiles[i].second);
            }

            // Ignore files on the current blacklist
            if (isOnReloadBlackList(modifiedFiles[i].second))
                continue;

            reloadFileIfOpen(modifiedFiles[i].second, false);
        }
    }

    // Now refresh all folders, which have been marked
    // as to be refreshed
    for (size_t i = 0; i < pathsToRefresh.size(); i++)
    {
        if (pathsToRefresh[i])
        {
            m_fileTree->DeleteChildren(m_projectFileFolders[i]);
            LoadFilesToTree(vPaths[i+LOADPATH], fileType[i], m_projectFileFolders[i]);
        }
    }

    g_logger.debug("All necessary file trees have been refreshed.");

    // Now refresh the procedure library
    if (refreshProcedureLibrary)
    {
        g_logger.info("Refreshing procedure library.");
        CreateProcedureTree(vPaths[PROCPATH]);
        m_terminal->UpdateLibrary();
    }

    // go back to previous selection
    if (m_book->GetSelection() != selection)
    {
        m_book->SetSelection(selection);

        if (focusedEd)
            focusedEd->SetFocus();
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private OnUpdateDebugUI
///  Updates the debug-related toolbar items
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::ToolbarStatusUpdate()
{
    wxToolBar* tb = GetToolBar();

    if (!tb->GetToolEnabled(ID_MENU_STOP_EXECUTION))
    {
        tb->EnableTool(ID_MENU_EXECUTE, true);
        tb->EnableTool(ID_MENU_STOP_EXECUTION, false);
    }

    if (!m_book->getCurrentEditor())
        return;

    if (m_book->getCurrentEditor()->getFileType() == FILE_NSCR
        || m_book->getCurrentEditor()->getFileType() == FILE_NPRC
        || m_book->getCurrentEditor()->getFileType() == FILE_MATLAB
        || m_book->getCurrentEditor()->getFileType() == FILE_CPP)
    {
        tb->EnableTool(ID_MENU_ADDEDITORBREAKPOINT, true);
        tb->EnableTool(ID_MENU_REMOVEEDITORBREAKPOINT, true);
        tb->EnableTool(ID_MENU_CLEAREDITORBREAKPOINTS, true);
        tb->EnableTool(ID_MENU_USEANALYZER, true);
        tb->ToggleTool(ID_MENU_USEANALYZER, m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
        tb->EnableTool(ID_MENU_INDENTONTYPE, true);
        tb->ToggleTool(ID_MENU_INDENTONTYPE, m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
    }
    else
    {
        tb->EnableTool(ID_MENU_ADDEDITORBREAKPOINT, false);
        tb->EnableTool(ID_MENU_REMOVEEDITORBREAKPOINT, false);
        tb->EnableTool(ID_MENU_CLEAREDITORBREAKPOINTS, false);
        tb->EnableTool(ID_MENU_USEANALYZER, false);
        tb->ToggleTool(ID_MENU_USEANALYZER, false);
        tb->EnableTool(ID_MENU_INDENTONTYPE, false);
        tb->ToggleTool(ID_MENU_INDENTONTYPE, false);
    }

    if (m_book->getCurrentEditor()->GetFileName().GetExt() == "m")
    {
        tb->EnableTool(ID_MENU_INDENTONTYPE, true);
        tb->ToggleTool(ID_MENU_INDENTONTYPE, m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
    }

    tb->ToggleTool(ID_MENU_LINEWRAP, m_book->getCurrentEditor()->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
}


//////////////////////////////////////////////////////////////////////////////
///  private UpdateStatusBar
///  Updates the status bar text as needed
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::UpdateStatusBar()
{
    if (m_statusBar == NULL)
        return;

    int pageCount = m_book->GetPageCount();

    if (pageCount < 1)
        return;

    wxString tabText = m_book->GetPageText(m_book->GetSelection());
    wxString filename;
    wxString filetype;
    std::string sExt = "";

    NumeReEditor* currEd = m_book->getCurrentEditor();
    NumeReEditor* focusEd = m_book->getFocusedEditor();

    filename = currEd->GetFileNameAndPath();

    if (currEd->defaultPage)
        filename = _guilang.get("GUI_STATUSBAR_WELCOMEPAGE");

    if (filename.find('.') != std::string::npos)
        sExt = filename.substr(filename.rfind('.')+1).ToStdString();


    if (tabText.StartsWith("<"))
    {
        filename = _guilang.get("GUI_STATUSBAR_UNSAVEDFILE");
        filetype = "N/A";
    }
    else if (currEd->defaultPage)
        filetype = _guilang.get("GUI_STATUSBAR_WELCOMEPAGE_FILETYPE");
    else if (sExt.length() && _guilang.get("GUI_STATUSBAR_"+toUpperCase(sExt)) != "GUI_STATUSBAR_"+toUpperCase(sExt))
    {
        filetype = _guilang.get("GUI_STATUSBAR_"+toUpperCase(sExt));
    }
    else
    {
        filetype = _guilang.get("GUI_STATUSBAR_UNKNOWN", sExt);
    }

    bool isEdReadOnly = currEd->GetReadOnly();

    wxString editable = isEdReadOnly ? "Read only" : "Read/Write";

    int curLine = focusEd->GetCurrentLine();
    int curPos = focusEd->GetColumn(focusEd->GetCurrentPos());
    wxString linecol;
    linecol.Printf(_(_guilang.get("GUI_STATUSBAR_LINECOL")), curLine+1, curPos+1);

    wxString sDebuggerMode = "";
    if (m_terminal->getKernelSettings().useDebugger() && currEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER))
         sDebuggerMode = _guilang.get("GUI_STATUSBAR_DEBUGGER_ANALYZER");
    else if (m_terminal->getKernelSettings().useDebugger() && !currEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER))
         sDebuggerMode = _guilang.get("GUI_STATUSBAR_DEBUGGER");
    else if (!m_terminal->getKernelSettings().useDebugger() && currEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER))
         sDebuggerMode = _guilang.get("GUI_STATUSBAR_ANALYZER");

    m_statusBar->SetStatus(NumeReStatusbar::STATUS_PATH, filename);
    m_statusBar->SetStatus(NumeReStatusbar::STATUS_FILETYPE, filetype);
    m_statusBar->SetStatus(NumeReStatusbar::STATUS_RWMODE, editable);
    m_statusBar->SetStatus(NumeReStatusbar::STATUS_CARETPOSITION, linecol);
    m_statusBar->SetStatus(NumeReStatusbar::STATUS_DEBUGGER, sDebuggerMode);
}


//////////////////////////////////////////////////////////////////////////////
///  private OnUpdateSaveUI
///  Updates the status of the active tab if modified, as well as enabling the save items.
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnUpdateSaveUI()//wxUpdateUIEvent &event)
{
    bool enable = m_book->getCurrentEditor()->Modified();
    int tabNum = m_book->GetSelection();
    wxString title = m_book->GetPageText(tabNum);

    if (enable)
    {
        if (title.find("*") == std::string::npos)
        {
            title += "*";
            m_book->SetPageText(tabNum, title);
            m_book->Refresh();
            UpdateWindowTitle(title);
        }
    }
    else
    {
        if (title.find("*") != std::string::npos)
        {
            title.RemoveLast(1);
            m_book->SetPageText(tabNum, title);
            m_book->Refresh();
            UpdateWindowTitle(title);
        }
    }

    GetToolBar()->EnableTool(ID_MENU_SAVE, enable);

    wxMenuBar* mb = GetMenuBar();
    WXWidget handle = mb->GetHandle();

    if (handle != NULL)
    {
        mb->FindItem(ID_MENU_SAVE)->Enable(enable);
    }
}


//////////////////////////////////////////////////////////////////////////////
///  public SetIntVar
///  A "one-size-fits-all" integer Set routine, to avoid pointless duplication
///
///  @param  variableName int  The ID of the member variable to set
///  @param  value        int  The value to set it to
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::SetIntVar(int variableName, int value)
{
    // figure out which integer I'm setting
    int* target = SelectIntVar(variableName);

    // assuming we assigned it properly, Set it
    if(target != NULL)
    {
        *target = value;
    }
}


//////////////////////////////////////////////////////////////////////////////
///  public GetIntVar
///  A "one-size-fits-all" integer Get routine, to avoid pointless duplication
///
///  @param  variableName int  The ID of the member variable to return
///
///  @return int          The variable's value
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int NumeReWindow::GetIntVar(int variableName)
{
    int* target = SelectIntVar(variableName);

    if(target != NULL)
    {
        return *target;
    }
    else
    {
        return 0;
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private SelectIntVar
///  Internal utility routine used by GetIntVar / SetIntVar
///
///  @param  variableName int  The ID of the variable to be get or set
///
///  @return int *        A pointer to the requested variable
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int* NumeReWindow::SelectIntVar(int variableName)
{
    switch(variableName)
    {
    case VN_NUMPAGES:
        return &m_numPages;
        break;
    case VN_CURRENTPAGE:
        return &m_currentPage;
        break;
    case VN_CLICKEDTAB:
        return &m_clickedTabNum;
        break;
    default:
#ifdef DO_LOG
        wxLogDebug("Failed to properly set variable.  variableName = %d", variableName);
#endif
        return NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////
///  public PassImageList
///  Allows the RemoteFileDialog to pass along its imagelist for use in the project tree
///
///  @param  imagelist wxImageList * The imagelist to use
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::PassImageList(wxImageList* imagelist)
{
    m_fileTree->AssignImageList(imagelist);
}


//////////////////////////////////////////////////////////////////////////////
///  public EvaluateOptions
///  Updates the GUI and other program options after the user has closed the
///  options dialog.  Called even if the user canceled the dialog because
///  the authorization code could have been changed.
///
///  @return void
///
///  @author Mark Erikson @date 03-29-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::EvaluateOptions()
{
    // Update the GUI elements by re-constructing them
    UpdateToolbar();

    // Prepare the contents of the navigators
    if (m_fileTree)
    {
        // If the left sidebar is not shown, display it here.
        // Otherwise simply clear the contents of the file
        // navigator tree
        if (!m_treeBook->IsShown())
        {
            m_splitProjectEditor->SplitVertically(m_treeBook, m_splitEditorOutput, m_options->getSetting(SETTING_V_POS_SASH_V).value());
            m_splitProjectEditor->SetMinimumPaneSize(30);
            m_treeBook->Show();
        }
        else
        {
            for (size_t i = 0; i < 5; i++)
            {
                m_fileTree->DeleteChildren(m_projectFileFolders[i]);
            }
        }

        // Expand the root node
        if (!m_fileTree->IsExpanded(m_fileTree->GetRootItem()))
            m_fileTree->Toggle(m_fileTree->GetRootItem());

        // Fill the contents to the tree
        std::vector<std::string> vPaths = m_terminal->getPathSettings();
        LoadFilesToTree(vPaths[LOADPATH], FILE_DATAFILES, m_projectFileFolders[0]);
        LoadFilesToTree(vPaths[SAVEPATH], FILE_DATAFILES, m_projectFileFolders[1]);
        LoadFilesToTree(vPaths[SCRIPTPATH], FILE_NSCR, m_projectFileFolders[2]);
        LoadFilesToTree(vPaths[PROCPATH], FILE_NPRC, m_projectFileFolders[3]);
        LoadFilesToTree(vPaths[PLOTPATH], FILE_IMAGEFILES, m_projectFileFolders[4]);

        // Construct the internal procedure tree
        // for the autocompletion feature
        CreateProcedureTree(vPaths[PROCPATH]);

        // Activate the file system watcher
        if (m_watcher)
            m_watcher->SetDefaultPaths(vPaths);
    }

    // Update the syntax highlighting in every
    // editor instance
    for (int i = 0; i < (int)m_book->GetPageCount(); i++)
    {
        NumeReEditor* edit = m_book->getEditor(i); // Needs adaption
        edit->UpdateSyntaxHighlighting();
        edit->SetCaretPeriod(m_options->GetCaretBlinkTime());
        edit->AnalyseCode();
        edit->SetUseTabs(m_options->isEnabled(SETTING_B_USETABS));
    }

    if (m_debugViewer)
        m_debugViewer->updateSettings();

    m_book->SetShowPathsOrIconsOnTabs(m_options->getSetting(SETTING_B_PATHSONTABS).active(),
                                      m_options->getSetting(SETTING_B_ICONSONTABS).active());

    if (m_book->GetSelection() != wxNOT_FOUND)
        UpdateWindowTitle(m_book->GetPageText(m_book->GetSelection()));

    // Copy the settings in the options object
    // into the configuration object
    m_termContainer->SetTerminalHistory(m_options->GetTerminalHistorySize());
    m_termContainer->SetCaretBlinkTime(m_options->GetCaretBlinkTime());
}


//////////////////////////////////////////////////////////////////////////////
///  private UpdateMenuBar
///  Recreates the menus, based on the current permissions
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::UpdateMenuBar()
{
    wxMenuBar* menuBar = GetMenuBar();

    if (!m_appStarting)
        return;

    if(menuBar != NULL)
    {
        SetMenuBar(NULL);
        delete menuBar;
    }

    menuBar = new wxMenuBar();
    SetMenuBar(menuBar);

    // Create new file menu
    wxMenu* menuNewFile = new wxMenu();

    menuNewFile->Append(ID_MENU_NEW_ASK, _guilang.get("GUI_MENU_NEWFILE_DLG"), _guilang.get("GUI_MENU_NEWFILE_TTP"));
    menuNewFile->AppendSeparator();
    menuNewFile->Append(ID_MENU_NEW_SCRIPT, _guilang.get("GUI_MENU_NEW_NSCR"), _guilang.get("GUI_MENU_NEW_NSCR_TTP"));
    menuNewFile->Append(ID_MENU_NEW_PROCEDURE, _guilang.get("GUI_MENU_NEW_NPRC"), _guilang.get("GUI_MENU_NEW_NPRC_TTP"));
    menuNewFile->Append(ID_MENU_NEW_LAYOUT, _guilang.get("GUI_MENU_NEW_LAYOUT"), _guilang.get("GUI_MENU_NEW_LAYOUT_TTP"));
    menuNewFile->Append(ID_MENU_NEW_PLUGIN, _guilang.get("GUI_MENU_NEW_PLUGIN"), _guilang.get("GUI_MENU_NEW_PLUGIN_TTP"));
    menuNewFile->AppendSeparator();
    menuNewFile->Append(ID_MENU_NEW_EMPTY, _guilang.get("GUI_MENU_NEW_EMPTYFILE"), _guilang.get("GUI_MENU_NEW_EMPTYFILE_TTP"));

    // Create file menu
    wxMenu* menuFile = new wxMenu();

    menuFile->Append(wxID_ANY, _guilang.get("GUI_MENU_NEWFILE"), menuNewFile, _guilang.get("GUI_MENU_NEWFILE_TTP"));

    menuFile->Append(ID_MENU_OPEN_SOURCE_LOCAL, _guilang.get("GUI_MENU_OPENFILE"), _guilang.get("GUI_MENU_OPENFILE_TTP"));
    menuFile->Append(ID_MENU_CLOSEPAGE, _guilang.get("GUI_MENU_CLOSEFILE"), _guilang.get("GUI_MENU_CLOSEFILE_TTP"));
    menuFile->Append(ID_MENU_CLOSEALL, _guilang.get("GUI_MENU_CLOSEALLFILES"));
    menuFile->AppendSeparator();
    menuFile->Append(ID_MENU_SAVE, _guilang.get("GUI_MENU_SAVEFILE"), _guilang.get("GUI_MENU_SAVEFILE_TTP"));
    menuFile->Append(ID_MENU_SAVE_SOURCE_LOCAL, _guilang.get("GUI_MENU_SAVEFILEAS"), _guilang.get("GUI_MENU_SAVEFILEAS_TTP"));
    menuFile->AppendSeparator();
    menuFile->Append(ID_MENU_PRINT_PAGE, _guilang.get("GUI_MENU_PRINT"), _guilang.get("GUI_MENU_PRINT_TTP"));
    menuFile->Append(ID_MENU_PRINT_PREVIEW, _guilang.get("GUI_MENU_PRINTPREVIEW"));
    menuFile->Append(ID_MENU_PRINT_SETUP, _guilang.get("GUI_MENU_PRINTSETUP"));
    menuFile->Append(ID_MENU_QUIT,_guilang.get("GUI_MENU_EXIT"), _guilang.get("GUI_MENU_EXIT_TTP"));

    menuBar->Append(menuFile, _guilang.get("GUI_MENU_FILE"));

    // Create strip spaces menu
    wxMenu* menuStripSpaces = new wxMenu();
    menuStripSpaces->Append(ID_MENU_STRIP_SPACES_BOTH, _guilang.get("GUI_MENU_STRIP_BOTH"), _guilang.get("GUI_MENU_STRIP_BOTH_TTP"));
    menuStripSpaces->Append(ID_MENU_STRIP_SPACES_FRONT, _guilang.get("GUI_MENU_STRIP_FRONT"), _guilang.get("GUI_MENU_STRIP_FRONT_TTP"));
    menuStripSpaces->Append(ID_MENU_STRIP_SPACES_BACK, _guilang.get("GUI_MENU_STRIP_BACK"), _guilang.get("GUI_MENU_STRIP_BACK_TTP"));

    // Create edit menu
    wxMenu* menuEdit = new wxMenu();

    menuEdit->Append(ID_MENU_UNDO, _guilang.get("GUI_MENU_UNDO"));
    menuEdit->Append(ID_MENU_REDO, _guilang.get("GUI_MENU_REDO"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_MENU_CUT, _guilang.get("GUI_MENU_CUT"));
    menuEdit->Append(ID_MENU_COPY, _guilang.get("GUI_MENU_COPY"));
    menuEdit->Append(ID_MENU_PASTE, _guilang.get("GUI_MENU_PASTE"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_MENU_SELECTION_UP, _guilang.get("GUI_MENU_SELECT_UP"), _guilang.get("GUI_MENU_SELECT_UP_TTP"));
    menuEdit->Append(ID_MENU_SELECTION_DOWN, _guilang.get("GUI_MENU_SELECT_DOWN"), _guilang.get("GUI_MENU_SELECT_DOWN_TTP"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_MENU_TRANSPOSESELECTION, _guilang.get("GUI_MENU_TRANSPOSESELECTION"), _guilang.get("GUI_MENU_TRANSPOSESELECTION_TTP"));
    menuEdit->Append(wxID_ANY, _guilang.get("GUI_MENU_STRIP"), menuStripSpaces);
    menuEdit->Append(ID_MENU_SORT_SELECTION_ASC, _guilang.get("GUI_MENU_SORT_ASC"), _guilang.get("GUI_MENU_SORT_ASC_TTP"));
    menuEdit->Append(ID_MENU_SORT_SELECTION_DESC, _guilang.get("GUI_MENU_SORT_DESC"), _guilang.get("GUI_MENU_SORT_DESC_TTP"));

    menuBar->Append(menuEdit, _guilang.get("GUI_MENU_EDIT"));

    // Create search menu
    wxMenu* menuSearch = new wxMenu();

    menuSearch->Append(ID_MENU_FIND, _guilang.get("GUI_MENU_FIND"));
    menuSearch->Append(ID_MENU_REPLACE, _guilang.get("GUI_MENU_REPLACE"));
    menuSearch->AppendSeparator();
    menuSearch->Append(ID_MENU_FIND_PROCEDURE, _guilang.get("GUI_MENU_FIND_PROCEDURE"), _guilang.get("GUI_MENU_FIND_PROCEDURE_TTP"));
    menuSearch->Append(ID_MENU_FIND_INCLUDE, _guilang.get("GUI_MENU_FIND_INCLUDE"), _guilang.get("GUI_MENU_FIND_INCLUDE_TTP"));
    menuSearch->Append(ID_MENU_GOTOLINE, _guilang.get("GUI_MENU_GOTOLINE"), _guilang.get("GUI_MENU_GOTOLINE_TTP"));
    menuSearch->AppendSeparator();
    menuSearch->Append(ID_MENU_BOOKMARK_TOGGLE, _guilang.get("GUI_MENU_BOOKMARK_TOGGLE"));
    menuSearch->Append(ID_MENU_BOOKMARK_CLEARMENU, _guilang.get("GUI_MENU_BOOKMARK_CLEAR"));
    menuSearch->Append(ID_MENU_BOOKMARK_PREVIOUS, _guilang.get("GUI_MENU_BOOKMARK_PREVIOUS"));
    menuSearch->Append(ID_MENU_BOOKMARK_NEXT, _guilang.get("GUI_MENU_BOOKMARK_NEXT"));

    menuBar->Append(menuSearch, _guilang.get("GUI_MENU_SEARCH"));

    // Create view menu
    wxMenu* menuView = new wxMenu();

    menuView->Append(ID_MENU_TOGGLE_CONSOLE, _guilang.get("GUI_MENU_TOGGLE_CONSOLE"));
    menuView->Append(ID_MENU_TOGGLE_FILETREE, _guilang.get("GUI_MENU_TOGGLE_FILETREE"));
    menuView->Append(ID_MENU_TOGGLE_HISTORY, _guilang.get("GUI_MENU_TOGGLE_HISTORY"));
    menuView->AppendSeparator();
    menuView->Append(ID_MENU_FOLD_ALL, _guilang.get("GUI_MENU_FOLDALL"), _guilang.get("GUI_MENU_FOLDALL_TTP"));
    menuView->Append(ID_MENU_UNFOLD_ALL, _guilang.get("GUI_MENU_UNFOLDALL"), _guilang.get("GUI_MENU_UNFOLDALL_TTP"));
    menuView->Append(ID_MENU_UNHIDE_ALL, _guilang.get("GUI_MENU_UNHIDEALL"), _guilang.get("GUI_MENU_UNHIDEALL_TTP"));
    menuView->AppendSeparator();
    m_menuItems[ID_MENU_LINEWRAP] = menuView->Append(ID_MENU_LINEWRAP, _guilang.get("GUI_MENU_LINEWRAP"), _guilang.get("GUI_MENU_LINEWRAP_TTP"), wxITEM_CHECK);
    m_menuItems[ID_MENU_DISPCTRLCHARS] = menuView->Append(ID_MENU_DISPCTRLCHARS, _guilang.get("GUI_MENU_DISPCTRLCHARS"), _guilang.get("GUI_MENU_DISPCTRLCHARS_TTP"), wxITEM_CHECK);
    m_menuItems[ID_MENU_USETXTADV] = menuView->Append(ID_MENU_USETXTADV, _guilang.get("GUI_MENU_USETXTADV"), _guilang.get("GUI_MENU_USETXTADV_TTP"), wxITEM_CHECK);
    //m_menuItems[ID_MENU_TOGGLE_NOTEBOOK_MULTIROW] = menuView->Append(ID_MENU_TOGGLE_NOTEBOOK_MULTIROW, _guilang.get("GUI_MENU_MULTIROW"), _guilang.get("GUI_MENU_MULTIROW_TTP"), wxITEM_CHECK);
    m_menuItems[ID_MENU_USESECTIONS] = menuView->Append(ID_MENU_USESECTIONS, _guilang.get("GUI_MENU_USESECTIONS"), _guilang.get("GUI_MENU_USESECTIONS_TTP"), wxITEM_CHECK);

    menuBar->Append(menuView, _guilang.get("GUI_MENU_VIEW"));

    // Create format menu
    wxMenu* menuFormat = new wxMenu();
    menuFormat->Append(ID_MENU_AUTOINDENT, _guilang.get("GUI_MENU_AUTOINDENT"), _guilang.get("GUI_MENU_AUTOINDENT_TTP"));
    m_menuItems[ID_MENU_INDENTONTYPE] = menuFormat->Append(ID_MENU_INDENTONTYPE, _guilang.get("GUI_MENU_INDENTONTYPE"), _guilang.get("GUI_MENU_INDENTONTYPE_TTP"), wxITEM_CHECK);
    menuFormat->Append(ID_MENU_AUTOFORMAT, _guilang.get("GUI_MENU_AUTOFORMAT"), _guilang.get("GUI_MENU_AUTOFORMAT_TTP"));

    // Create LaTeX menu
    wxMenu* menuLaTeX = new wxMenu();
    menuLaTeX->Append(ID_MENU_CREATE_LATEX_FILE, _guilang.get("GUI_MENU_CREATELATEX"), _guilang.get("GUI_MENU_CREATELATEX_TTP"));
    menuLaTeX->Append(ID_MENU_RUN_LATEX, _guilang.get("GUI_MENU_RUNLATEX"), _guilang.get("GUI_MENU_RUNLATEX_TTP"));
    menuLaTeX->Append(ID_MENU_COMPILE_LATEX, _guilang.get("GUI_MENU_COMPILE_TEX"), _guilang.get("GUI_MENU_COMPILE_TEX_TTP"));

    // Create refactoring menu
    wxMenu* menuRefactoring = new wxMenu();
    menuRefactoring->Append(ID_MENU_RENAME_SYMBOL, _guilang.get("GUI_MENU_RENAME_SYMBOL"), _guilang.get("GUI_MENU_RENAME_SYMBOL_TTP"));
    menuRefactoring->Append(ID_MENU_ABSTRAHIZE_SECTION, _guilang.get("GUI_MENU_ABSTRAHIZE_SECTION"), _guilang.get("GUI_MENU_ABSTRAHIZE_SECTION_TTP"));

    // Create analyzer menu
    wxMenu* menuAnalyzer = new wxMenu();
    m_menuItems[ID_MENU_USEANALYZER] = menuAnalyzer->Append(ID_MENU_USEANALYZER, _guilang.get("GUI_MENU_ANALYZER"), _guilang.get("GUI_MENU_ANALYZER_TTP"), wxITEM_CHECK);
    menuAnalyzer->Append(ID_MENU_FIND_DUPLICATES, _guilang.get("GUI_MENU_FIND_DUPLICATES"), _guilang.get("GUI_MENU_FIND_DUPLICATES_TTP"));
    menuAnalyzer->Append(ID_MENU_SHOW_DEPENDENCY_REPORT, _guilang.get("GUI_MENU_SHOW_DEPENDENCY_REPORT"), _guilang.get("GUI_MENU_SHOW_DEPENDENCY_REPORT_TTP"));
    menuAnalyzer->Append(ID_MENU_COMPARE_FILES, _guilang.get("GUI_MENU_COMPARE_FILES"), _guilang.get("GUI_MENU_COMPARE_FILES_TTP"));

    // Create exporter menu
    wxMenu* menuExporter = new wxMenu();
    menuExporter->Append(ID_MENU_EXPORT_AS_HTML, _guilang.get("GUI_MENU_EXPORT_AS_HTML"), _guilang.get("GUI_MENU_EXPORT_AS_HTML_TTP"));
    menuExporter->Append(ID_MENU_EXPORT_FORMATTED, _guilang.get("GUI_MENU_EXPORT_FORMATTED"), _guilang.get("GUI_MENU_EXPORT_FORMATTED_TTP"));

    // Create tools menu
    wxMenu* menuTools = new wxMenu();

    menuTools->Append(ID_MENU_OPTIONS, _guilang.get("GUI_MENU_OPTIONS"));
    menuTools->AppendSeparator();
    menuTools->Append(ID_MENU_EXECUTE, _guilang.get("GUI_MENU_EXECUTE"), _guilang.get("GUI_MENU_EXECUTE_TTP"));
    menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_FORMAT"), menuFormat);
    menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_REFACTORING"), menuRefactoring);
    menuTools->Append(ID_MENU_TOGGLE_COMMENT_LINE, _guilang.get("GUI_MENU_COMMENTLINE"), _guilang.get("GUI_MENU_COMMENTLINE_TTP"));
    menuTools->Append(ID_MENU_TOGGLE_COMMENT_SELECTION, _guilang.get("GUI_MENU_COMMENTSELECTION"), _guilang.get("GUI_MENU_COMMENTSELECTION_TTP"));
    menuTools->AppendSeparator();

    menuTools->Append(ID_MENU_CREATE_DOCUMENTATION, _guilang.get("GUI_MENU_CREATE_DOCUMENTATION"), _guilang.get("GUI_MENU_CREATE_DOCUMENTATION_TTP"));
    menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_LATEX"), menuLaTeX);
    menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_EXPORT"), menuExporter);

    menuTools->AppendSeparator();
    menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_ANALYSIS"), menuAnalyzer);
    m_menuItems[ID_MENU_TOGGLE_DEBUGGER] = menuTools->Append(ID_MENU_TOGGLE_DEBUGGER, _guilang.get("GUI_MENU_DEBUGGER"), _guilang.get("GUI_MENU_DEBUGGER_TTP"), wxITEM_CHECK);

    menuBar->Append(menuTools, _guilang.get("GUI_MENU_TOOLS"));

    // Create packages menu
    wxMenu* menuPackages = new wxMenu();

    menuPackages->Append(ID_MENU_PLUGINBROWSER, _guilang.get("GUI_MENU_SHOW_PACKAGE_BROWSER"), _guilang.get("GUI_MENU_SHOW_PACKAGE_BROWSER_TTP"));
    menuPackages->AppendSeparator();
    menuPackages->Append(ID_MENU_CREATE_PACKAGE, _guilang.get("GUI_MENU_CREATE_PACKAGE"), _guilang.get("GUI_MENU_CREATE_PACKAGE_TTP"));
    menuPackages->Append(ID_MENU_INSTALLPLUGINFROMFILE, _guilang.get("GUI_MENU_INSTALL_PACKAGE_FILE"), _guilang.get("GUI_MENU_INSTALL_PACKAGE_FILE_TTP"));
    menuPackages->AppendSeparator();
    wxMenuItem* item = menuPackages->Append(EVENTID_PLUGIN_MENU_END, _guilang.get("GUI_MENU_NO_PLUGINS_INSTALLED"));
    item->Enable(false);
    menuBar->Append(menuPackages, "Packages");

    // Create help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(ID_MENU_HELP, _guilang.get("GUI_MENU_SHOWHELP"));
    helpMenu->Append(ID_MENU_ABOUT, _guilang.get("GUI_MENU_ABOUT"), _guilang.get("GUI_MENU_ABOUT_TTP"));

    menuBar->Append(helpMenu, _guilang.get("GUI_MENU_HELP"));

    NumeReEditor* edit = m_book->getCurrentEditor();

    if (edit)
    {
        m_menuItems[ID_MENU_LINEWRAP]->Check(edit->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
        m_menuItems[ID_MENU_DISPCTRLCHARS]->Check(edit->getEditorSetting(NumeReEditor::SETTING_DISPCTRLCHARS));
        m_menuItems[ID_MENU_USETXTADV]->Check(edit->getEditorSetting(NumeReEditor::SETTING_USETXTADV));
        m_menuItems[ID_MENU_USESECTIONS]->Check(edit->getEditorSetting(NumeReEditor::SETTING_USESECTIONS));
        m_menuItems[ID_MENU_INDENTONTYPE]->Check(edit->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
        m_menuItems[ID_MENU_USEANALYZER]->Check(edit->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
    }
    else
    {
        m_menuItems[ID_MENU_LINEWRAP]->Check(false);
        m_menuItems[ID_MENU_DISPCTRLCHARS]->Check(false);
        m_menuItems[ID_MENU_USETXTADV]->Check(false);
        m_menuItems[ID_MENU_USESECTIONS]->Check(false);
        m_menuItems[ID_MENU_INDENTONTYPE]->Check(false);
        m_menuItems[ID_MENU_USEANALYZER]->Check(false);
    }

    //m_menuItems[ID_MENU_TOGGLE_NOTEBOOK_MULTIROW]->Check(m_multiRowState);
    m_menuItems[ID_MENU_TOGGLE_DEBUGGER]->Check(m_terminal->getKernelSettings().useDebugger());

    // Update now the package menu (avoids code duplication)
    UpdatePackageMenu();
}


/////////////////////////////////////////////////
/// \brief Updates the package menu after an
/// installation.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::UpdatePackageMenu()
{
    int id = GetMenuBar()->FindMenu("Packages");

    if (id == wxNOT_FOUND)
        return;

    // Get menu
    wxMenu* menuPackages = GetMenuBar()->GetMenu(id);

    // Remove all entries
    if (m_pluginMenuMap.size())
    {
        for (auto iter : m_pluginMenuMap)
            menuPackages->Delete(iter.first);

        m_pluginMenuMap.clear();
    }
    else
        menuPackages->Delete(EVENTID_PLUGIN_MENU_END);

    // Get new map
    std::map<std::string,std::string> mMenuMap = m_terminal->getMenuMap();

    // Fill the menu with new entries
    if (mMenuMap.size())
    {
        size_t id = EVENTID_PLUGIN_MENU_START;

        for (auto iter = mMenuMap.begin(); iter != mMenuMap.end(); ++iter, id++)
        {
            menuPackages->Append(id, removeMaskedStrings(iter->first));
            m_pluginMenuMap[id] = iter->second;
        }
    }
    else
    {
        wxMenuItem* item = menuPackages->Append(EVENTID_PLUGIN_MENU_END, _guilang.get("GUI_MENU_NO_PLUGINS_INSTALLED"));
        item->Enable(false);
    }
}


/////////////////////////////////////////////////
/// \brief Returns the bitmap corresponding to
/// the current icon style.
///
/// \param iconName const wxString&
/// \return wxBitmap
///
/////////////////////////////////////////////////
wxBitmap NumeReWindow::getToolbarIcon(const wxString& iconName)
{
    static wxString sAppIconFolder = getProgramFolder() + "/icons";
    std::string style = m_lastIconStyle;

    if (style == "Purist")
        return wxBitmap(sAppIconFolder + "/bw/" + iconName + ".png", wxBITMAP_TYPE_PNG);
    else if (style == "Colorful")
        return wxBitmap(sAppIconFolder + "/color/" + iconName + ".png", wxBITMAP_TYPE_PNG);
    else if (style == "Focused")
    {
        if (iconName == "run"
            || iconName == "stop"
            || iconName == "add-bp"
            || iconName == "remove-bp"
            || iconName == "clear-bp"
            || iconName == "debugger"
            || iconName == "continue"
            || iconName == "cancel-debugger"
            || iconName == "back"
            || iconName == "forward"
            || iconName == "save"
            || iconName == "analyzer")
            return wxBitmap(sAppIconFolder + "/color/" + iconName + ".png", wxBITMAP_TYPE_PNG);

        return wxBitmap(sAppIconFolder + "/bw/" + iconName + ".png", wxBITMAP_TYPE_PNG);
    }

    return wxNullBitmap;
}



//////////////////////////////////////////////////////////////////////////////
///  private UpdateToolbar
///  Recreates the toolbar, based on the current permissions
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::UpdateToolbar()
{
    int style = wxTB_FLAT | wxTB_HORIZONTAL;

    bool showText = m_options->GetShowToolbarText();

    if (showText)
        style |= wxTB_TEXT;

    // Only update the toolbar if it is really necessary
    if (GetToolBar()
        && GetToolBar()->GetWindowStyle() == style
        && m_lastIconStyle == m_options->getSetting(SETTING_S_TOOLBARICONSTYLE).stringval()
        && m_lastToolbarStretchState == m_options->getSetting(SETTING_B_TOOLBARSTRETCH).active())
        return;

    // Update the buffer vars
    m_lastIconStyle = m_options->getSetting(SETTING_S_TOOLBARICONSTYLE).stringval();
    m_lastToolbarStretchState = m_options->getSetting(SETTING_B_TOOLBARSTRETCH).active();

    wxToolBar* t = GetToolBar();
    delete t;
    SetToolBar(nullptr);
    t = CreateToolBar(style);//new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);
    t->SetBackgroundColour(*wxWHITE);
    //t->SetBackgroundColour(wxColour(221,230,255));
//    t->SetBackgroundColour(wxColour(46,110,150));

    t->AddTool(ID_MENU_NEW_ASK, _guilang.get("GUI_TB_NEW"), getToolbarIcon("new-file"), _guilang.get("GUI_TB_NEW_TTP"), wxITEM_DROPDOWN);

    wxMenu* menuNewFile = new wxMenu();
    menuNewFile->Append(ID_MENU_NEW_SCRIPT, _guilang.get("GUI_MENU_NEW_NSCR"), _guilang.get("GUI_MENU_NEW_NSCR_TTP"));
    menuNewFile->Append(ID_MENU_NEW_PROCEDURE, _guilang.get("GUI_MENU_NEW_NPRC"), _guilang.get("GUI_MENU_NEW_NPRC_TTP"));
    menuNewFile->Append(ID_MENU_NEW_LAYOUT, _guilang.get("GUI_MENU_NEW_LAYOUT"), _guilang.get("GUI_MENU_NEW_LAYOUT_TTP"));
    //menuNewFile->Append(ID_MENU_NEW_PLUGIN, _guilang.get("GUI_MENU_NEW_PLUGIN"), _guilang.get("GUI_MENU_NEW_PLUGIN_TTP"));
    menuNewFile->AppendSeparator();
    menuNewFile->Append(ID_MENU_NEW_EMPTY, _guilang.get("GUI_MENU_NEW_EMPTYFILE"), _guilang.get("GUI_MENU_NEW_EMPTYFILE_TTP"));
    t->SetDropdownMenu(ID_MENU_NEW_ASK, menuNewFile);

    t->AddTool(ID_MENU_OPEN_SOURCE_LOCAL, _guilang.get("GUI_TB_OPEN"), getToolbarIcon("open"), _guilang.get("GUI_TB_OPEN_TTP"));
    t->AddTool(ID_MENU_SAVE, _guilang.get("GUI_TB_SAVE"), getToolbarIcon("save"), _guilang.get("GUI_TB_SAVE_TTP"));

    t->AddSeparator();

    t->AddTool(ID_MENU_UNDO, _guilang.get("GUI_TB_UNDO"), getToolbarIcon("undo"), _guilang.get("GUI_TB_UNDO"));
    t->AddTool(ID_MENU_REDO, _guilang.get("GUI_TB_REDO"), getToolbarIcon("redo"), _guilang.get("GUI_TB_REDO"));

    t->AddSeparator();

    t->AddTool(ID_MENU_CUT, _guilang.get("GUI_TB_CUT"), getToolbarIcon("cut"), _guilang.get("GUI_TB_CUT"));
    t->AddTool(ID_MENU_COPY, _guilang.get("GUI_TB_COPY"), getToolbarIcon("copy"), _guilang.get("GUI_TB_COPY"));
    t->AddTool(ID_MENU_PASTE, _guilang.get("GUI_TB_PASTE"), getToolbarIcon("paste"), _guilang.get("GUI_TB_PASTE"));

    t->AddSeparator();

    t->AddTool(ID_MENU_FIND, _guilang.get("GUI_TB_SEARCH"), getToolbarIcon("search"), _guilang.get("GUI_TB_SEARCH"));
    t->AddTool(ID_MENU_REPLACE, _guilang.get("GUI_TB_REPLACE"), getToolbarIcon("replace"), _guilang.get("GUI_TB_REPLACE"));

    t->AddSeparator();

    t->AddTool(ID_MENU_INDENTONTYPE, _guilang.get("GUI_TB_INDENTONTYPE"), getToolbarIcon("indent"),
               _guilang.get("GUI_TB_INDENTONTYPE_TTP"), wxITEM_CHECK);
    t->ToggleTool(ID_MENU_INDENTONTYPE, false);
    t->EnableTool(ID_MENU_INDENTONTYPE, false);
    t->AddTool(ID_MENU_LINEWRAP, _guilang.get("GUI_TB_LINEWRAP"), getToolbarIcon("wrap-around"),
               _guilang.get("GUI_TB_LINEWRAP_TTP"), wxITEM_CHECK);

    if (m_lastToolbarStretchState)
        t->AddStretchableSpace();
    else
        t->AddSeparator();

    t->AddTool(ID_MENU_EXECUTE, _guilang.get("GUI_TB_RUN"), getToolbarIcon("run"), _guilang.get("GUI_TB_RUN_TTP"));
    t->AddTool(ID_MENU_STOP_EXECUTION, _guilang.get("GUI_TB_STOP"), getToolbarIcon("stop"), _guilang.get("GUI_TB_STOP_TTP"));

    t->AddSeparator();

    t->AddTool(ID_MENU_TOGGLE_DEBUGGER, _guilang.get("GUI_TB_DEBUGGER"), getToolbarIcon("debugger"),
               _guilang.get("GUI_TB_DEBUGGER_TTP"), wxITEM_CHECK);
    t->ToggleTool(ID_MENU_TOGGLE_DEBUGGER, m_terminal->getKernelSettings().useDebugger());
    t->AddTool(ID_MENU_ADDEDITORBREAKPOINT, _guilang.get("GUI_TB_ADD"), getToolbarIcon("add-bp"),
               _guilang.get("GUI_TB_ADD_TTP"));
    t->AddTool(ID_MENU_REMOVEEDITORBREAKPOINT, _guilang.get("GUI_TB_REMOVE"), getToolbarIcon("remove-bp"),
               _guilang.get("GUI_TB_REMOVE_TTP"));
    t->AddTool(ID_MENU_CLEAREDITORBREAKPOINTS, _guilang.get("GUI_TB_CLEAR"), getToolbarIcon("clear-bp"),
               _guilang.get("GUI_TB_CLEAR_TTP"));

    t->AddSeparator();

    t->AddTool(ID_MENU_USEANALYZER, _guilang.get("GUI_TB_ANALYZER"), getToolbarIcon("analyzer"),
               _guilang.get("GUI_TB_ANALYZER_TTP"), wxITEM_CHECK);
    t->ToggleTool(ID_MENU_USEANALYZER, false);
    t->EnableTool(ID_MENU_USEANALYZER, false);

    if (m_lastToolbarStretchState)
        t->AddStretchableSpace();
    else
        t->AddSeparator();

    t->AddTool(ID_MENU_OPTIONS, _guilang.get("GUI_MENU_OPTIONS"), getToolbarIcon("settings"), _guilang.get("GUI_MENU_OPTIONS"));
    t->AddTool(ID_MENU_HELP, _guilang.get("GUI_MENU_HELP"), getToolbarIcon("help"), _guilang.get("GUI_MENU_HELP"));

    NumeRe::DataBase db("<>/docs/find.ndb");

    if (m_options->useCustomLangFiles() && fileExists(m_options->ValidFileName("<>/user/docs/find.ndb", ".ndb")))
        db.addData("<>/user/docs/find.ndb");

    t->AddControl(new ToolBarSearchCtrl(t, wxID_ANY, db, this, m_terminal, _guilang.get("GUI_SEARCH_TELLME"),
                                        _guilang.get("GUI_SEARCH_CALLTIP_TOOLBAR"), _guilang.get("GUI_SEARCH_CALLTIP_TOOLBAR"),
                                        300, m_lastToolbarStretchState ? -200 : 200),
                  wxEmptyString);

    t->Realize();

    ToolbarStatusUpdate();

    if (m_debugViewer != nullptr)
        m_debugViewer->initializeToolbar();
}


//////////////////////////////////////////////////////////////////////////////
///  private UpdateTerminalNotebook
///  Recreates the notebook containing the terminal and other related widgets
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::UpdateTerminalNotebook()
{
    if (!m_appStarting)
        return;

    m_termContainer->Show();

    if (!m_splitEditorOutput->IsSplit())
    {
        m_splitEditorOutput->SplitHorizontally(m_book, m_splitCommandHistory, m_options->getSetting(SETTING_V_POS_SASH_H).value()/100.0);
        m_splitEditorOutput->SetMinimumPaneSize(20);

        if (!m_splitCommandHistory->IsSplit())
        {
            m_splitCommandHistory->SplitVertically(m_termContainer, m_noteTerm, m_options->getSetting(SETTING_V_POS_SASH_T).value()/100.0);
        }

        m_terminal->UpdateSize();
        m_termContainer->Show();
        m_noteTerm->Show();
    }

    m_termContainer->Refresh();
    m_book->Refresh();
    m_noteTerm->Refresh();
}


/////////////////////////////////////////////////
/// \brief This member function gets the current
/// variable list from the kernel and updates the
/// variable viewer widget correspondingly.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::UpdateVarViewer()
{
    if (m_varViewer)
    {
        NumeReVariables vars = m_terminal->getVariableList();
        m_varViewer->UpdateVariables(vars.vVariables, vars.nNumerics, vars.nStrings, vars.nTables, vars.nClusters);
    }
}


/////////////////////////////////////////////////
/// \brief This member function updates the
/// application's window title using the current
/// opened file's name.
///
/// \param filename const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::UpdateWindowTitle(const wxString& filename)
{
    wxTopLevelWindow::SetTitle((filename.length() ? filename + " - " : wxString()) + _guilang.get("COMMON_APPNAME") + " (v " + sVersion + ")");
}


/////////////////////////////////////////////////
/// \brief This member function toggles the
/// bottom part of the window containing the
/// terminal and the list view widgets.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::toggleConsole()
{
    if (m_termContainer->IsShown())
    {
        m_termContainer->Hide();
        m_noteTerm->Hide();
        m_options->getSetting(SETTING_V_POS_SASH_H).value() = 100 * m_splitEditorOutput->GetSplitPercentage();
        m_splitEditorOutput->Unsplit(m_splitCommandHistory);
    }
    else
    {
        m_splitEditorOutput->SplitHorizontally(m_book, m_splitCommandHistory, m_options->getSetting(SETTING_V_POS_SASH_H).value() / 100.0);
        m_splitEditorOutput->SetMinimumPaneSize(20);
        m_terminal->UpdateSize();
        m_termContainer->Show();

        if (m_splitCommandHistory->IsSplit())
            m_noteTerm->Show();
    }

    m_book->Refresh();
}


/////////////////////////////////////////////////
/// \brief This member function toggles the left
/// sidebar of the window containing both trees.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::toggleFiletree()
{
    if (m_treeBook->IsShown())
    {
        m_treeBook->Hide();
        m_options->getSetting(SETTING_V_POS_SASH_V).value() = m_splitProjectEditor->GetSashPosition();
        m_splitProjectEditor->Unsplit(m_treeBook);
    }
    else
    {
        m_splitProjectEditor->SplitVertically(m_treeBook, m_splitEditorOutput, m_options->getSetting(SETTING_V_POS_SASH_V).value());
        m_splitProjectEditor->SetMinimumPaneSize(30);
        m_treeBook->Show();
    }

    m_terminal->UpdateSize();
    m_book->Refresh();
}


/////////////////////////////////////////////////
/// \brief This member function toggles the
/// rightmost part of the lower window section
/// containing the history and the list view
/// widgets.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::toggleHistory()
{
    if (m_noteTerm->IsShown())
    {
        m_noteTerm->Hide();
        m_options->getSetting(SETTING_V_POS_SASH_T).value() = 100 * m_splitCommandHistory->GetSplitPercentage();
        m_splitCommandHistory->Unsplit(m_noteTerm);
    }
    else
    {
        m_splitCommandHistory->SplitVertically(m_termContainer, m_noteTerm, m_options->getSetting(SETTING_V_POS_SASH_T).value() / 100.0);
        m_noteTerm->Show();
    }

    m_terminal->UpdateSize();
    m_termContainer->Refresh();
    m_noteTerm->Refresh();
}


/////////////////////////////////////////////////
/// \brief This member function unhides the
/// terminal, if it was hidden before.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::showConsole()
{
    if (!m_termContainer->IsShown())
        toggleConsole();
}


/////////////////////////////////////////////////
/// \brief This member function opens a text
/// entry dialog, where the user can enter the
/// target line number, he wants to jump to. After
/// confirming, the editor jumps to this line.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::gotoLine()
{
    NumeReEditor* focusEd = m_book->getFocusedEditor();
    wxTextEntryDialog dialog(this,
                             _guilang.get("GUI_DLG_GOTO_QUESTION", toString(focusEd->GetLineCount())),
                             _guilang.get("GUI_DLG_GOTO"));
    int ret = dialog.ShowModal();

    if (ret == wxID_CANCEL)
        return;

    int line = StrToInt(dialog.GetValue().ToStdString())-1;

    if (line < 0 || line >= focusEd->GetLineCount())
        wxMessageBox(_guilang.get("GUI_DLG_GOTO_ERROR"), _guilang.get("GUI_DLG_GOTO"), wxCENTRE | wxICON_ERROR);
    else
    {
        focusEd->GotoLine(line);
        focusEd->SetFocus();
    }
}


/////////////////////////////////////////////////
/// \brief This member function focuses the
/// editor widget.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::setEditorFocus()
{
    m_book->getCurrentEditor()->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function focuses the last
/// opened ImageViewer window.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::setViewerFocus()
{
    m_currentView->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function is a simple
/// wrapper for refreshing the contents of the
/// function tree.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::refreshFunctionTree()
{
    wxWindow* focus = wxWindow::FindFocus();
    _guilang.loadStrings(m_options->useCustomLangFiles());
    g_logger.info("Refreshing function tree.");
    prepareFunctionTree();
    m_functionTree->Refresh();

    // Update the syntax colors in the editor windows
    for (size_t i = 0; i < m_book->GetPageCount(); i++)
    {
        m_book->getEditor(i)->UpdateSyntaxHighlighting(true); // Needs adaption
    }

    m_history->UpdateSyntaxHighlighting(true);

    if (focus)
        focus->SetFocus();
}


/////////////////////////////////////////////////
/// \brief Public access method for accessing the
/// currently viewed editor. Does only return a
/// pointer to the top or left editor and shall
/// not be used to modify settings.
///
/// \return NumeReEditor*
///
/////////////////////////////////////////////////
NumeReEditor* NumeReWindow::GetCurrentEditor()
{
    return m_book->getCurrentEditor();
}


/////////////////////////////////////////////////
/// \brief Static helper function to tokenize the
/// whitespace-separated list of keys.
///
/// \param sKeyList std::string
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
static std::vector<std::string> tokenize(std::string sKeyList)
{
    std::vector<std::string> vKeyList;

    while (sKeyList.length())
    {
        vKeyList.push_back(sKeyList.substr(0, sKeyList.find(' ')));
        sKeyList.erase(0, sKeyList.find(' '));

        while (sKeyList.front() == ' ')
            sKeyList.erase(0,1);
    }

    return vKeyList;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// contents of the symbols tree.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::prepareFunctionTree()
{
    // Insert the plugin and function definitions
    _guilang.addToLanguage(m_terminal->getPluginLanguageStrings());
    _guilang.addToLanguage(m_terminal->getFunctionLanguageStrings());

    // Clear all previously available items
    if (!m_functionTree->IsEmpty())
        m_functionTree->DeleteAllItems();

    int idxFolderOpen = m_iconManager->GetIconIndex("FOLDEROPEN");
    int idxFunctions = m_iconManager->GetIconIndex("FUNCTIONS");
    int idxCommands = m_iconManager->GetIconIndex("COMMANDS");
    int idxConstants = m_iconManager->GetIconIndex("CONSTANTS");
    int idxMethods = m_iconManager->GetIconIndex("METHODS");

    // Create the root node
    wxTreeItemId rootNode = m_functionTree->AddRoot(_guilang.get("GUI_TREE_WORKSPACE"), m_iconManager->GetIconIndex("WORKPLACE"));

    // Create the sections
    FileNameTreeData* rootData = new FileNameTreeData();
    wxTreeItemId commandNode = m_functionTree->AppendItem(rootNode, _guilang.get("GUI_TREE_COMMANDS"),
                                                          m_iconManager->GetIconIndex("WORKPLACE"), -1, rootData);

    rootData = new FileNameTreeData();
    wxTreeItemId functionNode = m_functionTree->AppendItem(rootNode, _guilang.get("GUI_TREE_FUNCTIONS"),
                                                           m_iconManager->GetIconIndex("WORKPLACE"), -1, rootData);

    rootData = new FileNameTreeData();
    wxTreeItemId methodNode = m_functionTree->AppendItem(rootNode, _guilang.get("GUI_TREE_METHODS"),
                                                         m_iconManager->GetIconIndex("WORKPLACE"), -1, rootData);

    rootData = new FileNameTreeData();
    wxTreeItemId constNode = m_functionTree->AppendItem(rootNode, _guilang.get("GUI_TREE_CONSTANTS"),
                                                        m_iconManager->GetIconIndex("WORKPLACE"), -1, rootData);

    wxTreeItemId currentNode;
    std::vector<std::string> vDirList;
    std::vector<std::string> vKeyList = tokenize(_guilang.get("GUI_TREE_CMD_KEYLIST"));

    // commands
    for (size_t i = 0; i < vKeyList.size(); i++)
    {
        FileNameTreeData* dir = new FileNameTreeData();
        dir->isDir = true;
        currentNode = m_functionTree->AppendItem(commandNode, _guilang.get("PARSERFUNCS_LISTCMD_TYPE_" + toUpperCase(vKeyList[i])),
                                                 idxFolderOpen, -1, dir);
        vDirList = _guilang.getList("PARSERFUNCS_LISTCMD_CMD_*_[" + toUpperCase(vKeyList[i]) + "]");

        for (size_t j = 0; j < vDirList.size(); j++)
        {
            FileNameTreeData* data = new FileNameTreeData();
            data->isCommand = true;
            data->tooltip = prepareTooltip(vDirList[j]);
            m_functionTree->AppendItem(currentNode, vDirList[j].substr(0, vDirList[j].find(' ')), idxCommands, -1, data);
        }
    }

    m_functionTree->Toggle(commandNode);

    // functions
    vKeyList = tokenize(_guilang.get("GUI_TREE_FUNC_KEYLIST"));

    for (size_t i = 0; i < vKeyList.size(); i++)
    {
        FileNameTreeData* dir = new FileNameTreeData();
        dir->isDir = true;
        currentNode = m_functionTree->AppendItem(functionNode, _guilang.get("PARSERFUNCS_LISTFUNC_TYPE_" + toUpperCase(vKeyList[i])),
                                                 idxFolderOpen, -1, dir);
        vDirList = _guilang.getList("PARSERFUNCS_LISTFUNC_FUNC_*_[" + toUpperCase(vKeyList[i]) + "]");

        for (size_t j = 0; j < vDirList.size(); j++)
        {
            FileNameTreeData* data = new FileNameTreeData();
            data->isFunction = true;
            data->tooltip = prepareTooltip(vDirList[j]);
            m_functionTree->AppendItem(currentNode, vDirList[j].substr(0, vDirList[j].find(')')+1), idxFunctions, -1, data);
        }
    }

    m_functionTree->Toggle(functionNode);

    // methods
    vKeyList = tokenize(_guilang.get("GUI_TREE_METHOD_KEYLIST"));

    for (size_t i = 0; i < vKeyList.size(); i++)
    {
        FileNameTreeData* dir = new FileNameTreeData();
        dir->isDir = true;
        currentNode = m_functionTree->AppendItem(methodNode, _guilang.get("PARSERFUNCS_LISTFUNC_METHODS_TYPE_" + toUpperCase(vKeyList[i])),
                                                 idxFolderOpen, -1, dir);
        vDirList = _guilang.getList("PARSERFUNCS_LISTFUNC_METHOD_*_[" + toUpperCase(vKeyList[i]) + "]");
        std::string sPrefix = vKeyList[i] == "data" ? "TABLE()" : "STRINGVAR";

        for (size_t j = 0; j < vDirList.size(); j++)
        {
            FileNameTreeData* data = new FileNameTreeData();
            data->isMethod = true;
            data->tooltip = sPrefix + prepareTooltip("." + vDirList[j]);
            m_functionTree->AppendItem(currentNode, sPrefix + "." + vDirList[j].substr(0, vDirList[j].find("  ")), idxMethods, -1, data);
        }
    }

    m_functionTree->Toggle(methodNode);

    // Constants
    vKeyList = tokenize(_guilang.get("GUI_TREE_CONST_KEYLIST"));

    for (size_t i = 0; i < vKeyList.size(); i++)
    {
        FileNameTreeData* dir = new FileNameTreeData();
        dir->isDir = true;
        currentNode = m_functionTree->AppendItem(constNode, _guilang.get("PARSERFUNCS_LISTCONST_TYPE_" + toUpperCase(vKeyList[i])),
                                                 idxFolderOpen, -1, dir);
        vDirList = _guilang.getList("GUI_EDITOR_CALLTIP_CONST_*_[" + toUpperCase(vKeyList[i]) + "]");

        for (size_t j = 0; j < vDirList.size(); j++)
        {
            FileNameTreeData* data = new FileNameTreeData();
            data->isConstant = true;
            data->tooltip = vDirList[j];
            m_functionTree->AppendItem(currentNode, vDirList[j].substr(0, vDirList[j].find(" = ")), idxConstants, -1, data);
        }
    }

    m_functionTree->Toggle(constNode);

}


/////////////////////////////////////////////////
/// \brief This member function prepares the
/// tooltip shown by the symbols tree.
///
/// \param sTooltiptext const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string NumeReWindow::prepareTooltip(const std::string& sTooltiptext)
{
    size_t nClosingParens = sTooltiptext.find(')');
    std::string sTooltip = sTooltiptext;

    if (sTooltiptext.front() == '.')
    {
        nClosingParens = sTooltip.find("  ");
        sTooltip.replace(nClosingParens, sTooltip.find_first_not_of(' ', nClosingParens)-nClosingParens, "  ->  ");
    }
    else if (sTooltiptext.find(' ') < nClosingParens && sTooltiptext.find(' ') < sTooltiptext.find('('))
    {
        nClosingParens = sTooltiptext.find(' ')-1;
        sTooltip.replace(nClosingParens+1, sTooltip.find_first_not_of(' ', nClosingParens+1)-nClosingParens-1, "   ");
    }
    else
        sTooltip.replace(nClosingParens+1, sTooltip.find_first_not_of(' ', nClosingParens+1)-nClosingParens-1, "  ->  ");

    return sTooltip;
}


/////////////////////////////////////////////////
/// \brief This member function sets the default
/// printer settings.
///
/// \return wxPrintData*
///
/////////////////////////////////////////////////
wxPrintData* NumeReWindow::setDefaultPrinterSettings()
{
    wxPrintData* printdata = new wxPrintData();
    printdata->SetPaperId(wxPAPER_A4);
    printdata->SetBin(wxPRINTBIN_AUTO);
    printdata->SetOrientation(wxPORTRAIT);
    printdata->SetQuality(wxPRINT_QUALITY_HIGH);
    printdata->SetPrinterName(wxEmptyString);
    return printdata;
}


//////////////////////////////////////////////////////////////////////////////
///  private OnTreeItemRightClick
///  Pops up a menu with appropriate items when the project tree is right-clicked.
///  Also sets the last selected tree item and file type for use in other functions.
///
///  @param  event wxTreeEvent & The generated tree event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnTreeItemRightClick(wxTreeEvent& event)
{
    if (event.GetId() == ID_PROJECTTREE)
    {
        VersionControlSystemManager manager(this);
        wxTreeItemId clickedItem = event.GetItem();
        m_clickedTreeItem = clickedItem;
        wxMenu popupMenu;
        wxString editableExt = ".dat;.txt;.nscr;.nprc;.dx;.jcm;.jdx;.csv;.log;.tex;.xml;.nhlp;.npkp;.cpp;.cxx;.c;.hpp;.hxx;.h;.m;.nlyt;";
        wxString loadableExt = ".dat;.txt;.dx;.jcm;.jdx;.xls;.xlsx;.ods;.ndat;.labx;.ibw;.csv;";
        wxString showableImgExt = ".png;.jpeg;.jpg;.gif;.bmp;";

        wxString fname_ext = m_fileTree->GetItemText(m_clickedTreeItem);
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));

        if (m_clickedTreeItem == m_fileTree->GetRootItem())
            return;

        if (!data || data->isDir)
        {
            popupMenu.Append(ID_MENU_NEW_FOLDER_IN_TREE, _guilang.get("GUI_TREE_PUP_NEWFOLDER"));

            if (data)
                popupMenu.Append(ID_MENU_REMOVE_FOLDER_FROM_TREE, _guilang.get("GUI_TREE_PUP_REMOVEFOLDER"));

            popupMenu.AppendSeparator();
            popupMenu.Append(ID_MENU_COPY_AS_PATH, _guilang.get("GUI_TREE_PUP_COPYASPATH"));
            popupMenu.AppendSeparator();
            popupMenu.Append(ID_MENU_OPEN_IN_EXPLORER, _guilang.get("GUI_TREE_PUP_OPENINEXPLORER"));
            wxPoint p = event.GetPoint();
            m_fileTree->PopupMenu(&popupMenu, p);
            return;
        }

        fname_ext = fname_ext.substr(fname_ext.rfind('.')).Lower() + ";";

        if (loadableExt.find(fname_ext) != std::string::npos)
        {
            popupMenu.Append(ID_MENU_OPEN_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_LOAD"));
            popupMenu.Append(ID_MENU_OPEN_FILE_FROM_TREE_TO_TABLE, _guilang.get("GUI_TREE_PUP_LOADTOTABLE"));
        }
        else if (fname_ext == ".nscr;")
            popupMenu.Append(ID_MENU_OPEN_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_START"));
        else if (fname_ext == ".nprc;")
            popupMenu.Append(ID_MENU_OPEN_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_RUN"));

        if (editableExt.find(fname_ext) != std::string::npos)
            popupMenu.Append(ID_MENU_EDIT_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_EDIT"));

        if (showableImgExt.find(fname_ext) != std::string::npos)
            popupMenu.Append(ID_MENU_OPEN_IMAGE_FROM_TREE, _guilang.get("GUI_TREE_PUP_OPENIMAGE"));

        if (manager.hasRevisions(data->filename))
        {
            popupMenu.AppendSeparator();
            popupMenu.Append(ID_MENU_SHOW_REVISIONS, _guilang.get("GUI_TREE_PUP_SHOWREVISIONS"));
            popupMenu.Append(ID_MENU_TAG_CURRENT_REVISION, _guilang.get("GUI_TREE_PUP_TAGCURRENTREVISION"));
        }

        popupMenu.AppendSeparator();
        popupMenu.Append(ID_MENU_COPY_AS_PATH, _guilang.get("GUI_TREE_PUP_COPYASPATH"));
        popupMenu.AppendSeparator();
        popupMenu.Append(ID_MENU_DELETE_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_DELETEFILE"));
        popupMenu.Append(ID_MENU_COPY_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_COPYFILE"));

        if (m_copiedTreeItem)
            popupMenu.Append(ID_MENU_INSERT_FILE_INTO_TREE, _guilang.get("GUI_TREE_PUP_INSERTFILE"));

        popupMenu.Append(ID_MENU_RENAME_FILE_IN_TREE, _guilang.get("GUI_TREE_PUP_RENAMEFILE"));

        wxPoint p = event.GetPoint();
        m_fileTree->PopupMenu(&popupMenu, p);
    }
    else
    {
        wxTreeItemId clickedItem = event.GetItem();
        m_clickedTreeItem = clickedItem;
        wxMenu popupMenu;

        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(clickedItem));

        if (data->isDir)
            return;

        popupMenu.Append(ID_MENU_INSERT_IN_EDITOR_FROM_TREE, _guilang.get("GUI_TREE_PUP_INSERT_EDITOR"));
        popupMenu.Append(ID_MENU_INSERT_IN_CONSOLE_FROM_TREE, _guilang.get("GUI_TREE_PUP_INSERT_CONSOLE"));

        if (data->isCommand)
        {
            popupMenu.AppendSeparator();
            popupMenu.Append(ID_MENU_HELP_ON_ITEM, _guilang.get("GUI_TREE_PUP_HELPONITEM", m_functionTree->GetItemText(clickedItem).ToStdString()));
        }

        wxPoint p = event.GetPoint();
        m_functionTree->PopupMenu(&popupMenu, p);
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private OnTreeItemActivated
///  Attempts to open a file when an item is double-clicked in the project tree
///
///  @param  event wxTreeEvent & The generated tree event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnTreeItemActivated(wxTreeEvent &event)
{
    if (event.GetId() == ID_PROJECTTREE)
    {
        wxTreeItemId item = event.GetItem();

        if(item == m_projectFileFolders[0]
            || item == m_projectFileFolders[1]
            || item == m_projectFileFolders[2]
            || item == m_projectFileFolders[3]
            || item == m_projectFileFolders[4])
        {
            m_fileTree->Toggle(item);
            return;
        }

        wxTreeItemId rootItem = m_fileTree->GetRootItem();

        if (item != rootItem)
        {
            //wxTreeItemId parentItem = m_projectTree->GetItemParent(item);
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_fileTree->GetItemData(item));
            wxFileName pathname = data->filename;
            if (data->isDir && m_fileTree->HasChildren(item))
            {
                m_fileTree->Toggle(item);
                return;
            }
            else if (data->filename.find('.') == std::string::npos || data->isDir)
                return;

            OpenFileByType(pathname);
        }
        m_book->getCurrentEditor()->Refresh();
        m_book->Refresh();
    }
    else
    {
        NumeReEditor* edit = m_book->getFocusedEditor();
        wxTreeItemId item = event.GetItem();
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(item));
        if (data->isDir)
        {
            m_functionTree->Toggle(item);
        }
        else if (data->isCommand)
        {
            edit->InsertText(edit->GetCurrentPos(), (data->tooltip).substr(0, (data->tooltip).find(' ')+1));
            edit->GotoPos(edit->GetCurrentPos()+(data->tooltip).find(' ')+1);
        }
        else if (data->isFunction)
        {
            edit->InsertText(edit->GetCurrentPos(), (data->tooltip).substr(0, (data->tooltip).find('(')+1));
            edit->GotoPos(edit->GetCurrentPos()+(data->tooltip).find('(')+1);
        }
        else if (data->isMethod)
        {
            size_t dot = (data->tooltip).find('.');
            size_t endPos = (data->tooltip).find_first_of("( ", dot);

            if ((data->tooltip)[endPos] == '(')
                endPos++;

            edit->InsertText(edit->GetCurrentPos(), (data->tooltip).substr(dot, endPos-dot));
            edit->GotoPos(edit->GetCurrentPos()+endPos-dot);
        }
        else if (data->isConstant)
        {
            edit->InsertText(edit->GetCurrentPos(), (data->tooltip).substr(0, (data->tooltip).find(' ')));
            edit->GotoPos(edit->GetCurrentPos()+(data->tooltip).find(' '));
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// tooltip requested for the item below the mouse
/// cursor.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnTreeItemToolTip(wxTreeEvent& event)
{
    if (event.GetId() == ID_FUNCTIONTREE)
    {
        wxTreeItemId item = event.GetItem();

        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(item));

        if (data->isDir)
            return;
        else if (data->isFunction || data->isCommand || data->isMethod)
            event.SetToolTip(this->addLinebreaks(data->tooltip));
        else if (data->isConstant)
            event.SetToolTip(data->tooltip);
    }
    else
    {
        wxTreeItemId item = event.GetItem();

        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(item));

        if (!data || data->isDir)
            return;

        // Show revision count (if any)
        VersionControlSystemManager manager(this);

        wxFileName pathname = data->filename;
        wxString tooltip;

        if (pathname.GetExt() == "nprc")
        {
            NumeRe::CallTip _cTip = NumeRe::addLinebreaks(NumeRe::FindProcedureDefinition((pathname.GetPath() + "\\" + pathname.GetName()).ToStdString(), "$" + pathname.GetName().ToStdString()), 75);

            tooltip = _guilang.get("COMMON_FILETYPE_NPRC");

            if (manager.hasRevisions(pathname.GetFullPath()))
            {
                std::unique_ptr<FileRevisions> revisions(manager.getRevisions(pathname.GetFullPath()));

                if (revisions.get())
                    tooltip += " (" + revisions->getCurrentRevision() + ")";
            }

            tooltip += "\n";
            tooltip.append(80u, '-');
            tooltip += "\n" + _cTip.sDefinition + "\n" + _cTip.sDocumentation;
        }
        else
        {
            tooltip = _guilang.get("COMMON_FILETYPE_" + toUpperCase(pathname.GetExt().ToStdString()));

            if (pathname.GetExt() == "ndat")
                tooltip += getFileDetails(pathname);

            if (manager.hasRevisions(pathname.GetFullPath()))
            {
                std::unique_ptr<FileRevisions> revisions(manager.getRevisions(pathname.GetFullPath()));

                if (revisions.get())
                    tooltip += "\n(" + revisions->getCurrentRevision() + ")";
            }
        }

        event.SetToolTip(tooltip);
    }
}


/////////////////////////////////////////////////
/// \brief This member function prepares the data
/// to be dragged from one of the both trees.
///
/// \param event wxTreeEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnTreeDragDrop(wxTreeEvent& event)
{
    if (event.GetId() == ID_FUNCTIONTREE)
    {
        wxTreeItemId item = event.GetItem();
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(item));

        if (!data->isCommand && !data->isFunction && !data->isConstant && !data->isMethod)
            return;

        wxString token = data->tooltip;

        if (data->isMethod)
        {
            size_t dot = token.find('.');
            size_t endPos = token.find_first_of("( ", dot);

            if (token[endPos] == '(')
                endPos++;

            token = token.substr(dot, endPos-dot);
        }
        else
        {
            token.erase(token.find(' '));

            if (token.find('(') != std::string::npos)
                token.erase(token.find('(')+1);
            else
                token += " ";
        }

        wxTextDataObject _dataObject(token);
        wxDropSource dragSource(this);
        dragSource.SetData(_dataObject);
        dragSource.DoDragDrop(wxDrag_AllowMove);
    }
    else
    {
        wxTreeItemId item = event.GetItem();
        m_dragDropSourceItem = item;
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(item));

        if (!data || data->isDir)
            return;

        wxFileName pathname = data->filename;
        wxString dragableExtensions = ";nscr;nprc;ndat;nlyt;txt;dat;log;tex;csv;xls;xlsx;ods;jdx;jcm;dx;labx;ibw;png;jpg;jpeg;gif;bmp;eps;svg;m;cpp;cxx;c;hpp;hxx;h;";

        if (dragableExtensions.find(";" + pathname.GetExt().Lower() + ";") != std::string::npos)
        {
            wxFileDataObject _dataObject;
            _dataObject.AddFile(pathname.GetFullPath());
            wxDropSource dragSource(this);
            dragSource.SetData(_dataObject);
            dragSource.DoDragDrop(wxDrag_AllowMove);
        }

    }
}


/////////////////////////////////////////////////
/// \brief This member function adds line break
/// characters to the passed string to stick below
/// a line length of 70 characters. Used by the
/// tree tooltips.
///
/// \param sLine const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReWindow::addLinebreaks(const wxString& sLine)
{
    const unsigned int nMAXLINE = 70;

    wxString sReturn = sLine;

    while (sReturn.find("\\$") != std::string::npos)
        sReturn.erase(sReturn.find("\\$"),1);

    unsigned int nDescStart = sReturn.find("- ");
    unsigned int nIndentPos = 4;//
    unsigned int nLastLineBreak = 0;
    sReturn.replace(nDescStart, 2,"\n    ");
    nLastLineBreak = nDescStart;

    for (unsigned int i = nDescStart; i < sReturn.length(); i++)
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


/////////////////////////////////////////////////
/// \brief This member function displays extended
/// file informations of NDAT files, if this was
/// enabled in the settings.
///
/// \param filename const wxFileName&
/// \return wxString
///
/////////////////////////////////////////////////
wxString NumeReWindow::getFileDetails(const wxFileName& filename)
{
    if (m_options->showExtendedFileInfo())
        return "\n" + getFileInfo(filename.GetFullPath().ToStdString());
    else
        return "NOTHING";
}


/////////////////////////////////////////////////
/// \brief This member function loads the file
/// details to the file tree.
///
/// \param fromPath wxString
/// \param fileType FileFilterType
/// \param treeid wxTreeItemId
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::LoadFilesToTree(wxString fromPath, FileFilterType fileType, wxTreeItemId treeid)
{
    g_logger.info("Loading files from '" + fromPath.ToStdString() + "' into file tree.");
    wxDir currentDir(fromPath);
    DirTraverser _traverser(m_fileTree, m_iconManager, treeid, fromPath, fileType);
    currentDir.Traverse(_traverser);
}


//////////////////////////////////////////////////////////////////////////////
///  private OnFindEvent
///  Handles find/replace events
///
///  @param  event wxFindDialogEvent & The generated find/replace event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnFindEvent(wxFindDialogEvent& event)
{
    wxEventType type = event.GetEventType();
    wxString findString = event.GetFindString();
    long flags = event.GetFlags();
    NumeReEditor* edit = m_book->getFocusedEditor();
    int pos = edit->GetCurrentPos();

    if ((type == wxEVT_COMMAND_FIND) ||
        (type == wxEVT_COMMAND_FIND_NEXT))
    {
        pos = FindString(findString, pos, flags, true);

        if (pos < 0)
        {
            wxMessageBox(_guilang.get("GUI_SEARCH_END", findString.ToStdString()),
                _guilang.get("GUI_SEARCH_END_HEAD"), wxOK | wxICON_EXCLAMATION, this);
        }
    }
    else if (type == wxEVT_COMMAND_FIND_REPLACE)
    {
        if ((flags & wxFR_MATCHCASE && edit->GetSelectedText() != findString)
            || (!(flags & wxFR_MATCHCASE) && toLowerCase(edit->GetSelectedText().ToStdString()) != toLowerCase(findString.ToStdString())))
        {
            wxBell();
            return;
        }

        pos = edit->GetSelectionStart();
        wxString replaceString = event.GetReplaceString();
        edit->ReplaceSelection(replaceString);
        edit->EnsureCaretVisible();
        edit->SetSelection(pos, pos + replaceString.length());
    }
    else if (type == wxEVT_COMMAND_FIND_REPLACE_ALL)
    {
        wxString replaceString = event.GetReplaceString();
        if (findString == replaceString)
            return;

        wxBusyCursor busy;
        int count = ReplaceAllStrings(findString, replaceString, flags);

        g_findReplace->toggleSkipFocus();
        wxMessageBox(_guilang.get("GUI_REPLACE_END", toString(count), findString.ToStdString(), replaceString.ToStdString()), _guilang.get("GUI_REPLACE_END_HEAD"), wxOK, this);
        g_findReplace->toggleSkipFocus();
    }
    else if (type == wxEVT_COMMAND_FIND_CLOSE)
    {
        if (wxDynamicCast(event.GetEventObject(), wxDialog))
        {
            ((wxDialog*)event.GetEventObject())->Destroy();
        }
        g_findReplace = nullptr;
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private FindString
///  Looks for a given string in the current editor
///
///  @param  findString const wxString & The string to find
///  @param  start_pos  int              [=-1] The offset to begin searching (-1 for the whole document)
///  @param  flags      int              [=-1] The selected find/replace options
///  @param  highlight  bool             [=1] Whether or not to select the found text
///
///  @return int        The offset of the found text
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int NumeReWindow::FindString(const wxString &findString, int start_pos, int flags, bool highlight)
{
    NumeReEditor* edit = m_book->getFocusedEditor();

    if (findString.IsEmpty())
        return wxNOT_FOUND;

    int stc_flags = 0;
    if ((flags & wxFR_MATCHCASE) != 0)
    {
        stc_flags |= wxSTC_FIND_MATCHCASE;
    }
    if ((flags & wxFR_WHOLEWORD) != 0)
    {
        stc_flags |= wxSTC_FIND_WHOLEWORD;
    }

    int pos = start_pos == -1 ? edit->GetCurrentPos() : start_pos;

    if ((flags & wxFR_DOWN) != 0)
    {
        edit->SetTargetStart(wxMax(0, pos));
        edit->SetTargetEnd(wxMax(0, edit->GetTextLength()));
    }
    else
    {
        if (labs(edit->GetTargetEnd() - edit->GetTargetStart()) == long(findString.length()))
        {
            pos -= findString.length() + 1; // doesn't matter if it matches or not, skip it
        }

        edit->SetTargetStart(wxMax(0, pos));
        edit->SetTargetEnd(0);
    }

    edit->SetSearchFlags(stc_flags);
    pos = edit->SearchInTarget(findString);

    if (pos >= 0)
    {
        if (highlight)
        {
            edit->GotoPos(pos+findString.length());
            edit->SetSelection(pos, pos + findString.length());
            edit->EnsureLineVisibility(edit->GetCurrentLine());
            edit->EnsureCaretVisible();
        }
    }
    else if (flags & wxFR_WRAPAROUND)
    {
        if ((flags & wxFR_DOWN) && start_pos)
            return FindString(findString, 0, flags, highlight);
        else if (!(flags & wxFR_DOWN) && start_pos != edit->GetLastPosition())
            return FindString(findString, edit->GetLastPosition(), flags, highlight);
    }

    return pos;
}


//////////////////////////////////////////////////////////////////////////////
///  private ReplaceAllStrings
///  Replaces all occurrences of the given string in the current editor
///
///  @param  findString    const wxString & The string to find and replace
///  @param  replaceString const wxString & The string to insert
///  @param  flags         int              [=-1] The selected find/replace flags
///
///  @return int           The number of matches replaced
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int NumeReWindow::ReplaceAllStrings(const wxString &findString, const wxString &replaceString, int flags)
{
    int count = 0;
    NumeReEditor* edit = m_book->getFocusedEditor();

    if (findString.IsEmpty() || (findString == replaceString))
        return count;

    int cursor_pos = edit->GetCurrentPos();  // return here when done
    if (flags & wxFR_WRAPAROUND)
    {
        edit->GotoPos(0);
        flags &= flags & ~wxFR_WRAPAROUND;
    }
    edit->BeginUndoAction();
    if (edit->GetSelectedText() == findString)
    {
        ++count;
        edit->ReplaceSelection(replaceString);
    }

    int pos = FindString(findString, -1, flags, true);

    while (pos != -1)
    {
        ++count;
        edit->ReplaceSelection(replaceString);
        pos = FindString(findString, -1, flags, true);
    }

    // return to starting pos or as close as possible
    edit->GotoPos(wxMin(cursor_pos, edit->GetLength()));
    edit->EndUndoAction();

    return count;
}


//////////////////////////////////////////////////////////////////////////////
///  private DeterminePrintSize
///  Returns an appropriate size for printing
///
///  @return wxRect The dimensions of the print area
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxRect NumeReWindow::DeterminePrintSize()
{
    wxSize scr = wxGetDisplaySize();

    // determine position and size (shifting 16 left and down)
    wxRect rect = GetRect();
    rect.x += 16;
    rect.y += 16;
    rect.width = wxMin (rect.width, (scr.x - rect.x));
    rect.height = wxMin (rect.height, (scr.x - rect.y));

    return rect;
}


/////////////////////////////////////////////////
/// \brief Wrapper for the corresponding function
/// of the editor.
///
/// \param procedureName const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::FindAndOpenProcedure(const wxString& procedureName)
{
    m_book->getFocusedEditor()->FindAndOpenProcedure(procedureName);
}


/////////////////////////////////////////////////
/// \brief Updates the editor's and notebook's
/// filename location, if it already opened and
/// not modified.
///
/// \param fname const wxFileName&
/// \param newFName const wxFileName&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::UpdateLocationIfOpen(const wxFileName& fname, const wxFileName& newFName)
{
    int num = GetPageNum(fname, true, 0);

    if (num != wxNOT_FOUND)
    {
        NumeReEditor* edit = m_book->getEditor(num); // Might need adaption

        if (edit->Modified())
            return;

        edit->SetFilename(newFName, false);
        m_book->SetTabText(num, edit->GetFileNameAndPath());
        m_book->Refresh();
        UpdateWindowTitle(m_book->GetPageText(m_book->GetSelection()));
    }
}


/////////////////////////////////////////////////
/// \brief Reloads a file if it is open in any
/// editor.
///
/// \param fname const wxString&
/// \param force bool
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::reloadFileIfOpen(const wxString& fname, bool force)
{
    NumeReEditor* edit;
    wxString fileContents;
    wxString fileNameNoPath;

    if (!GetFileContents(fname, fileContents, fileNameNoPath))
        return;

    // Search the file in the list of currently
    // opened files
    for (size_t j = 0; j < m_book->GetPageCount(); j++)
    {
        edit = m_book->getEditor(j);

        // Found it?
        if (edit && edit->GetFileNameAndPath() == fname)
        {
            g_logger.info("Reloading '" + fname.ToStdString() + "' to editor.");
            m_filesLastSaveTime[fname] = 0;

            // If the user has modified the file, as
            // him to reload the file, otherwise re-
            // load automatically
            if (edit->IsModified() && !force)
            {
                m_book->SetSelection(j);
                int answer = wxMessageBox(_guilang.get("GUI_DLG_FILEMODIFIED_QUESTION", fname.ToStdString()), _guilang.get("GUI_DLG_FILEMODIFIED"), wxYES_NO | wxICON_QUESTION, this);

                if (answer == wxYES)
                {
                    int pos = m_book->getFocusedEditor()->GetCurrentPos();
                    m_book->getCurrentEditor()->LoadFileText(fileContents);
                    m_book->getCurrentEditor()->MarkerDeleteAll(MARKER_SAVED);
                    m_book->getFocusedEditor()->GotoPos(pos);
                    m_filesLastSaveTime[fname] = time(0);
                }
            }
            else
            {
                int pos = edit->GetCurrentPos();
                edit->LoadFileText(fileContents);
                edit->MarkerDeleteAll(MARKER_SAVED);
                edit->GotoPos(pos);
                m_filesLastSaveTime[fname] = time(0);
            }

            return;
        }
    }
}


/////////////////////////////////////////////////
/// \brief Registers a new window in the internal
/// map.
///
/// \param window wxWindow*
/// \param type WindowType
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::registerWindow(wxWindow* window, WindowType type)
{
    if (m_openedWindows.find(window) == m_openedWindows.end())
        m_openedWindows[window] = type;
}


/////////////////////////////////////////////////
/// \brief Removes the passed window form the
/// internal window list (only if it exists).
///
/// \param window wxWindow*
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::unregisterWindow(wxWindow* window)
{
    if (m_openedWindows.find(window) != m_openedWindows.end())
        m_openedWindows.erase(window);
}


/////////////////////////////////////////////////
/// \brief Find a window of a specified type.
///
/// \param type WindowType
/// \return wxWindow*
///
/////////////////////////////////////////////////
wxWindow* NumeReWindow::findWindow(WindowType type)
{
    for (const auto& iter : m_openedWindows)
    {
        if (iter.second == type)
            return iter.first;
    }

    return nullptr;
}


/////////////////////////////////////////////////
/// \brief Close all windows of the selected
/// WindowType or simply use WT_ALL to close all
/// terminal-closable floating windows at once.
///
/// \param type WindowType
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::closeWindows(WindowType type)
{
    auto iter = m_openedWindows.begin();

    while (iter != m_openedWindows.end())
    {
        if (type == WT_ALL || type == iter->second)
        {
            // Copy pointer to avoid issues with
            // iterator invalidation
            wxWindow* w = iter->first;
            iter = m_openedWindows.erase(iter);
            w->Destroy();
        }
        else
            ++iter;
    }
}


/////////////////////////////////////////////////
/// \brief This public member function returns
/// the default icon usable by different windows.
///
/// \return wxIcon
///
/////////////////////////////////////////////////
wxIcon NumeReWindow::getStandardIcon()
{
    return wxIcon(getProgramFolder()+"\\icons\\icon.ico", wxBITMAP_TYPE_ICO);
}


/////////////////////////////////////////////////
/// \brief Notifies all instances of the
/// PackagRepoBrowser to refresh its internal
/// list of installed packages and refreshes the
/// package menu.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::notifyInstallationDone()
{
    UpdatePackageMenu();

    wxWindowList& winlist = GetChildren();

    for (size_t win = 0; win < winlist.GetCount(); win++)
    {
        if (winlist[win]->IsShown() && winlist[win]->GetLabel() == PACKAGE_REPO_BROWSER_TITLE)
        {
            static_cast<PackageRepoBrowser*>(winlist[win])->DetectInstalledPackages();
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private OnSplitterDoubleClick
///  Cancels the ability to "close" a split window by double-clicking the splitter bar
///
///  @param  event wxSplitterEvent & The generated splitter event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnSplitterDoubleClick(wxSplitterEvent &event)
{
    event.Veto();
}


/////////////////////////////////////////////////
/// \brief This member function opens a file
/// dialog to let the user choose the files to
/// open in the editor.
///
/// \param id int
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnOpenSourceFile(int id )
{
    if( id == ID_MENU_OPEN_SOURCE_LOCAL)
        m_remoteMode = false;
    else if(id == ID_MENU_OPEN_SOURCE_REMOTE)
        m_remoteMode = true;

    wxArrayString fnames = OpenFile(FILE_SUPPORTEDFILES);

    if(fnames.Count() > 0)
        OpenFilesFromList(fnames);
}


/////////////////////////////////////////////////
/// \brief This member function saves the file in
/// the current editor.
///
/// \param id int
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnSaveSourceFile( int id )
{
    if( id == ID_MENU_SAVE_SOURCE_LOCAL)
        m_remoteMode = false;
    else if(id == ID_MENU_SAVE_SOURCE_REMOTE)
        m_remoteMode = true;

    SaveCurrentFile(true);
}


/////////////////////////////////////////////////
/// \brief This function executes the file in the
/// current editor.
///
/// \param sFileName const std::string&
/// \param id int
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnExecuteFile(const std::string& sFileName, int id)
{
    if (!sFileName.length())
        return;

    std::string command = replacePathSeparator(sFileName);
    std::vector<std::string> vPaths = m_terminal->getPathSettings();

    if (command.rfind(".nprc") != std::string::npos)
    {
        if (m_options->getSetting(SETTING_B_AUTOSAVEEXECUTION).active())
            SaveAll(true);

        command.erase(command.rfind(".nprc"));

        if (command.substr(0, vPaths[PROCPATH].length()) == vPaths[PROCPATH])
        {
            command.erase(0, vPaths[PROCPATH].length());

            while (command.front() == '/')
                command.erase(0, 1);

            while (command.find('/') != std::string::npos)
                command[command.find('/')] = '~';
        }
        else
            command = "'" + command + "'";

        command = "$" + command + "()";
    }
    else if (command.rfind(".nscr") != std::string::npos)
    {
        if (m_options->getSetting(SETTING_B_AUTOSAVEEXECUTION).active())
            SaveAll(true);

        command.erase(command.rfind(".nscr"));

        if (command.substr(0, vPaths[SCRIPTPATH].length()) == vPaths[SCRIPTPATH])
            command.erase(0, vPaths[SCRIPTPATH].length());

        while (command.front() == '/')
            command.erase(0, 1);

        command = "start \"" + command + "\"";
    }
    else if (command.rfind(".nlyt") != std::string::npos)
    {
        if (m_options->getSetting(SETTING_B_AUTOSAVEEXECUTION).active())
            SaveAll(true);

        command.erase(command.rfind(".nlyt"));

        if (command.substr(0, vPaths[SCRIPTPATH].length()) == vPaths[SCRIPTPATH])
            command.erase(0, vPaths[SCRIPTPATH].length());

        if (command.substr(0, vPaths[PROCPATH].length()) == vPaths[PROCPATH])
            command = "<procpath>" + command.substr(vPaths[PROCPATH].length());

        while (command.front() == '/')
            command.erase(0, 1);

        command = "window \"" + command + "\"";
    }
    else if (id == ID_MENU_OPEN_FILE_FROM_TREE_TO_TABLE)
        command = "load \"" + command + "\" -totable";
    else
        command = "load \"" + command + "\" -app -ignore";

    showConsole();
    m_terminal->pass_command(command, false);
}


/////////////////////////////////////////////////
/// \brief This member function runs the dependency
/// calculating process in the procedure library.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnCalculateDependencies()
{
    if (m_book->getCurrentEditor()->getFileType() != FILE_NPRC)
        return;

    ProcedureLibrary& procLib = m_terminal->getKernel().getProcedureLibrary();

    try
    {
        DependencyDialog dlg(this,
                             wxID_ANY,
                             _guilang.get("GUI_DEPDLG_HEAD", m_book->getCurrentEditor()->GetFilenameString().ToStdString()),
                             m_book->getCurrentEditor()->GetFileNameAndPath().ToStdString(), procLib);
        dlg.ShowModal();
    }
    catch (SyntaxError& e)
    {
        wxMessageBox(_guilang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3])), _guilang.get("ERR_NR_HEAD"), wxCENTER | wxICON_ERROR | wxOK);
    }

}


/////////////////////////////////////////////////
/// \brief This member function launches the
/// package creator dialog and creates the install
/// file, if the user confirms his selection.
///
/// \param projectFile const wxString&
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnCreatePackage(const wxString& projectFile)
{
    try
    {
        PackageDialog dlg(this, m_terminal, m_iconManager);
        NumeReEditor* edit = m_book->getCurrentEditor();

        if (projectFile.length())
            dlg.loadProjectFile(projectFile);
        else if (edit->getFileType() == FILE_NPRC
            || (edit->getFileType() == FILE_NSCR && edit->GetFileName().GetExt() == "nlyt"))
            dlg.setMainFile(edit->GetFileNameAndPath());

        if (dlg.ShowModal() == wxID_OK)
        {
            wxString installinfo = dlg.getInstallInfo();
            wxString identifier = dlg.getPackageIdentifier();
            wxArrayString procedures = dlg.getProcedures();
            std::string sProcPath = m_terminal->getPathSettings()[PROCPATH];

            // Ensure that the user provided at least a single
            // procedure for the new package
            if (!procedures.size())
                return;

            NewFile(FILE_NSCR, "packages/" + identifier);
            edit = m_book->getCurrentEditor();

            edit->AddText("<install>\r\n" + installinfo + "\r\n");

            for (size_t i = 0; i < procedures.size(); i++)
            {
                std::vector<std::string> contents;

                // Get the file's contents
                contents = getFileForInstaller(procedures[i].ToStdString());

                // Insert the prepared contents
                for (size_t j = 0; j < contents.size(); j++)
                {
                    edit->AddText("\t" + contents[j] + "\r\n");
                }

                edit->AddText("\r\n");
            }

            if (dlg.includeDocs())
            {
                wxString template_file, dummy;
                GetFileContents(getProgramFolder() + "\\lang\\tmpl_documentation.nlng", template_file, dummy);

                // Replace the tokens in the file
                template_file.Replace("%%IDENTIFIER%%", identifier);
                template_file.Replace("%%PACKAGENAME%%", dlg.getPackageName());
                template_file.Replace("%%IDXKEY%%", dlg.getPackageIdentifier());
                template_file.Replace("%%KEYWORD%%", dlg.getPackageIdentifier());

                edit->AddText(template_file + "\r\n");
            }
            else if (dlg.getDocFile().length())
            {
                wxFile docfile(dlg.getDocFile());
                wxString fcontents;

                if (docfile.ReadAll(&fcontents))
                    edit->AddText(fcontents + "\r\n");
            }

            edit->AddText("\treturn;\r\n<endinstall>\r\n");
            edit->AddText("\r\nwarn \"" + _guilang.get("GUI_PKGDLG_INSTALLERWARNING", identifier.ToStdString()) + "\"\r\n");
            edit->UpdateSyntaxHighlighting(true);
            edit->ApplyAutoIndentation(0, edit->GetNumberOfLines());

            // Deactivate the indent on type in this case
            if (edit->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE))
                edit->ToggleSettings(NumeReEditor::SETTING_INDENTONTYPE);

            // Save the file directly
            edit->SaveFileLocal();

            std::string sPackageName = dlg.getPackageName().ToStdString();

            if (wxYES == wxMessageBox(_guilang.get("GUI_PKGDLG_UPDATEINSTALLED", sPackageName),
                                      _guilang.get("GUI_PKGDLG_UPDATEINSTALLED_HEAD"), wxYES_NO | wxICON_QUESTION, this))
            {
                std::string sPackage = installinfo.ToStdString();
                replaceAll(sPackage, "\r\n", " ");
                replaceAll(sPackage, "\t", " ");
                replaceAll(sPackage, "<info>", "");
                replaceAll(sPackage, "<endinfo>", "");

                m_terminal->updatePackage(sPackage);
                notifyInstallationDone();
            }

            // Check, whether this package is already installed and
            // ask the user to synchronize the package versions
            //const std::vector<Package>& vPackages = m_terminal->getInstalledPackages();
            //std::string sPackageName = dlg.getPackageName().ToStdString();
            //
            //for (const auto& package : vPackages)
            //{
            //    if (package.getName() == sPackageName)
            //    {
            //        size_t nInstalledVersion = versionToInt(package.sVersion);
            //
            //        // Compare the installed version with the currently
            //        // packed version and ask the user, whether we shall
            //        // synchronize those two
            //        if (nInstalledVersion < versionToInt(dlg.getPackageVersion().ToStdString())
            //            && wxYES == wxMessageBox(_guilang.get("GUI_PKGDLG_UPDATEINSTALLED", sPackageName),
            //                                     _guilang.get("GUI_PKGDLG_UPDATEINSTALLED_HEAD"), wxYES_NO | wxICON_QUESTION, this))
            //        {
            //            std::string sPackage = installinfo.ToStdString();
            //            replaceAll(sPackage, "\r\n", " ");
            //            replaceAll(sPackage, "\t", " ");
            //            replaceAll(sPackage, "<info>", "");
            //            replaceAll(sPackage, "<endinfo>", "");
            //
            //            m_terminal->updatePackage(sPackage);
            //        }
            //
            //        break;
            //    }
            //}
        }
    }
    catch (SyntaxError& e)
    {
        wxMessageBox(_guilang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3])),
                     _guilang.get("ERR_NR_HEAD"),
                     wxCENTER | wxICON_ERROR | wxOK);
    }
}


/////////////////////////////////////////////////
/// \brief Event handler for comparing two files
/// using the diff functionalities.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnCompareFiles()
{
    // Search first file
    std::string file1 = wxFileSelector(_guilang.get("GUI_DLG_COMPARE_FILE_1"), "", wxEmptyString, wxEmptyString,
                                       wxFileSelectorDefaultWildcardStr, wxFD_OPEN | wxFD_FILE_MUST_EXIST, this).ToStdString();

    // Abort, if the user clicked cancel
    if (!file1.length())
        return;

    // Search second file
    std::string file2 = wxFileSelector(_guilang.get("GUI_DLG_COMPARE_FILE_1"), "", wxEmptyString, wxEmptyString,
                                       wxFileSelectorDefaultWildcardStr, wxFD_OPEN | wxFD_FILE_MUST_EXIST, this).ToStdString();

    // Abort, if the user clicked cancel
    if (!file2.length())
        return;

    // Calculate the unified diff of both files
    std::string diff = compareFiles(file1, file2);

    wxFileName fn1(file1);
    wxFileName fn2(file2);

    // Show the calculated diff with the names of both files
    // as "file name"
    ShowRevision(fn1.GetName() + "." + fn1.GetExt() + "-" + fn2.GetName() + "." + fn2.GetExt() + ".diff", diff);
}


/////////////////////////////////////////////////
/// \brief This member function displays the find
/// and replace dialog.
///
/// \param id int
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnFindReplace(int id)
{
    if (m_book->getFocusedEditor()->HasSelection())
        m_findData.SetFindString(m_book->getFocusedEditor()->GetSelectedText());

    if (g_findReplace != nullptr)
    {
        bool isReplaceDialog = g_findReplace->GetWindowStyle() & wxFR_REPLACEDIALOG;

        if ((isReplaceDialog && (id == ID_MENU_REPLACE)) || (!isReplaceDialog && (id == ID_MENU_FIND)))
            return;
        else
            delete g_findReplace;
    }
    else
        m_findData.SetFlags(wxFR_DOWN | wxFR_WRAPAROUND);

    bool showFind = (id == ID_MENU_FIND);
    int dialogFlags = showFind ? 0 : wxFR_REPLACEDIALOG;
    wxString title = showFind ? _guilang.get("GUI_DLG_FIND") : _guilang.get("GUI_DLG_REPLACE");

    g_findReplace = new FindReplaceDialog(this,	&m_findData, title, dialogFlags);
    g_findReplace->Show(true);
    g_findReplace->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// settings dialog and performs all necessary
/// updates, if the user confirms his changes.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnOptions()
{
    m_options->copySettings(m_terminal->getKernelSettings());
    m_optionsDialog->InitializeDialog();

    int result = m_optionsDialog->ShowModal();

    if (result == wxID_OK)
    {
        m_terminal->setKernelSettings(*m_options);
        EvaluateOptions();
        m_history->UpdateSyntaxHighlighting();
        m_terminal->SetFont(m_options->toFont(m_options->getSetting(SETTING_S_TERMINALFONT).stringval()));
        m_terminal->UpdateColors();

        m_splitEditorOutput->SetCharHeigth(m_terminal->GetCharHeight());
        m_splitCommandHistory->SetCharHeigth(m_terminal->GetCharHeight());
        m_splitEditorOutput->SetSashPositionFloat(m_splitEditorOutput->GetSplitPercentage());
        m_splitCommandHistory->SetSashPositionFloat(m_splitCommandHistory->GetSplitPercentage());

        m_termContainer->SetBackgroundColour(m_options->GetSyntaxStyle(Options::CONSOLE_STD).background);
        m_termContainer->Refresh();

        for (size_t i = 0; i < m_book->GetPageCount(); i++)
        {
            NumeReEditor* edit = m_book->getEditor(i); // Might need adaption
            edit->UpdateSyntaxHighlighting(true);
            edit->SetEditorFont(m_options->GetEditorFont());

            if (m_book->getEditor(i, true))
            {
                edit = m_book->getEditor(i, true);
                edit->UpdateSyntaxHighlighting(true);
                edit->SetEditorFont(m_options->GetEditorFont());
            }
        }
    }

    m_book->getCurrentEditor()->SetFocus();
}


/////////////////////////////////////////////////
/// \brief This member function prints the styled
/// text of the current editor.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnPrintPage()
{
    m_book->getCurrentEditor()->SetPrintColourMode(m_options->GetPrintStyle());

    if (!g_printData->IsOk())
        this->OnPrintSetup();

    wxPrintDialogData printDialogData( *g_printData);
    wxPrinter printer (&printDialogData);
    NumeRePrintout printout (m_book->getCurrentEditor(), m_options);

    if (!printer.Print (this, &printout, true))
    {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
        {
            wxMessageBox (_guilang.get("GUI_PRINT_ERROR"), _guilang.get("GUI_PRINT_ERROR_HEAD"), wxOK | wxICON_WARNING);
            return;
        }
    }

    (*g_printData) = printer.GetPrintDialogData().GetPrintData();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// styled text of the current editor in the print
/// preview window.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnPrintPreview()
{
    m_book->getCurrentEditor()->SetPrintColourMode(m_options->GetPrintStyle());

    if (!g_printData->IsOk())
        this->OnPrintSetup();

    wxPrintDialogData printDialogData( *g_printData);
    wxPrintPreview *preview = new wxPrintPreview(new NumeRePrintout(m_book->getCurrentEditor(), m_options),
                                                 new NumeRePrintout(m_book->getCurrentEditor(), m_options),
                                                 &printDialogData);

    if (!preview->Ok())
    {
        delete preview;
        wxMessageBox (_guilang.get("GUI_PREVIEW_ERROR"), _guilang.get("GUI_PREVIEW_ERROR_HEAD"), wxOK | wxICON_WARNING);
        return;
    }

    wxRect rect = DeterminePrintSize();
    wxPreviewFrame *frame = new wxPreviewFrame (preview, this, _guilang.get("GUI_PREVIEW_HEAD"));
    frame->SetSize (rect);
    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
    frame->Maximize();
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// printing page setup dialog.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnPrintSetup()
{
    (*g_pageSetupData) = *g_printData;
    wxPageSetupDialog pageSetupDialog(this, g_pageSetupData);
    pageSetupDialog.ShowModal();
    (*g_printData) = pageSetupDialog.GetPageSetupData().GetPrintData();
    (*g_pageSetupData) = pageSetupDialog.GetPageSetupData();
}


/////////////////////////////////////////////////
/// \brief This member function catches the new
/// file toolbar button and asks the user for a
/// specific file type.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnAskForNewFile()
{
    NewFileDialog dialog(this, getProgramFolder() + "/icons");
    dialog.SetIcon(getStandardIcon());
    int ret = dialog.ShowModal();

    if (ret != wxID_CANCEL)
        NewFile(dialog.GetSelection());
}


/////////////////////////////////////////////////
/// \brief This member function displays the help
/// root page.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnHelp()
{
    ShowHelp("numere");
}


/////////////////////////////////////////////////
/// \brief This member function displays the help
/// page for the selected documentation ID.
///
/// \param sDocId const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReWindow::ShowHelp(const wxString& sDocId)
{
    wxWindow* openedBrowser = findWindow(WT_DOCVIEWER);

    if (openedBrowser)
    {
        DocumentationBrowser* browser = static_cast<DocumentationBrowser*>(openedBrowser);

        if (browser->IsIconized())
            browser->Restore();

        return browser->createNewPage(sDocId);
    }

    DocumentationBrowser* browser = new DocumentationBrowser(this, _guilang.get("DOC_HELP_HEADLINE", "%s"), this);
    registerWindow(browser, WT_DOCVIEWER);
    return browser->SetStartPage(sDocId);
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// "About" dialog.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReWindow::OnAbout()
{
    AboutChameleonDialog acd(this, 10000, _guilang.get("GUI_ABOUT_TITLE"));
    acd.ShowModal();
}

