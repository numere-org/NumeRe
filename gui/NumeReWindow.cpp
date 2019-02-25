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
#include <wx/msw/helpchm.h>


//need to include <wx/filename.h> and <wx/stdpaths.h>
#include <math.h>

#include "NumeReNotebook.h"
#include "numeredroptarget.hpp"
#include "DirTraverser.hpp"
#include "IconManager.h"
#include "wxProportionalSplitterWindow.h"
#include "ChameleonProjectManager.h"
#include "viewerframe.hpp"
#include "imagepanel.hpp"
#include "helpviewer.hpp"
#include "documentationbrowser.hpp"
#include "tableviewer.hpp"
#include "tableeditpanel.hpp"
#include "graphviewer.hpp"
#include "textsplashscreen.hpp"

#include "editor/editor.h"
#include "editor/history.hpp"
#include "editor/NumeRePrintout.h"

#include "dialogs/OptionsDialog.h"
#include "dialogs/RemoteFileDialog.h"
#include "dialogs/wxTermContainer.h"
//#include "dialogs/VariableWatchPanel.h"
#include "dialogs/AboutChameleonDialog.h"
#include "dialogs/debugviewer.hpp"

#include "terminal/wxssh.h"
#include "terminal/networking.h"

#include "../kernel/core/version.h"
#include "../kernel/core/utils/tools.hpp"

#include "../common/debug.h"
#include "../common/fixvsbug.h"
#include "../common/ProjectInfo.h"
#include "../common/recycler.hpp"
#include "../common/Options.h"
#include "../common/DebugEvent.h"

#include "icons/newstart1.xpm"
#include "icons/newcontinue1.xpm"
#include "icons/newstop1.xpm"
#include "icons/gtk-apply.xpm"
#include "icons/stepnext.xpm"
#include "icons/wraparound.xpm"
#include "icons/breakpoint_octagon.xpm"
#include "icons/breakpoint_octagon_crossed.xpm"
#include "icons/breakpoint_octagon_disable.xpm"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const string sVersion = toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + AutoVersion::STATUS + "\"";
std::string replacePathSeparator(const std::string&);

Language _guilang;
FindReplaceDialog* g_findReplace;

//! global print data, to remember settings during the session
wxPrintData* g_printData = (wxPrintData*) nullptr;
wxPageSetupData* g_pageSetupData = (wxPageSetupData*) nullptr;

BEGIN_EVENT_TABLE(NumeReWindow, wxFrame)
    EVT_MENU_RANGE                  (EVENTID_MENU_START, EVENTID_MENU_END-1, NumeReWindow::OnMenuEvent)

	EVT_MENU						(ID_TEST, NumeReWindow::Test)
	EVT_MENU						(ID_STARTCONNECT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_DISCONNECT, NumeReWindow::OnMenuEvent)
	EVT_MENU_RANGE					(ID_COMPILE, ID_COMPILE_PROJECT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PROJECT_ADDFILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PROJECT_REMOVEFILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PROJECT_EXCLUDE_FILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PROJECT_INCLUDE_FILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_NEW_PROJECT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_OPEN_PROJECT_LOCAL, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_OPEN_PROJECT_REMOTE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_CLOSE_PROJECT, NumeReWindow::OnMenuEvent)
//	EVT_MENU						(ID_DEBUG_CONTINUE, NumeReWindow::OnDebugCommand)
//	EVT_MENU						(ID_DEBUG_STEPNEXT, NumeReWindow::OnDebugCommand)
//	EVT_MENU						(ID_DEBUG_STEPOVER, NumeReWindow::OnDebugCommand)
//	EVT_MENU						(ID_DEBUG_STEPOUT, NumeReWindow::OnDebugCommand)

	EVT_FIND						(-1, NumeReWindow::OnFindEvent)
	EVT_FIND_NEXT					(-1, NumeReWindow::OnFindEvent)
	EVT_FIND_REPLACE				(-1, NumeReWindow::OnFindEvent)
	EVT_FIND_REPLACE_ALL			(-1, NumeReWindow::OnFindEvent)
	EVT_FIND_CLOSE					(-1, NumeReWindow::OnFindEvent)

	EVT_CLOSE						(NumeReWindow::OnClose)

	EVT_NOTEBOOK_PAGE_CHANGED		(ID_NOTEBOOK_ED, NumeReWindow::OnPageChange)
	EVT_SPLITTER_SASH_POS_CHANGED	(ID_SPLITEDITOROUTPUT, NumeReWindow::OnTermResize)

	EVT_SIZE						(NumeReWindow::OnSize)

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
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)
//----------------------------------------------------------------------
// `Main program' equivalent: the program execution "starts" here

bool MyApp::OnInit()
{
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    //wxString appPath(f.GetPath());
    wxInitAllImageHandlers();
    //wxImage::AddHandler(new wxPNGHandler());
    wxBitmap splashImage;
    if (splashImage.LoadFile(f.GetPath(true)+"icons\\splash.png", wxBITMAP_TYPE_PNG))
    {
        /*TextSplashScreen* splash = new TextSplashScreen(splashImage, wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT, 4000, nullptr);
        splash->SetText("Loading configuration ...");
        wxMilliSleep(500);
        splash->SetText("Checking filesystem ...");
        wxMilliSleep(500);
        splash->SetText("Loading language files ...");
        wxMilliSleep(500);
        splash->Destroy();*/
        wxSplashScreen* splash = new wxSplashScreen(splashImage, wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT, 4000, nullptr, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        wxApp::Yield();
        wxSleep(3);
        splash->Destroy();
    }

    g_findReplace = nullptr;


    NumeReWindow *frame = new NumeReWindow("NumeRe: Framework für Numerische Rechnungen (v" + sVersion + ")", wxDefaultPosition, wxDefaultSize);

    frame->Show(true);
    frame->Maximize();

    // Passed commandline items
    wxArrayString wxArgV;
    for (int i = 0; i < argc; i++)
    {
        wxArgV.Add(argv[i]);
    }
    // force the history window to perform a
    // page down to scroll to its actually
    // last position
    frame->forceHistoryPageDown();
    frame->EvaluateCommandLine(wxArgV);
    // Tip of the day
    if (frame->showTipAtStartup)
    {
        wxArrayString DialogText;
        DialogText.Add(_guilang.get("GUI_TIPOTD_HEAD"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_DYK"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_NEXT"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_STAS"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_CLOSE"));
        frame->updateTipAtStartupSetting(ShowTip(frame, frame->tipProvider, DialogText));
    }
    delete frame->tipProvider;
    frame->tipProvider = nullptr;
    if (frame->m_UnrecoverableFiles.length())
    {
        frame->Refresh();
        frame->Update();
        wxMessageBox(_guilang.get("GUI_DLG_SESSION_RECREATIONERROR", frame->m_UnrecoverableFiles), _guilang.get("GUI_DLG_SESSION_ERROR"), wxICON_ERROR);
    }

    frame->Ready();
    return true;
}

MyApp::~MyApp()
{
}

//----------------------------------------------------------------------




//////////////////////////////////////////////////////////////////////////////
///  public constructor ChameleonWindow
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
       : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	// should be approximately 80x15 for the terminal
	this->SetSize(1024, 768);
    m_optionsDialog = nullptr;
    m_config = nullptr;
    m_options = nullptr;
    m_network = nullptr;
    m_iconManager = nullptr;
    m_debugViewer = nullptr;
    m_projectManager = nullptr;
    m_watcher = nullptr;
    m_currentEd = nullptr;
    m_currentView = nullptr;
    m_statusBar = nullptr;
    fSplitPercentage = -0.65f;
    fVerticalSplitPercentage = 0.7f; // positive number to deactivate internal default algorithm

    m_copiedTreeItem = 0;
    m_multiRowState = false;
    m_sessionSaved = false;
    m_UnrecoverableFiles = "";
    m_loadingFilesDuringStartup = false;
    m_appStarting = true;


	// Show a log window for all debug messages
#ifdef _DEBUG
	logWindow = new wxLogWindow(this, "Debug messages");
	wxLog::SetActiveTarget(logWindow);
#endif

	wxString programPath = getProgramFolder();

	m_updateTimer = NULL;

	SetIcon(wxIcon(programPath + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO));

	m_remoteMode = true;

	m_options = new Options();

	InitializeProgramOptions();


	m_network = new Networking(m_options);

	m_splitProjectEditor = new wxSplitterWindow(this, ID_SPLITPROJECTEDITOR, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
	m_splitEditorOutput = new wxProportionalSplitterWindow(m_splitProjectEditor, ID_SPLITEDITOROUTPUT, 0.75, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH);
	m_splitCommandHistory = new wxProportionalSplitterWindow(m_splitEditorOutput, wxID_ANY, 0.75, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH);
	m_book = new NumeReNotebook(m_splitEditorOutput, ID_NOTEBOOK_ED, wxDefaultPosition, wxDefaultSize, wxBORDER_STATIC);
	m_book->SetTopParent(this);
	m_noteTerm = new ViewerBook(m_splitCommandHistory, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_STATIC);

	m_termContainer = new wxTermContainer(m_splitCommandHistory, ID_CONTAINER_TERM);

	m_terminal = new wxSSH(m_termContainer, ID_TERMINAL, m_network, m_options, programPath, wxPoint(0, 0));
	m_terminal->set_mode_flag(GenericTerminal::CURSORINVISIBLE);
	m_termContainer->SetTerminal(m_terminal);
	m_terminal->SetParent(this);
	m_splitEditorOutput->SetCharHeigth(m_terminal->getTextHeight());
	m_splitCommandHistory->SetCharHeigth(m_terminal->getTextHeight());
	_guilang.setTokens("<>="+getProgramFolder().ToStdString()+";");
	if (m_terminal->getKernelSettings().getUseCustomLanguageFiles())
	{
        _guilang.loadStrings(true);
	}
	else
        _guilang.loadStrings(false);

	m_optionsDialog = new OptionsDialog(this, m_options, ID_OPTIONSDIALOG, _guilang.get("GUI_DLG_OPTIONS"));
	m_projectManager = new ChameleonProjectManager(m_book);

	// project setup
	m_projMultiFiles = NULL;
	m_treeBook = new ViewerBook(m_splitProjectEditor, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_STATIC);
	m_fileTree = new FileTree(m_treeBook, ID_PROJECTTREE, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_HAS_BUTTONS | wxTR_NO_LINES | wxTR_FULL_ROW_HIGHLIGHT);
	m_treeBook->AddPage(m_fileTree, _guilang.get("GUI_FILETREE"));
	m_functionTree = new FileTree(m_treeBook, ID_FUNCTIONTREE, wxDefaultPosition, wxDefaultSize, wxTR_TWIST_BUTTONS | wxTR_HAS_BUTTONS | wxTR_NO_LINES | wxTR_FULL_ROW_HIGHLIGHT | wxTR_HIDE_ROOT);
	m_treeBook->AddPage(m_functionTree, _guilang.get("GUI_FUNCTIONTREE"));
	m_treeBook->Hide();
	m_watcher = new Filewatcher();
	m_watcher->SetOwner(this);

	m_iconManager = new IconManager(getProgramFolder());

	wxImageList* fileIcons = m_iconManager->GetImageList();
	// Sets the list, but doesn't take control of it away
	m_fileTree->SetImageList(fileIcons);
	m_functionTree->SetImageList(fileIcons);

	int idxFolderOpen = m_iconManager->GetIconIndex("FOLDEROPEN");

	m_remoteFileDialog = new RemoteFileDialog(this, ID_REMOTEFILEDIALOG);
	m_remoteFileDialog->SetNetworking(m_network);

	// This also sets the imagelist for the listview
	m_remoteFileDialog->SetIconManager(m_iconManager);


	wxTreeItemId rootNode = m_fileTree->AddRoot(_guilang.get("GUI_TREE_WORKSPACE"), m_iconManager->GetIconIndex("WORKPLACE"));
	m_projectFileFolders[0] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_DATAFILES"), idxFolderOpen);
	m_projectFileFolders[1] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_SAVEDFILES"), idxFolderOpen);
	m_projectFileFolders[2] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_SCRIPTS"), idxFolderOpen);
	m_projectFileFolders[3] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_PROCEDURES"), idxFolderOpen);
	m_projectFileFolders[4] = m_fileTree->AppendItem(rootNode, _guilang.get("GUI_TREE_PLOTS"), idxFolderOpen);

	_guilang.addToLanguage(m_terminal->getPluginLanguageStrings());
	_guilang.addToLanguage(m_terminal->getFunctionLanguageStrings());
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
	SendSizeEvent();

	m_updateTimer = new wxTimer (this, ID_STATUSTIMER);

	m_splitEditorOutput->Initialize(m_book);
	m_splitProjectEditor->Initialize(m_splitEditorOutput);
	m_splitEditorOutput->Initialize(m_splitCommandHistory);
	m_splitProjectEditor->Show();
	m_splitEditorOutput->Show();
	m_splitCommandHistory->Show();
	m_book->Show();
    m_history = new NumeReHistory(this, m_options, new ProjectInfo(), m_noteTerm, -1, m_terminal->getSyntax(), m_terminal, wxDefaultPosition, wxDefaultSize);
    m_varViewer = new VariableViewer(m_noteTerm);

    m_noteTerm->AddPage(m_history, _guilang.get("GUI_HISTORY"));
    m_noteTerm->AddPage(m_varViewer->control, _guilang.get("GUI_VARVIEWER"));

    wxMilliSleep(500);

    UpdateMenuBar();
	UpdateTerminalNotebook();

	EvaluateOptions();

	// create the initial blank open file or recreate the last session
	if (wxFileExists(getProgramFolder()+"\\numere.session") && m_options->GetSaveSession())
	{
	    m_loadingFilesDuringStartup = true;
        NewFile();
	    ifstream if_session;
	    vector<string> vSessionFile;
	    string sLine;
	    string sFileName;
	    int activefile = 0;
	    int nId = 0;
        int nLine = 0;
        int nSetting = 0;
	    bool modifiedFile = false;
        if_session.open((getProgramFolder().ToStdString()+"\\numere.session").c_str());
        if (if_session.is_open())
        {
            while (!if_session.eof())
            {
                getline(if_session, sLine);
                if (!sLine.length() || sLine.front() == '#')
                    continue;
                if (sLine.substr(0,12) == "ACTIVEFILEID")
                {
                    activefile = StrToInt(sLine.substr(sLine.find('\t')+1));
                    continue;
                }
                vSessionFile.push_back(sLine);
            }
            if_session.close();
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
                sFileName = sLine.substr(0,sLine.find('\t'));
                // erase until id
                sLine.erase(0,sLine.find('\t')+1);
                nId = StrToInt(sLine.substr(0,sLine.find('\t')));
                // erase until position
                sLine.erase(0,sLine.find('\t')+1);
                nLine = StrToInt(sLine.substr(0,sLine.find('\t')));
                if (sLine.find('\t') != string::npos)
                {
                    nSetting = StrToInt(sLine.substr(sLine.rfind('\t')+1));
                }
                // create the files
                if (sFileName == "<NEWFILE>")
                {
                    if (vSessionFile.size() != 1)
                    {
                        NewFile();
                        m_currentEd->SetUnsaved();
                    }
                    m_currentEd->GotoPos(nLine);
                    m_currentEd->ToggleSettings(nSetting);
                    continue;
                }
                if (wxFileExists(sFileName))
                {
                    OpenSourceFile(wxArrayString(1, sFileName));
                    m_currentEd->GotoPos(nLine);
                    m_currentEd->ToggleSettings(nSetting);
                }
                else
                {
                    if (!modifiedFile)
                    {
                        m_UnrecoverableFiles += sFileName + "\n";
                    }
                    int nUnloadableID = nId;
                    if (nUnloadableID < activefile)
                        activefile--;
                }
            }
            if (activefile >= (int)m_book->GetPageCount())
                m_book->SetSelection(m_book->GetPageCount()-1);
            else
                m_book->SetSelection(activefile);
            /*if (sUnloadableFiles.length())
                wxMessageBox(_guilang.get("GUI_DLG_SESSION_RECREATIONERROR", sUnloadableFiles), _guilang.get("GUI_DLG_SESSION_ERROR"), wxCENTRE | wxICON_ERROR, nullptr, 10,10);*/
        }
        m_loadingFilesDuringStartup = false;
	}
	else
	{
        NewFile();
	}
	PageHasChanged(m_currentPage);

	// bind the event after the loading of the files - FIX for Win10 1803
	Connect(wxEVT_FSWATCHER, wxFileSystemWatcherEventHandler(NumeReWindow::OnFileSystemEvent));


	m_filterNSCRFiles = _guilang.get("GUI_FILTER_SCRIPTS") + " (*.nscr)|*.nscr";//"NumeRe scripts (*.nscr)|*.nscr";
	m_filterNPRCFiles = _guilang.get("GUI_FILTER_PROCEDURES") + " (*.nprc)|*.nprc";//"NumeRe procedures (*.nprc)|*.nprc";
	m_filterExecutableFiles = _guilang.get("GUI_FILTER_EXECUTABLES") + " (*.nscr, *.nprc)|*.nscr;*.nprc";
	m_filterNumeReFiles = _guilang.get("GUI_FILTER_NUMEREFILES") + " (*.ndat, *.nscr, *.nprc)|*.ndat;*.nscr;*.nprc";//"NumeRe files (*.ndat, *.nscr, *.nprc)|*.ndat;*.nscr;*.nprc";
    m_filterDataFiles = _guilang.get("GUI_FILTER_DATAFILES");// + " (*.dat, *.txt, *.csv, *.jdx, *.dx, *.jcm)|*.dat;*.txt;*.csv;*.jdx;*.dx;*.jcm";
    m_filterImageFiles = _guilang.get("GUI_FILTER_IMAGEFILES") + " (*.png, *.jpeg, *.eps, *.svg, *.gif)|*.png;*.jpg;*.jpeg;*.eps;*.svg;*.gif";
    m_filterTeXSource = _guilang.get("GUI_FILTER_TEXSOURCE") + " (*.tex)|*.tex";
    m_filterNonsource = _guilang.get("GUI_FILTER_NONSOURCE");
    m_filterSupportedFiles = _guilang.get("GUI_FILTER_ALLSUPPORTEDFILES");

	// If this ever gets ported to Linux, we'd probably want to add
	// Linux library extensions here (.a, .so).  The other issue is that
	// the remote file dialog only looks in ~, which might need to be changed.
	m_filterAllFiles = _guilang.get("GUI_FILTER_ALLFILES") + " (*.*)|*.*";

	m_extensionMappings["cpj"] = ICON_PROJECT;
	m_extensionMappings["c"] = ICON_SOURCE_C;
	m_extensionMappings["cpp"] = ICON_SOURCE_CPP;
	m_extensionMappings["h"] = ICON_SOURCE_H;
	m_extensionMappings["hpp"] = ICON_SOURCE_H;
	m_extensionMappings["lib"] = ICON_LIBRARY;
	m_extensionMappings["c_disabled"] = ICON_DISABLED_SOURCE_C;
	m_extensionMappings["cpp_disabled"] = ICON_DISABLED_SOURCE_CPP;
	m_extensionMappings["h_disabled"] = ICON_DISABLED_SOURCE_H;
	m_extensionMappings["hpp_disabled"] = ICON_DISABLED_SOURCE_H;

	m_projMultiFiles = nullptr;

	g_findReplace = nullptr;

	/// Interesting Bug: obviously it is necessary to declare the paper size first
	g_printData = this->setDefaultPrinterSettings();
	g_pageSetupData = new wxPageSetupDialogData(*g_printData);

	m_appStarting = false;
	m_compileProject = false;

	HINSTANCE hInstance = wxGetInstance();
	char *pStr, szPath[_MAX_PATH];
	GetModuleFileNameA(hInstance, szPath, _MAX_PATH);
	pStr = strrchr(szPath, '\\');
	if (pStr != NULL)
		*(++pStr)='\0';


	///Msgbox
	Settings _option = m_terminal->getKernelSettings();
    vector<string> vTipList;
    if (_option.getUseCustomLanguageFiles() && fileExists(_option.ValidFileName("<>/user/docs/hints.ndb", ".ndb")))
        vTipList = getDBFileContent("<>/user/docs/hints.ndb", _option);
    else
        vTipList = getDBFileContent("<>/docs/hints.ndb", _option);

    if (!vTipList.size())
        vTipList.push_back("CANNOT READ TIPLIST");


	tipProvider = new MyTipProvider(vTipList);
	showTipAtStartup = _option.getbShowHints();
}

//////////////////////////////////////////////////////////////////////////////
///  public destructor ~ChameleonWindow
///  Responsible for cleaning up almost everything.
///
///  @return void
///
///  @remarks Networking needs to be deleted last, since several other classes refer to it in their destructors.
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
NumeReWindow::~NumeReWindow()
{
	// make sure the config file got written to disk
	m_config->Flush();

	delete g_printData;
	delete g_pageSetupData;

	if(g_findReplace != nullptr)
	{
		delete g_findReplace;
	}

	if (m_updateTimer)
	{
		delete m_updateTimer;
		m_updateTimer = NULL;
	}

	// (hopefully) fixes some issues with widgets not getting deleted for some reason
	///m_noteTerm->DestroyChildren();

	//CleanupDropMenu();


    if (m_optionsDialog)
        delete m_optionsDialog;
//    if (m_compiler)
//        delete m_compiler;
    if (m_config)
        delete m_config;
    if (m_options)
        delete m_options;
//    if (m_debugger)
//        delete m_debugger;
    if (m_network)
        delete m_network;
    if (m_iconManager)
        delete m_iconManager;
//    if (m_debugManager)
//        delete m_debugManager;
    if (m_projectManager)
        delete m_projectManager;
    if (m_watcher)
        delete m_watcher;
}

void NumeReWindow::updateTipAtStartupSetting(bool bTipAtStartup)
{
    Settings _option = m_terminal->getKernelSettings();
    if (_option.getbShowHints() == bTipAtStartup)
        return;
    _option.setbShowHints(bTipAtStartup);
    m_terminal->setKernelSettings(_option);
}

wxString NumeReWindow::getProgramFolder()
{
    wxStandardPaths systemPaths = wxStandardPaths::Get();
	wxString appPath = systemPaths.GetExecutablePath();
	wxFileName fullProgramPath(appPath);
	return fullProgramPath.GetPath();
}

void NumeReWindow::AddToHistory(const wxString& sCommand)
{
    m_history->AddToHistory(sCommand);
}

wxString NumeReWindow::GetDocContent(wxString docid)
{
    return m_terminal->getDocumentation(docid.ToStdString());
}

vector<string> NumeReWindow::GetDocIndex()
{
    return m_terminal->getDocIndex();
}

void NumeReWindow::forceHistoryPageDown()
{
    m_history->PageDown();
}

vector<string> NumeReWindow::getPathDefs()
{
    return m_terminal->getPathSettings();
}


void NumeReWindow::addToReloadBlackList(const wxString& sFilename)
{
    for (size_t i = 0; i < vReloadBlackList.size(); i++)
    {
        if (vReloadBlackList[i] == sFilename)
            return;
    }
    vReloadBlackList.push_back(sFilename);
}

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

// This member function loads the configuration file available
// for the graphical user interface. If no configuration file is
// available or if the config file is of an older version, default
// values are used
void NumeReWindow::InitializeProgramOptions()
{
	// Open up the configuration file, assumed to be located
	// in the program's root directory
	wxString path = wxStandardPaths::Get().GetExecutablePath();

	// Construct a file name class from the path
	wxFileName configName(path.substr(0, path.rfind('\\')+1), "numeregui.ini");

	// Prepare the configuration file
	m_config = new wxFileConfig("numeregui", wxEmptyString, configName.GetFullPath());

	// Depending on whether the file exists, load the contents
	// or use the default values
	if (configName.FileExists())
	{
		bool printInColor = (m_config->Read("Miscellaneous/PrintInColor", "true") == "true");
		int printStyle = printInColor ? wxSTC_PRINT_COLOURONWHITE : wxSTC_PRINT_BLACKONWHITE;
		m_options->SetPrintStyle(printStyle);

		bool showToolbarText = (m_config->Read("Interface/ShowToolbarText", "true") == "true");
		m_options->SetShowToolbarText(showToolbarText);

		bool printLineNumbers = (m_config->Read("Miscellaneous/PrintLineNumbers", "false") == "true");
		m_options->SetLineNumberPrinting(printLineNumbers);

		bool saveSession = (m_config->Read("Miscellaneous/SaveSession", "false") == "true");
		m_options->SetSaveSession(saveSession);

		bool formatBeforeSaving = (m_config->Read("Miscellaneous/FormatBeforeSaving", "false") == "true");
		m_options->SetFormatBeforeSaving(formatBeforeSaving);

		bool keepBackups = (m_config->Read("Miscellaneous/KeepBackups", "false") == "true");
		m_options->SetFormatBeforeSaving(keepBackups);

		int terminalHistory = 300;
		m_config->Read("Miscellaneous/TerminalHistory", &terminalHistory, 300);
		if (terminalHistory >= 100 && terminalHistory <= 1000)
            m_options->SetTerminalHistorySize(terminalHistory);
        else
            m_options->SetTerminalHistorySize(300);

		int caretBlinkTime = 500;
		m_config->Read("Interface/CaretBlinkTime", &caretBlinkTime, 500);
		if (caretBlinkTime >= 100 && caretBlinkTime <= 1000)
            m_options->SetCaretBlinkTime(caretBlinkTime);
        else
            m_options->SetCaretBlinkTime(500);

        // Read the color codes from the configuration file,
        // if they exist
        m_options->readColoursFromConfig(m_config);

        wxFont font;
        wxString nativeInfo;
        m_config->Read("Styles/EditorFont", &nativeInfo, "Consolas 10");
        font.SetNativeFontInfoUserDesc(nativeInfo);
        m_options->SetEditorFont(font);

        wxString latexroot;
        m_config->Read("Miscellaneous/LaTeXRoot", &latexroot, "C:/Program Files");
        m_options->SetLaTeXRoot(latexroot);
	}
	else
	{
	    // Use the default configuration set
#ifdef DO_LOG
		wxLogDebug("Failed to locate config file, loading default permissions");
#endif
		m_config->Write("Miscellaneous/TerminalHistory", m_options->GetTerminalHistorySize());
		m_config->Write("Interface/CaretBlinkTime", m_options->GetCaretBlinkTime());
		m_config->Write("Miscellaneous/LaTeXRoot", m_options->GetLaTeXRoot());
		m_config->Write("Miscellaneous/PrintInColor", "false");
		m_config->Write("Miscellaneous/PrintLineNumbers", "false");
		m_config->Write("Miscellaneous/SaveSession", "false");
		m_config->Write("Miscellaneous/FormatBeforeSaving", "false");
		m_config->Write("Miscellaneous/KeepBackups", "false");
		m_options->writeColoursToConfig(m_config);
		m_config->Write("Styles/EditorFont", m_options->GetEditorFont().GetNativeFontInfoUserDesc());
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
	m_appClosing = true;

	if (!CloseAllFiles())
    {
        if (event.CanVeto())
            event.Veto(true);
        return;
    }

	// double check in case something went wrong
	if (m_currentEd && m_currentEd->Modified())
	{
		if (event.CanVeto())
		{
			event.Veto (true);
		}

		return;
	}

	if(m_projMultiFiles != NULL)
	{
		CloseProjectFile();
	}

	if(m_updateTimer)
	{
		m_updateTimer->Stop();
	}

	if(m_terminal->IsConnected())
	{
		m_terminal->Disconnect();
	}
	if (m_terminal->IsWorking())
	{
        m_terminal->EndKernelTask();
        wxMilliSleep(200);
	}

	if (m_debugViewer != nullptr)
        m_debugViewer->Destroy();

	/*if(m_debugTerminal->IsConnected())
	{
		m_debugTerminal->Disconnect();
	}*/

	Destroy();
}

// Main menu and debug stuff begins here

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
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_fileTree->GetItemData(m_clickedTreeItem));
            OnExecuteFile(data->filename.ToStdString());
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
            m_currentEd->InsertText(m_currentEd->GetCurrentPos(), command);
            m_currentEd->GotoPos(m_currentEd->GetCurrentPos()+command.length());
            break;
        }
        case ID_MENU_INSERT_IN_CONSOLE_FROM_TREE:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_functionTree->GetItemData(m_clickedTreeItem));
            string command;
            if (data->isCommand)
                command = (data->tooltip).substr(0, (data->tooltip).find(' ')).ToStdString() + " ";
            else if (data->isFunction)
                command = (data->tooltip).substr(0, (data->tooltip).find('(')+1).ToStdString();
            else if (data->isConstant)
                command = (data->tooltip).substr(0, (data->tooltip).find(' ')).ToStdString();
            showConsole();
            m_terminal->ProcessInput(command.length(), command);
            break;
        }
        case ID_MENU_HELP_ON_ITEM:
        {
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_functionTree->GetItemData(m_clickedTreeItem));
            string command = (data->tooltip).substr(0, (data->tooltip).find(' ')).ToStdString();
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
        case ID_MENU_OPEN_IN_EXPLORER:
            OnOpenInExplorer();
            break;
        case ID_MENU_AUTOINDENT:
        {
            m_currentEd->ApplyAutoIndentation();
            break;
        }
        case ID_MENU_LINEWRAP:
        {
            wxToolBar* t = GetToolBar();
            m_currentEd->ToggleSettings(NumeReEditor::SETTING_WRAPEOL);
            t->ToggleTool(ID_MENU_LINEWRAP, m_currentEd->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
            wxMenu* tools = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_guilang.get("GUI_MENU_VIEW")));
            tools->Check(ID_MENU_LINEWRAP, m_currentEd->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
            break;
        }
        case ID_MENU_DISPCTRLCHARS:
        {
            m_currentEd->ToggleSettings(NumeReEditor::SETTING_DISPCTRLCHARS);
            break;
        }
        case ID_MENU_USETXTADV:
        {
            m_currentEd->ToggleSettings(NumeReEditor::SETTING_USETXTADV);
            break;
        }
        case ID_MENU_USEANALYZER:
        {
            wxToolBar* t = GetToolBar();
            m_currentEd->ToggleSettings(NumeReEditor::SETTING_USEANALYZER);
            t->ToggleTool(ID_MENU_USEANALYZER, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
            wxMenu* tools = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_guilang.get("GUI_MENU_TOOLS")));
            tools->Check(ID_MENU_USEANALYZER, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
            break;
        }
        case ID_MENU_INDENTONTYPE:
        {
            wxToolBar* t = GetToolBar();
            m_currentEd->ToggleSettings(NumeReEditor::SETTING_INDENTONTYPE);
            t->ToggleTool(ID_MENU_INDENTONTYPE, m_currentEd->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
            wxMenu* tools = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_guilang.get("GUI_MENU_TOOLS")));
            tools->Check(ID_MENU_INDENTONTYPE, m_currentEd->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
            break;
        }
        case ID_MENU_USESECTIONS:
        {
            //wxToolBar* t = GetToolBar();
            m_currentEd->ToggleSettings(NumeReEditor::SETTING_USESECTIONS);
            //t->ToggleTool(ID_USESECTIONS, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USESECTIONS));
            wxMenu* tools = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_guilang.get("GUI_MENU_VIEW")));
            tools->Check(ID_MENU_USESECTIONS, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USESECTIONS));
            break;
        }
        case ID_MENU_AUTOFORMAT:
        {
            m_currentEd->ApplyAutoFormat();
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
            m_currentEd->Refresh();
            m_multiRowState = !m_multiRowState;
            break;
        }
        case ID_MENU_TOGGLE_COMMENT_LINE:
        {
            m_currentEd->ToggleCommentLine();
            break;
        }
        case ID_MENU_TOGGLE_COMMENT_SELECTION:
        {
            m_currentEd->ToggleCommentSelection();
            break;
        }
        case ID_MENU_FOLD_ALL:
        {
            m_currentEd->FoldAll();
            break;
        }
        case ID_MENU_UNFOLD_ALL:
        {
            m_currentEd->UnfoldAll();
            break;
        }
        case ID_MENU_UNHIDE_ALL:
        {
            m_currentEd->OnUnhideAllFromMenu();
            break;
        }
        case ID_MENU_SELECTION_UP:
        {
            m_currentEd->MoveSelection(false);
            break;
        }
        case ID_MENU_SELECTION_DOWN:
        {
            m_currentEd->MoveSelection();
            break;
        }
        case ID_MENU_SORT_SELECTION_ASC:
        {
            m_currentEd->sortSelection();
            break;
        }
        case ID_MENU_SORT_SELECTION_DESC:
        {
            m_currentEd->sortSelection(false);
            break;
        }
        case ID_MENU_BOOKMARK_TOGGLE:
        {
            m_currentEd->toggleBookmark();
            break;
        }
        case ID_MENU_BOOKMARK_CLEARMENU:
        {
            m_currentEd->clearBookmarks();
            break;
        }
        case ID_MENU_BOOKMARK_PREVIOUS:
        {
            m_currentEd->JumpToBookmark(false);
            break;
        }
        case ID_MENU_BOOKMARK_NEXT:
        {
            m_currentEd->JumpToBookmark(true);
            break;
        }
        case ID_MENU_FIND_DUPLICATES:
        {
            // only if the file type is matching
            m_currentEd->InitDuplicateCode();
            break;
        }
        case ID_MENU_STRIP_SPACES_BOTH:
        {
            m_currentEd->removeWhiteSpaces(RM_WS_BOTH);
            break;
        }
        case ID_MENU_STRIP_SPACES_FRONT:
        {
            m_currentEd->removeWhiteSpaces(RM_WS_FRONT);
            break;
        }
        case ID_MENU_STRIP_SPACES_BACK:
        {
            m_currentEd->removeWhiteSpaces(RM_WS_BACK);
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

		case ID_MENU_OPEN_SOURCE_LOCAL:
		case ID_MENU_OPEN_SOURCE_REMOTE:
		{
			OnOpenSourceFile(id);
			break;
		}

		case ID_MENU_SAVE:
		{
			SaveFile(false, true, FILE_ALLSOURCETYPES);
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
			SaveFile(true, true, FILE_NUMERE);
			break;
		}

		case ID_OPEN_PROJECT_LOCAL:
		case ID_OPEN_PROJECT_REMOTE:
		{
			OpenProjectFile(id == ID_OPEN_PROJECT_REMOTE);
			break;
		}

		case ID_CLOSE_PROJECT:
		{
			CloseProjectFile();
			break;
		}

		case ID_MENU_CLOSETAB:
		{
			CloseTab();
			break;
		}
		case ID_MENU_RUN_FROM_TAB:
		{
			EvaluateTab();
			break;
		}

		case ID_MENU_CLOSEPAGE:
		{
			if (!m_currentEd)
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


		case ID_STARTCONNECT:
		{
			OnStartConnect();
			break;
		}

		case ID_DISCONNECT:
		{
			m_terminal->Disconnect();
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
			{
				m_currentEd->Redo();
			}
			else
			{
				m_currentEd->Undo();
			}

			m_book->Refresh();
			break;
		}

		case ID_MENU_COPY:
		{
            if (m_currentEd->HasFocus())
                OnCopy();
            else
                event.Skip();
			break;
		}

		case ID_MENU_CUT:
		{
			m_currentEd->Cut();
			break;
		}

		case ID_MENU_PASTE:
		{
			m_currentEd->Paste();
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
            m_currentEd->OnFindProcedureFromMenu();
            break;
        }
		case ID_MENU_FIND_INCLUDE:
        {
            m_currentEd->OnFindIncludeFromMenu();
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
            _option.setDebbuger(!_option.getUseDebugger());
            m_terminal->setKernelSettings(_option);
            wxToolBar* tb = GetToolBar();
            tb->ToggleTool(ID_MENU_TOGGLE_DEBUGGER, _option.getUseDebugger());
            wxMenu* view = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_guilang.get("GUI_MENU_TOOLS")));
            view->Check(ID_MENU_TOGGLE_DEBUGGER, _option.getUseDebugger());
            break;
		}
		case ID_MENU_RENAME_SYMBOL:
        {
            m_currentEd->OnRenameSymbolsFromMenu();
            break;
        }
		case ID_MENU_ABSTRAHIZE_SECTION:
        {
            m_currentEd->OnAbstrahizeSectionFromMenu();
            break;
        }

		case ID_COMPILE_PROJECT:
		case ID_COMPILE:
		{
			// We didn't have time to really test compile cancellation, so this
			// code is commented out.
			/*
			if(m_compiler->IsCompiling())
			{
				m_compiler->HaltCompiling();
			}
			else
			{
				Compile();
			}
			*/

			if(id == ID_COMPILE_PROJECT)
			{
				m_compileProject = true;
			}
			Compile();
			break;
		}

		case ID_PROJECT_ADDFILE:
		{
			AddFileToProject();
			break;
		}

		case ID_PROJECT_REMOVEFILE:
		{
			RemoveFileFromProject();
			break;
		}

		case ID_PROJECT_EXCLUDE_FILE:
		case ID_PROJECT_INCLUDE_FILE:
		{
			OnProjectIncludeExcludeFile(id);
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
			m_currentEd->OnAddBreakpoint(event);
			break;
		}
		case ID_MENU_REMOVEEDITORBREAKPOINT:
		{
			m_currentEd->OnRemoveBreakpoint(event);
			break;
		}
		case ID_MENU_CLEAREDITORBREAKPOINTS:
		{
			m_currentEd->OnClearBreakpoints(event);
			break;
		}

		case ID_MENU_EXECUTE:
        {
            if(!m_currentEd->HasBeenSaved() || m_currentEd->Modified())
            {
                int tabNum = m_book->FindPagePosition(m_currentEd);
                int result = HandleModifiedFile(tabNum, MODIFIEDFILE_COMPILE);

                if (result == wxCANCEL)
                {
                    return;
                }
            }
            string command = replacePathSeparator((m_currentEd->GetFileName()).GetFullPath().ToStdString());
            OnExecuteFile(command);
            break;
        }
        case ID_MENU_STOP_EXECUTION:
            m_terminal->CancelCalculation();
            break;
	}
}


void NumeReWindow::OnFileSystemEvent(wxFileSystemWatcherEvent& event)
{
    if (!m_fileTree)
        return;
    int type = event.GetChangeType();
    if (type == wxFSW_EVENT_DELETE || type == wxFSW_EVENT_CREATE || type == wxFSW_EVENT_RENAME)
    {
        //m_currentSavedFile.clear();
        string sEventpath = replacePathSeparator(event.GetPath().GetFullPath().ToStdString());
        vector<string> vPaths = m_terminal->getPathSettings();
        const FileFilterType fileType[] = {FILE_DATAFILES, FILE_DATAFILES, FILE_NSCR, FILE_NPRC, FILE_IMAGEFILES};
        for (size_t i = LOADPATH; i < vPaths.size(); i++)
        {
            if (sEventpath.find(replacePathSeparator(vPaths[i])) != string::npos)
            {
                m_fileTree->DeleteChildren(m_projectFileFolders[i-LOADPATH]);
                LoadFilesToTree(vPaths[i], fileType[i-LOADPATH], m_projectFileFolders[i-LOADPATH]);
                break;
            }
        }
        m_dragDropSourceItem = wxTreeItemId();
        CreateProcedureTree(vPaths[PROCPATH]);
        m_terminal->UpdateLibrary();
    }
    else if (type == wxFSW_EVENT_MODIFY)
    {
        wxString sEventPath = event.GetPath().GetFullPath();
        m_terminal->UpdateLibrary();
        if (isOnReloadBlackList(sEventPath))
            return;
        if (m_currentSavedFile == toString((int)time(0))+"|"+sEventPath || m_currentSavedFile == toString((int)time(0)-1)+"|"+sEventPath || m_currentSavedFile == "BLOCKALL|"+sEventPath)
        {
            return;
        }
        NumeReEditor* edit;
        // store current selection
        int selection = m_book->GetSelection();
        for (size_t i = 0; i < m_book->GetPageCount(); i++)
        {
            edit = static_cast<NumeReEditor*>(m_book->GetPage(i));
            if (edit->GetFileNameAndPath() == sEventPath)
            {
                m_currentSavedFile = "BLOCKALL|"+sEventPath;
                if (edit->IsModified())
                {
                    m_book->SetSelection(i);
                    int answer = wxMessageBox(_guilang.get("GUI_DLG_FILEMODIFIED_QUESTION", sEventPath.ToStdString()), _guilang.get("GUI_DLG_FILEMODIFIED"), wxYES_NO | wxICON_QUESTION, this);
                    if (answer == wxYES)
                    {
                        int pos = m_currentEd->GetCurrentPos();
                        m_currentEd->LoadFile(sEventPath);
                        m_currentEd->MarkerDeleteAll(MARKER_SAVED);
                        m_currentEd->UpdateSyntaxHighlighting(true);
                        m_currentEd->GotoPos(pos);
                        m_currentSavedFile = toString((int)time(0))+"|"+sEventPath;
                    }
                }
                else
                {
                    int pos = edit->GetCurrentPos();
                    edit->LoadFile(sEventPath);
                    edit->MarkerDeleteAll(MARKER_SAVED);
                    edit->UpdateSyntaxHighlighting(true);
                    edit->GotoPos(pos);
                    m_currentSavedFile = toString((int)time(0))+"|"+sEventPath;
                }
                break;
            }
        }
        // go back to previous selection
        m_book->SetSelection(selection);
    }
}

void NumeReWindow::CreateProcedureTree(const string& sProcedurePath)
{
    vector<string> vProcedureTree;
    vector<string> vCurrentTree;
    string sPath = sProcedurePath;
    Settings _option = m_terminal->getKernelSettings();

    do
    {
        sPath += "/*";
        vCurrentTree = getFileList(sPath, _option, 1);
        if (vCurrentTree.size())
            vProcedureTree.insert(vProcedureTree.end(), vCurrentTree.begin(), vCurrentTree.end());
    }
    while (vCurrentTree.size());

    for (size_t i = 0; i < vProcedureTree.size(); i++)
    {
        if (vProcedureTree[i].substr(0, sProcedurePath.length()) == sProcedurePath)
        {
            vProcedureTree[i].erase(0, sProcedurePath.length());
        }
        while (vProcedureTree[i].front() == '/' || vProcedureTree[i].front() == '\\')
            vProcedureTree[i].erase(0, 1);
    }

    m_terminal->getSyntax()->setProcedureTree(vProcedureTree);
}

void NumeReWindow::openImage(wxFileName filename)
{
	wxString programPath = getProgramFolder();

    ViewerFrame* frame = new ViewerFrame(this, "NumeRe-ImageViewer: " + filename.GetName());
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    ImagePanel* _panel = nullptr;
    if (filename.GetExt() == "png")
        _panel = new ImagePanel(frame, filename.GetFullPath(), wxBITMAP_TYPE_PNG);
    else if (filename.GetExt() == "bmp")
        _panel = new ImagePanel(frame, filename.GetFullPath(), wxBITMAP_TYPE_BMP);
    else if (filename.GetExt() == "gif")
        _panel = new ImagePanel(frame, filename.GetFullPath(), wxBITMAP_TYPE_GIF);
    else if (filename.GetExt() == "jpg" || filename.GetExt() == "jpeg")
        _panel = new ImagePanel(frame, filename.GetFullPath(), wxBITMAP_TYPE_JPEG);
    else
    {
        delete frame;
        delete sizer;
        return;
    }
    sizer->Add(_panel, 1, wxEXPAND);
    _panel->SetSize(_panel->getRelation()*600,600);
    frame->SetSizer(sizer);
    frame->SetClientSize(_panel->GetSize());
    frame->SetIcon(wxIcon(programPath + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
    m_currentView = frame;
    frame->Show();
    frame->SetFocus();
}

void NumeReWindow::openPDF(wxFileName filename)
{
    ShellExecuteA(NULL, "open", filename.GetFullPath().ToStdString().c_str(), "", "", SW_SHOW);
}

void NumeReWindow::openHTML(wxString HTMLcontent)
{
	wxString programPath = getProgramFolder();
	if (!HTMLcontent.length())
        return;

    ViewerFrame* frame = new ViewerFrame(this, "NumeRe-Hilfe:");
    frame->CreateStatusBar();
    HelpViewer* html = new HelpViewer(frame, this);
    html->SetRelatedFrame(frame, _guilang.get("DOC_HELP_HEADLINE", "%s"));
    html->SetRelatedStatusBar(0);
    html->SetPage(HTMLcontent);
    frame->SetSize(1000,600);
    frame->SetIcon(wxIcon(programPath + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
    frame->Show();
    frame->SetFocus();
}


void NumeReWindow::openTable(NumeRe::Container<string> _stringTable, const string& sTableName)
{
    ViewerFrame* frame = new ViewerFrame(this, "NumeRe: " + sTableName + "()");
    frame->SetSize(800,600);
    TableViewer* grid = new TableViewer(frame, wxID_ANY, frame->CreateStatusBar(3));
    grid->SetData(_stringTable);
    frame->SetSize(min(800u, grid->GetWidth()), min(600u, grid->GetHeight()+30));
    frame->SetIcon(wxIcon(getProgramFolder()+"\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
    frame->Show();
    frame->SetFocus();
}

void NumeReWindow::editTable(NumeRe::Container<string> _stringTable, const string& sTableName)
{
    ViewerFrame* frame = new ViewerFrame(this, _guilang.get("GUI_TABLEEDITOR") + " " + sTableName + "()");
    frame->SetSize(800,600);
    TableEditPanel* panel = new TableEditPanel(frame, wxID_ANY, frame->CreateStatusBar(3));
    panel->SetTerminal(m_terminal);
    panel->grid->SetTableReadOnly(false);
    panel->grid->SetData(_stringTable);
    frame->SetSize(min(800u, panel->grid->GetWidth()), min(600u, panel->grid->GetHeight()+30));
    frame->SetIcon(wxIcon(getProgramFolder()+"\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
    frame->Show();
    frame->SetFocus();
}

void NumeReWindow::showGraph(GraphHelper* _helper)
{
    GraphViewer* viewer = new GraphViewer(this, "NumeRe: " + _helper->getTitle(), _helper, m_terminal);

    viewer->SetIcon(wxIcon(getProgramFolder()+"\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
    viewer->Show();
    viewer->SetFocus();
}

void NumeReWindow::evaluateDebugInfo(const vector<string>& vDebugInfo)
{
    // initialize the debugger, if necessary and pass the new contents
    string sSizes = vDebugInfo[0];
    string sTitle = vDebugInfo[1];
    size_t nModuleSize = 0, nStackSize = 0, nNumVarSize = 0, sStrVarSize = 0, sTabVarSize = 0;
    vector<string> vModule;
    vector<string> vStack;
    vector<string> vVars;

    if (m_debugViewer == nullptr)
    {
        m_debugViewer = new DebugViewer(this, sTitle);
        m_debugViewer->SetSize(800,600);
        m_debugViewer->SetIcon(wxIcon(getProgramFolder()+"\\icons\\icon.ico", wxBITMAP_TYPE_ICO));
        m_debugViewer->setTerminal(m_terminal);
    }
    if (!m_debugViewer->IsShown())
        m_debugViewer->Show();
    nModuleSize = StrToInt(sSizes.substr(0, sSizes.find(';')));
    sSizes.erase(0, sSizes.find(';')+1);
    nStackSize = StrToInt(sSizes.substr(0, sSizes.find(';')));
    sSizes.erase(0, sSizes.find(';')+1);
    nNumVarSize = StrToInt(sSizes.substr(0, sSizes.find(';')));
    sSizes.erase(0, sSizes.find(';')+1);
    sStrVarSize = StrToInt(sSizes.substr(0, sSizes.find(';')));
    sSizes.erase(0, sSizes.find(';')+1);
    sTabVarSize = StrToInt(sSizes.substr(0, sSizes.find(';')));
    //sSizes.erase(0, sSizes.find(';')+1);

    for (size_t i = 2; i < vDebugInfo.size(); i++)
    {
        if (i < nModuleSize+2)
        {
            vModule.push_back(vDebugInfo[i]);
        }
        else if (i < nStackSize + nModuleSize+2)
        {
            vStack.push_back(vDebugInfo[i]);
        }
        else
        {
            vVars.push_back(vDebugInfo[i]);
        }
    }

    m_debugViewer->setDebugInfo(sTitle, vModule, vStack, vVars, nNumVarSize, sStrVarSize, sTabVarSize);
}

void NumeReWindow::createLaTeXFile()
{
    wxFileName filename = m_currentEd->GetFileName();
    filename.SetName(filename.GetName() + "_" + filename.GetExt());
    filename.SetExt("tex");
    filename.SetPath(m_terminal->getPathSettings()[SAVEPATH] + "/docs");
    if (m_currentEd->writeLaTeXFile(filename.GetFullPath().ToStdString()))
        wxMessageBox(_guilang.get("GUI_DLG_LATEX_SUCCESS_MESSAGE", filename.GetFullPath().ToStdString()), _guilang.get("GUI_DLG_LATEX_SUCCESS"), wxCENTER | wxOK, this);
    else
        wxMessageBox(_guilang.get("GUI_DLG_LATEX_ERROR_MESSAGE", filename.GetFullPath().ToStdString()), _guilang.get("GUI_DLG_LATEX_ERROR"), wxCENTER | wxOK, this);
}

string NumeReWindow::createLaTeXMain(const string& sRootPath, const string& sIncludes)
{
    ofstream fMain;

    createLaTeXHeader(sRootPath);

    fMain.open((sRootPath + "/" + sIncludes + "_main.tex").c_str());
    if (!fMain.good())
        return "";
    string sHeadLine = sIncludes;
    for (size_t i = 0; i < sHeadLine.length(); i++)
    {
        if (sHeadLine[i] == '_' && (!i || sHeadLine[i-1] != '\\'))
        {
            sHeadLine.insert(i, 1, '\\');
            i++;
        }
    }
    fMain << "\\documentclass[DIV=17]{scrartcl}" << endl;
    fMain << "% Main file for the documentation file " << sIncludes << endl << endl;
    fMain << "\\input{numereheader}" << endl << endl;
    fMain << "\\title{Documentation: " << sHeadLine << "}" << endl;
    fMain << "\\begin{document}" << endl;
    fMain << "    \\maketitle" << endl;
    if (sIncludes.length())
        fMain << "    \\input{" << sIncludes << "}" << endl;
    fMain << "\\end{document}" << endl;

    return sRootPath + "/" + sIncludes + "_main.tex";
}

string NumeReWindow::constructLaTeXHeaderKeywords(const string& sKeywordList)
{
    string _sKeywordList = sKeywordList;
    for (size_t i = 0; i < _sKeywordList.length(); i++)
    {
        if (_sKeywordList[i] == ' ')
            _sKeywordList[i] = ',';
    }
    if (_sKeywordList.back() == ',')
        _sKeywordList.erase(_sKeywordList.length()-1);
    return _sKeywordList;
}

void NumeReWindow::createLaTeXHeader(const string& sRootPath)
{
    ofstream fHeader;
    fHeader.open((sRootPath + "/numereheader.tex").c_str());
    if (!fHeader.good())
        return;

    NumeReSyntax* syntax = m_terminal->getSyntax();

    fHeader << "% Header file for NumeRe documentations" << endl;

    fHeader << "\\usepackage{xcolor}" << endl;
    fHeader << "\\usepackage{listings}" << endl;
    fHeader << "\\usepackage{etoolbox}" << endl;
    fHeader << "\\usepackage{amsmath}" << endl;
    fHeader << "\\usepackage{amssymb}" << endl;
    fHeader << "\\usepackage{fontspec,unicode-math}" << endl;
    fHeader << "\\setmainfont{Palatino Linotype}" << endl;
    fHeader << "\\setmathfont{Cambria Math}" << endl;
    fHeader << "\\setsansfont{Arial}" << endl;
    fHeader << "\\setmonofont{Consolas}" << endl << endl;

    fHeader << "% Define the language contents" << endl;
    fHeader << "\\lstdefinelanguage{nscr}" << endl;
    fHeader << "{" << endl;
    fHeader << "    keywordsprefix=$," << endl;
    fHeader << "    alsoletter={~\\#}," << endl;
    fHeader << "    keywords=[1]{}," << endl;
    fHeader << "    keywords=[2]{" << constructLaTeXHeaderKeywords(syntax->getCommands() + syntax->getNPRCCommands()) << "}," << endl;
    fHeader << "    keywords=[3]{" << constructLaTeXHeaderKeywords(syntax->getFunctions()) << "}," << endl;
    fHeader << "    keywords=[4]{" << constructLaTeXHeaderKeywords(syntax->getConstants()) << "}," << endl;
    fHeader << "    keywords=[5]{" << constructLaTeXHeaderKeywords(syntax->getOptions()) << "}," << endl;
    fHeader << "    keywords=[6]{" << constructLaTeXHeaderKeywords(syntax->getMethods()) << "}," << endl;
    fHeader << "    keywords=[7]{" << constructLaTeXHeaderKeywords(syntax->getSpecial()) << "}," << endl;
    fHeader << "    sensitive=true," << endl;
    fHeader << "    morecomment=[s]{\\#*}{*\\#}," << endl;
    fHeader << "    morecomment=[l][commentstyle]{\\#\\#}," << endl;
    fHeader << "    string=[b]\"" << endl;
    fHeader << "}" << endl << endl;
    fHeader << "\\newcommand\\realnumberstyle[1]{\\tiny}" << endl;
    fHeader << "\\newcommand\\oprts[1]{\\textcolor{red}{\\upshape{#1}}}" << endl << endl;

    fHeader << "% Apply a patch for the closing parenthesis" <<  endl;
    fHeader << "\\makeatletter" << endl;
    fHeader << "\\patchcmd{\\lsthk@SelectCharTable}{`)}{``}{}{}" << endl;
    fHeader << "\\makeatother" << endl << endl;

    fHeader << "% Define the colors needed for the language" << endl;
    fHeader << "\\definecolor{ProcedureStyle}{RGB}{128,0,0}" << endl;
    fHeader << "\\definecolor{CommandStyle}{RGB}{0,128,255}" << endl;
    fHeader << "\\definecolor{StringStyle}{RGB}{128,128,255}" << endl;
    fHeader << "\\definecolor{BGColorTwo}{RGB}{245,245,245}" << endl;
    fHeader << "\\definecolor{BGColorOne}{RGB}{230,230,230}" << endl;
    fHeader << "\\definecolor{CommentStyle}{RGB}{0,128,0}" << endl;
    fHeader << "\\definecolor{ConstantStyle}{RGB}{255,0,128}" << endl;
    fHeader << "\\definecolor{MethodStyle}{RGB}{0,180,50}" << endl;
    fHeader << "\\definecolor{OptionStyle}{RGB}{0,128,100}" << endl << endl;

    fHeader << "% Activate the language" << endl;
    fHeader << "\\lstset{" << endl;
    fHeader << "    language=nscr," << endl;
    fHeader << "    basicstyle={\\footnotesize\\ttfamily\\itshape}," << endl;
    fHeader << "    extendedchars=true," << endl;
    fHeader << "    tabsize=4," << endl;
    fHeader << "    columns=fixed," << endl;
    fHeader << "    keepspaces=false," << endl;
    fHeader << "    breaklines=true," << endl;
    fHeader << "    showstringspaces=false," << endl;
    fHeader << "    numbers=left, numberstyle=\\tiny, stepnumber=2, numbersep=5pt," << endl;
    fHeader << "    commentstyle={\\color{CommentStyle}\\bfseries\\upshape}," << endl;
    fHeader << "    keywordstyle=[1]{\\color{ProcedureStyle}\\bfseries\\upshape}," << endl;
    fHeader << "    keywordstyle=[2]{\\color{CommandStyle}\\bfseries\\upshape\\underbar}," << endl;
    fHeader << "    keywordstyle=[3]{\\color{blue}\\bfseries\\upshape}," << endl;
    fHeader << "    keywordstyle=[4]{\\color{ConstantStyle}\\bfseries\\upshape}," << endl;
    fHeader << "    keywordstyle=[5]{\\color{OptionStyle}\\upshape}," << endl;
    fHeader << "    keywordstyle=[6]{\\color{MethodStyle}\\bfseries\\upshape}," << endl;
    fHeader << "    keywordstyle=[7]{\\bfseries\\upshape}," << endl;
    fHeader << "    stringstyle={\\color{StringStyle}\\upshape}," << endl;
    fHeader << "    backgroundcolor=\\color{BGColorTwo}," << endl;
    fHeader << "    literate=*{(}{{\\oprts{(}}}1" << endl;
	fHeader << "        {)}{{\\oprts{)}}}1" << endl;
	fHeader << "        {[}{{\\oprts{[}}}1" << endl;
	fHeader << "        {]}{{\\oprts{]}}}1" << endl;
	fHeader << "        {\\{}{{\\oprts{\\{}}}1" << endl;
	fHeader << "        {\\}}{{\\oprts{\\}}}}1" << endl;
	fHeader << "        {+}{{\\oprts{+}}}1" << endl;
	fHeader << "        {-}{{\\oprts{-}}}1" << endl;
	fHeader << "        {*}{{\\oprts{*}}}1" << endl;
	fHeader << "        {/}{{\\oprts{/}}}1" << endl;
	fHeader << "        {\\^}{{\\oprts{\\^{}}}}1" << endl;
	fHeader << "        {\\%}{{\\oprts{\\%}}}1" << endl;
	fHeader << "        {=}{{\\oprts{=}}}1" << endl;
	fHeader << "        {!}{{\\oprts{!}}}1" << endl;
	fHeader << "        {?}{{\\oprts{?}}}1" << endl;
	fHeader << "        {>}{{\\oprts{>}}}1" << endl;
	fHeader << "        {<}{{\\oprts{<}}}1" << endl;
	fHeader << "        {\\&}{{\\oprts{\\&}}}1" << endl;
	fHeader << "        {|}{{\\oprts{|}}}1" << endl;
	fHeader << "        {;}{{\\oprts{;}}}1" << endl;
	fHeader << "        {,}{{\\oprts{,}}}1" << endl;
	fHeader << "        {:}{{\\oprts{:}}}1" << endl;
    fHeader << "}" << endl;

}

void NumeReWindow::runLaTeX()
{
    wxFileName filename = m_currentEd->GetFileName();
    filename.SetName(filename.GetName() + "_" + filename.GetExt());
    filename.SetExt("tex");
    filename.SetPath(m_terminal->getPathSettings()[SAVEPATH] + "/docs");
    if (!m_currentEd->writeLaTeXFile(filename.GetFullPath().ToStdString()))
    {
        wxMessageBox(_guilang.get("GUI_DLG_LATEX_ERROR_MESSAGE", filename.GetFullPath().ToStdString()), _guilang.get("GUI_DLG_LATEX_ERROR"), wxCENTER | wxOK, this);
        return;
    }
    string sMain = createLaTeXMain(filename.GetPath().ToStdString(), filename.GetName().ToStdString());
    if (fileExists((m_options->GetLaTeXRoot() + "/xelatex.exe").ToStdString()))
    {
        ShellExecuteA(NULL, "open", (m_options->GetLaTeXRoot()+"/xelatex.exe").ToStdString().c_str(), sMain.c_str(), filename.GetPath().ToStdString().c_str(), SW_SHOW);
    }
    else
        wxMessageBox(_guilang.get("GUI_DLG_NOTEXBIN_ERROR", m_options->GetLaTeXRoot().ToStdString()), _guilang.get("GUI_DLG_NOTEXBIN"), wxCENTER | wxOK | wxICON_ERROR, this);
}

void NumeReWindow::compileLaTeX()
{
    FileFilterType fileType = m_currentEd->getFileType();
    if (fileType == FILE_TEXSOURCE)
    {
        wxFileName filename = m_currentEd->GetFileName();
        ShellExecuteA(NULL, "open", (m_options->GetLaTeXRoot()+"/xelatex.exe").ToStdString().c_str(), filename.GetName().ToStdString().c_str(), filename.GetPath().ToStdString().c_str(), SW_SHOW);
    }
}

void NumeReWindow::deleteFile()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
    if (wxYES != wxMessageBox(_guilang.get("GUI_DLG_DELETE_QUESTION", data->filename.ToStdString()), _guilang.get("GUI_DLG_DELETE"), wxCENTRE | wxICON_QUESTION | wxYES_NO, this))
        return;
    if (m_clickedTreeItem == m_copiedTreeItem)
        m_copiedTreeItem = 0;
    Recycler _recycler;
    _recycler.recycle(data->filename.c_str());
    //wxRemoveFile(data->filename);
}


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


void NumeReWindow::renameFile()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
    wxFileName target_filename = data->filename;
    wxFileName source_filename = target_filename;
    wxTextEntryDialog* textentry = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_RENAME_QUESTION"), _guilang.get("GUI_DLG_RENAME"), target_filename.GetName());
    int retval = textentry->ShowModal();

    if (retval == wxID_CANCEL)
    {
        delete textentry;
        return;
    }
    target_filename.SetName(textentry->GetValue());

    delete textentry;

    if (wxFileExists(target_filename.GetFullPath()))
    {
        wxMessageBox(_guilang.get("GUI_DLG_RENAME_ERROR"), _guilang.get("GUI_DLG_RENAME"), wxCENTRE | wxICON_ERROR | wxOK, this);
        return;
    }

    wxRenameFile(source_filename.GetFullPath(), target_filename.GetFullPath());
}


void NumeReWindow::EvaluateCommandLine(wxArrayString& wxArgV)
{
    wxArrayString filestoopen;
    wxString ext;
    for (size_t i = 1; i < wxArgV.size(); i++)
    {
        if (wxArgV[i].find('.') == string::npos)
            continue;
        if (wxArgV[i].find(".exe") != string::npos)
            continue;
        ext = toLowerCase(wxArgV[i].substr(wxArgV[i].rfind('.')).ToStdString());
        if (ext == ".nscr")
        {
            if (i+1 < wxArgV.size() && wxArgV[i+1] == "-e")
            {
                m_terminal->pass_command("start \"" + replacePathSeparator(wxArgV[i].ToStdString()) + "\"");
                i++;
            }
            else
                filestoopen.Add(wxArgV[i]);
        }
        if (ext == ".nprc")
        {
            if (i+1 < wxArgV.size() && wxArgV[i+1] == "-e")
            {
                m_terminal->pass_command("$'" + replacePathSeparator(wxArgV[i].substr(0, wxArgV[i].rfind('.')).ToStdString()) + "'()");
                i++;
            }
            else
                filestoopen.Add(wxArgV[i]);
        }
        if (ext == ".dat"
            || ext == ".txt"
            || ext == ".tex")
            filestoopen.Add(wxArgV[i]);
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
            m_terminal->pass_command("append \"" + replacePathSeparator(wxArgV[i].ToStdString()) + "\"");
    }
    if (filestoopen.size())
        OpenSourceFile(filestoopen);
}


// File and editor manipulation code begins here

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
        ProjectInfo* singleFileProject = new ProjectInfo();
        NumeReEditor* edit = new NumeReEditor (this, m_options, singleFileProject, m_book, -1, m_terminal->getSyntax(), m_terminal);
        //edit->SetSyntax(m_terminal->getSyntax());

    #if wxUSE_DRAG_AND_DROP
        edit->SetDropTarget(new NumeReDropTarget(this, edit, NumeReDropTarget::EDITOR));
    #endif

        edit->SetText("\r\n");
        CopyEditorSettings(edit, _filetype);

        m_currentEd = edit;

        m_currentEd->EmptyUndoBuffer();
        m_currentPage = m_book->GetPageCount();
        m_book->AddPage (edit, noname, true);
    }
    else
    {
        wxString filename;
        wxString folder;
        wxTextEntryDialog* textentry;

        // If no default file name was passed, ask
        // the user
        if (!defaultfilename.length())
        {
            if (_filetype == FILE_NSCR)
                textentry = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_NEWNSCR_QUESTION"), _guilang.get("GUI_DLG_NEWNSCR"), _guilang.get("GUI_DLG_NEWNSCR_DFLT"));
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
            filename = defaultfilename;

        // Remove the dollar sign, if there is one
        if (filename.find('$') != string::npos)
            filename.erase(filename.find('$'),1);

        // Remove the path parts from the file name
        // These are either the tilde, the slash or the
        // backslash
        if (filename.find('~') != string::npos)
        {
            folder = filename.substr(0, filename.rfind('~')+1);
            filename.erase(0, filename.rfind('~')+1);
        }

        if (filename.find('/') != string::npos)
        {
            if (folder.length())
                folder += "/" + filename.substr(0, filename.rfind('/')+1);
            else
                folder = filename.substr(0, filename.rfind('/')+1);

            filename.erase(0, filename.rfind('/')+1);
        }

        if (filename.find('\\') != string::npos)
        {
            if (folder.length())
                folder += "/" + filename.substr(0, filename.rfind('\\')+1);
            else
                folder = filename.substr(0, filename.rfind('\\')+1);

            filename.erase(0, filename.rfind('\\')+1);
        }

        // Replace all path separators
        if (folder.length())
        {
            while (folder.find('~') != string::npos)
                folder[folder.find('~')] = '\\';
            while (folder.find('/') != string::npos)
                folder[folder.find('/')] = '\\';
        }

        if (folder == "main\\" && _filetype == FILE_NPRC)
            folder.clear();
        else
            folder.insert(0,"\\");

        // Prepare the template file
        wxString template_file, dummy, timestamp;

        // Search the correct filename
        if (_filetype == FILE_NSCR)
            dummy = "tmpl_script.nlng";
        else if (_filetype == FILE_PLUGIN)
            dummy = "tmpl_plugin.nlng";
        else
            dummy = "tmpl_procedure.nlng";

        timestamp = getTimeStamp(false);

        // Get the template file contents
        if (m_terminal->getKernelSettings().getUseCustomLanguageFiles() && wxFileExists(getProgramFolder() + "\\user\\lang\\"+dummy))
            GetFileContents(getProgramFolder() + "\\user\\lang\\"+dummy, template_file, dummy);
        else
            GetFileContents(getProgramFolder() + "\\lang\\"+dummy, template_file, dummy);

        // Replace the tokens in the file
        while (template_file.find("%%1%%") != string::npos)
            template_file.replace(template_file.find("%%1%%"), 5, filename);

        while (template_file.find("%%2%%") != string::npos)
            template_file.replace(template_file.find("%%2%%"), 5, timestamp);

        // Determine the file extension
        if (_filetype == FILE_NSCR)
            filename += ".nscr";
        else if (_filetype == FILE_PLUGIN)
            filename = "plgn_" + filename + ".nscr";
        else
            filename += ".nprc";

        vector<string> vPaths = m_terminal->getPathSettings();

        m_fileNum += 1;

        // Create a new editor
        ProjectInfo* singleFileProject = new ProjectInfo();
        NumeReEditor* edit = new NumeReEditor (this, m_options, singleFileProject, m_book, -1, m_terminal->getSyntax(), m_terminal);
        edit->SetText(template_file);

        CopyEditorSettings(edit, _filetype);

        m_currentEd = edit;

        // Set the corresponding full file name
        if (_filetype == FILE_NSCR || _filetype == FILE_PLUGIN)
            m_currentEd->SetFilename(wxFileName(vPaths[SCRIPTPATH] + folder, filename), false);
        else
            m_currentEd->SetFilename(wxFileName(vPaths[PROCPATH] + folder, filename), false);

        // Jump to the standard input line in the corresponding template file
        if (_filetype == FILE_NSCR)
            m_currentEd->GotoLine(13);
        else if (_filetype == FILE_NPRC)
            m_currentEd->GotoLine(6);
        else
            m_currentEd->GotoLine(28);

        m_currentEd->EmptyUndoBuffer();
        m_currentPage = m_book->GetPageCount();
        m_currentEd->UpdateSyntaxHighlighting();
        m_currentEd->SetUnsaved();

        // Add a new tab for the editor
        m_book->AddPage (edit, filename, true);
    }
}

void NumeReWindow::CopyEditorSettings(NumeReEditor* edit, FileFilterType _fileType)
{
    if (m_currentEd && edit && !m_loadingFilesDuringStartup)
    {
        int settings = m_currentEd->getSettings();
        if (_fileType != FILE_NSCR && _fileType != FILE_NPRC && _fileType != FILE_MATLAB && _fileType != FILE_PLUGIN)
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
        edit->ToggleSettings(settings);
    }
}

void NumeReWindow::DefaultPage()
{
    wxString template_file, dummy;
    dummy = "tmpl_defaultpage.nlng";

    if (m_terminal->getKernelSettings().getUseCustomLanguageFiles() && wxFileExists(getProgramFolder() + "\\user\\lang\\"+dummy))
        GetFileContents(getProgramFolder() + "\\user\\lang\\"+dummy, template_file, dummy);
    else
        GetFileContents(getProgramFolder() + "\\lang\\"+dummy, template_file, dummy);

    vector<string> vPaths = m_terminal->getPathSettings();

    m_fileNum += 1;

    ProjectInfo* singleFileProject = new ProjectInfo();
    NumeReEditor* edit = new NumeReEditor (this, m_options, singleFileProject, m_book, -1, m_terminal->getSyntax(), m_terminal);

#if wxUSE_DRAG_AND_DROP
    edit->SetDropTarget(new NumeReDropTarget(this, edit, NumeReDropTarget::EDITOR));
#endif

    m_currentEd = edit;
    m_currentEd->LoadFileText(template_file);
    m_currentEd->defaultPage = true;
    m_currentEd->SetReadOnly(true);
    m_currentEd->ToggleSettings(NumeReEditor::SETTING_USETXTADV);
    m_currentPage = m_book->GetPageCount();
    m_book->AddPage (edit, _guilang.get("GUI_EDITOR_TAB_WELCOMEPAGE"), true);
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
		m_currentEd = NULL;
		return;
	}

	// no page passed in
	if (pageNr == -1)
	{
		pageNr = m_book->GetSelection();

	}

	if ((int)m_book->GetPageCount() <= pageNr)
	{
		pageNr = pageNr - 1;
	}

	// activate the selected page
	if (pageNr >= 0)
	{
        if (m_currentEd->AutoCompActive())
            m_currentEd->AutoCompCancel();
        if (m_currentEd->CallTipActive())
            m_currentEd->AdvCallTipCancel();
		m_currentPage = pageNr;
		m_currentEd = static_cast< NumeReEditor * > (m_book->GetPage (m_currentPage));
		m_book->SetSelection(pageNr);
		if (!m_book->GetMouseFocus())
            m_currentEd->SetFocus();
	}
	else
	{
		m_currentPage = -1;
		m_currentEd = NULL;
	}
	m_book->Refresh();
	if (m_currentEd != NULL)
	{
        wxMenu* view = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_guilang.get("GUI_MENU_VIEW")));
        view->Check(ID_MENU_LINEWRAP, m_currentEd->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
        view->Check(ID_MENU_DISPCTRLCHARS, m_currentEd->getEditorSetting(NumeReEditor::SETTING_DISPCTRLCHARS));
        view->Check(ID_MENU_USETXTADV, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USETXTADV));
        view->Check(ID_MENU_USESECTIONS, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USESECTIONS));
        view = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_guilang.get("GUI_MENU_TOOLS")));
        view->Check(ID_MENU_USEANALYZER, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
        view->Check(ID_MENU_INDENTONTYPE, m_currentEd->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
        m_currentEd->Refresh();

		wxString tabText = m_book->GetPageText(m_currentPage);
		wxString statusText;
		// set the title of the main window according the current opened file
		UpdateWindowTitle(tabText);

		// else assume unsaved file and don't change anything
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private CloseTab
///  Closes a tab after the user right-clicks it and selects "Close"
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::CloseTab()
{
	int tab = GetIntVar(VN_CLICKEDTAB);
	CloseFile(tab);
	m_book->Refresh();
}
void NumeReWindow::CloseOtherTabs()
{
	int tab = GetIntVar(VN_CLICKEDTAB);
	while (tab)
	{
        CloseFile(0);
        tab--;
	}
	while (m_book->GetPageCount() > 1)
        CloseFile(1);
	m_book->Refresh();
}

void NumeReWindow::OpenContainingFolder()
{
    int tab = GetIntVar(VN_CLICKEDTAB);
    NumeReEditor* edit = static_cast<NumeReEditor*>(m_book->GetPage(tab));
    wxFileName filename = edit->GetFileName();
    wxExecute("explorer " + filename.GetPath(), wxEXEC_ASYNC, nullptr, nullptr);
}

void NumeReWindow::EvaluateTab()
{
	int tab = GetIntVar(VN_CLICKEDTAB);
	NumeReEditor* edit = static_cast<NumeReEditor*>(m_book->GetPage(tab));
    if(!edit->HasBeenSaved() || edit->Modified())
    {
        int result = HandleModifiedFile(tab, MODIFIEDFILE_COMPILE);

        if (result == wxCANCEL)
        {
            return;
        }
    }
    string command = replacePathSeparator((edit->GetFileName()).GetFullPath().ToStdString());
    OnExecuteFile(command);
}

//////////////////////////////////////////////////////////////////////////////
///  private CloseFile
///  Closes a given editor, based on its index in the editor notebook
///
///  @param  pageNr int  [=-1] The index of the editor to close
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::CloseFile (int pageNr, bool askforsave)
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
		wxFileName currentFileName;
		m_terminal->_guimessenger.clearBreakpoints(m_currentEd->GetFileNameAndPath().ToStdString());
		if ((m_book->GetPageCount() > 1) || m_appClosing)
		{
			currentFileName = m_currentEd->GetFileName();
			//NumeReEditor* pEdit = static_cast <NumeReEditor* >(m_book->GetPage(pageNr));
			m_book->DeletePage (pageNr);
			m_watcher->Remove(currentFileName);
		}
		// closing out the last buffer, reset it to act as a new one
		else
		{
			m_fileNum = 1;
			m_watcher->Remove(m_currentEd->GetFileName());
			//wxString locationPrefix = "(?) ";
			wxString noname = _guilang.get("GUI_NEWFILE_UNTITLED") + " " + wxString::Format ("%d", m_fileNum);
			m_book->SetPageText (pageNr, noname);
			m_currentEd->ResetEditor();
            m_currentEd->SetText("\r\n");
            m_currentEd->EmptyUndoBuffer();
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
	ofstream of_session;
	NumeReEditor* edit;
	string sSession = "# numere.session: Session save file. Do not edit!\n# ACTIVEFILE\tFILEID\nACTIVEFILEID\t" + toString(m_book->GetSelection()) + "\n# FILENAME\t\tFILEID\t\tCHARPOSITION\t\tSETTING\n";

	for (int i = 0; i < cnt; i++)
	{
        edit = static_cast<NumeReEditor*>(m_book->GetPage(i));
        if (edit->defaultPage)
            continue;
        // gives the user a chance to save if the file has been modified
        int nReturn = HandleModifiedFile(i, MODIFIEDFILE_CLOSE);
        if (nReturn == wxCANCEL)
            return false;

        if (edit->Modified())
            sSession += "*";
        if (edit->GetFileNameAndPath().length())
            sSession += edit->GetFileNameAndPath().ToStdString() + "\t" + toString(i) + "\t" + toString(edit->GetCurrentPos()) + "\t" + toString(edit->getSettings()) + "\n";
        else
        {
            sSession += "<NEWFILE>\t" +toString(i) + "\t" + toString(edit->GetCurrentPos()) + "\n";
        }
	}
	for (int i = 0; i < cnt; i++)
	{
		CloseFile(-1, false);
	}

	if (m_appClosing && !m_sessionSaved && m_options->GetSaveSession())
	{
        of_session.open((getProgramFolder().ToStdString()+"/numere.session").c_str(), ios_base::out | ios_base::trunc);
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
		edit = static_cast <NumeReEditor *> (m_book->GetPage(pageNum));

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
void NumeReWindow::OnPageChange (wxNotebookEvent &WXUNUSED(event))
{
	if (!m_setSelection)
	{
        ToolbarStatusUpdate();
		PageHasChanged();
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private HandleModifiedFile
///  Handle user interaction when closing or reloading an editor
///
///  @param  pageNr      int   The index of the editor being closed or reloaded
///  @param  closingFile bool  True if the file is being closed, false if it's being reloaded
///
///  @return int         What the user chose to do.  The only real meaningful return value is wxCANCEL.
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int NumeReWindow::HandleModifiedFile(int pageNr, ModifiedFileAction fileAction)
{
	NumeReEditor *edit = static_cast <NumeReEditor * > (m_book->GetPage (pageNr));

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

		int result = wxMessageBox (saveMessage, _guilang.get("GUI_SAVE_QUESTION"), options);//wxYES_NO | wxCANCEL | wxICON_QUESTION);
		if( result == wxYES)
		{
			NumeReEditor* tmpCurrentEd = m_currentEd;
			m_currentEd = edit;
			// only do a Save As if necessary
			SaveFile(false, true, FILE_ALLSOURCETYPES);
			m_currentEd = tmpCurrentEd;
			m_currentEd->SetFocus();

			if (edit->Modified())
			{
				wxString errorMessage = fileName + " could not be saved!";
				wxMessageBox (errorMessage, "File not closed",
					wxOK | wxICON_EXCLAMATION);
				m_currentEd->Refresh();
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
	if (!m_currentEd)
	{
		return fnames;
	}

	wxString filterString = ConstructFilterString(filterType);

	if(!m_remoteMode)
	{
		wxFileDialog dlg (this, _(_guilang.get("GUI_DLG_OPEN")), "", "", filterString,
			wxFD_OPEN | wxFD_FILE_MUST_EXIST  | wxFD_CHANGE_DIR);
		if (dlg.ShowModal() != wxID_OK)
		{
			return fnames;
		}
		m_currentEd->SetFocus();
		dlg.GetPaths (fnames);
	}
	else
	{
		if(!CheckForBlankPassword())
		{
			return wxArrayString();
		}

		SetStatusText("Doing network operation...", 3);

		wxBusyCursor busyCursor;
		if(m_remoteFileDialog->Prepare(true, filterString))
		{
			int result = m_remoteFileDialog->ShowModal();
			SetStatusText(wxEmptyString, 3);
			m_currentEd->SetFocus();

			if(result != wxOK)
			{
				return fnames;
			}

			wxString remoteFileName = m_remoteFileDialog->GetRemoteFileNameAndPath();

			wxString userHome;
			if(!m_network->GetHomeDirPath(userHome))
			{
				CheckNetworkStatus();
			}
			else
			{
				remoteFileName.Replace("~", userHome);
				fnames.Add(remoteFileName);
			}
		}
		else
		{
			CheckNetworkStatus();
			SetStatusText(wxEmptyString, 3);
			return fnames;
		}
	}

	return fnames;
}


void NumeReWindow::OpenFileByType(const wxFileName& filename)
{
    if (filename.GetExt() == "nscr"
        || filename.GetExt() == "nprc"
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
        || filename.GetExt() == "tex")
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
        || filename.GetExt() == "gif")
    {
        openImage(filename);
        CallAfter(&NumeReWindow::setViewerFocus);
        return;
    }
    else if (filename.GetExt() == "pdf")
    {
        openPDF(filename);
        return;
    }
    else
    {
        wxString path = "load \"" + replacePathSeparator(filename.GetFullPath().ToStdString()) + "\" -app -ignore";
        showConsole();
        m_terminal->pass_command(path.ToStdString());
    }
}

void NumeReWindow::OpenFilesFromList(const wxArrayString& filenameslist)
{
    for (size_t i = 0; i < filenameslist.size(); i++)
    {
        OpenFileByType(wxFileName(filenameslist[i]));
    }
}


//////////////////////////////////////////////////////////////////////////////
///  public OpenSourceFile
///  Opens the given list of source files in the editor
///
///  @param  fnames wxArrayString  The source files to open
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
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
		{
			return;
		}

        if (nOpenFileFlag & OPENFILE_BLACKLIST_ADD)
        {
            addToReloadBlackList(fnames[n]);
        }
        if (nOpenFileFlag & OPENFILE_BLACKLIST_REMOVE)
        {
            removeFromReloadBlackList(fnames[n]);
        }
		// filename is already open
		if (pageNr >= 0)
		{
            if (nLine)
            {
                PageHasChanged(pageNr);
                m_currentEd->FocusOnLine(nLine, true);
                //m_currentEd->GotoLine(nLine);
            }
            else
            {
                if (!(nOpenFileFlag & OPENFILE_BLACKLIST_ADD))
                {
                    if (!(nOpenFileFlag & OPENFILE_FORCE))
                    {
                        int modifiedFileResult = HandleModifiedFile(pageNr, MODIFIEDFILE_RELOAD);

                        // user canceled the open request, skip the reload
                        if(modifiedFileResult == wxCANCEL)
                        {
                            continue;
                        }
                    }
                    m_setSelection = true;
                    m_book->SetSelection (pageNr);
                    m_setSelection = false;
                    m_currentPage = pageNr;

                    m_currentEd = static_cast< NumeReEditor* > (m_book->GetPage (m_currentPage));
                    m_currentEd->LoadFileText(fileContents);
                }
			}
		}
		else
		{
			ProjectInfo* proj;

			FileFilterType _fileType;
			if (fnames[n].rfind(".nscr") != string::npos)
                _fileType = FILE_NSCR;
            else if (fnames[n].rfind(".nprc") != string::npos)
                _fileType = FILE_NPRC;
            else
                _fileType = FILE_NOTYPE;

			if( (m_projMultiFiles != NULL) && (m_projMultiFiles->FileExistsInProject(fnames[n])))
			{
				proj = m_projMultiFiles;
			}
			else
			{
				proj = new ProjectInfo();
				proj->SetRemote(m_remoteMode);

				/*
				wxString fullFileName = newFileName.GetFullPath();
				FileFilterType filterType = proj->GetFileType(fullFileName);
				proj->AddFileToProject(fullFileName, filterType);
				*/
			}

			// current buffer is empty and untouched, so load the file into it
			if ((!m_currentEd->Modified()
                && !m_currentEd->HasBeenSaved()
                && (m_currentEd->GetText().IsEmpty() || m_currentEd->GetText() == "\r\n")) || m_currentEd->defaultPage )
			{
				m_book->SetPageText(m_currentPage, /*locationPrefix + */fileNameNoPath);
				m_currentEd->SetProject(proj);
				m_currentEd->LoadFileText(fileContents);
			}
			// need to create a new buffer for the file
			else
			{
				NumeReEditor *edit = new NumeReEditor (this, m_options, proj, m_book, -1, m_terminal->getSyntax(), m_terminal);
#if wxUSE_DRAG_AND_DROP
				edit->SetDropTarget(new NumeReDropTarget(this, edit, NumeReDropTarget::EDITOR));
#endif
                edit->LoadFileText(fileContents);
                CopyEditorSettings(edit, _fileType);
				m_currentEd = edit;
				m_currentPage = m_book->GetPageCount();
				m_book->AddPage (m_currentEd, /*locationPrefix + */fileNameNoPath, true);
			}


			m_currentEd->SetFilename(newFileName, m_remoteMode);

            m_watcher->Add(newFileName);
			if (nLine)
                m_currentEd->GotoLine(nLine);
		}

		if (firstPageNr < 0)
		{
			firstPageNr = m_currentPage;
		}

		m_currentEd->UpdateSyntaxHighlighting();
	}

	// show the active tab, new or otherwise
	if (firstPageNr >= 0)
	{
		PageHasChanged(firstPageNr);
	}
    m_currentEd->SetFocus();
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
	if (false)
	{
		wxString remotePath = fn.GetPath(false, wxPATH_UNIX);
		wxString remoteFile = fn.GetFullName();

		if (!m_network->GetFileContents(fn, fileContents))
		{
			CheckNetworkStatus();
		}
	}
	else
	{
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
	}

	fileName = fn.GetFullName();
	return true;
}

wxTreeItemId NumeReWindow::getDragDropSourceItem()
{
    wxTreeItemId retVal = m_dragDropSourceItem;
    m_dragDropSourceItem = wxTreeItemId();
    return retVal;
}

wxString NumeReWindow::getTreeFolderPath(const wxTreeItemId& itemId)
{
    if (!itemId.IsOk())
        return wxString();

    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(itemId));
    wxString pathName;

    if (!data)
    {
        vector<string> vPaths = m_terminal->getPathSettings();
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
    {
        pathName = data->filename;
    }

    return pathName;
}

// This member function tells NumeRe that
// it should display the "ready" state to the user
void NumeReWindow::Ready()
{
    if (m_statusBar)
        m_statusBar->Ready();
    wxToolBar* tb = GetToolBar();
    tb->EnableTool(ID_MENU_EXECUTE, true);
    tb->EnableTool(ID_MENU_STOP_EXECUTION, false);

    UpdateVarViewer();
}

// This member function tells NumeRe that
// it should display the "busy" state to the user
void NumeReWindow::Busy()
{
    if (m_statusBar)
        m_statusBar->Busy();
    wxToolBar* tb = GetToolBar();
    tb->EnableTool(ID_MENU_EXECUTE, false);
    tb->EnableTool(ID_MENU_STOP_EXECUTION, true);
}

//////////////////////////////////////////////////////////////////////////////
///  private SaveFile
///  Saves a text file, abstracting out local / remote issues.
///
///  @param  saveas         bool            True if this is explicitly a "Save-As" command and a file dialog must be shown
///  @param  askLocalRemote bool            True if the user should be asked whether to save the file locally or remotely
///  @param  filterType     FileFilterType  The type of files to show in the dialog
///
///  @return bool           Whether the save operation succeeded or not
///
///  @remarks This function is also used for creating new project files, since that's effectively
///  @remarks just saving a text file as well.
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReWindow::SaveFile(bool saveas, bool askLocalRemote, FileFilterType filterType)
{
	wxString filename;
	wxString fileContents;

	//bool originalRemoteMode = m_remoteMode;
    m_remoteMode = false;
	bool doSaveAs = saveas || !m_currentEd->HasBeenSaved() /*|| (m_remoteMode != m_currentEd->LastSavedRemotely())*/;

	bool isSourceFile = !(m_currentEd->getFileType() == FILE_NUMERE);

	wxString filterString = ConstructFilterString(m_currentEd->getFileType());


	if (isSourceFile)
	{
		fileContents = m_currentEd->GetText();
	}
	// we must be saving a new project
	else
	{
		fileContents = "[Headers]\n\n[Sources]\n\n[Libraries]\n\n[Other]";
	}

	//if(true)
	{
		if(doSaveAs)
		{
			// the last item in a filter's list will be the default extension if none is given
			// ie, right now, .cpp is the default extension for C++ files


			wxString title = _guilang.get("GUI_DLG_SAVEAS");
			vector<string> vPaths = m_terminal->getPathSettings();
			int i = 0;

            if (m_currentEd->getFileType() == FILE_NSCR)
                i = SCRIPTPATH;
            else if (m_currentEd->getFileType() == FILE_NPRC)
                i = PROCPATH;
            else if (m_currentEd->getFileType() == FILE_DATAFILES
                || m_currentEd->getFileType() == FILE_TEXSOURCE
                || m_currentEd->getFileType() == FILE_NONSOURCE)
                i = SAVEPATH;
			wxFileDialog dlg (this, title, vPaths[i], "", filterString,
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);

            if (m_currentEd->getFileType() == FILE_DATAFILES)
            {
                if (m_currentEd->GetFileName().GetExt() == "dat")
                    dlg.SetFilterIndex(0);
                else if (m_currentEd->GetFileName().GetExt() == "txt")
                    dlg.SetFilterIndex(1);
                else if (m_currentEd->GetFileName().GetExt() == "csv")
                    dlg.SetFilterIndex(2);
                else if (m_currentEd->GetFileName().GetExt() == "jdx"
                    || m_currentEd->GetFileName().GetExt() == "dx"
                    || m_currentEd->GetFileName().GetExt() == "jcm")
                    dlg.SetFilterIndex(3);
            }
            if (m_currentEd->getFileType() == FILE_NONSOURCE)
            {
                if (m_currentEd->GetFileName().GetExt() == "txt")
                    dlg.SetFilterIndex(0);
                else if (m_currentEd->GetFileName().GetExt() == "log")
                    dlg.SetFilterIndex(1);
            }
			// ie, user clicked cancel
			if(dlg.ShowModal() != wxID_OK)
			{
				return false;
			}
			if (m_currentEd->GetFileName().IsOk())
                m_watcher->Remove(m_currentEd->GetFileName());
			filename = dlg.GetPath();
			m_watcher->Add(wxFileName(filename));
			if (filename.find('.') == string::npos || filename.find('.', filename.rfind('\\')) == string::npos)
			{
                if (m_currentEd->getFileType() == FILE_NSCR)
                    filename += ".nscr";
                else if (m_currentEd->getFileType() == FILE_NPRC)
                    filename += ".nprc";
                else if (m_currentEd->getFileType() == FILE_DATAFILES)
                {
                    if (m_currentEd->GetFilenameString().find('.') != string::npos)
                        filename += m_currentEd->GetFilenameString().substr(m_currentEd->GetFilenameString().rfind('.'));
                    else
                        filename += ".dat";
                }
                else if (m_currentEd->getFileType() == FILE_TEXSOURCE)
                    filename += ".tex";
                else if (m_currentEd->getFileType() == FILE_NONSOURCE)
                {
                    if (m_currentEd->GetFilenameString().find('.') != string::npos)
                        filename += m_currentEd->GetFilenameString().substr(m_currentEd->GetFilenameString().rfind('.'));
                    else
                        filename += ".txt";
                }
			}
		}
		else
		{
			filename = m_currentEd->GetFileNameAndPath();
			string sPath = filename.ToStdString();
			sPath = replacePathSeparator(sPath);
			sPath.erase(sPath.rfind('/'));
			FileSystem _fSys;
			// Make the folder, if it doesn't exist
			_fSys.setPath(sPath, true, replacePathSeparator(getProgramFolder().ToStdString()));
		}

		if (isSourceFile)
		{
			m_currentEd->SetFocus();

			wxFileName fn(filename);
			m_currentEd->SetFilename(fn, false);
			m_currentSavedFile = toString((int)time(0)) + "|" +filename;
			if (!m_currentEd->SaveFile(filename))
            {
                wxMessageBox(_guilang.get("GUI_DLG_SAVE_ERROR"), _guilang.get("GUI_DLG_SAVE"), wxCENTRE | wxOK | wxICON_ERROR, this);
                return false;
            }
			wxString simpleFileName = m_currentEd->GetFilenameString();

			int currentTab = m_book->GetSelection();

			wxString locationPrefix = "(L) ";
			m_book->SetPageText(currentTab, simpleFileName);
			m_book->Refresh();
			UpdateWindowTitle(simpleFileName);

		}
		else
		{
			wxFile newProjectFile(filename, wxFile::write);
			newProjectFile.Write(fileContents);
			newProjectFile.Close();
		}

	}
	// remote mode
	/*else
	{
		wxString remotePath, remoteFile;

		if(!CheckForBlankPassword())
		{
			return false;
		}


		if(doSaveAs)
		{
			SetStatusText("Doing network operation...", 3);
			if(m_remoteFileDialog->Prepare(false, filterString))
			{
				int result = m_remoteFileDialog->ShowModal();
				SetStatusText(wxEmptyString, 3);
				m_currentEd->SetFocus();

				if(result != wxOK)
				{
					return false;
				}

				remotePath = m_remoteFileDialog->GetRemotePath();
				remoteFile = m_remoteFileDialog->GetRemoteFileName();

				wxString userHome;
				if(!m_network->GetHomeDirPath(userHome))
				{
					CheckNetworkStatus();
				}
				else
				{
					remotePath.Replace("~", userHome);
				}
			}
			else
			{
				CheckNetworkStatus();
				SetStatusText(wxEmptyString, 3);
				return false;
			}
		}
		else
		{
			remoteFile = m_currentEd->GetFilenameString();
			remotePath = m_currentEd->GetFilePath();
		}

		wxFileName remoteFN(remotePath, remoteFile);
		wxBeginBusyCursor();
		if(!m_network->SendFileContents(fileContents, remoteFN))
		{
			CheckNetworkStatus();
		}
		wxEndBusyCursor();

		if(isSourceFile)
		{
			wxString currentFilenameAndPath = m_currentEd->GetFileNameAndPath();
			if((currentFilenameAndPath == wxEmptyString) ||
				(currentFilenameAndPath != wxFileName(remotePath, remoteFile, wxPATH_UNIX).GetFullPath()))
			{
				int currentTab = m_book->GetSelection();
				wxString locationPrefix = "(R) ";
				m_book->SetPageText(currentTab, locationPrefix + remoteFile);
			}

			wxFileName fn(remotePath, remoteFile, wxPATH_UNIX);
			m_currentEd->SetFilename(fn, true);
		}
		else
		{
			// store the name so we can set the name in the project tree
			filename = wxFileName(remotePath, remoteFile, wxPATH_UNIX).GetFullPath();
		}
	}*/

	if(isSourceFile)
	{
		m_currentEd->EmptyUndoBuffer();
		m_currentEd->SetSavePoint();
        m_currentEd->UpdateSyntaxHighlighting();
		m_book->Refresh();
	}
	else
	{
		if(m_projMultiFiles != NULL)
		{
			CloseProjectFile();
		}

		m_projMultiFiles = new ProjectInfo(false);
		m_projMultiFiles->SetRemote(m_remoteMode);

		wxFileName projectFile(filename);
		m_projMultiFiles->SetProjectFile(projectFile);
		m_projMultiFiles->SetProjectName(projectFile.GetName());

		wxTreeItemId rootItem = m_fileTree->GetRootItem();
		m_fileTree->SetItemText(rootItem, m_projMultiFiles->GetProjectName());

	}
	return true;
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
	case FILE_ALLSOURCETYPES: /// TODO: find a senseful order
		filterString = m_filterExecutableFiles;
		filterString += "|";
		filterString += m_filterNSCRFiles;
		filterString += "|";
		filterString += m_filterNPRCFiles;
		break;
	case FILE_NSCR:
		filterString = m_filterNSCRFiles;
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


// not currently used.  may come into play later regarding terminal resizing
void NumeReWindow::CheckSize()
{
	int x, y;
	this->GetClientSize(&x, &y);
	wxString result;
	result << x << ", " << y;
	wxMessageBox(result );
	m_currentEd->SetScrollWidth(x);

}

// UI update code begins here

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
		OnUpdateConnectionUI();
		OnUpdateProjectUI();
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
    if (!m_currentEd)
        return;
	if (m_currentEd->getFileType() == FILE_NSCR || m_currentEd->getFileType() == FILE_NPRC || m_currentEd->getFileType() == FILE_MATLAB || m_currentEd->getFileType() == FILE_CPP)
	{
        tb->EnableTool(ID_MENU_ADDEDITORBREAKPOINT, true);
        tb->EnableTool(ID_MENU_REMOVEEDITORBREAKPOINT, true);
        tb->EnableTool(ID_MENU_CLEAREDITORBREAKPOINTS, true);
        tb->EnableTool(ID_MENU_USEANALYZER, true);
        tb->ToggleTool(ID_MENU_USEANALYZER, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
        tb->EnableTool(ID_MENU_INDENTONTYPE, true);
        tb->ToggleTool(ID_MENU_INDENTONTYPE, m_currentEd->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
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
	if (m_currentEd->GetFileName().GetExt() == "m")
    {
        tb->EnableTool(ID_MENU_INDENTONTYPE, true);
        tb->ToggleTool(ID_MENU_INDENTONTYPE, m_currentEd->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
    }
    tb->ToggleTool(ID_MENU_LINEWRAP, m_currentEd->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));

	/*if(isDebugging)
	{
		tb->EnableTool(ID_DEBUG_START, false);
		tb->EnableTool(ID_DEBUG_CONTINUE, isPaused);
		tb->EnableTool(ID_DEBUG_STOP, true);

		tb->EnableTool(ID_DEBUG_STEPNEXT, isPaused);
		tb->EnableTool(ID_DEBUG_STEPOVER, isPaused);
		tb->EnableTool(ID_DEBUG_STEPOUT, isPaused);
	}
	else
	{
		// skip ID_DEBUG_START;
		for(int i = ID_DEBUG_IDS_FIRST + 1; i < ID_DEBUG_IDS_LAST; i++)
		{
			tb->EnableTool(i, false);
		}

		//ProjectInfo* proj = m_currentEd->GetProject();

		//bool canStartDebug = proj->IsCompiled();
		tb->EnableTool(ID_DEBUG_START, true);
	}*/
}

//////////////////////////////////////////////////////////////////////////////
///  private OnUpdateConnectionUI
///  Updates the terminal connection buttons and menu items
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnUpdateConnectionUI()//wxUpdateUIEvent &event)
{
	bool termConnected = m_terminal->IsConnected();

	wxToolBar* tb = GetToolBar();
	tb->EnableTool(ID_STARTCONNECT, !termConnected);
	tb->EnableTool(ID_DISCONNECT, termConnected);

	wxMenuBar* mb = GetMenuBar();
	WXWidget handle = mb->GetHandle();
	if(handle != NULL)
	{
		wxMenu* toolsMenu = mb->GetMenu(mb->FindMenu(_guilang.get("GUI_MENU_TOOLS")));
		toolsMenu->Enable(ID_STARTCONNECT, !termConnected);
		toolsMenu->Enable(ID_DISCONNECT, termConnected);
	}


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
	{
		return;
	}

	int pageCount = m_book->GetPageCount();
	if (pageCount < 1 || pageCount <= m_currentPage)
	{
		return;
	}
	wxString tabText = m_book->GetPageText(m_currentPage);
	wxString filename;
	wxString filetype;
	string sExt = "";

	filename = m_currentEd->GetFileNameAndPath();
	if (m_currentEd->defaultPage)
        filename = _guilang.get("GUI_STATUSBAR_WELCOMEPAGE");

    if (filename.find('.') != string::npos)
        sExt = filename.substr(filename.rfind('.')+1).ToStdString();


	if (tabText.StartsWith("<"))
	{
		filename = _guilang.get("GUI_STATUSBAR_UNSAVEDFILE");
		filetype = "N/A";
	}
	else if (m_currentEd->defaultPage)
        filetype = _guilang.get("GUI_STATUSBAR_WELCOMEPAGE_FILETYPE");
	else if (sExt.length() && _guilang.get("GUI_STATUSBAR_"+toUpperCase(sExt)) != "GUI_STATUSBAR_"+toUpperCase(sExt))
	{
        filetype = _guilang.get("GUI_STATUSBAR_"+toUpperCase(sExt));
	}
	else
	{
        filetype = _guilang.get("GUI_STATUSBAR_UNKNOWN", sExt);
	}

	bool isEdReadOnly = m_currentEd->GetReadOnly();

	wxString editable = isEdReadOnly ? "Read only" : "Read/Write";

	int curLine = m_currentEd->GetCurrentLine();
	int curPos = m_currentEd->GetCurrentPos() - m_currentEd->PositionFromLine (-curLine);
	wxString linecol;
	linecol.Printf (_(_guilang.get("GUI_STATUSBAR_LINECOL")), curLine+1, curPos+1);

	wxString sDebuggerMode = "";
    if (m_terminal->getKernelSettings().getUseDebugger() && m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER))
         sDebuggerMode = _guilang.get("GUI_STATUSBAR_DEBUGGER_ANALYZER");
	else if (m_terminal->getKernelSettings().getUseDebugger() && !m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER))
         sDebuggerMode = _guilang.get("GUI_STATUSBAR_DEBUGGER");
	else if (!m_terminal->getKernelSettings().getUseDebugger() && m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER))
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
	bool enable = m_currentEd->Modified();

	int tabNum = m_book->GetSelection();
	wxString title = m_book->GetPageText(tabNum);

	if(enable)
	{
		int tabNum = m_book->GetSelection();
		wxString title = m_book->GetPageText(tabNum);

		if(!title.Contains("*"))
		{
			title += "*";
			m_book->SetPageText(tabNum, title);
			m_book->Refresh();
			UpdateWindowTitle(title);
		}
	}
	else
	{
		if(title.Contains("*"))
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
	if(handle != NULL)
	{
		mb->FindItem(ID_MENU_SAVE)->Enable(enable);
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private OnUpdateProjectUI
///  Updates project-related menu items
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnUpdateProjectUI ()
{
	/*
	bool enable = (m_book->GetPageCount() > 1) ||
		(m_currentEd && (m_currentEd->GetLength() > 0));
	*/
	bool projectOpen = (m_projMultiFiles != NULL);

	wxMenuBar* mb = GetMenuBar();
	WXWidget handle = mb->GetHandle();
	if(handle != NULL)
	{
		mb->FindItem(ID_CLOSE_PROJECT)->Enable(projectOpen);
		mb->FindItem(ID_COMPILE_PROJECT)->Enable(projectOpen);
	}

}

// Gets / sets begin here

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
///  public IsDebugging
///  Returns the debugger's active status
///
///  @return bool Whether or not the debugger is active
///
///  @remarks Used by ChameleonEditor
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReWindow::IsDebugging()
{
//	return m_debugger->isDebugging();
    return false;
}

//////////////////////////////////////////////////////////////////////////////
///  public IsDebuggerPaused
///  Returns if the debugger is paused or not
///
///  @return bool Whether or not the debugger is paused
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool NumeReWindow::IsDebuggerPaused()
{
//	return m_debugger->isPaused();
    return false;
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

// Options evaluation and GUI reconfiguration code begins here

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
	m_network->PingOptions();

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
            m_splitProjectEditor->SplitVertically(m_treeBook, m_splitEditorOutput, 200);
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
        vector<string> vPaths = m_terminal->getPathSettings();
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
        {
            m_watcher->SetDefaultPaths(vPaths);
        }
    }

    // Update the syntax highlighting in every
    // editor instance
	for(int i = 0; i < (int)m_book->GetPageCount(); i++)
	{
		NumeReEditor* edit = static_cast<NumeReEditor*> (m_book->GetPage(i));
		edit->UpdateSyntaxHighlighting();
		edit->SetCaretPeriod(m_options->GetCaretBlinkTime());
	}

	// Copy the settings in the options object
	// into the configuration object
	int newMaxTermSize = m_options->GetTerminalHistorySize();
	m_termContainer->SetTerminalHistory(newMaxTermSize);
	m_config->Write("Miscellaneous/TerminalHistory", newMaxTermSize);

	int newCaretBlinkTime = m_options->GetCaretBlinkTime();
	m_termContainer->SetCaretBlinkTime(newCaretBlinkTime);
	m_config->Write("Interface/CaretBlinkTime", newCaretBlinkTime);

	bool printInColor = (m_options->GetPrintStyle() == wxSTC_PRINT_COLOURONWHITE);
	m_config->Write("Miscellaneous/PrintInColor", printInColor ? "true" : "false");

	bool printLineNumbers = m_options->GetLineNumberPrinting();
	m_config->Write("Miscellaneous/PrintLineNumbers", printLineNumbers ? "true" : "false");

	bool saveSession = m_options->GetSaveSession();
	m_config->Write("Miscellaneous/SaveSession", saveSession ? "true" : "false");

	bool formatBeforeSaving = m_options->GetFormatBeforeSaving();
	m_config->Write("Miscellaneous/FormatBeforeSaving", formatBeforeSaving ? "true" : "false");

	m_options->writeColoursToConfig(m_config);
	m_config->Write("Styles/EditorFont", m_options->GetEditorFont().GetNativeFontInfoUserDesc());

	m_config->Write("Miscellaneous/LateXRoot", m_options->GetLaTeXRoot());

	m_config->Write("Miscellaneous/KeepBackups", m_options->GetKeepBackupFile() ? "true" : "false");

    // Write the configuration to its file
	m_config->Flush();
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

	wxMenu* menuNewFile = new wxMenu();

	menuNewFile->Append(ID_MENU_NEW_EMPTY, _guilang.get("GUI_MENU_NEW_EMPTYFILE"), _guilang.get("GUI_MENU_NEW_EMPTYFILE_TTP"));
	menuNewFile->AppendSeparator();
	menuNewFile->Append(ID_MENU_NEW_SCRIPT, _guilang.get("GUI_MENU_NEW_NSCR"), _guilang.get("GUI_MENU_NEW_NSCR_TTP"));
	menuNewFile->Append(ID_MENU_NEW_PROCEDURE, _guilang.get("GUI_MENU_NEW_NPRC"), _guilang.get("GUI_MENU_NEW_NPRC_TTP"));
	menuNewFile->Append(ID_MENU_NEW_PLUGIN, _guilang.get("GUI_MENU_NEW_PLUGIN"), _guilang.get("GUI_MENU_NEW_PLUGIN_TTP"));

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

	// TODO Implement these Edit menu items

	/*
    menuEdit->AppendSeparator();
	menuEdit->Append(ID_SELECTALL, "Select all\tCtrl-A");
	menuEdit->Append(ID_SELECTLINE, "Select line");
	*/

	menuBar->Append(menuEdit, _guilang.get("GUI_MENU_EDIT"));

	wxMenu* menuSearch = new wxMenu();
	menuSearch->Append(ID_MENU_FIND, _guilang.get("GUI_MENU_FIND"));
	menuSearch->Append(ID_MENU_REPLACE, _guilang.get("GUI_MENU_REPLACE"));
    menuSearch->AppendSeparator();
	menuSearch->Append(ID_MENU_FIND_PROCEDURE, _guilang.get("GUI_MENU_FIND_PROCEDURE"), _guilang.get("GUI_MENU_FIND_PRCOCEDURE_TTP"));
	menuSearch->Append(ID_MENU_FIND_INCLUDE, _guilang.get("GUI_MENU_FIND_INCLUDE"), _guilang.get("GUI_MENU_FIND_INCLUDE_TTP"));
	menuSearch->Append(ID_MENU_GOTOLINE, _guilang.get("GUI_MENU_GOTOLINE"), _guilang.get("GUI_MENU_GOTOLINE_TTP"));
	menuSearch->AppendSeparator();
	menuSearch->Append(ID_MENU_BOOKMARK_TOGGLE, _guilang.get("GUI_MENU_BOOKMARK_TOGGLE"));
	menuSearch->Append(ID_MENU_BOOKMARK_CLEARMENU, _guilang.get("GUI_MENU_BOOKMARK_CLEAR"));
	menuSearch->Append(ID_MENU_BOOKMARK_PREVIOUS, _guilang.get("GUI_MENU_BOOKMARK_PREVIOUS"));
	menuSearch->Append(ID_MENU_BOOKMARK_NEXT, _guilang.get("GUI_MENU_BOOKMARK_NEXT"));

	menuBar->Append(menuSearch, _guilang.get("GUI_MENU_SEARCH"));


    wxMenu* menuView = new wxMenu();
	menuView->Append(ID_MENU_TOGGLE_CONSOLE, _guilang.get("GUI_MENU_TOGGLE_CONSOLE"));
	menuView->Append(ID_MENU_TOGGLE_FILETREE, _guilang.get("GUI_MENU_TOGGLE_FILETREE"));
	menuView->Append(ID_MENU_TOGGLE_HISTORY, _guilang.get("GUI_MENU_TOGGLE_HISTORY"));
	menuView->AppendSeparator();
	menuView->Append(ID_MENU_FOLD_ALL, _guilang.get("GUI_MENU_FOLDALL"), _guilang.get("GUI_MENU_FOLDALL_TTP"));
	menuView->Append(ID_MENU_UNFOLD_ALL, _guilang.get("GUI_MENU_UNFOLDALL"), _guilang.get("GUI_MENU_UNFOLDALL_TTP"));
	menuView->Append(ID_MENU_UNHIDE_ALL, _guilang.get("GUI_MENU_UNHIDEALL"), _guilang.get("GUI_MENU_UNHIDEALL_TTP"));
	menuView->AppendSeparator();
	menuView->Append(ID_MENU_LINEWRAP, _guilang.get("GUI_MENU_LINEWRAP"), _guilang.get("GUI_MENU_LINEWRAP_TTP"), true);
	menuView->Append(ID_MENU_DISPCTRLCHARS, _guilang.get("GUI_MENU_DISPCTRLCHARS"), _guilang.get("GUI_MENU_DISPCTRLCHARS_TTP"), true);
	menuView->Append(ID_MENU_USETXTADV, _guilang.get("GUI_MENU_USETXTADV"), _guilang.get("GUI_MENU_USETXTADV_TTP"), true);
	menuView->Append(ID_MENU_TOGGLE_NOTEBOOK_MULTIROW, _guilang.get("GUI_MENU_MULTIROW"), _guilang.get("GUI_MENU_MULTIROW_TTP"), true);
	menuView->Check(ID_MENU_TOGGLE_NOTEBOOK_MULTIROW, m_multiRowState);
	menuView->Append(ID_MENU_USESECTIONS, _guilang.get("GUI_MENU_USESECTIONS"), _guilang.get("GUI_MENU_USESECTIONS_TTP"), true);

	if (m_currentEd)
	{
        menuView->Check(ID_MENU_LINEWRAP, m_currentEd->getEditorSetting(NumeReEditor::SETTING_WRAPEOL));
        menuView->Check(ID_MENU_DISPCTRLCHARS, m_currentEd->getEditorSetting(NumeReEditor::SETTING_DISPCTRLCHARS));
        menuView->Check(ID_MENU_USETXTADV, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USETXTADV));
        menuView->Check(ID_MENU_USESECTIONS, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USESECTIONS));
    }
    else
    {
        menuView->Check(ID_MENU_LINEWRAP, false);
        menuView->Check(ID_MENU_DISPCTRLCHARS, false);
        menuView->Check(ID_MENU_USETXTADV, false);
        menuView->Check(ID_MENU_USESECTIONS, false);
    }

	menuBar->Append(menuView, _guilang.get("GUI_MENU_VIEW"));

	// stripspaces (only front, only back, all)
	wxMenu* menuStripSpaces = new wxMenu();
	menuStripSpaces->Append(ID_MENU_STRIP_SPACES_BOTH, _guilang.get("GUI_MENU_STRIP_BOTH"), _guilang.get("GUI_MENU_STRIP_BOTH_TTP"));
	menuStripSpaces->Append(ID_MENU_STRIP_SPACES_FRONT, _guilang.get("GUI_MENU_STRIP_FRONT"), _guilang.get("GUI_MENU_STRIP_FRONT_TTP"));
	menuStripSpaces->Append(ID_MENU_STRIP_SPACES_BACK, _guilang.get("GUI_MENU_STRIP_BACK"), _guilang.get("GUI_MENU_STRIP_BACK_TTP"));

	wxMenu* menuFormat = new wxMenu();
	menuFormat->Append(ID_MENU_AUTOINDENT, _guilang.get("GUI_MENU_AUTOINDENT"), _guilang.get("GUI_MENU_AUTOINDENT_TTP"));
	menuFormat->Append(ID_MENU_INDENTONTYPE, _guilang.get("GUI_MENU_INDENTONTYPE"), _guilang.get("GUI_MENU_INDENTONTYPE_TTP"), true);
	menuFormat->Append(ID_MENU_AUTOFORMAT, _guilang.get("GUI_MENU_AUTOFORMAT"), _guilang.get("GUI_MENU_AUTOFORMAT_TTP"));

	wxMenu* menuLaTeX = new wxMenu();
	menuLaTeX->Append(ID_MENU_CREATE_LATEX_FILE, _guilang.get("GUI_MENU_CREATELATEX"), _guilang.get("GUI_MENU_CREATELATEX_TTP"));
	menuLaTeX->Append(ID_MENU_RUN_LATEX, _guilang.get("GUI_MENU_RUNLATEX"), _guilang.get("GUI_MENU_RUNLATEX_TTP"));
	menuLaTeX->Append(ID_MENU_COMPILE_LATEX, _guilang.get("GUI_MENU_COMPILE_TEX"), _guilang.get("GUI_MENU_COMPILE_TEX_TTP"));

	wxMenu* menuRefactoring = new wxMenu();
	menuRefactoring->Append(ID_MENU_RENAME_SYMBOL, _guilang.get("GUI_MENU_RENAME_SYMBOL"), _guilang.get("GUI_MENU_RENAME_SYMBOL_TTP"));
	menuRefactoring->Append(ID_MENU_ABSTRAHIZE_SECTION, _guilang.get("GUI_MENU_ABSTRAHIZE_SECTION"), _guilang.get("GUI_MENU_ABSTRAHIZE_SECTION_TTP"));

	wxMenu* menuTools = new wxMenu();

	menuTools->Append(ID_MENU_OPTIONS, _guilang.get("GUI_MENU_OPTIONS"));
	menuTools->AppendSeparator();
	menuTools->Append(ID_MENU_EXECUTE, _guilang.get("GUI_MENU_EXECUTE"), _guilang.get("GUI_MENU_EXECUTE_TTP"));
	menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_FORMAT"), menuFormat);
    menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_REFACTORING"), menuRefactoring);
	menuTools->Append(ID_MENU_TOGGLE_COMMENT_LINE, _guilang.get("GUI_MENU_COMMENTLINE"), _guilang.get("GUI_MENU_COMMENTLINE_TTP"));
	menuTools->Append(ID_MENU_TOGGLE_COMMENT_SELECTION, _guilang.get("GUI_MENU_COMMENTSELECTION"), _guilang.get("GUI_MENU_COMMENTSELECTION_TTP"));
	menuTools->AppendSeparator();
	menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_STRIP"), menuStripSpaces);
	menuTools->Append(ID_MENU_SORT_SELECTION_ASC, _guilang.get("GUI_MENU_SORT_ASC"), _guilang.get("GUI_MENU_SORT_ASC_TTP"));
	menuTools->Append(ID_MENU_SORT_SELECTION_DESC, _guilang.get("GUI_MENU_SORT_DESC"), _guilang.get("GUI_MENU_SORT_DESC_TTP"));
	menuTools->AppendSeparator();
	menuTools->Append(wxID_ANY, _guilang.get("GUI_MENU_LATEX"), menuLaTeX);
	menuTools->Append(ID_MENU_USEANALYZER, _guilang.get("GUI_MENU_ANALYZER"), _guilang.get("GUI_MENU_ANALYZER_TTP"), true);
    if (m_currentEd)
	{
        menuTools->Check(ID_MENU_INDENTONTYPE, m_currentEd->getEditorSetting(NumeReEditor::SETTING_INDENTONTYPE));
        menuTools->Check(ID_MENU_USEANALYZER, m_currentEd->getEditorSetting(NumeReEditor::SETTING_USEANALYZER));
    }
    else
    {
        menuTools->Check(ID_MENU_INDENTONTYPE, false);
        menuTools->Check(ID_MENU_USEANALYZER, false);
    }
    menuTools->Append(ID_MENU_FIND_DUPLICATES, _guilang.get("GUI_MENU_FIND_DUPLICATES"), _guilang.get("GUI_MENU_FIND_DUPLICATES_TTP"));
	menuTools->Append(ID_MENU_TOGGLE_DEBUGGER, _guilang.get("GUI_MENU_DEBUGGER"), _guilang.get("GUI_MENU_DEBUGGER_TTP"), true);
	menuTools->Check(ID_MENU_TOGGLE_DEBUGGER, m_terminal->getKernelSettings().getUseDebugger());

	menuBar->Append(menuTools, _guilang.get("GUI_MENU_TOOLS"));

	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(ID_MENU_HELP, _guilang.get("GUI_MENU_SHOWHELP"));
	helpMenu->Append(ID_MENU_ABOUT, _guilang.get("GUI_MENU_ABOUT"), _guilang.get("GUI_MENU_ABOUT_TTP"));

	menuBar->Append(helpMenu, _guilang.get("GUI_MENU_HELP"));
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
	{
		style |= wxTB_TEXT;
	}

	if (GetToolBar() && GetToolBar()->GetWindowStyle() == style)
        return;

	wxToolBar* t = GetToolBar();
	delete t;
	SetToolBar(nullptr);
	m_config->Write("Interface/ShowToolbarText", showText ? "true" : "false");
	t = CreateToolBar(style);//new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);

	t->AddTool(ID_MENU_NEW_EMPTY, _guilang.get("GUI_TB_NEW"), wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), _guilang.get("GUI_TB_NEW_TTP"), wxITEM_DROPDOWN);
	wxMenu* menuNewFile = new wxMenu();
	menuNewFile->Append(ID_MENU_NEW_EMPTY, _guilang.get("GUI_MENU_NEW_EMPTYFILE"), _guilang.get("GUI_MENU_NEW_EMPTYFILE_TTP"));
	menuNewFile->AppendSeparator();
	menuNewFile->Append(ID_MENU_NEW_SCRIPT, _guilang.get("GUI_MENU_NEW_NSCR"), _guilang.get("GUI_MENU_NEW_NSCR_TTP"));
	menuNewFile->Append(ID_MENU_NEW_PROCEDURE, _guilang.get("GUI_MENU_NEW_NPRC"), _guilang.get("GUI_MENU_NEW_NPRC_TTP"));
	menuNewFile->Append(ID_MENU_NEW_PLUGIN, _guilang.get("GUI_MENU_NEW_PLUGIN"), _guilang.get("GUI_MENU_NEW_PLUGIN_TTP"));
	t->SetDropdownMenu(ID_MENU_NEW_EMPTY, menuNewFile);

    t->AddTool(ID_MENU_OPEN_SOURCE_LOCAL, _guilang.get("GUI_TB_OPEN"), wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), _guilang.get("GUI_TB_OPEN_TTP"));

	t->AddTool(ID_MENU_SAVE, _guilang.get("GUI_TB_SAVE"), wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), _guilang.get("GUI_TB_SAVE_TTP"));

    t->AddSeparator();

    t->AddTool(ID_MENU_UNDO, _guilang.get("GUI_TB_UNDO"), wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR), _guilang.get("GUI_TB_UNDO"));

    t->AddTool(ID_MENU_REDO, _guilang.get("GUI_TB_REDO"), wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR), _guilang.get("GUI_TB_REDO"));

    t->AddSeparator();
    t->AddTool(ID_MENU_CUT, _guilang.get("GUI_TB_CUT"), wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR), _guilang.get("GUI_TB_CUT"));
    t->AddTool(ID_MENU_COPY, _guilang.get("GUI_TB_COPY"), wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), _guilang.get("GUI_TB_COPY"));
    t->AddTool(ID_MENU_PASTE, _guilang.get("GUI_TB_PASTE"), wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR), _guilang.get("GUI_TB_PASTE"));

    t->AddSeparator();
    t->AddTool(ID_MENU_FIND, _guilang.get("GUI_TB_SEARCH"), wxArtProvider::GetBitmap(wxART_FIND, wxART_TOOLBAR), _guilang.get("GUI_TB_SEARCH"));
    t->AddTool(ID_MENU_REPLACE, _guilang.get("GUI_TB_REPLACE"), wxArtProvider::GetBitmap(wxART_FIND_AND_REPLACE, wxART_TOOLBAR), _guilang.get("GUI_TB_REPLACE"));

    t->AddSeparator();
    wxBitmap bmIndent(stepnext_xpm);
    t->AddTool(ID_MENU_INDENTONTYPE, _guilang.get("GUI_TB_INDENTONTYPE"), bmIndent, _guilang.get("GUI_TB_INDENTONTYPE_TTP"), wxITEM_CHECK);
    t->ToggleTool(ID_MENU_INDENTONTYPE, false);
    t->EnableTool(ID_MENU_INDENTONTYPE, false);
    wxBitmap bmWrapAround(wraparound_xpm);
    t->AddTool(ID_MENU_LINEWRAP, _guilang.get("GUI_TB_LINEWRAP"), bmWrapAround, _guilang.get("GUI_TB_LINEWRAP_TTP"), wxITEM_CHECK);

    t->AddSeparator();
    wxBitmap bmStart(newstart1_xpm);
    t->AddTool(ID_MENU_EXECUTE, _guilang.get("GUI_TB_RUN"), bmStart, _guilang.get("GUI_TB_RUN_TTP"));

    wxBitmap bmStop(newstop1_xpm);
    t->AddTool(ID_MENU_STOP_EXECUTION, _guilang.get("GUI_TB_STOP"), bmStop, _guilang.get("GUI_TB_STOP_TTP"));

    t->AddSeparator();

    wxBitmap bmStartDebugger(newcontinue1_xpm);
    t->AddTool(ID_MENU_TOGGLE_DEBUGGER, _guilang.get("GUI_TB_DEBUGGER"), bmStartDebugger, _guilang.get("GUI_TB_DEBUGGER_TTP"), wxITEM_CHECK);
    t->ToggleTool(ID_MENU_TOGGLE_DEBUGGER, m_terminal->getKernelSettings().getUseDebugger());

    wxBitmap bmAddBreakpoint(breakpoint_xpm);
    t->AddTool(ID_MENU_ADDEDITORBREAKPOINT, _guilang.get("GUI_TB_ADD"), bmAddBreakpoint,
        _guilang.get("GUI_TB_ADD_TTP"));

    wxBitmap bmRemoveBreakpoint(breakpoint_octagon_disable_xpm);
    t->AddTool(ID_MENU_REMOVEEDITORBREAKPOINT, _guilang.get("GUI_TB_REMOVE"), bmRemoveBreakpoint,
        _guilang.get("GUI_TB_REMOVE_TTP"));

    wxBitmap bmClearBreakpoint(breakpoint_crossed_xpm);
    t->AddTool(ID_MENU_CLEAREDITORBREAKPOINTS, _guilang.get("GUI_TB_CLEAR"), bmClearBreakpoint,
        _guilang.get("GUI_TB_CLEAR_TTP"));

    /*for(int i = ID_DEBUG_IDS_FIRST; i < ID_DEBUG_IDS_LAST; i++)
    {
        t->EnableTool(i, false);
    }*/

    t->AddSeparator();
    wxBitmap bmAnalyzer(gtk_apply_xpm);
    t->AddTool(ID_MENU_USEANALYZER, _guilang.get("GUI_TB_ANALYZER"), bmAnalyzer, _guilang.get("GUI_TB_ANALYZER_TTP"), wxITEM_CHECK);
    t->ToggleTool(ID_MENU_USEANALYZER, false);
    t->EnableTool(ID_MENU_USEANALYZER, false);

	t->Realize();

	ToolbarStatusUpdate();


	m_config->Write("Interface/ShowToolbarText", showText ? "true" : "false");
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
        m_splitEditorOutput->SplitHorizontally(m_book, m_splitCommandHistory, fSplitPercentage);//-260);//-200);
        m_splitEditorOutput->SetMinimumPaneSize(20);

        if (!m_splitCommandHistory->IsSplit())
        {
            m_splitCommandHistory->SplitVertically(m_termContainer, m_noteTerm, fVerticalSplitPercentage);
        }

        m_terminal->UpdateSize();
        m_termContainer->Show();
        m_noteTerm->Show();
    }

	m_termContainer->Refresh();
	m_book->Refresh();
	m_noteTerm->Refresh();
}

void NumeReWindow::UpdateVarViewer()
{
    if (m_varViewer)
    {
        NumeReVariables vars = m_terminal->getVariableList();
        m_varViewer->UpdateVariables(vars.vVariables, vars.nNumerics, vars.nStrings, vars.nTables);
    }
}

void NumeReWindow::UpdateWindowTitle(const wxString& filename)
{
    wxTopLevelWindow::SetTitle(filename + " - NumeRe: Framework für Numerische Rechnungen (v " + sVersion + ")");
}


void NumeReWindow::toggleConsole()
{
    if (m_termContainer->IsShown())
    {
        m_termContainer->Hide();
        m_noteTerm->Hide();
        fSplitPercentage = m_splitEditorOutput->GetSplitPercentage();
        m_splitEditorOutput->Unsplit(m_splitCommandHistory);
    }
    else
    {
        m_splitEditorOutput->SplitHorizontally(m_book, m_splitCommandHistory, fSplitPercentage);
        m_splitEditorOutput->SetMinimumPaneSize(20);
        m_terminal->UpdateSize();
        m_termContainer->Show();
        if (m_splitCommandHistory->IsSplit())
            m_noteTerm->Show();
    }
    m_book->Refresh();
}

void NumeReWindow::toggleFiletree()
{
    if (m_treeBook->IsShown())
    {
        m_treeBook->Hide();
        //fSplitPercentage = m_splitEditorOutput->GetSplitPercentage();
        m_splitProjectEditor->Unsplit(m_treeBook);
    }
    else
    {
        m_splitProjectEditor->SplitVertically(m_treeBook, m_splitEditorOutput, 200);
        m_splitProjectEditor->SetMinimumPaneSize(30);
        m_treeBook->Show();
    }
    m_terminal->UpdateSize();
    m_book->Refresh();
}

void NumeReWindow::toggleHistory()
{
    if (m_noteTerm->IsShown())
    {
        m_noteTerm->Hide();
        fVerticalSplitPercentage = m_splitCommandHistory->GetSplitPercentage();
        m_splitCommandHistory->Unsplit(m_noteTerm);
    }
    else
    {
        m_splitCommandHistory->SplitVertically(m_termContainer, m_noteTerm, fVerticalSplitPercentage);
        m_noteTerm->Show();
    }
    m_terminal->UpdateSize();
    m_termContainer->Refresh();
    m_noteTerm->Refresh();
}

void NumeReWindow::showConsole()
{
    if (!m_termContainer->IsShown())
    {
        m_splitEditorOutput->SplitHorizontally(m_book, m_splitCommandHistory, fSplitPercentage);
        m_splitEditorOutput->SetMinimumPaneSize(20);
        m_terminal->UpdateSize();
        m_termContainer->Show();
        if (m_splitCommandHistory->IsSplit())
            m_noteTerm->Show();
        m_book->Refresh();
    }
}

void NumeReWindow::gotoLine()
{
    wxTextEntryDialog* dialog = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_GOTO_QUESTION", toString(m_currentEd->GetLineCount())), _guilang.get("GUI_DLG_GOTO"));
    int ret = dialog->ShowModal();
    if (ret == wxID_CANCEL)
    {
        delete dialog;
        return;
    }
    int line = StrToInt(dialog->GetValue().ToStdString())-1;
    delete dialog;
    if (line < 0 || line >= m_currentEd->GetLineCount())
        wxMessageBox(_guilang.get("GUI_DLG_GOTO_ERROR"), _guilang.get("GUI_DLG_GOTO"), wxCENTRE | wxICON_ERROR);
    else
    {
        m_currentEd->GotoLine(line);
        m_currentEd->SetFocus();
    }
}

void NumeReWindow::setEditorFocus()
{
    m_currentEd->SetFocus();
}

void NumeReWindow::setViewerFocus()
{
    m_currentView->SetFocus();
}

void NumeReWindow::prepareFunctionTree()
{
    vector<string> vDirList;
    vector<string> vKeyList;
    string sKeyList = _guilang.get("GUI_TREE_CMD_KEYLIST");
    while (sKeyList.length())
    {
        vKeyList.push_back(sKeyList.substr(0, sKeyList.find(' ')));
        sKeyList.erase(0, sKeyList.find(' '));
        while (sKeyList.front() == ' ')
            sKeyList.erase(0,1);
    }
    int idxFolderOpen = m_iconManager->GetIconIndex("FOLDEROPEN");
    int idxFunctions = m_iconManager->GetIconIndex("FUNCTIONS");
    int idxCommands = m_iconManager->GetIconIndex("COMMANDS");
    int idxConstants = m_iconManager->GetIconIndex("CONSTANTS");
	wxTreeItemId rootNode = m_functionTree->AddRoot(_guilang.get("GUI_TREE_WORKSPACE"), m_iconManager->GetIconIndex("WORKPLACE"));
	FileNameTreeData* root = new FileNameTreeData();
	wxTreeItemId commandNode = m_functionTree->AppendItem(rootNode, _guilang.get("GUI_TREE_COMMANDS"), m_iconManager->GetIconIndex("WORKPLACE"), -1, root);
	root = new FileNameTreeData();
	wxTreeItemId functionNode = m_functionTree->AppendItem(rootNode, _guilang.get("GUI_TREE_FUNCTIONS"), m_iconManager->GetIconIndex("WORKPLACE"), -1, root);
	root = new FileNameTreeData();
	wxTreeItemId constNode = m_functionTree->AppendItem(rootNode, _guilang.get("GUI_TREE_CONSTANTS"), m_iconManager->GetIconIndex("WORKPLACE"), -1, root);
	wxTreeItemId currentNode;

	/// commands
	for (size_t i = 0; i < vKeyList.size(); i++)
	{
        FileNameTreeData* dir = new FileNameTreeData();
        dir->isDir = true;
        currentNode = m_functionTree->AppendItem(commandNode, _guilang.get("PARSERFUNCS_LISTCMD_TYPE_" + toUpperCase(vKeyList[i])), idxFolderOpen, -1, dir);
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

	/// functions
    sKeyList = _guilang.get("GUI_TREE_FUNC_KEYLIST");
    vKeyList.clear();
    while (sKeyList.length())
    {
        vKeyList.push_back(sKeyList.substr(0, sKeyList.find(' ')));
        sKeyList.erase(0, sKeyList.find(' '));
        while (sKeyList.front() == ' ')
            sKeyList.erase(0,1);
    }

	for (size_t i = 0; i < vKeyList.size(); i++)
	{
        FileNameTreeData* dir = new FileNameTreeData();
        dir->isDir = true;
        currentNode = m_functionTree->AppendItem(functionNode, _guilang.get("PARSERFUNCS_LISTFUNC_TYPE_" + toUpperCase(vKeyList[i])), idxFolderOpen, -1, dir);
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

    /// Constants
    sKeyList = _guilang.get("GUI_TREE_CONST_KEYLIST");
    vKeyList.clear();
    while (sKeyList.length())
    {
        vKeyList.push_back(sKeyList.substr(0, sKeyList.find(' ')));
        sKeyList.erase(0, sKeyList.find(' '));
        while (sKeyList.front() == ' ')
            sKeyList.erase(0,1);
    }

	for (size_t i = 0; i < vKeyList.size(); i++)
	{
        FileNameTreeData* dir = new FileNameTreeData();
        dir->isDir = true;
        currentNode = m_functionTree->AppendItem(constNode, _guilang.get("PARSERFUNCS_LISTCONST_TYPE_" + toUpperCase(vKeyList[i])), idxFolderOpen, -1, dir);
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

string NumeReWindow::prepareTooltip(const string& sTooltiptext)
{
    size_t nClosingParens = sTooltiptext.find(')');
    string sTooltip = sTooltiptext;

    if (sTooltiptext.find(' ') < nClosingParens && sTooltiptext.find(' ') < sTooltiptext.find('('))
    {
        nClosingParens = sTooltiptext.find(' ')-1;
        sTooltip.replace(nClosingParens+1, sTooltip.find_first_not_of(' ', nClosingParens+1)-nClosingParens-1, "   ");
    }
    else
        sTooltip.replace(nClosingParens+1, sTooltip.find_first_not_of(' ', nClosingParens+1)-nClosingParens-1, "  ->  ");
    return sTooltip;
}

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

// Project code begins here

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
    if (m_treeBook->GetCurrentPage() == m_fileTree)
    {
        wxTreeItemId clickedItem = event.GetItem();
        m_clickedTreeItem = clickedItem;
        wxMenu popupMenu;
        wxString editableExt = ".dat;.txt;.nscr;.nprc;.dx;.jcm;.jdx;.csv;.log;.tex;";
        wxString loadableExt = ".dat;.txt;.dx;.jcm;.jdx;.xls;.xlsx;.ods;.ndat;.labx;.ibw;.csv;";
        wxString showableImgExt = ".png;.jpeg;.jpg;.gif;.bmp;";

        //wxTreeItemId rootItem = m_projectTree->GetRootItem();
        //wxTreeItemId parentItem = m_projectTree->GetItemParent(clickedItem);
        wxString fname_ext = m_fileTree->GetItemText(m_clickedTreeItem);
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
        if (m_clickedTreeItem == m_fileTree->GetRootItem())
            return;
        if (!data || data->isDir)
        {
            popupMenu.Append(ID_MENU_NEW_FOLDER_IN_TREE, _guilang.get("GUI_TREE_POP_NEWFOLDER"));
            if (data)
                popupMenu.Append(ID_MENU_REMOVE_FOLDER_FROM_TREE, _guilang.get("GUI_TREE_POP_REMOVEFOLDER"));
            popupMenu.AppendSeparator();
            popupMenu.Append(ID_MENU_OPEN_IN_EXPLORER, _guilang.get("GUI_TREE_POP_OPENINEXPLORER"));
            /*if (loadableExt.find(fname_ext) != string::npos)
                popupMenu.Append(ID_OPEN_FILE, _guilang.get("GUI_TREE_PUP_LOAD"));
            else if (fname_ext == ".nscr;")
                popupMenu.Append(ID_OPEN_FILE, _guilang.get("GUI_TREE_PUP_START"));
            else if (fname_ext == ".nprc;")
                popupMenu.Append(ID_OPEN_FILE, _guilang.get("GUI_TREE_PUP_RUN"));

            if (editableExt.find(fname_ext) != string::npos)
                popupMenu.Append(ID_EDIT_FILE, _guilang.get("GUI_TREE_PUP_EDIT"));
            if (showableImgExt.find(fname_ext) != string::npos)
                popupMenu.Append(ID_OPEN_IMAGE, _guilang.get("GUI_TREE_PUP_OPENIMAGE"));
            popupMenu.AppendSeparator();
            popupMenu.Append(ID_DELETE_FILE, _guilang.get("GUI_TREE_PUP_DELETEFILE"));
            popupMenu.Append(ID_COPY_FILE, _guilang.get("GUI_TREE_PUP_COPYFILE"));
            if (m_copiedTreeItem)
                popupMenu.Append(ID_INSERT_FILE, _guilang.get("GUI_TREE_PUP_INSERTFILE"));
            popupMenu.Append(ID_RENAME_FILE, _guilang.get("GUI_TREE_PUP_RENAMEFILE"));*/

            wxPoint p = event.GetPoint();
            m_fileTree->PopupMenu(&popupMenu, p);
            return;
        }
        fname_ext = fname_ext.substr(fname_ext.rfind('.')) + ";";

        if (loadableExt.find(fname_ext) != string::npos)
            popupMenu.Append(ID_MENU_OPEN_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_LOAD"));
        else if (fname_ext == ".nscr;")
            popupMenu.Append(ID_MENU_OPEN_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_START"));
        else if (fname_ext == ".nprc;")
            popupMenu.Append(ID_MENU_OPEN_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_RUN"));

        if (editableExt.find(fname_ext) != string::npos)
            popupMenu.Append(ID_MENU_EDIT_FILE_FROM_TREE, _guilang.get("GUI_TREE_PUP_EDIT"));
        if (showableImgExt.find(fname_ext) != string::npos)
            popupMenu.Append(ID_MENU_OPEN_IMAGE_FROM_TREE, _guilang.get("GUI_TREE_PUP_OPENIMAGE"));
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
        //popupMenu.Append(ID_SHOW_DESCRIPTION, _guilang.get("GUI_TREE_PUP_SHOW_DESCRIPTION"));

        wxPoint p = event.GetPoint();
        m_functionTree->PopupMenu(&popupMenu, p);
    }
}


//////////////////////////////////////////////////////////////////////////////
///  private OpenProjectFile
///  Opens a project file
///
///  @param  isRemote bool  Whether the user wants to open a remote or local file
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OpenProjectFile(bool isRemote)
{
	m_remoteMode = isRemote;
	wxPathFormat currentPathFormat = m_remoteMode ? wxPATH_UNIX : wxPATH_DOS;

	if(m_projMultiFiles != NULL)
	{
		wxString message = "A project is already open.  Do you want to replace the currently open project";
		message += " with a different one?";
		int result = wxMessageBox(message, "Project already open", wxOK | wxCANCEL | wxICON_QUESTION);

		if(result == wxCANCEL)
		{
			return;
		}
	}

	wxArrayString fileNames = OpenFile(FILE_NUMERE);

	if(fileNames.Count() < 1)
	{
#ifdef DO_LOG
		wxLogDebug("No files selected when adding to project");
#endif
		return;
	}

	m_fileTree->Freeze();
	if(m_projMultiFiles != NULL)
	{
		CloseProjectFile();
	}

	m_projMultiFiles = new ProjectInfo(false);

	m_fileTree->DeleteChildren(m_projectFileFolders[0]);
	m_fileTree->DeleteChildren(m_projectFileFolders[1]);
	m_fileTree->DeleteChildren(m_projectFileFolders[2]);
	m_fileTree->DeleteChildren(m_projectFileFolders[3]);
	m_fileTree->DeleteChildren(m_projectFileFolders[4]);

	wxString fileContents;
	wxString fileName;

	GetFileContents(fileNames[0], fileContents, fileName);


	// set the current project's base path based on the project file's directory,
	// choosing the path separators based on where the file is stored

	wxFileName projectFileName(fileNames[0]);

	m_projMultiFiles->SetProjectFile(projectFileName);
	m_projMultiFiles->SetRemote(m_remoteMode);

	wxTreeItemId rootItem = m_fileTree->GetRootItem();
	m_fileTree->SetItemText(rootItem, fileName);
	m_fileTree->SetItemBold(rootItem);

	wxMemoryInputStream projectFileStream(fileContents, fileContents.Len());

	wxFileConfig config(projectFileStream);

	LoadFilesIntoProjectTree("/Sources", FILE_NSCR, m_projectFileFolders[0], config, currentPathFormat);
	LoadFilesIntoProjectTree("/Headers", FILE_NPRC, m_projectFileFolders[1], config, currentPathFormat);
	LoadFilesIntoProjectTree("/Libraries", FILE_DATAFILES, m_projectFileFolders[2], config, currentPathFormat);
	LoadFilesIntoProjectTree("/Other", FILE_NONSOURCE, m_projectFileFolders[3], config, currentPathFormat);

	m_fileTree->ExpandAll();

	m_fileTree->Thaw();
}

//////////////////////////////////////////////////////////////////////////////
///  private AddFileToProject
///  Adds a file to the current multi-file project
///
///  @return void
///
///  @remarks This function currently requires the user to have right-clicked in
///  @remarks the project tree.  It might be useful to expand this to, say,
///  @remarks two overloads and a grunt work function.  One overload would be called
///  @remarks from a menubar item, one would be called from a right-click, and both
///  @remarks would call the actual add function with the right settings
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::AddFileToProject()
{
	m_remoteMode = m_projMultiFiles->IsRemote();
	wxArrayString fileNames = OpenFile(m_projectSelectedFolderType);

	if(fileNames.Count() < 1)
	{
#ifdef DO_LOG
		wxLogDebug("No files selected when adding to project");
#endif
		return;
	}

	wxString fileToOpen = fileNames[0];

	if(m_projMultiFiles->FileExistsInProject(fileToOpen))
	{
		wxString message = "The file " + fileToOpen + " already exists in this project, so it was not added.";
		wxMessageBox(message, "File already in project", wxOK | wxCENTRE | wxICON_EXCLAMATION);
		return;
	}

	wxString fileContents;
	wxString fileName;

	m_remoteMode = m_projMultiFiles->IsRemote();

	m_projMultiFiles->AddFileToProject(fileToOpen, m_projectSelectedFolderType);

	int iconIndex = m_extensionMappings[wxFileName(fileToOpen).GetExt()];

	FileNameTreeData* data = new FileNameTreeData();
	data->filename = fileToOpen;

	wxString simpleName = wxFileName(fileToOpen).GetFullName();

	m_fileTree->AppendItem(m_clickedTreeItem, simpleName, iconIndex, -1, data);
	m_fileTree->SortChildren(m_clickedTreeItem);
	m_fileTree->Refresh();

	wxFileName fn(fileToOpen);
	int pageNum = GetPageNum(fn);

	if(pageNum != -1)
	{
		NumeReEditor* edit = static_cast<NumeReEditor*> (m_book->GetPage(pageNum));
		edit->SetProject(m_projMultiFiles);
	}

	SaveProjectFile();
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
    if (m_treeBook->GetCurrentPage() == m_fileTree)
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
            else if (data->filename.find('.') == string::npos || data->isDir)
                return;

            OpenFileByType(pathname);
        }
        m_currentEd->Refresh();
        m_book->Refresh();
    }
    else
    {
        wxTreeItemId item = event.GetItem();
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(item));
        if (data->isDir)
        {
            m_functionTree->Toggle(item);
        }
        else if (data->isCommand)
        {
            m_currentEd->InsertText(m_currentEd->GetCurrentPos(), (data->tooltip).substr(0, (data->tooltip).find(' ')+1));
            m_currentEd->GotoPos(m_currentEd->GetCurrentPos()+(data->tooltip).find(' ')+1);
        }
        else if (data->isFunction)
        {
            m_currentEd->InsertText(m_currentEd->GetCurrentPos(), (data->tooltip).substr(0, (data->tooltip).find('(')+1));
            m_currentEd->GotoPos(m_currentEd->GetCurrentPos()+(data->tooltip).find('(')+1);
        }
        else if (data->isConstant)
        {
            m_currentEd->InsertText(m_currentEd->GetCurrentPos(), (data->tooltip).substr(0, (data->tooltip).find(' ')));
            m_currentEd->GotoPos(m_currentEd->GetCurrentPos()+(data->tooltip).find(' '));
        }
    }
}

void NumeReWindow::OnTreeItemToolTip(wxTreeEvent& event)
{
    if (m_treeBook->GetCurrentPage() == m_functionTree)
    {
        wxTreeItemId item = event.GetItem();

        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(item));
        if (data->isDir)
            return;
        else if (data->isFunction || data->isCommand)
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
        wxFileName pathname = data->filename;
        wxString tooltip = _guilang.get("COMMON_FILETYPE_" + toUpperCase(pathname.GetExt().ToStdString()));
        if (pathname.GetExt() == "ndat")
        {
            tooltip += getFileDetails(pathname);
        }
        event.SetToolTip(tooltip);
    }
}

void NumeReWindow::OnTreeDragDrop(wxTreeEvent& event)
{
    if (m_treeBook->GetCurrentPage() == m_functionTree)
    {
        wxTreeItemId item = event.GetItem();
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_functionTree->GetItemData(item));
        if (!data->isCommand && !data->isFunction && !data->isConstant)
            return;
        wxString token = data->tooltip;
        token.erase(token.find(' '));
        if (token.find('(') != string::npos)
            token.erase(token.find('(')+1);
        else
            token += " ";
        wxTextDataObject _dataObject(token);
        wxDropSource dragSource(this);
        dragSource.SetData(_dataObject);
        dragSource.DoDragDrop(wxDrag_AllowMove);
    }
    else
    {
        //event.Allow();
        wxTreeItemId item = event.GetItem();
        m_dragDropSourceItem = item;
        FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(item));
        if (!data || data->isDir)
            return;
        wxFileName pathname = data->filename;
        wxString dragableExtensions = ";nscr;nprc;ndat;txt;dat;log;tex;csv;xls;xlsx;ods;jdx;jcm;dx;labx;ibw;png;jpg;jpeg;gif;bmp;eps;svg;m;cpp;cxx;c;hpp;hxx;h;";
        if (dragableExtensions.find(";" + pathname.GetExt() + ";") != string::npos)
        {
            wxFileDataObject _dataObject;
            _dataObject.AddFile(pathname.GetFullPath());
            wxDropSource dragSource(this);
            dragSource.SetData(_dataObject);
            dragSource.DoDragDrop(wxDrag_AllowMove);
        }

    }
}

wxString NumeReWindow::addLinebreaks(const wxString& sLine)
{
    const unsigned int nMAXLINE = 70;
    /*if (sLine.length() < nMAXLINE)
        return sLine;*/

    wxString sReturn = sLine;
    while (sReturn.find("\\$") != string::npos)
        sReturn.erase(sReturn.find("\\$"),1);
    unsigned int nDescStart = sReturn.find("- ");
    unsigned int nIndentPos = 6;//
    unsigned int nLastLineBreak = 0;
    sReturn.replace(nDescStart, 2,"\n      ");
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

wxString NumeReWindow::getFileDetails(const wxFileName& filename)
{
    if (m_terminal->getKernelSettings().getbShowExtendedFileInfo())
        return "\n" + getFileInfo(filename.GetFullPath().ToStdString());
    else
        return "NOTHING";
}

//////////////////////////////////////////////////////////////////////////////
///  private CloseProjectFile
///  Closes the currently open multi-file project
///
///  @param  canUserCancel bool  [=true] Whether or not to let the user cancel the project close
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::CloseProjectFile(bool canUserCancel)
{
	if(!m_appClosing)
	{
		EditorPointerArray edList = m_projMultiFiles->GetEditors();

		if(edList.GetCount() > 0)
		{
			int allowedResponse = wxYES_NO;

			if(canUserCancel)
			{
				allowedResponse |= wxCANCEL;
			}

			int response = wxMessageBox("Do you want to close all files from this project?",
				"Close Project", allowedResponse | wxCENTER | wxICON_QUESTION);

			if(response == wxYES)
			{
				for(int i = 0; i < (int)edList.GetCount(); i++)
				{
					NumeReEditor* ed = edList[i];
					int tabnum = m_book->FindPagePosition(ed);
					CloseFile(tabnum);
				}
			}
			else if(response == wxNO)
			{
				for(int i = 0; i < (int)edList.GetCount(); i++)
				{
					NumeReEditor* ed = edList[i];
					ProjectInfo* proj = new ProjectInfo();
					proj->SetRemote(ed->LastSavedRemotely());
					ed->SetProject(proj);
				}
			}
			else
			{
				return;
			}
		}
	}

	SaveProjectFile();

	// Clear out all items in the project tree
	for(int i = 0; i < 4; i++)
	{
		m_fileTree->DeleteChildren(m_projectFileFolders[i]);
	}

	m_fileTree->SetItemText(m_fileTree->GetRootItem(), "No project");

	delete m_projMultiFiles;
	m_projMultiFiles = NULL;
}

//////////////////////////////////////////////////////////////////////////////
///  private SaveProjectFile
///  Saves the currently open multi-file project
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::SaveProjectFile()
{
	wxPathFormat currentPathFormat = (m_projMultiFiles->IsRemote() ? wxPATH_UNIX : wxPATH_DOS);
	wxMemoryInputStream mis("", 0);
	wxFileConfig config(mis);

	config.SetPath("/Headers");
	wxArrayString headers = m_projMultiFiles->GetHeaders();
	for(size_t i = 0; i < headers.Count(); i++)
	{
		wxString headerName;
		headerName << (i + 1);
		wxFileName relativeName(headers[i]);
		config.Write(headerName, relativeName.GetFullPath(currentPathFormat));
	}

	config.SetPath("/Sources");
	wxArrayString sources = m_projMultiFiles->GetSources();
	for(size_t i = 0; i < sources.Count(); i++)
	{
		wxString sourceName;
		sourceName << (i + 1);
		wxFileName relativeName(sources[i]);
		config.Write(sourceName, relativeName.GetFullPath(currentPathFormat));
	}

	config.SetPath("/Libraries");
	wxArrayString libraries = m_projMultiFiles->GetLibraries();
	for(size_t i = 0; i < libraries.Count(); i++)
	{
		wxString libraryName;
		libraryName << (i + 1);
		wxFileName relativeName(libraries[i]);
		config.Write(libraryName, relativeName.GetFullPath(currentPathFormat));
	}

	config.SetPath("/Other");
	wxArrayString other = m_projMultiFiles->GetNonSources();
	for(size_t i = 0; i < other.Count(); i++)
	{
		wxString otherName;
		otherName << (i + 1);
		wxFileName relativeName(other[i]);
		config.Write(otherName, relativeName.GetFullPath(currentPathFormat));
	}

	wxMemoryOutputStream outputStream;
	//config.FlushToStream(outputStream);
	config.Save(outputStream);

	wxString resultContents;
	size_t streamsize = outputStream.GetSize();

	if(streamsize == 0)
	{
		resultContents = "[Headers]\n\n[Sources]\n\n[Libraries]\n\n[Other]";
	}
	else
	{
		//char* bufptr = resultContents.GetWriteBuf(streamsize);
		outputStream.CopyTo((void*)resultContents.fn_str(),streamsize);
		//resultContents.UngetWriteBuf();
	}

	wxString projBasePath = m_projMultiFiles->GetProjectBasePath();
	wxString projName = m_projMultiFiles->GetProjectFile().GetFullName();
	if(m_projMultiFiles->IsRemote())
	{
		wxFileName fn(projBasePath, projName);
		m_network->SendFileContents(resultContents, fn);
	}
	else
	{
		wxString filename = projBasePath + "\\" + projName;
		wxFile projectFile(filename, wxFile::write);
		projectFile.Write(resultContents);
		projectFile.Close();
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private LoadFilesIntoProjectTree
///  Loads the given files into the project tree
///
///  @param  configPath          wxString       The internal path the wxConfig should look in for file entries
///  @param  fileType			 FileFilterType The type of files being loaded
///  @param  treeid              wxTreeItemId   The tree ID of the folder to add the files to
///  @param  config              wxFileConfig & The wxFileConfig containing the project file
///  @param  currentPathFormat   wxPathFormat   The path format to use for the filenames
///
///  @return void
///
///  @author Mark Erikson @date 03-29-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::LoadFilesIntoProjectTree(wxString configPath,  FileFilterType fileType, wxTreeItemId treeid,
										   wxFileConfig& config, wxPathFormat currentPathFormat)
{
	config.SetPath(configPath);

	int numEntries = config.GetNumberOfEntries();

	for(int i = 1; i <= numEntries; i++)
	{
		wxString keyname;
		keyname << i;
		wxString fileName = config.Read(keyname);

		if(fileName != wxEmptyString)
		{
			wxFileName newFileName(fileName);
			//int iconNum = m_extensionMappings[newFileName.GetExt()];
			int iconNum = m_iconManager->GetIconIndex(newFileName.GetExt());

			FileNameTreeData* data = new FileNameTreeData();
			data->filename = newFileName.GetFullPath(currentPathFormat);

			m_fileTree->AppendItem(treeid, newFileName.GetFullName(), iconNum, -1, data);

			m_projMultiFiles->AddFileToProject(data->filename, fileType);

			int pageNum = GetPageNum(newFileName);

			if(pageNum != -1)
			{
				NumeReEditor* edit = static_cast<NumeReEditor*> (m_book->GetPage(pageNum));
				edit->SetProject(m_projMultiFiles);
			}
		}
	}

	m_fileTree->SortChildren(treeid);
}

/*enum FileFilterType
{
	FILE_ALLSOURCETYPES,
	FILE_NUMERE,
	FILE_DATAFILES,
	FILE_NSCR,
	FILE_NPRC,
	FILE_ALLFILES,
	FILE_NONSOURCE,
	FILE_LASTTYPE,
};*/


void NumeReWindow::LoadFilesToTree(wxString fromPath, FileFilterType fileType, wxTreeItemId treeid)
{
    wxDir currentDir(fromPath);
    DirTraverser _traverser(m_fileTree, m_iconManager, treeid, fromPath, fileType);
    currentDir.Traverse(_traverser);
}
//////////////////////////////////////////////////////////////////////////////
///  private RemoveFileFromProject
///  Removes a file from the current multi-file project
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::RemoveFileFromProject()
{
	FileNameTreeData* treeData = static_cast<FileNameTreeData*> (m_fileTree->GetItemData(m_clickedTreeItem));

	if(m_projMultiFiles->FileExistsInProject(treeData->filename))
	{
		m_projMultiFiles->RemoveFileFromProject(treeData->filename, m_projectSelectedFolderType);

		wxFileName fn(treeData->filename);
		int pageNum = GetPageNum(fn);

		if(pageNum != -1)
		{
			NumeReEditor* pEdit = static_cast <NumeReEditor* >(m_book->GetPage(pageNum));
			ProjectInfo* proj = new ProjectInfo();
			pEdit->SetProject(proj);
		}

		m_fileTree->Delete(m_clickedTreeItem);

		SaveProjectFile();
	}
}

// Other major functions

//////////////////////////////////////////////////////////////////////////////
///  public CheckNetworkStatus
///  Shows an appropriate error message if a network operation failed.
///
///  @return NetworkCallResult Whether to redo the operation or not
///
///  @remarks The NetworkCallResult enum is outdated and unneeded, so this
///  @remarks should probably be revised at some point to get rid of it.
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
NetworkCallResult NumeReWindow::CheckNetworkStatus()
{
	NetworkStatus result = m_network->GetStatus();

	switch((int)result)
	{
	case NET_UNKNOWN_HOST:
		{
			wxString hostname = m_options->GetHostname();
			wxString fingerprint = m_network->GetStatusDetails();

			wxString message = "The SSH fingerprint for the server " + hostname + " was not recognized.";
			message += "\nThe fingerprint was " + fingerprint + ".  \nDo you want to cache it?";
			int result = wxMessageBox(message, "Unknown SSH Fingerprint", wxYES_NO | wxICON_QUESTION);

			if(result == wxYES)
			{
				m_network->SSHCacheFingerprint();
				return NETCALL_REDO;
			}

			return NETCALL_FAILED;
			break;
		}
	case NET_ERROR_MESSAGE:
		{
			wxString message = "An unknown network error has occurred.";
			wxString statusDetails = m_network->GetStatusDetails();;
			message += "\nError details: " + statusDetails;
#ifdef DO_LOG
			wxLogDebug("NET_ERROR_MESSAGE: %s", statusDetails);
#endif
			wxMessageBox(message, "Unknown network error", wxOK | wxICON_EXCLAMATION);
			return NETCALL_FAILED;
			break;
		}
	case NET_AUTH_FAILED:
		{
			// If everything's still default, go ahead and ask the user for the password.
			if(m_options->GetPassphrase() == "" &&
				m_options->GetHostname() != "127.0.0.1" &&
				m_options->GetUsername() != "username")
			{
				if(AskUserForPassword())
				{
					return NETCALL_REDO;
				}

			}
			else
			{
				wxString message = "Chameleon was unable to log you in using the current username and password.";
				message += "\nPlease check them in the Options menu and try again.";
				wxMessageBox(message, "Login failed", wxOK | wxICON_EXCLAMATION);
				return NETCALL_FAILED;
			}

		}
	case NET_CONN_REFUSED:
		{
			wxString message = "Connection to the remote server was refused.";
			message += "\nPlease check the remote hostname in the Options menu and try again.";
			wxMessageBox(message, "Connection Refused", wxOK | wxICON_EXCLAMATION);
			return NETCALL_FAILED;
			break;
		}
	default:
		return NETCALL_WORKED;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private Compile
///  Compiles the active editor's project
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::Compile()
{
	//bool doCompile = true;

	if(!m_compileProject)
	{
		/*
		if(!m_currentEd->HasBeenSaved())
		{
			wxMessageBox("Please save this file before trying to compile it.",
				"Unsaved File", wxOK | wxICON_INFORMATION);

			doCompile = false;
		}
		else if(m_currentEd->Modified())
		{
			// yes, same message... this can later be changed to ask about
			// saving, then do the appropriate thing.
			wxMessageBox("Please save this file before trying to compile it.", "Unsaved File",
				wxOK | wxICON_INFORMATION);

			doCompile = false;
		}
		*/

		if(!m_currentEd->HasBeenSaved() || m_currentEd->Modified())
		{
			int tabNum = m_book->FindPagePosition(m_currentEd);
			int result = HandleModifiedFile(tabNum, MODIFIEDFILE_COMPILE);

			if(result == wxCANCEL || result == wxNO)
			{
				return;
			}
		}
	}

	/*if(doCompile)
	{
		ProjectInfo* projToCompile;
		if(m_compileProject)
		{
			projToCompile = m_projMultiFiles;
		}
		else
		{
			projToCompile = m_currentEd->GetProject();
		}


		if(!projToCompile->IsRemote())
		{
			wxMessageBox("Local compiling has not been implemented.\nPlease save this file to a network server\nand try again",
						"Can't Compile Locally", wxOK | wxICON_WARNING);
			return;
		}



		//m_outputPanel->ClearOutput();

		//m_compiler->CompileProject(projToCompile, m_outputPanel);

		int outputIndex = m_noteTerm->FindPagePosition(m_outputPanel);
		m_noteTerm->SetSelection(outputIndex);
		m_outputPanel->SetFocus();
	}*/
}

//////////////////////////////////////////////////////////////////////////////
///  public FocusOnLine
///  Moves the cursor to a given file and line, with an optional line marker
///
///  @param  filename     wxString  The filename to look for
///  @param  linenumber   int       The line number to go to
///  @param  showMarker   bool      [=true] Whether or not to show a marker to highlight the line
///  @param  linecontents wxString  [=wxEmptyString] The text that the given line should have in it
///
///  @return void
///
///  @remarks The line contents idea isn't implemented, but could be used to ensure
///  @remarks that the right file was selected if two open files had the same name
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::FocusOnLine(wxString filename, int linenumber, bool showMarker, wxString linecontents /* = wxEmptyString */)
{
	wxFileName fn(filename);

	int tabNum = GetPageNum(fn, false);

	if(tabNum == -1)
	{
		if(m_projMultiFiles != NULL)
		{
			if(m_projMultiFiles->FileExistsInProject(filename))
			{
				wxArrayString filesToOpen;
				filesToOpen.Add(filename);
				OpenSourceFile(filesToOpen);
				tabNum = GetPageNum(fn, false);
			}
		}
	}

	if(tabNum != -1)
	{
		PageHasChanged(tabNum);

		// Adjust for Scintilla's zero-based line numbers
		int adjustedLineNum = linenumber - 1;

		if(linecontents != wxEmptyString)
		{
			wxString edLineContents = m_currentEd->GetLine(adjustedLineNum);
		}

		m_currentEd->FocusOnLine(adjustedLineNum, showMarker);
	}
}

// Find/replace code begins here

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
	int pos = m_currentEd->GetCurrentPos();

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
		if ((flags & wxFR_MATCHCASE && m_currentEd->GetSelectedText() != findString)
            || (!(flags & wxFR_MATCHCASE) && toLowerCase(m_currentEd->GetSelectedText().ToStdString()) != toLowerCase(findString.ToStdString())))
		{
			wxBell();
			return;
		}

		int pos = m_currentEd->GetSelectionStart();
		wxString replaceString = event.GetReplaceString();
		m_currentEd->ReplaceSelection(replaceString);
		m_currentEd->EnsureCaretVisible();
		m_currentEd->SetSelection(pos, pos + replaceString.Length());

		// TODO Do a Find after this for the next item automatically?
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

	int pos = start_pos == -1 ? m_currentEd->GetCurrentPos() : start_pos;

	if ((flags & wxFR_DOWN) != 0)
	{
		m_currentEd->SetTargetStart(wxMax(0, pos));
		m_currentEd->SetTargetEnd(wxMax(0, m_currentEd->GetTextLength()));
	}
	else
	{
		if (labs(m_currentEd->GetTargetEnd() - m_currentEd->GetTargetStart()) == long(findString.Length()))
		{
			pos -= findString.Length() + 1; // doesn't matter if it matches or not, skip it
		}

		m_currentEd->SetTargetStart(wxMax(0, pos));
		m_currentEd->SetTargetEnd(0);
	}

	m_currentEd->SetSearchFlags(stc_flags);
	pos = m_currentEd->SearchInTarget(findString);

	if (pos >= 0)
	{
		if (highlight)
		{
			m_currentEd->SetSelection(pos, pos + findString.Length());
			m_currentEd->EnsureCaretVisible();
		}
	}
	else if (flags & wxFR_WRAPAROUND)
	{
        if ((flags & wxFR_DOWN) && start_pos)
            return FindString(findString, 0, flags, highlight);
        else if (!(flags & wxFR_DOWN) && start_pos != m_currentEd->GetLastPosition())
            return FindString(findString, m_currentEd->GetLastPosition(), flags, highlight);
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

	if (findString.IsEmpty() || (findString == replaceString))
		return count;

	int cursor_pos = m_currentEd->GetCurrentPos();  // return here when done
	if (flags & wxFR_WRAPAROUND)
	{
        m_currentEd->GotoPos(0);
        flags &= flags & ~wxFR_WRAPAROUND;
    }
    m_currentEd->BeginUndoAction();
	if (m_currentEd->GetSelectedText() == findString)
	{
		++count;
		m_currentEd->ReplaceSelection(replaceString);
	}

	int pos = FindString(findString, -1, flags, true);

	while (pos != -1)
	{
		++count;
		m_currentEd->ReplaceSelection(replaceString);
		pos = FindString(findString, -1, flags, true);
	}

	// return to starting pos or as close as possible
	m_currentEd->GotoPos(wxMin(cursor_pos, m_currentEd->GetLength()));
	m_currentEd->EndUndoAction();

	return count;
}

// Various utility functions and random other stuff

//////////////////////////////////////////////////////////////////////////////
///  private DeterminePrintSize
///  Returns an appropriate size for printing
///
///  @return wxRect The dimensions of the print area
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxRect NumeReWindow::DeterminePrintSize ()
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

//////////////////////////////////////////////////////////////////////////////
///  private OnSize
///  Not actually used at the moment - could be used to cleanly resize the terminal notebook
///
///  @param  event wxSizeEvent & The generated size event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnSize(wxSizeEvent &event)
{
	event.Skip();
}

void NumeReWindow::OnTermResize(wxSplitterEvent &event)
{
}

bool NumeReWindow::CheckForBlankPassword()
{
	wxString password = m_options->GetPassphrase();
	if(password.IsEmpty())
	{
		bool passwordEntered = AskUserForPassword();
		if(!passwordEntered)
		{
			return false;
		}

		m_network->GetStatus();
	}

	return true;
}

bool NumeReWindow::AskUserForPassword()
{
	wxTextEntryDialog getPW(this, "Please enter your password and try again.", "Missing Password", "", wxTE_PASSWORD | wxOK | wxCANCEL);
	int oked = getPW.ShowModal();
	if(oked == wxID_OK)
	{
		m_options->SetPassphrase(getPW.GetValue());
		return true;
	}

	return false;
}

void NumeReWindow::OnOpenSourceFile(int id )
{
	if( id == ID_MENU_OPEN_SOURCE_LOCAL)
	{
		m_remoteMode = false;
	}
	else if(id == ID_MENU_OPEN_SOURCE_REMOTE)
	{
		m_remoteMode = true;
	}

	wxArrayString fnames = OpenFile(FILE_SUPPORTEDFILES);

	if(fnames.Count() > 0)
	{
		OpenFilesFromList(fnames);
	}
}

void NumeReWindow::OnSaveSourceFile( int id )
{
	if( id == ID_MENU_SAVE_SOURCE_LOCAL)
	{
		m_remoteMode = false;
	}
	else if(id == ID_MENU_SAVE_SOURCE_REMOTE)
	{
		m_remoteMode = true;
	}
	SaveFile(true, false, FILE_ALLSOURCETYPES);
}

void NumeReWindow::OnExecuteFile(const string& sFileName)
{
    if (!sFileName.length())
        return;
    //wxToolBar* tb = GetToolBar();
    string command = replacePathSeparator(sFileName);
    vector<string> vPaths = m_terminal->getPathSettings();
    if (command.rfind(".nprc") != string::npos)
    {
        command.erase(command.rfind(".nprc"));
        if (command.substr(0, vPaths[PROCPATH].length()) == vPaths[PROCPATH])
        {
            command.erase(0, vPaths[PROCPATH].length());
            while (command.front() == '/')
                command.erase(0, 1);
            while (command.find('/') != string::npos)
                command[command.find('/')] = '~';
        }
        else
            command = "'" + command + "'";
        command = "$" + command + "()";
        Busy();
    }
    else if (command.rfind(".nscr") != string::npos)
    {
        command.erase(command.rfind(".nscr"));
        if (command.substr(0, vPaths[SCRIPTPATH].length()) == vPaths[SCRIPTPATH])
            command.erase(0, vPaths[SCRIPTPATH].length());
        while (command.front() == '/')
            command.erase(0, 1);
        if (command.find(' ') != string::npos)
            command = "\"" + command + "\"";
        command = "start " + command;
        Busy();
    }
    else
    {
        command = "load \"" + command + "\" -app -ignore";
    }
    showConsole();
    m_terminal->pass_command(command);
}

void NumeReWindow::OnStartConnect()
{
checkPassword:
	if(!CheckForBlankPassword())
	{
		return;
	}
restartConnection:
	NetworkStatus isok = m_network->GetStatus();
	if(isok == NET_GOOD)
	{

		m_terminal->Connect();
#ifdef DO_LOG
		wxLogDebug("Connected: %d", m_terminal->IsConnected());
#endif

		// Focus on the terminal
		///int terminalIndex = m_noteTerm->FindPagePosition(m_termContainer);
		///m_noteTerm->SetSelection(terminalIndex);
		m_terminal->SetFocus();
	}
	else if(isok == NET_AUTH_FAILED)
	{
#ifdef DO_LOG
		wxLogDebug("User gave us an invalid password");
#endif
		wxMessageBox("Your password was not accepted.  Perhaps you typed it wrong?", "Invalid Password",
		wxOK | wxICON_HAND);

		// reset the password to blank so we go into CheckForBlankPassword()
		m_options->SetPassphrase(wxEmptyString);
		goto checkPassword;
	}
	else
	{
#ifdef DO_LOG
		wxLogDebug("Tried to start Terminal with invalid networking status: %d", isok);
#endif
		if(CheckNetworkStatus() == NETCALL_REDO)
		{
			goto restartConnection;
		}
	}
}

void NumeReWindow::OnCopy()
{
	/* HACK !!!
	* Okay, here's the problem.  When keyboard shortcuts like CTRL-C are listed in the text
	* of a menu item, that shortcut is automatically assigned.  But, this means that the menu's
	* enclosing frame will always capture that shortcut.
	*
	* I'd really like to have CTRL-C available to the editor for "Copy", but also available to
	* the terminal for killing programs.  My initial experiments didn't come up with anything,
	* but I did come up with this workaround.  It'll do for now.  Basically, if we detect a
	* CTRL-C, we check to see if a terminal is active.  If it is, we fake a CTRL-C keyboard
	* event, and pass it on to the terminal.  Otherwise, we just copy text as usual.
	*/
	/*wxWindow* focusedWindow = wxWindow::FindFocus();

	if(focusedWindow == m_terminal || focusedWindow == m_debugTerminal)
	{
		wxSSH* focusedTerminal = (focusedWindow == m_terminal) ? m_terminal : m_debugTerminal;

		// magic codes copied from a real CTRL-C event.
		wxKeyEvent ctrlCEvent(wxEVT_CHAR);
		ctrlCEvent.m_controlDown = true;
		ctrlCEvent.m_keyCode = 3;
		ctrlCEvent.m_rawCode = 3;
		ctrlCEvent.m_rawFlags = 3014657;

		focusedTerminal->SafelyProcessEvent(ctrlCEvent);
	}
	else*/
	{
		m_currentEd->Copy();
	}
}

void NumeReWindow::OnOpenInExplorer()
{
    wxString fileName = getTreeFolderPath(m_clickedTreeItem);

    if (fileName.length())
        ShellExecute(nullptr, nullptr, fileName.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void NumeReWindow::OnCreateNewFolder()
{
    wxString fileName = getTreeFolderPath(m_clickedTreeItem);

    if (!fileName.length())
        return;

    wxTextEntryDialog* textentry = new wxTextEntryDialog(this, _guilang.get("GUI_DLG_NEWFOLDER_QUESTION"), _guilang.get("GUI_DLG_NEWFOLDER"), _guilang.get("GUI_DLG_NEWFOLDER_DFLT"));
    int retval = textentry->ShowModal();

    if (retval == wxID_CANCEL)
    {
        delete textentry;
        return;
    }

    if (textentry->GetValue().length())
    {
        wxString foldername = fileName + "\\" + textentry->GetValue();
        wxMkdir(foldername);
    }
    delete textentry;
}

void NumeReWindow::OnRemoveFolder()
{
    FileNameTreeData* data = static_cast<FileNameTreeData*>(m_fileTree->GetItemData(m_clickedTreeItem));
    if (wxYES != wxMessageBox(_guilang.get("GUI_DLG_DELETE_QUESTION", data->filename.ToStdString()), _guilang.get("GUI_DLG_DELETE"), wxCENTRE | wxICON_QUESTION | wxYES_NO, this))
        return;
    if (m_clickedTreeItem == m_copiedTreeItem)
        m_copiedTreeItem = 0;
    Recycler _recycler;
    int error = _recycler.recycle(data->filename.ToStdString().c_str());
    error;
    return;
}

void NumeReWindow::OnFindReplace(int id)
{
    if (m_currentEd->HasSelection())
        m_findData.SetFindString(m_currentEd->GetSelectedText());
	if (g_findReplace != nullptr)
	{
		bool isReplaceDialog = g_findReplace->GetWindowStyle() & wxFR_REPLACEDIALOG;

		if( (isReplaceDialog && (id == ID_MENU_REPLACE)) ||
			(!isReplaceDialog && (id == ID_MENU_FIND)) )
		{
			return;
		}
		else
		{
			delete g_findReplace;
		}
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

void NumeReWindow::OnOptions()
{
	Settings _option = m_terminal->getKernelSettings();
    m_optionsDialog->_option = &_option;

	m_optionsDialog->InitializeDialog();
	int result = m_optionsDialog->ShowModal();
	m_optionsDialog->_option = nullptr;

	if (result == wxID_OK)
	{
        m_terminal->setKernelSettings(_option);
        //Permission* perms = m_options->GetPerms();
        // because the authorization could be updated and followed by a cancel,
        // go ahead and write the authcode out.  Odds are it hasn't changed, but
        // it's worth doing anyway to make sure it's current.
        //m_config->Write("Permissions/authorized", perms->GetAuthCode());

        // For the same reason, ALWAYS re-evaluate the options.  If the user canceled
        // the dialog, things won't have changed.
        EvaluateOptions();
        m_history->UpdateSyntaxHighlighting();
        m_terminal->UpdateColors();
        m_termContainer->SetBackgroundColour(m_options->GetSyntaxStyle(Options::CONSOLE_STD).background);
        m_termContainer->Refresh();
        for (size_t i = 0; i < m_book->GetPageCount(); i++)
        {
            NumeReEditor* edit = static_cast<NumeReEditor*>(m_book->GetPage(i));
            edit->UpdateSyntaxHighlighting(true);
            edit->SetEditorFont(m_options->GetEditorFont());
        }
        /*wxWindowList children = this->GetChildren();
        for (size_t i = 0; i < children.size(); i++)
        {
            children[i]->SetBackgroundColour(*wxBLACK);
            children[i]->SetForegroundColour(*wxWHITE);
            wxWindowList subchildren = children[i]->GetChildren();
            for (size_t j = 0; j < subchildren.size(); j++)
            {
                subchildren[j]->SetBackgroundColour(*wxBLACK);
                subchildren[j]->SetForegroundColour(*wxWHITE);
                wxWindowList subsubchildren = subchildren[j]->GetChildren();
                for (size_t k = 0; k < subsubchildren.size(); k++)
                {
                    subsubchildren[k]->SetBackgroundColour(*wxBLACK);
                    subsubchildren[k]->SetForegroundColour(*wxWHITE);
                }
            }
        }*/
    }
	m_currentEd->SetFocus();
}

void NumeReWindow::OnProjectIncludeExcludeFile( int id )
{
	bool include = (id == ID_PROJECT_INCLUDE_FILE);
	FileNameTreeData* data = static_cast <FileNameTreeData* > (m_fileTree->GetItemData(m_clickedTreeItem));
	m_projMultiFiles->SetFileBuildInclusion(data->filename, m_projectSelectedFolderType, include);

	wxString extension = wxFileName(data->filename).GetExt();
	int iconIndex;
	if(include)
	{
		iconIndex = m_extensionMappings[extension];
	}
	else
	{
		extension += "_disabled";
		iconIndex = m_extensionMappings[extension];
	}
	m_fileTree->SetItemImage(m_clickedTreeItem, iconIndex);
}

void NumeReWindow::OnPrintPage()
{
	m_currentEd->SetPrintColourMode(m_options->GetPrintStyle());
    if (!g_printData->IsOk())
        this->OnPrintSetup();
	wxPrintDialogData printDialogData( *g_printData);
	wxPrinter printer (&printDialogData);
	NumeRePrintout printout (m_currentEd, m_options);
	if (!printer.Print (this, &printout, true))
	{
		if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
		{
			wxMessageBox (_guilang.get("GUI_PRINT_ERROR"),
				_guilang.get("GUI_PRINT_ERROR_HEAD"), wxOK | wxICON_WARNING);
			return;
		}
	}
	(*g_printData) = printer.GetPrintDialogData().GetPrintData();
}


void NumeReWindow::OnPrintPreview()
{
	m_currentEd->SetPrintColourMode(m_options->GetPrintStyle());
	if (!g_printData->IsOk())
        this->OnPrintSetup();
	wxPrintDialogData printDialogData( *g_printData);
	//printDialogData.SetToPage(999);
	wxPrintPreview *preview = new wxPrintPreview (new NumeRePrintout (m_currentEd, m_options), new NumeRePrintout (m_currentEd, m_options), &printDialogData);
	if (!preview->Ok())
	{
		delete preview;
		wxMessageBox (_guilang.get("GUI_PREVIEW_ERROR"),
			_guilang.get("GUI_PREVIEW_ERROR_HEAD"), wxOK | wxICON_WARNING);
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

void NumeReWindow::OnPrintSetup()
{
	(*g_pageSetupData) = * g_printData;
	wxPageSetupDialog pageSetupDialog(this, g_pageSetupData);
	pageSetupDialog.ShowModal();
	(*g_printData) = pageSetupDialog.GetPageSetupData().GetPrintData();
	(*g_pageSetupData) = pageSetupDialog.GetPageSetupData();
}

void NumeReWindow::OnHelp()
{
    ShowHelp("numere");
}

bool NumeReWindow::ShowHelp(const wxString& sDocId)
{
    DocumentationBrowser* browser = new DocumentationBrowser(this, _guilang.get("DOC_HELP_HEADLINE", "%s"), this);
    return browser->SetStartPage(sDocId);
}

void NumeReWindow::OnAbout()
{
	AboutChameleonDialog acd(this, 10000, _guilang.get("GUI_ABOUT_TITLE"));
	acd.ShowModal();
	//m_watchPanel->TestParsing();
}

