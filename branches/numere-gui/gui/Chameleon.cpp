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

//need to include <wx/filename.h> and <wx/stdpaths.h>
#include <math.h>

#include "../common/fixvsbug.h"
#include "../editor/editor.h"
#include "NumeReNotebook.h"
#include "dialogs/OptionsDialog.h"
#include "dialogs/RemoteFileDialog.h"
#include "dialogs/wxTermContainer.h"
#include "dialogs/VariableWatchPanel.h"
#include "dialogs/CompilerOutputPanel.h"
#include "dialogs/AboutChameleonDialog.h"
#include "../common/NumeRePrintout.h"
#include "../perms/p.h"
#include "../common/ProjectInfo.h"
#include "../network/networking.h"
#include "../compiler/compiler.h"
#include "../common/Options.h"
#include "../debugger/cham_db.h"
#include "../debugger/DebugManager.h"
#include "../common/DebugEvent.h"
#include "DirTraverser.hpp"
#include "wxssh.h"
#include "../compiler/compilerevent.h"
#include "IconManager.h"
#include "wxProportionalSplitterWindow.h"
#include "ChameleonProjectManager.h"

#include <wx/msw/helpchm.h>
//#include "mmDropMenu.h"

#include "newfile.xpm"
#include "openremote.xpm"
#include "openlocal.xpm"
#include "savefile.xpm"

#include "build.xpm"
#include "compilestop.xpm"
#include "button.xpm"

#include "connect16.xpm"
#include "disconnect16.xpm"

#include "newstart1.xpm"
#include "newcontinue1.xpm"
#include "newstop1.xpm"
#include "stepnext.xpm"
#include "stepout.xpm"
#include "stepover.xpm"
#include "breakpoint_octagon.xpm"
#include "breakpoint_octagon_crossed.xpm"
#include "breakpoint_octagon_disable.xpm"

#include "../common/debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "../kernel/core/version.h"
#include "../kernel/core/tools.hpp"
const string sVersion = toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + AutoVersion::STATUS + "\"";
std::string replacePathSeparator(const std::string&);

Language _guilang;


//! global print data, to remember settings during the session
wxPrintData *g_printData = (wxPrintData*) NULL;
wxPageSetupData *g_pageSetupData = (wxPageSetupData*) NULL;

BEGIN_EVENT_TABLE(NumeReWindow, wxFrame)
	EVT_MENU						(ID_NEW_SOURCE, NumeReWindow::OnMenuEvent)
	EVT_MENU_RANGE					(ID_OPEN_SOURCE_LOCAL, ID_OPEN_SOURCE_REMOTE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_QUIT,  NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_ABOUT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_TEST, NumeReWindow::Test)
	EVT_MENU						(ID_SAVE, NumeReWindow::OnMenuEvent)
	EVT_MENU_RANGE					(ID_SAVE_SOURCE_LOCAL, ID_SAVE_SOURCE_REMOTE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_CLOSEPAGE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_CLOSETAB, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_CLOSEALL, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_STARTCONNECT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_DISCONNECT, NumeReWindow::OnMenuEvent)
	EVT_MENU_RANGE					(ID_COMPILE, ID_COMPILE_PROJECT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_UNDO, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_REDO, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_DEBUGGER, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_OPTIONS, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_CUT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_COPY, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PASTE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_FIND, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_REPLACE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_OPEN_FILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_EDIT_FILE, NumeReWindow::OnMenuEvent)
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
	EVT_MENU						(ID_PROJECT_ADDFILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PROJECT_REMOVEFILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PROJECT_EXCLUDE_FILE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PROJECT_INCLUDE_FILE, NumeReWindow::OnMenuEvent)
	EVT_TREE_ITEM_ACTIVATED			(ID_PROJECTTREE, NumeReWindow::OnTreeItemActivated)
	EVT_MENU						(ID_NEW_PROJECT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_OPEN_PROJECT_LOCAL, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_OPEN_PROJECT_REMOTE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PRINT_PAGE, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PRINT_PREVIEW, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_PRINT_SETUP, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_CLOSE_PROJECT, NumeReWindow::OnMenuEvent)
	EVT_MENU						(ID_DEBUG_START, NumeReWindow::OnDebugCommand)
	EVT_MENU						(ID_DEBUG_CONTINUE, NumeReWindow::OnDebugCommand)
	EVT_MENU						(ID_DEBUG_STOP, NumeReWindow::OnDebugCommand)
	EVT_MENU						(ID_DEBUG_STEPNEXT, NumeReWindow::OnDebugCommand)
	EVT_MENU						(ID_DEBUG_STEPOVER, NumeReWindow::OnDebugCommand)
	EVT_MENU						(ID_DEBUG_STEPOUT, NumeReWindow::OnDebugCommand)
	EVT_MENU						(ID_DEBUG_ADDEDITORBREAKPOINT, NumeReWindow::OnDebugBreakpointCommand)
	EVT_MENU						(ID_DEBUG_REMOVEEDITORBREAKPOINT, NumeReWindow::OnDebugBreakpointCommand)
	EVT_MENU						(ID_DEBUG_CLEAREDITORBREAKPOINTS, NumeReWindow::OnDebugBreakpointCommand)
	EVT_DEBUG						(NumeReWindow::OnDebugEvent)
	EVT_IDLE						(NumeReWindow::OnIdle)
	EVT_TIMER						(ID_STATUSTIMER, NumeReWindow::OnStatusTimer)
	EVT_MENU						(ID_HELP, NumeReWindow::OnMenuEvent)
END_EVENT_TABLE()

IMPLEMENT_APP(MyApp)
//----------------------------------------------------------------------
// `Main program' equivalent: the program execution "starts" here

bool MyApp::OnInit()
{
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    //wxString appPath(f.GetPath());
    _guilang.setTokens("<>="+replacePathSeparator(f.GetPath(true).ToStdString())+";");
    _guilang.loadStrings();
    wxInitAllImageHandlers();
    wxBitmap splashImage;
    if (splashImage.LoadFile("folder.png", wxBITMAP_TYPE_PNG))
    {
        wxSplashScreen* splash = new wxSplashScreen(splashImage, wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT, 4000, nullptr, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSTAY_ON_TOP | wxNO_BORDER);
        wxApp::Yield();
        wxSleep(3);
        splash->Destroy();
    }



    NumeReWindow *frame = new NumeReWindow("NumeRe: Framework für Numerische Rechnungen (v" + sVersion + ")", wxDefaultPosition, wxDefaultSize);

    frame->Show(true);
    frame->Maximize();
    if (frame->showTipAtStartup)
        frame->updateTipAtStartupSetting(wxShowTip(frame, frame->tipProvider));
    delete frame->tipProvider;
    frame->tipProvider = nullptr;
    return true;
}

MyApp::~MyApp()
{
}

//----------------------------------------------------------------------


#pragma hdrstop

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
	this->SetSize(800, 600);
    m_optionsDialog = nullptr;
    m_compiler = nullptr;
    m_config = nullptr;
    m_options = nullptr;
    m_debugger = nullptr;
    m_network = nullptr;
    m_iconManager = nullptr;
    m_debugManager = nullptr;
    m_projectManager = nullptr;

	// Show a log window for all debug messages
#ifdef _DEBUG
	logWindow = new wxLogWindow(this, "Debug messages");
	wxLog::SetActiveTarget(logWindow);
#endif

	m_updateTimer = NULL;

	//wxIcon icon(wxICON(IDI_APPICON));
	//SetIcon(icon);
	SetIcon(wxIcon("icon.ico", wxBITMAP_TYPE_ICO));

	m_remoteMode = true;

	m_options = new Options();

	InitializeProgramOptions();


	wxFileName plinkPath(wxGetCwd(), "plink.exe");

	if (!plinkPath.FileExists())
	{
		wxLogDebug("Plink.exe not found! Path used: %s", plinkPath.GetFullPath());
	}

	m_options->SetPlinkApp(plinkPath.GetFullPath());


	wxFileName pscpPath(wxGetCwd(), "pscp.exe");

	if (!pscpPath.FileExists())
	{
		wxLogDebug("PSCP.exe not found! Path used: %s", pscpPath.GetFullPath());
	}

	m_options->SetPscpApp(pscpPath.GetFullPath());


	m_network = new Networking(m_options);
	m_compiler = new Compiler(m_options, m_network);
	m_debugger = new Debugger(m_network, m_options, this);

	m_optionsDialog = new OptionsDialog(this, m_options, ID_OPTIONSDIALOG, _guilang.get("GUI_DLG_OPTIONS"));

	/**m_splitEditorOutput = new wxProportionalSplitterWindow(this, ID_SPLITEDITOROUTPUT, 0.75, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH);
	m_splitProjectEditor = new wxSplitterWindow(m_splitEditorOutput, ID_SPLITPROJECTEDITOR, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER);
	m_book = new NumeReNotebook(m_splitProjectEditor, ID_NOTEBOOK_ED);*/

	m_splitProjectEditor = new wxSplitterWindow(this, ID_SPLITPROJECTEDITOR, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER);
	m_splitEditorOutput = new wxProportionalSplitterWindow(m_splitProjectEditor, ID_SPLITEDITOROUTPUT, 0.75, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH);
	m_book = new NumeReNotebook(m_splitEditorOutput, ID_NOTEBOOK_ED);
	m_book->SetTopParent(this);

	///m_noteTerm = new NumeReNotebook(m_splitEditorOutput, ID_NOTEBOOK_TERM);
	///m_termContainer = new wxTermContainer(m_noteTerm, ID_CONTAINER_TERM);
	m_termContainer = new wxTermContainer(m_splitEditorOutput, ID_CONTAINER_TERM);
    //m_debugTermContainer = new wxTermContainer(m_noteTerm, ID_CONTAINER_DEBUGTERM);

	m_terminal = new wxSSH(m_termContainer, ID_TERMINAL, m_network, wxPoint(0, 0));
	m_terminal->set_mode_flag(GTerm::CURSORINVISIBLE);
	m_termContainer->SetTerminal(m_terminal);
	m_terminal->SetParent(this);

	//m_debugTerminal = new wxSSH(m_debugTermContainer, ID_DEBUGTERMINAL, m_network, wxPoint(0, 0));
	//m_debugTerminal->set_mode_flag(GTerm::CURSORINVISIBLE);
	//m_debugTermContainer->SetTerminal(m_debugTerminal);


	//Permission* perms = m_options->GetPerms();
	//m_outputPanel = new CompilerOutputPanel(m_noteTerm, this, m_options, ID_COMPILEROUTPUT);
	//m_outputPanel->SetAdvanced(perms->isEnabled(PERM_ADVANCEDCOMPILE));

    //m_watchPanel = new VariableWatchPanel(m_noteTerm, this, ID_VARWATCHPANEL);

	/*m_watchPanelContainer = new wxPanel(m_noteTerm, 0, 0, 40, 40);
	wxBoxSizer* watchSizer = new wxBoxSizer(wxHORIZONTAL);
	m_watchPanelContainer->SetSizer(watchSizer);
	m_watchPanelSplitter = new wxProportionalSplitterWindow(m_watchPanelContainer);
	watchSizer->Add(m_watchPanelSplitter, wxSizerFlags().Expand().Proportion(1));
	m_watchPanelContainer->Hide();*/

	/*m_container1 = new wxPanel(m_noteTerm, 0, 0, 20, 80);
	wxBoxSizer* c1Sizer = new wxBoxSizer(wxHORIZONTAL);
	m_container1->SetSizer(c1Sizer);

	m_container2 = new wxPanel(m_noteTerm, 0, 0, 80, 20);
	wxBoxSizer* c2Sizer = new wxBoxSizer(wxHORIZONTAL);
	m_container2->SetSizer(c2Sizer);*/

	m_projectManager = new ChameleonProjectManager(m_book);

	//m_debugManager = new DebugManager(m_debugger, m_debugTerminal, m_projectManager, this, m_watchPanel);
	// TODO Go ahead and activate the DebugManager here
	//m_debugger->SetEventHandler(m_debugManager);
	//m_watchPanel->SetEventHandler(m_debugManager);


	// project setup
	m_projMultiFiles = NULL;
	m_projectTree = new wxTreeCtrl(m_splitProjectEditor, ID_PROJECTTREE);
	m_projectTree->Hide();

	m_iconManager = new IconManager();

	wxImageList* fileIcons = m_iconManager->GetImageList();
	// Sets the list, but doesn't take control of it away
	m_projectTree->SetImageList(fileIcons);

	int idxFolderClosed = m_iconManager->GetIconIndex("FOLDERCLOSED");
	int idxFolderOpen = m_iconManager->GetIconIndex("FOLDEROPEN");

	m_remoteFileDialog = new RemoteFileDialog(this, ID_REMOTEFILEDIALOG);
	m_remoteFileDialog->SetNetworking(m_network);

	// This also sets the imagelist for the listview
	m_remoteFileDialog->SetIconManager(m_iconManager);


	wxTreeItemId rootNode = m_projectTree->AddRoot(_guilang.get("GUI_TREE_WORKSPACE"), idxFolderClosed, idxFolderOpen);
	m_projectFileFolders[0] = m_projectTree->AppendItem(rootNode, _guilang.get("GUI_TREE_DATAFILES"), idxFolderClosed, idxFolderOpen);
	m_projectFileFolders[1] = m_projectTree->AppendItem(rootNode, _guilang.get("GUI_TREE_SAVEDFILES"), idxFolderClosed, idxFolderOpen);
	m_projectFileFolders[2] = m_projectTree->AppendItem(rootNode, _guilang.get("GUI_TREE_SCRIPTS"), idxFolderClosed, idxFolderOpen);
	m_projectFileFolders[3] = m_projectTree->AppendItem(rootNode, _guilang.get("GUI_TREE_PROCEDURES"), idxFolderClosed, idxFolderOpen);



	m_currentPage = 0;
	m_fileNum = 0;

	m_appClosing = false;
	m_setSelection = false;

	int charWidth = GetCharWidth();

	int fileNameChars = 75;
	int filetypeChars = 20;
	int readEditChars = 15;
	//int networkStatusChars = 25;
	int lineColChars = 20;
	int debuggerchars = 25;

	const int widths[] = {fileNameChars * charWidth, filetypeChars * charWidth, readEditChars * charWidth, lineColChars * charWidth, debuggerchars * charWidth};
    const int styles[] = {wxSB_FLAT, wxSB_FLAT, wxSB_FLAT, wxSB_FLAT, wxSB_FLAT};
	m_statusBar = CreateStatusBar (WXSIZEOF(widths), wxST_SIZEGRIP);
	//m_statusBar = new wxStatusBar(this);
	//m_statusBar->SetFieldsCount(WXSIZEOF(widths), widths);
	m_statusBar->SetStatusWidths(WXSIZEOF(widths), widths);
	m_statusBar->SetStatusStyles(WXSIZEOF(styles), styles);
	SendSizeEvent();

	m_updateTimer = new wxTimer (this, ID_STATUSTIMER);

	///m_splitProjectEditor->Initialize(m_book);
	///m_splitEditorOutput->Initialize(m_splitProjectEditor);
	m_splitEditorOutput->Initialize(m_book);
	m_splitProjectEditor->Initialize(m_splitEditorOutput);
	m_splitProjectEditor->Show();
	m_splitEditorOutput->Show();
	m_book->Show();


	EvaluateOptions();

	// create the initial blank open file
	NewFile();
	PageHasChanged(m_currentPage);



	m_filterNSCRFiles = _guilang.get("GUI_FILTER_SCRIPTS") + " (*.nscr)|*.nscr";//"NumeRe scripts (*.nscr)|*.nscr";
	m_filterNPRCFiles = _guilang.get("GUI_FILTER_PROCEDURES") + " (*.nprc)|*.nprc";//"NumeRe procedures (*.nprc)|*.nprc";
	m_filterNumeReFiles = _guilang.get("GUI_FILTER_NUMEREFILES") + " (*.ndat, *.nscr, *.nprc)|*.ndat;*.nscr;*.nprc";//"NumeRe files (*.ndat, *.nscr, *.nprc)|*.ndat;*.nscr;*.nprc";

	// If this ever gets ported to Linux, we'd probably want to add
	// Linux library extensions here (.a, .so).  The other issue is that
	// the remote file dialog only looks in ~, which might need to be changed.
	m_filterLibraryFiles = "C++ libraries (*.lib)|*.lib";
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

	m_projMultiFiles = NULL;

	m_findReplace = NULL;

	/// Interesting Bug: obviously it is necessary to declare the paper size first
	g_printData = new wxPrintData;
	g_printData->SetPaperId(wxPAPER_A4);
	g_pageSetupData = new wxPageSetupDialogData(*g_printData);

	m_appStarting = false;
	m_compileProject = false;

	HINSTANCE hInstance = wxGetInstance();
	char *pStr, szPath[_MAX_PATH];
	GetModuleFileNameA(hInstance, szPath, _MAX_PATH);
	pStr = strrchr(szPath, '\\');
	if (pStr != NULL)
		*(++pStr)='\0';

	wxStandardPaths systemPaths = wxStandardPaths::Get();
	wxString appPath = systemPaths.GetExecutablePath();
	wxFileName fullProgramPath(appPath);
	wxString programPath = fullProgramPath.GetPath();

	wxFileName helpFile(programPath, "Chameleon.chm");
	m_helpController = new wxCHMHelpController();
	m_helpController->Initialize(helpFile.GetFullPath());
	//m_terminal->ProcessInput(4, "|-> WILLKOMMEN BEI NUMERE\n|-> Ein Kommando oder ein Ausdruck eingeben\n|\n|<- ");
	//m_terminal->ProcessInput(4, "|<- ");

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

	delete m_helpController;

	if(m_findReplace != NULL)
	{
		delete m_findReplace;
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
    if (m_compiler)
        delete m_compiler;
    if (m_config)
        delete m_config;
    if (m_options)
        delete m_options;
    if (m_debugger)
        delete m_debugger;
    if (m_network)
        delete m_network;
    if (m_iconManager)
        delete m_iconManager;
    if (m_debugManager)
        delete m_debugManager;
    if (m_projectManager)
        delete m_projectManager;
}

void NumeReWindow::updateTipAtStartupSetting(bool bTipAtStartup)
{
    Settings _option = m_terminal->getKernelSettings();
    if (_option.getbShowHints() == bTipAtStartup)
        return;
    _option.setbShowHints(bTipAtStartup);
    m_terminal->setKernelSettings(_option);
}

void NumeReWindow::InitializeProgramOptions()
{
	// Open up the configuration file, assumed to be in the user's home directory

	wxString authorizedCode = "0";
	wxString enabledCode = "0";

	// by default, enable nothing
	wxString defaultAuthorizedCode = "10A80000000";
	wxString defaultEnableCode = "0";



	//Permission* perms = m_options->GetPerms();
	wxString path = wxStandardPaths::Get().GetExecutablePath();

	wxFileName configName(path.substr(0, path.rfind('\\')+1), "numeregui.ini");
	//path = configName.GetFullPath();

	m_config = new wxFileConfig("numeregui", wxEmptyString, configName.GetFullPath());

	if(configName.FileExists())
	{
		//m_options->SetHostname(m_config->Read("Network/hostname"));
		//m_options->SetUsername(m_config->Read("Network/username"));
		//m_options->SetMingwPath(m_config->Read("Compiler/mingwpath"));

		bool printInColor = (m_config->Read("Miscellaneous/PrintInColor", "true") == "true");
		int printStyle = printInColor ? wxSTC_PRINT_COLOURONWHITE : wxSTC_PRINT_BLACKONWHITE;
		m_options->SetPrintStyle(printStyle);

		bool showToolbarText = (m_config->Read("Interface/ShowToolbarText", "true") == "true");
		m_options->SetShowToolbarText(showToolbarText);

		bool printLineNumbers = (m_config->Read("Miscellaneous/PrintLineNumbers", "false") == "true");
		m_options->SetLineNumberPrinting(printLineNumbers);

		//bool combineWatchWindow = (m_config->Read("Miscellaneous/CombineWatchWindow", "true") == "true");
		//m_options->SetCombineWatchWindow(combineWatchWindow);

		//bool showCompileCommands = (m_config->Read("Miscellaneous/ShowCompileCommands", "true") == "true");
		//m_options->SetCombineWatchWindow(combineWatchWindow);

        ///FIXME
		int terminalHistory = m_config->Read("Miscellaneous/TerminalHistory", 100);

		//authorizedCode = m_config->Read("Permissions/authorized", defaultAuthorizedCode);
		//enabledCode = m_config->Read("Permissions/enabled", defaultEnableCode);


		//wxString mingwBasePath = m_config->Read("Compiler/MingwBasePath", wxEmptyString);
		//m_options->SetMingwBasePath(mingwBasePath);

		//m_config->SetPath("/MinGW Bin Paths");
		/*int numEntries = m_config->GetNumberOfEntries();

		wxArrayString binPaths;
		for(int i = 0; i < numEntries; i++)
		{
			wxString keyName;
			keyName.Printf("BinPath%d", i + 1);

			wxString binPath = m_config->Read(keyName, wxEmptyString);
			binPaths.Add(binPath);
		}
		m_options->SetMingwBinPaths(binPaths);

		wxArrayString programNames = m_options->GetMingwProgramNames();
		StringFilenameHash programPaths;

		m_config->SetPath("/MinGW Program Names");
		for(int i = 0; i < programNames.GetCount(); i++)
		{
			wxString programName = programNames[i];
			wxString programPath = m_config->Read(programName, wxEmptyString);
			programPaths[programName] = programPath;
		}
		m_options->SetMingwExecutables(programPaths);

		m_config->SetPath("/");*/
	}
	else
	{
		wxLogDebug("Failed to locate config file, loading default permissions");
		authorizedCode = defaultAuthorizedCode;
		enabledCode = defaultEnableCode;

		m_config->Write("Miscellaneous/TerminalHistory", "100");
		m_config->Write("Miscellaneous/PrintInColor", "false");
		m_config->Write("Miscellaneous/PrintLineNumbers", "false");
	}

	/*if(authorizedCode == wxEmptyString)
	{
		authorizedCode = defaultAuthorizedCode;
	}
	if(enabledCode == wxEmptyString)
	{
		enabledCode = defaultEnableCode;
	}

	if(!perms->setGlobalAuthorized(authorizedCode))
	{
		wxLogDebug("Authcode initialization failed!  Code: %s", authorizedCode.c_str());
	}
	else
	{
		perms->setGlobalEnabled(enabledCode);
	}*/

#if wxUSE_DRAG_AND_DROP
	SetDropTarget(new ChameleonFileDropTarget(this));
#endif //wxUSE_DRAG_AND_DROP
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

	CloseAllFiles();

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
        case ID_OPEN_FILE:
        {
			FileNameTreeData* data = static_cast <FileNameTreeData* > (m_projectTree->GetItemData(m_clickedTreeItem));

            string command = replacePathSeparator(data->filename.ToStdString());

            if (command.rfind(".nprc") != string::npos)
            {
                command = "$'" + command.substr(0,command.rfind(".nprc")) + "'()";
            }
            else if (command.rfind(".nscr") != string::npos)
            {
                command = "start \"" + command + "\"";
            }
            else
            {
                command = "append \"" + command + "\"";
            }

            m_terminal->pass_command(command);

            break;
        }
        case ID_EDIT_FILE:
        {
			FileNameTreeData* data = static_cast <FileNameTreeData* > (m_projectTree->GetItemData(m_clickedTreeItem));

            wxArrayString fnames;
            fnames.Add(data->filename);
            OpenSourceFile(fnames);
            break;
        }

		case ID_NEW_SOURCE:
		{
			NewFile();
			break;
		}

		case ID_OPEN_SOURCE_LOCAL:
		case ID_OPEN_SOURCE_REMOTE:
		{
			OnOpenSourceFile(id);
			break;
		}

		case ID_SAVE:
		{
			SaveFile(false, true, FILE_ALLSOURCETYPES);
			break;
		}

		case ID_SAVE_SOURCE_LOCAL:
		case ID_SAVE_SOURCE_REMOTE:
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

		case ID_CLOSETAB:
		{
			CloseTab();
			break;
		}

		case ID_CLOSEPAGE:
		{
			if (!m_currentEd)
			{
				return;
			}

			CloseFile();
			PageHasChanged();
			break;
		}

		case ID_CLOSEALL:
		{
			CloseAllFiles();
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

		case ID_QUIT:
		{
			Close(true);
			break;
		}

		case ID_REDO:
		case ID_UNDO:
		{
			if(id == ID_REDO)
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

		case ID_COPY:
		{
			OnCopy();
			break;
		}

		case ID_CUT:
		{
			m_currentEd->Cut();
			break;
		}

		case ID_PASTE:
		{
			m_currentEd->Paste();
			break;
		}

		case ID_FIND:
		case ID_REPLACE:
		{
			OnFindReplace(id);
			break;
		}

		case ID_ABOUT:
		{
			OnAbout();
			break;
		}

		case ID_OPTIONS:
		{
			OnOptions();
			break;
		}
		case ID_DEBUGGER:
		{
            Settings _option = m_terminal->getKernelSettings();
            _option.setDebbuger(!_option.getUseDebugger());
            m_terminal->setKernelSettings(_option);
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

		case ID_PRINT_PAGE:
		{
			OnPrintPage();
			break;
		}

		case ID_PRINT_PREVIEW:
		{
			OnPrintPreview();
			break;
		}

		case ID_PRINT_SETUP:
		{
			OnPrintSetup();
			break;
		}

		case ID_HELP:
		{
			OnHelp();
			break;
		}
	}
}

void NumeReWindow::OnDebugBreakpointCommand(wxCommandEvent &event)
{
	switch(event.GetId())
	{
		case ID_DEBUG_ADDEDITORBREAKPOINT:
		{
			m_currentEd->OnAddBreakpoint(event);
			break;
		}
		case ID_DEBUG_REMOVEEDITORBREAKPOINT:
		{
			m_currentEd->OnRemoveBreakpoint(event);
			break;
		}
		case ID_DEBUG_CLEAREDITORBREAKPOINTS:
		{
			m_currentEd->OnClearBreakpoints(event);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private OnDebugCommand
///  Responsible for handling all debug-related menu / toolbar commands
///
///  @param  event wxCommandEvent & The menu event generated
///
///  @return void
///
///  @remarks Basically just creates a debug event with the same ID and passes it to
///  @remarks the debugger.  Sets up the start debug information if necessary.
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnDebugCommand(wxCommandEvent &event)
{
	int eventID = event.GetId();

	//ProjectInfo* currentProject = NULL;

	if(eventID == ID_DEBUG_START)
	{
        string command = replacePathSeparator((m_currentEd->GetFileName()).GetFullPath().ToStdString());
        if (!command.length())
            return;
        if (command.rfind(".nprc") != string::npos)
        {
            command = "$'" + command.substr(0,command.rfind(".nprc")) + "'()";
        }
        else
        {
            command = "start \"" + command + "\"";
        }
        m_terminal->pass_command(command);
        return;
		/*if(m_projMultiFiles != NULL)
		{
			currentProject = m_projMultiFiles;
		}
		else
		{
			currentProject = m_currentEd->GetProject();
		}*/
	}

	//m_debugManager->OnDebugCommand(eventID, currentProject, m_currentEd);

}

//////////////////////////////////////////////////////////////////////////////
///  private OnDebugEvent
///  Handles debug-related events created by the debugger or other widgets
///
///  @param  event wxDebugEvent & The debug event to handle
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnDebugEvent(wxDebugEvent &event)
{
	int eventID = event.GetId();

	wxLogDebug("ChameleonWindow:OnDebugEvent - shouldn't be here any more!  Event ID: %d", eventID);
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
void NumeReWindow::NewFile()
{
	m_fileNum += 1;

	//wxString locationPrefix = "(?) ";

	wxString noname = _guilang.get("GUI_NEWFILE_UNTITLED") + " " + wxString::Format ("%d", m_fileNum);
	ProjectInfo* singleFileProject = new ProjectInfo();
	NumeReEditor* edit = new NumeReEditor (this, m_debugManager, m_options, singleFileProject, m_book, -1, m_terminal->getSyntax());
	//edit->SetSyntax(m_terminal->getSyntax());

#if wxUSE_DRAG_AND_DROP
	edit->SetDropTarget(new ChameleonFileDropTarget(this));
#endif

	m_currentEd = edit;
	m_currentPage = m_book->GetPageCount();
	m_book->AddPage (edit, noname, true);
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
		m_currentPage = pageNr;
		m_currentEd = static_cast< NumeReEditor * > (m_book->GetPage (m_currentPage));
		m_book->SetSelection(pageNr);
		m_currentEd->SetFocus();
	}
	else
	{
		m_currentPage = -1;
		m_currentEd = NULL;
	}
	m_book->Refresh();
	if(m_currentEd != NULL)
	{
		m_currentEd->Refresh();

		wxString tabText = m_book->GetPageText(m_currentPage);
		wxString statusText;

		if(tabText.StartsWith("(R)"))
		{
			m_remoteMode = true;
		}
		else if(tabText.StartsWith("(L)"))
		{
			m_remoteMode = false;
		}
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
void NumeReWindow::CloseFile (int pageNr)
{
	if (pageNr == -1)
	{
		pageNr = m_book->GetSelection();
	}

	// gives the user a chance to save if the file has been modified
	int modifiedFileResult = HandleModifiedFile(pageNr, MODIFIEDFILE_CLOSE);

	// a wxYES result is taken care of inside HandleModifiedFile, and a
	// wxNO is handled implicitly by the fact that the file isn't saved.
	if(modifiedFileResult == wxCANCEL)
	{
		return;
	}

	if (m_book->GetPageCount() > 0)
	{
		wxFileName currentFileName;
		if ((m_book->GetPageCount() > 1) || m_appClosing)
		{
			currentFileName = m_currentEd->GetFileName();
			//NumeReEditor* pEdit = static_cast <NumeReEditor* >(m_book->GetPage(pageNr));
			m_book->DeletePage (pageNr);
		}
		// closing out the last buffer, reset it to act as a new one
		else
		{
			m_fileNum = 1;
			//wxString locationPrefix = "(?) ";
			wxString noname = _guilang.get("GUI_NEWFILE_UNTITLED") + " " + wxString::Format ("%d", m_fileNum);
			m_book->SetPageText (pageNr, noname);
			m_currentEd->ResetEditor();
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
void NumeReWindow::CloseAllFiles()
{
	int cnt = m_book->GetPageCount();
	for (int i = 0; i < cnt; i++)
	{
		CloseFile();
	}

	PageHasChanged();
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
				saveMessage += "Do you want to save them and compile the file / project?";
				break;
			}
		}

		int options = wxYES_NO | wxICON_QUESTION;

		if(fileAction != MODIFIEDFILE_COMPILE)
		{
			options |= wxCANCEL;
		}

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
void NumeReWindow::OpenSourceFile (wxArrayString fnames, unsigned int nLine)
{
	int firstPageNr = -1;
	wxString fileContents = wxEmptyString;
	wxString fileNameNoPath;

	wxString locationPrefix;

	if(false)//m_remoteMode)
	{
		locationPrefix = "(R) ";
	}
	else
	{
		locationPrefix = "(L) ";
	}

	for (size_t n = 0; n < fnames.GetCount(); n++)
	{
		fileNameNoPath = wxEmptyString;
		wxFileName newFileName(fnames[n]);
		int pageNr = GetPageNum(newFileName);

		if(!GetFileContents(fnames[n], fileContents, fileNameNoPath))
		{
			return;
		}

		// I'd previously canceled the open here if the file was empty,
		// but we opted to skip that behavior
		/*
		if(fileContents == wxEmptyString)
		{
			continue;
		}
		*/

		// filename is already open
		if (pageNr >= 0)
		{
			int modifiedFileResult = HandleModifiedFile(pageNr, MODIFIEDFILE_RELOAD);

			// user canceled the open request, skip the reload
			if(modifiedFileResult == wxCANCEL)
			{
				continue;
			}
			m_setSelection = true;
			m_book->SetSelection (pageNr);
			m_setSelection = false;
			m_currentPage = pageNr;

			m_currentEd = static_cast< NumeReEditor* > (m_book->GetPage (m_currentPage));
			m_currentEd->LoadFileText(fileContents);
			if (nLine)
                m_currentEd->GotoLine(nLine);
		}

		else
		{
			ProjectInfo* proj;

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
			if ((!m_currentEd->Modified() &&
				 !m_currentEd->HasBeenSaved() &&
				  m_currentEd->GetText().IsEmpty()) )
			{
				m_book->SetPageText(m_currentPage, /*locationPrefix + */fileNameNoPath);
				m_currentEd->SetProject(proj);
			}
			// need to create a new buffer for the file
			else
			{
				NumeReEditor *edit = new NumeReEditor (this, m_debugManager, m_options, proj, m_book, -1, m_terminal->getSyntax());
#if wxUSE_DRAG_AND_DROP
				edit->SetDropTarget(new ChameleonFileDropTarget(this));
#endif
				m_currentEd = edit;
				m_currentPage = m_book->GetPageCount();
				m_book->AddPage (m_currentEd, /*locationPrefix + */fileNameNoPath, true);
			}

			m_currentEd->LoadFileText(fileContents);
			m_currentEd->SetFilename(newFileName, m_remoteMode);
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
	if(false)
	{
		wxString remotePath = fn.GetPath(false, wxPATH_UNIX);
		wxString remoteFile = fn.GetFullName();

		if(!m_network->GetFileContents(fn, fileContents))
		{
			CheckNetworkStatus();
		}
	}
	else
	{
		wxFile file (fileToLoad);

		if( !file.IsOpened() )
		{
			return false;
		}

		long lng = file.Length();

		if( lng > 0 )
		{
            file.ReadAll(&fileContents);
			/*char *pFileContents = wxStringBuffer(fileContents, lng);//fileContents.DoGetWriteBuf(lng);
			file.Read(pFileContents, lng);
			fileContents.DoUngetWriteBuf();*/
		}
	}

	fileName = fn.GetFullName();
	return true;
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

	bool isSourceFile = !(filterType == FILE_NUMERE);

	wxString filterString = ConstructFilterString(filterType);

	/*if(doSaveAs && askLocalRemote)
	{
		// default to saving remotely
		m_remoteMode = false;

		if(m_options->GetPerms()->isEnabled(PERM_LOCALMODE))
		{
			wxArrayString choices;
			choices.Add("Remote");
			choices.Add("Local");
			wxString choice = wxGetSingleChoice("Would you like to save this file: \n\n1) Remotely (on the server) \n2) Locally (on the computer you're using)?",
				"Save File - location",choices);
			if(choice == "Local")
			{
				m_remoteMode = false;
			}
			else if(choice == wxEmptyString)
			{
				// user hit Cancel - don't save
				m_remoteMode = originalRemoteMode;
				return false;
			}
		}
	}*/

	if(isSourceFile)
	{
		fileContents = m_currentEd->GetText();
	}
	// we must be saving a new project
	else
	{
		fileContents = "[Headers]\n\n[Sources]\n\n[Libraries]\n\n[Other]";
	}

	if(true)
	{
		if(doSaveAs)
		{
			// the last item in a filter's list will be the default extension if none is given
			// ie, right now, .cpp is the default extension for C++ files

			wxString title = _guilang.get("GUI_DLG_SAVEAS");

			if(filterType == FILE_NUMERE)
			{
				title = "Save New Project";
			}
			wxFileDialog dlg (this, title, "", "", filterString,
				wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);

			// ie, user clicked cancel
			if(dlg.ShowModal() != wxID_OK)
			{
				return false;
			}

			filename = dlg.GetPath();
		}

		else
		{
			filename = m_currentEd->GetFileNameAndPath();
		}

		if(isSourceFile)
		{
			m_currentEd->SetFocus();

			wxFileName fn(filename);
			m_currentEd->SetFilename(fn, false);
			m_currentEd->SaveFile(filename);

			wxString simpleFileName = m_currentEd->GetFilenameString();

			int currentTab = m_book->GetSelection();

			wxString locationPrefix = "(L) ";
			m_book->SetPageText(currentTab, simpleFileName);
			m_book->Refresh();
		}
		else
		{
			wxFile newProjectFile(filename, wxFile::write);
			newProjectFile.Write(fileContents);
			newProjectFile.Close();
		}

	}
	// remote mode
	else
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
	}

	if(isSourceFile)
	{
		m_currentEd->EmptyUndoBuffer();
		m_currentEd->SetSavePoint();
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

		wxTreeItemId rootItem = m_projectTree->GetRootItem();
		m_projectTree->SetItemText(rootItem, m_projMultiFiles->GetProjectName());

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
	case FILE_ALLSOURCETYPES:
		filterString = m_filterNumeReFiles;
		filterString += "|";
		filterString += m_filterNSCRFiles;
		filterString += "|";
		filterString += m_filterNPRCFiles;
		filterString += "|";
		filterString += m_filterAllFiles;
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
	case FILE_DATAFILES:
		filterString = m_filterLibraryFiles;
		break;
	case FILE_ALLFILES:
	default:
		filterString = m_filterAllFiles;
		break;
	}
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
		OnUpdateDebugUI();
		OnUpdateConnectionUI();
		OnUpdateCompileUI();
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
void NumeReWindow::OnUpdateDebugUI()
{
	/*if(!m_options->GetPerms()->isEnabled(PERM_DEBUG))
	{
		return;
	}*/
	bool isDebugging = m_debugger->isDebugging();
	bool isPaused = m_debugger->isPaused();

	wxToolBar* tb = GetToolBar();

	if(isDebugging)
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
	}
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
	if(!m_options->GetPerms()->isEnabled(PERM_TERMINAL))
	{
		return;
	}
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
///  private OnUpdateCompileUI
///  Updates the compile button
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReWindow::OnUpdateCompileUI()//wxUpdateUIEvent &event)
{
	if(!m_options->GetPerms()->isEnabled(PERM_COMPILE))
	{
		return;
	}

	ProjectInfo* edProj = m_currentEd->GetProject();
	wxToolBar* tb = GetToolBar();
	//wxToolBarToolBase* compileButton = tb->FindById(ID_COMPILE);

	bool canCompile = !edProj->IsCompiled();
	bool currProjIsCompiling = m_compiler->IsCompiling() && edProj->IsBeingCompiled();

	// The other half of the compile cancellation code.  If this is ever activated,
	// then a check needs to be made so that tb->Realize() is only called when it's
	// actually needed.  If it's called every time, there's some nasty flickering
	// in the toolbar.

	/*
	// Choose the image & caption:
	if(currProjIsCompiling)
	{
	wxBitmap bmStopCompile(stop1_xpm);
	compileButton->SetNormalBitmap(bmStopCompile);
	compileButton->SetLabel("Stop");
	}
	else
	{
	wxBitmap bmBuild(build_xpm);
	compileButton->SetNormalBitmap(bmBuild);
	compileButton->SetLabel("Compile");
	}
	//tb->Realize();
	*/

	bool enableButton = canCompile && !currProjIsCompiling; //|| currProjIsCompiling;
	tb->EnableTool(ID_COMPILE, enableButton);
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
	if(m_statusBar == NULL)
	{
		return;
	}

	int pageCount = m_book->GetPageCount();
	if(pageCount < 1 || pageCount <= m_currentPage)
	{
		return;
	}
	wxString tabText = m_book->GetPageText(m_currentPage);
	wxString filename;
	wxString filetype;

	filename = m_currentEd->GetFileNameAndPath();

	if(filename.find(".nprc") != string::npos)
	{
		filetype = _guilang.get("GUI_STATUSBAR_PROCEDURE");
	}
	else if(filename.find(".nscr") != string::npos)
	{
		filetype = _guilang.get("GUI_STATUSBAR_SCRIPT");//("Script");
	}
	else if(tabText.StartsWith("<"))
	{
		filename = _guilang.get("GUI_STATUSBAR_UNSAVEDFILE");
		filetype = "N/A";
	}

	if(filename != m_statusBar->GetStatusText(0))
	{
		m_statusBar->SetStatusText(filename, 0);
	}

	if(filetype != m_statusBar->GetStatusText(1))
	{
		m_statusBar->SetStatusText(filetype, 1);
	}

	bool isEdReadOnly = m_currentEd->GetProject()->IsReadOnly();

	wxString editable = isEdReadOnly ? "Read only" : "Read/Write";

	if(editable != m_statusBar->GetStatusText(2))
	{
		m_statusBar->SetStatusText(editable, 2);
	}

	bool synchronousOp = m_network->DoingSynchronousOperation();

	if(synchronousOp)
	{
		wxLogDebug("UI aware of synchronous op");
	}

	/*
	wxString netStatus = m_network->GetStatusDetails();
	wxString statusText;
	if(netStatus == "NET_STARTING")
	{
		statusText = "Networking starting";
	}
	else if(netStatus == "NET_GOOD")
	{
		statusText = "Network connected";
	}
	else if(netStatus == "NET_UNKNOWN_HOST")
	{
		statusText = "Can't find host";
	}
	else if(netStatus == "NET_CONN_REFUSED")
	{
		statusText = "Connection refused";
	}
	else if (netStatus = "NET_AUTH_FAILED")
	{
		statusText = "Login failed";
	}
	else if(netStatus == "NET_ERROR_MESSAGE")
	{
		statusText = "Unknown network error";

	}

	SetStatusText(statusText, 3);
	*/

	int curLine = m_currentEd->GetCurrentLine();
	int curPos = m_currentEd->GetCurrentPos() - m_currentEd->PositionFromLine (-curLine);
	wxString linecol;
	linecol.Printf (_(_guilang.get("GUI_STATUSBAR_LINECOL")), curLine+1, curPos+1);
	//linecol.Printf (_("Line: %d, Col: %d"), curLine+1, curPos+1);
	if (linecol != m_statusBar->GetStatusText (3))
	{
		SetStatusText (linecol, 3);
	}
	if (m_terminal->getKernelSettings().getUseDebugger())
        SetStatusText(_guilang.get("GUI_STATUSBAR_DEBUGGER"), 4);
    else
        SetStatusText("", 4);
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
		}
	}
	else
	{
		if(title.Contains("*"))
		{
			title.RemoveLast(1);
			m_book->SetPageText(tabNum, title);
			m_book->Refresh();
		}
	}
	GetToolBar()->EnableTool(ID_SAVE, enable);

	wxMenuBar* mb = GetMenuBar();
	WXWidget handle = mb->GetHandle();
	if(handle != NULL)
	{
		mb->FindItem(ID_SAVE)->Enable(enable);
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
		wxLogDebug("Failed to properly set variable.  variableName = %d", variableName);
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
	return m_debugger->isDebugging();
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
	return m_debugger->isPaused();
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
	m_projectTree->AssignImageList(imagelist);
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

	//Permission* perms = m_options->GetPerms();

	UpdateToolbar();
	UpdateMenuBar();
	UpdateTerminalNotebook();

	if (true) //(perms->isEnabled(PERM_PROJECTS))
	{
		if(!m_projectTree->IsShown())
		{
			///m_splitProjectEditor->SplitVertically(m_book, m_projectTree, this->GetSize().GetWidth()-200);
			///m_splitProjectEditor->SplitVertically(m_projectTree, m_book, 200);
			m_splitProjectEditor->SplitVertically(m_projectTree, m_splitEditorOutput, 200);
			m_splitProjectEditor->SetMinimumPaneSize(30);
			m_projectTree->Show();
			m_projectTree->Toggle(m_projectTree->GetRootItem());
			vector<string> vPaths = m_terminal->getPathSettings();
			LoadFilesToTree(vPaths[2], FILE_DATAFILES, m_projectFileFolders[0]);
			LoadFilesToTree(vPaths[3], FILE_DATAFILES, m_projectFileFolders[1]);
			LoadFilesToTree(vPaths[4], FILE_NSCR, m_projectFileFolders[2]);
			LoadFilesToTree(vPaths[5], FILE_NPRC, m_projectFileFolders[3]);
		}
	}
	else
	{
		if(m_projMultiFiles != NULL)
		{
			CloseProjectFile(false);
		}
		if(m_projectTree->IsShown())
		{
			m_splitProjectEditor->Unsplit(m_projectTree);
			m_projectTree->Hide();
			m_book->Refresh();
		}
	}

	for(int i = 0; i < (int)m_book->GetPageCount(); i++)
	{
		NumeReEditor* edit = static_cast<NumeReEditor*> (m_book->GetPage(i));
		edit->UpdateSyntaxHighlighting();
	}

	int newMaxTermSize = m_options->GetTerminalHistorySize();
	m_termContainer->SetTerminalHistory(newMaxTermSize);

	/*m_config->Write("Permissions/enabled", perms->getGlobalEnabled());
	m_config->Write("Network/hostname", m_options->GetHostname());
	m_config->Write("Network/username", m_options->GetUsername());*/
	//m_config->Write("Compiler/mingwpath", m_options->GetMingwPath());
	m_config->Write("Miscellaneous/TerminalHistory", newMaxTermSize);


	bool printInColor = (m_options->GetPrintStyle() == wxSTC_PRINT_COLOURONWHITE);
	m_config->Write("Miscellaneous/PrintInColor", printInColor ? "true" : "false");

	bool printLineNumbers = m_options->GetLineNumberPrinting();
	m_config->Write("Miscellaneous/PrintLineNumbers", printLineNumbers ? "true" : "false");

	/*bool combineWatchWindow = m_options->GetCombineWatchWindow();
	m_config->Write("Miscellaneous/CombineWatchWindow", combineWatchWindow ? "true" : "false");*/

	/*bool showCompileCommands = m_options->GetShowCompileCommands();
	m_config->Write("Miscellaneous/ShowCompileCommands", showCompileCommands ? "true" : "false");*/

	/*m_config->Write("Compiler/MingwBasePath", m_options->GetMingwBasePath());

	wxArrayString mingwBinPaths = m_options->GetMingwBinPaths();

	for(int i = 0; i < mingwBinPaths.GetCount(); i++)
	{
		wxString keyName;
		keyName.Printf("MinGW Bin Paths/BinPath%d", i + 1);
		m_config->Write(keyName, mingwBinPaths[i]);
	}

	wxArrayString programNames = m_options->GetMingwProgramNames();
	StringFilenameHash programPaths = m_options->GetMingwExecutables();

	for(int i = 0; i < programNames.GetCount(); i++)
	{
		wxString programName = programNames[i];
		wxFileName programPath = programPaths[programName];

		wxString keyName;
		keyName.Printf("MinGW Programs/%s", programName);
		m_config->Write(keyName, programPath.GetFullPath());
	}

	m_options->VerifyMingwPath(m_options->GetMingwBasePath());*/

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

	//Permission* perms = m_options->GetPerms();

	//bool localModeEnabled = perms->isEnabled(PERM_LOCALMODE);
	if(menuBar != NULL)
	{
		SetMenuBar(NULL);
		delete menuBar;
	}

	menuBar = new wxMenuBar();
	SetMenuBar(menuBar);

	wxMenu* menuFile = new wxMenu();

	menuFile->Append(ID_NEW_SOURCE, _guilang.get("GUI_MENU_NEWFILE"), _guilang.get("GUI_MENU_NEWFILE_TTP"));
	//menuFile->Append(ID_NEW_SOURCE, "&New File\tCtrl-N", "Create a new file");
	//if(localModeEnabled)
	//{
		menuFile->Append(ID_OPEN_SOURCE_LOCAL, _guilang.get("GUI_MENU_OPENFILE"), _guilang.get("GUI_MENU_OPENFILE_TTP"));
		//menuFile->Append(ID_OPEN_SOURCE_LOCAL, "&Open File", "Open an existing file");
	//}

	//menuFile->Append(ID_OPEN_SOURCE_REMOTE, "&Open File (Remote)", "Open an existing C++ file");
	menuFile->Append(ID_CLOSEPAGE, _guilang.get("GUI_MENU_CLOSEFILE"), _guilang.get("GUI_MENU_CLOSEFILE_TTP"));
	//menuFile->Append(ID_CLOSEPAGE, "&Close File", "Close the active file");
	menuFile->Append(ID_CLOSEALL, _guilang.get("GUI_MENU_CLOSEALLFILES"));
	//menuFile->Append(ID_CLOSEALL, "Close All Files");
	menuFile->AppendSeparator();
	menuFile->Append(ID_SAVE, _guilang.get("GUI_MENU_SAVEFILE"), _guilang.get("GUI_MENU_SAVEFILE_TTP"));
	//menuFile->Append(ID_SAVE, "&Save File\tCtrl-S", "Save the current file");

	//if(localModeEnabled)
	//{
		menuFile->Append(ID_SAVE_SOURCE_LOCAL, _guilang.get("GUI_MENU_SAVEFILEAS"), _guilang.get("GUI_MENU_SAVEFILEAS_TTP"));
		//menuFile->Append(ID_SAVE_SOURCE_LOCAL, "Save File &As ...", "Save the current file as a different name");
	//}

	//menuFile->Append(ID_SAVE_SOURCE_REMOTE, "Save File &As (Remote)", "Save the current file as a different name");
	menuFile->AppendSeparator();

	menuFile->Append(ID_PRINT_PAGE, _guilang.get("GUI_MENU_PRINT"), _guilang.get("GUI_MENU_PRINT_TTP"));
	menuFile->Append(ID_PRINT_PREVIEW, _guilang.get("GUI_MENU_PRINTPREVIEW"));
	menuFile->Append(ID_PRINT_SETUP, _guilang.get("GUI_MENU_PRINTSETUP"));

	menuFile->Append(ID_QUIT,_guilang.get("GUI_MENU_EXIT"), _guilang.get("GUI_MENU_EXIT_TTP"));

	menuBar->Append(menuFile, _guilang.get("GUI_MENU_FILE"));

    /**menuFile->Append(ID_PRINT_PAGE, "&Print\tCtrl-P", "Print the current file");
	menuFile->Append(ID_PRINT_PREVIEW, "Print preview");
	menuFile->Append(ID_PRINT_SETUP, "Print setup");

	menuFile->Append(ID_QUIT, "E&xit\tAlt-F4", "Quit this program");

	menuBar->Append(menuFile, "&File");*/


	wxMenu* menuEdit = new wxMenu();

	menuEdit->Append(ID_UNDO, _guilang.get("GUI_MENU_UNDO"));
	menuEdit->Append(ID_REDO, _guilang.get("GUI_MENU_REDO"));
	menuEdit->AppendSeparator();
	menuEdit->Append(ID_CUT, _guilang.get("GUI_MENU_CUT"));
	menuEdit->Append(ID_COPY, _guilang.get("GUI_MENU_COPY"));
	menuEdit->Append(ID_PASTE, _guilang.get("GUI_MENU_PASTE"));
	menuEdit->AppendSeparator();
	menuEdit->Append(ID_FIND, _guilang.get("GUI_MENU_FIND"));
	menuEdit->Append(ID_REPLACE, _guilang.get("GUI_MENU_REPLACE"));

	/**menuEdit->Append(ID_UNDO, "&Undo\tCtrl-Z");
	menuEdit->Append(ID_REDO, "&Redo\tCtrl-Y");
	menuEdit->AppendSeparator();
	menuEdit->Append(ID_CUT, "Cu&t\tCtrl-X");
	menuEdit->Append(ID_COPY, "&Copy\tCtrl-C");
	menuEdit->Append(ID_PASTE, "&Paste\tCtrl-V");
	menuEdit->AppendSeparator();
	menuEdit->Append(ID_FIND, "Find\tCtrl-F");
	menuEdit->Append(ID_REPLACE, "Replace\tCtrl-H");*/

	// TODO Implement these Edit menu items

	/*
	menuEdit->AppendSeparator();
	menuEdit->Append(ID_INDENT, "Indent text\tTab");
	menuEdit->Append(ID_UNINDENT, "Unindent text\tShift-Tab");
	menuEdit->AppendSeparator();
	menuEdit->Append(ID_SELECTALL, "Select all\tCtrl-A");
	menuEdit->Append(ID_SELECTLINE, "Select line");
	*/

	menuBar->Append(menuEdit, _guilang.get("GUI_MENU_EDIT"));
	///menuBar->Append(menuEdit, "&Edit");

	/*if(perms->isEnabled(PERM_PROJECTS))
	{
		wxMenu* menuProject = new wxMenu();
		menuProject->Append(ID_NEW_PROJECT, "Create New Project");

		if(localModeEnabled)
		{
			menuProject->Append(ID_OPEN_PROJECT_LOCAL, "Open Project File (Local)");
		}

		menuProject->Append(ID_OPEN_PROJECT_REMOTE, "Open Project File (Remote)");
		menuProject->Append(ID_CLOSE_PROJECT, "Close Project");

		menuProject->AppendSeparator();
		menuProject->Append(ID_COMPILE_PROJECT, "Compile Active Project");

		menuBar->Append(menuProject, "&Projects");
	}*/

	/*if(perms->isEnabled(PERM_COMPILE))
	{
		wxMenu* compileMenu = new wxMenu();
		compileMenu->Append(ID_COMPILE, "Compile File");
		menuBar->Append(compileMenu, "&Compile");
	}*/


	wxMenu* menuTools = new wxMenu();

	menuTools->Append(ID_OPTIONS, _guilang.get("GUI_MENU_OPTIONS"));
	menuTools->AppendSeparator();
	menuTools->Append(ID_DEBUGGER, _guilang.get("GUI_MENU_DEBUGGER"), _guilang.get("GUI_MENU_DEBUGGER_TTP"), true);
	menuTools->Check(ID_DEBUGGER, m_terminal->getKernelSettings().getUseDebugger());
	///menuTools->Append(ID_OPTIONS, "&Options");

	/*if(perms->isEnabled(PERM_TERMINAL))
	{
		menuTools->AppendSeparator();
		menuTools->Append(ID_STARTCONNECT, "&Connect to server");
		menuTools->Append(ID_DISCONNECT, "&Disconnect from server");
	}*/
	menuBar->Append(menuTools, _guilang.get("GUI_MENU_TOOLS"));
	///menuBar->Append(menuTools, "&Tools");

	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(ID_HELP, _guilang.get("GUI_MENU_SHOWHELP"));
	helpMenu->Append(ID_ABOUT, _guilang.get("GUI_MENU_ABOUT"), _guilang.get("GUI_MENU_ABOUT_TTP"));

	menuBar->Append(helpMenu, _guilang.get("GUI_MENU_HELP"));

	/**helpMenu->Append(ID_HELP, "Chameleon help contents");
	helpMenu->Append(ID_ABOUT, "&About Chameleon", "Show about dialog");

	menuBar->Append(helpMenu, "&Help");*/
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
	//Permission* perms = m_options->GetPerms();

	//CleanupDropMenu();

	wxToolBar* t = GetToolBar();
	delete t;
	SetToolBar(NULL);
	int style = wxTB_FLAT | wxTB_HORIZONTAL;

	bool showText = m_options->GetShowToolbarText();
	if(showText)
	{
		style |= wxTB_TEXT;
	}
	m_config->Write("Interface/ShowToolbarText", showText ? "true" : "false");
	t = CreateToolBar(style);//new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);
	//SetToolBar(t);

	wxBitmap bmNew(newfile_xpm);
	t->AddTool(ID_NEW_SOURCE, _guilang.get("GUI_TB_NEW"), bmNew, _guilang.get("GUI_TB_NEW_TTP"));

	//wxBitmap bmOpen((open_xpm);

	//if(perms->isEnabled(PERM_LOCALMODE))
	//{
		wxBitmap bmOpenLocal(openlocal_xpm);
		t->AddTool(ID_OPEN_SOURCE_LOCAL, _guilang.get("GUI_TB_OPEN"), bmOpenLocal, _guilang.get("GUI_TB_OPEN_TTP"));
	//}
	//wxBitmap bmOpenRemote(openremote_xpm);
	//t->AddTool(ID_OPEN_SOURCE_REMOTE, "Open (R)", bmOpenRemote, "Open a file from the remote server");

	wxBitmap bmSave(savefile_xpm);
	t->AddTool(ID_SAVE, _guilang.get("GUI_TB_SAVE"), bmSave, _guilang.get("GUI_TB_SAVE_TTP"));

	/*if(perms->isEnabled(PERM_TERMINAL))
	{
		t->AddSeparator();

		wxBitmap bmConnect(connect16_xpm);
		t->AddTool(ID_STARTCONNECT, "Connect", bmConnect, "Connect the terminal to the remote server");

		wxBitmap bmDisconnect(disconnect16_xpm);
		t->AddTool(ID_DISCONNECT, "Disconnect", bmDisconnect, "Disconnect the terminal from the remote server");
	}*/

	/*if(perms->isEnabled(PERM_COMPILE))
	{
		t->AddSeparator();
		wxBitmap bmCompile(build_xpm);
		t->AddTool(ID_COMPILE, "Compile", bmCompile, "Compile the current project or file");
	}*/

	//if(perms->isEnabled(PERM_DEBUG))
	//{
		t->AddSeparator();
		wxBitmap bmStart(newstart1_xpm);
		t->AddTool(ID_DEBUG_START, _guilang.get("GUI_TB_RUN"), bmStart, _guilang.get("GUI_TB_RUN_TTP"));

		wxBitmap bmStop(newstop1_xpm);
		t->AddTool(ID_DEBUG_STOP, _guilang.get("GUI_TB_STOP"), bmStop, _guilang.get("GUI_TB_STOP_TTP"));

		/*wxBitmap bmContinue(newcontinue1_xpm);
		t->AddTool(ID_DEBUG_CONTINUE, "Continue", bmContinue, "Resume when paused at a breakpoint");*/

		/*wxBitmap bmStepNext(stepnext_xpm);
		t->AddTool(ID_DEBUG_STEPNEXT, "Step next", bmStepNext,
			"Run the next line of code, stepping into it if it's a function call");

		wxBitmap bmStepOver(stepover_xpm);
		t->AddTool(ID_DEBUG_STEPOVER, "Step over", bmStepOver,
			"Run the next line of code, but don't go inside if it's a function call");

		wxBitmap bmStepOut(stepout_xpm);
		t->AddTool(ID_DEBUG_STEPOUT, "Step out", bmStepOut,
			"Run all the code in the current function");*/

		t->AddSeparator();

		wxBitmap bmAddBreakpoint(breakpoint_xpm);
		t->AddTool(ID_DEBUG_ADDEDITORBREAKPOINT, _guilang.get("GUI_TB_ADD"), bmAddBreakpoint,
			_guilang.get("GUI_TB_ADD_TTP"));

		wxBitmap bmRemoveBreakpoint(breakpoint_octagon_disable_xpm);
		t->AddTool(ID_DEBUG_REMOVEEDITORBREAKPOINT, _guilang.get("GUI_TB_REMOVE"), bmRemoveBreakpoint,
			_guilang.get("GUI_TB_REMOVE_TTP"));

		wxBitmap bmClearBreakpoint(breakpoint_crossed_xpm);
		t->AddTool(ID_DEBUG_CLEAREDITORBREAKPOINTS, _guilang.get("GUI_TB_CLEAR"), bmClearBreakpoint,
			_guilang.get("GUI_TB_CLEAR_TTP"));

		for(int i = ID_DEBUG_IDS_FIRST; i < ID_DEBUG_IDS_LAST; i++)
		{
			t->EnableTool(i, false);
		}
	//}

	t->Realize();


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
	/**Permission* perms = m_options->GetPerms();

	int numPages = m_noteTerm->GetPageCount();
	for(int i = 0; i < numPages; i++)
	{
		wxNotebookPage* page = m_noteTerm->GetPage(0);
		page->Hide();
		m_noteTerm->RemovePage(0);
	}*/

//	m_container1->GetSizer()->Detach(m_watchPanel);
//	m_container2->GetSizer()->Detach(m_debugTermContainer);

	/*if(m_watchPanel->GetParent() == m_watchPanelSplitter)
	{
		m_watchPanelSplitter->Unsplit(m_debugTermContainer);
		m_watchPanelSplitter->RemoveChild(m_watchPanel);
	}*/

	//if(perms->isEnabled(PERM_TERMINAL))
	//{
		m_termContainer->Show();
		///m_noteTerm->AddPage(m_termContainer, "NumeRe-Konsole");

		//m_terminal->ProcessInput(4,"TEST");
	/*}
	else
	{
		m_termContainer->Hide();
	}*/

	/*if(perms->isEnabled(PERM_COMPILE))
	{
		m_outputPanel->Show();
		m_outputPanel->SetAdvanced(true);
		m_noteTerm->AddPage(m_outputPanel, "Compiler Output");
	}
	else
	{*/
		//m_outputPanel->Hide();
	//}

	/*if(false)//perms->isEnabled(PERM_DEBUG))
	{
		if(m_options->GetCombineWatchWindow())
		{
			m_watchPanelContainer->Show();

			m_watchPanel->Reparent(m_watchPanelSplitter);
			m_debugTermContainer->Reparent(m_watchPanelSplitter);

			m_watchPanelSplitter->SplitVertically(m_watchPanel, m_debugTermContainer);

			m_noteTerm->AddPage(m_watchPanelContainer, "Debug Information");

			m_container1->Hide();
			m_container2->Hide();
		}
		else
		{
			wxWindow* parent = m_watchPanel->GetParent();

			m_watchPanel->Show();
			m_debugTermContainer->Show();

			if(m_watchPanel->GetParent() != m_container1)
			{
				m_watchPanel->Reparent(m_container1);
				m_container1->GetSizer()->Add(m_watchPanel, wxSizerFlags().Expand().Proportion(1));
				m_debugTermContainer->Reparent(m_container2);
				m_container2->GetSizer()->Add(m_debugTermContainer, wxSizerFlags().Expand().Proportion(1));
			}

			m_noteTerm->AddPage(m_container1, "Variable Watches");
			m_noteTerm->AddPage(m_container2, "Debug Console");

			m_watchPanel->Refresh();
			m_debugTermContainer->Refresh();

			wxSize size = m_termContainer->GetSize();
			wxPoint origin(0, 0);
			m_debugTermContainer->SetPosition(origin);
			m_debugTermContainer->SetSize(size);

			m_watchPanelContainer->Hide();

			m_splitProjectEditor->UpdateSize();
		}
	}
	else
	{
		m_watchPanelContainer->Hide();
		m_watchPanel->Hide();
		m_debugTermContainer->Hide();
		m_container1->Hide();
		m_container2->Hide();
	}*/

	/**if(m_noteTerm->GetPageCount() == 0)
	{
		m_splitEditorOutput->Unsplit();
		m_noteTerm->Hide();
	}
	else*/
	{
		if(!m_splitEditorOutput->IsSplit())
		{
			///m_splitEditorOutput->SplitHorizontally(m_splitProjectEditor, m_termContainer, 0.6f);//-260);//-200);
			m_splitEditorOutput->SplitHorizontally(m_book, m_termContainer, 0.65f);//-260);//-200);
			///m_splitEditorOutput->SplitHorizontally(m_splitProjectEditor, m_noteTerm, 0.6f);//-260);//-200);
			m_splitEditorOutput->SetMinimumPaneSize(20);
			m_terminal->UpdateSize();
			m_termContainer->Show();
			///m_noteTerm->Show();
		}
		///m_noteTerm->SetSelection(0);
		///m_noteTerm->GetPage(0)->SetFocus();
	}
	m_termContainer->Refresh();
	///m_noteTerm->Refresh();
	m_book->Refresh();

	//m_watchPanel->Refresh();
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
	wxTreeItemId clickedItem = event.GetItem();
	m_clickedTreeItem = clickedItem;
	wxMenu popupMenu;
	wxString editableExt = ".dat;.txt;.nscr;.nprc;.dx;.jcm;.jdx;.csv;";
	wxString loadableExt = ".dat;.txt;.dx;.jcm;.jdx;.xls;.xlsx;.ods;.ndat;.labx;.ibw;.csv;";

	//wxTreeItemId rootItem = m_projectTree->GetRootItem();
	//wxTreeItemId parentItem = m_projectTree->GetItemParent(clickedItem);
    wxString fname_ext = m_projectTree->GetItemText(m_clickedTreeItem);
    if (fname_ext.find('.') == string::npos)
        return;
    fname_ext = fname_ext.substr(fname_ext.rfind('.')) + ";";
	//bool rootClicked = (clickedItem == rootItem);

	/*if(m_projMultiFiles == NULL && !rootClicked)
	{
		wxMessageBox("You do not currently have a project open.", "No project open",
			wxOK | wxCENTRE | wxICON_EXCLAMATION);
		return;
	}*/

	/*if(parentItem == rootItem)
	{
		wxString label = "Add file";
		if(m_clickedTreeItem == m_projectFileFolders[0])
		{
			m_projectSelectedFolderType = FILE_NSCR;
			label = "Add source file";
		}
		else if(m_clickedTreeItem == m_projectFileFolders[1])
		{
			m_projectSelectedFolderType = FILE_NPRC;
			label = "Add header file";
		}
		else if(m_clickedTreeItem == m_projectFileFolders[2])
		{
			m_projectSelectedFolderType = FILE_DATAFILES;
			label = "Add library";
		}
		else if(m_clickedTreeItem == m_projectFileFolders[3])
		{
			m_projectSelectedFolderType = FILE_NONSOURCE;
			label = "Add non-source file";
		}

		popupMenu.Append(ID_PROJECT_ADDFILE, label);
	}*/
	// user right-clicked a file in the project
	/*else if(clickedItem == rootItem)
	{
		if(m_projMultiFiles != NULL)
		{
			popupMenu.Append(ID_COMPILE_PROJECT, "Compile this project");
			popupMenu.Append(ID_CLOSE_PROJECT, "Close this project");
		}
		else
		{
			Permission* perms = m_options->GetPerms();
			popupMenu.Append(ID_NEW_PROJECT, "Create a new project");
			if(perms->isEnabled(PERM_LOCALMODE))
			{
				popupMenu.Append(ID_OPEN_PROJECT_LOCAL, "Open a local project");
			}
			popupMenu.Append(ID_OPEN_PROJECT_REMOTE, "Open a remote project");
		}
	}
	else*/
	{
		//if(parentItem == m_projectFileFolders[0] || parentItem == m_projectFileFolders[1]) // data oder save
		{
            if (loadableExt.find(fname_ext) != string::npos)
                popupMenu.Append(ID_OPEN_FILE, _guilang.get("GUI_TREE_PUP_LOAD"));
            else if (fname_ext == ".nscr;")
                popupMenu.Append(ID_OPEN_FILE, _guilang.get("GUI_TREE_PUP_START"));
            else if (fname_ext == ".nprc;")
                popupMenu.Append(ID_OPEN_FILE, _guilang.get("GUI_TREE_PUP_RUN"));

			if (editableExt.find(fname_ext) != string::npos)
                popupMenu.Append(ID_EDIT_FILE, _guilang.get("GUI_TREE_PUP_EDIT"));
		}
		/*else if(parentItem == m_projectFileFolders[2]) // scripts
		{
			m_projectSelectedFolderType = FILE_DATAFILES;
		}
		else if(parentItem == m_projectFileFolders[3]) // procedures
		{
			m_projectSelectedFolderType = FILE_NONSOURCE;
		}

		popupMenu.Append(ID_PROJECT_REMOVEFILE, "Remove file from project");

		if(m_projectSelectedFolderType != FILE_DATAFILES)
		{
			FileNameTreeData* data = static_cast <FileNameTreeData* > (m_projectTree->GetItemData(clickedItem));
			if(m_projMultiFiles->FileIncludedInBuild(data->filename, m_projectSelectedFolderType))
			{
				popupMenu.Append(ID_PROJECT_EXCLUDE_FILE, "Exclude file from compilation");
			}
			else
			{
				popupMenu.Append(ID_PROJECT_INCLUDE_FILE, "Include file in compilation");
			}
		}*/
	}

	wxPoint p = event.GetPoint();
	m_projectTree->PopupMenu(&popupMenu, p);
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
		wxLogDebug("No files selected when adding to project");
		return;
	}

	m_projectTree->Freeze();
	if(m_projMultiFiles != NULL)
	{
		CloseProjectFile();
	}

	m_projMultiFiles = new ProjectInfo(false);

	m_projectTree->DeleteChildren(m_projectFileFolders[0]);
	m_projectTree->DeleteChildren(m_projectFileFolders[1]);
	m_projectTree->DeleteChildren(m_projectFileFolders[2]);
	m_projectTree->DeleteChildren(m_projectFileFolders[3]);

	wxString fileContents;
	wxString fileName;

	GetFileContents(fileNames[0], fileContents, fileName);


	// set the current project's base path based on the project file's directory,
	// choosing the path separators based on where the file is stored

	wxFileName projectFileName(fileNames[0]);

	m_projMultiFiles->SetProjectFile(projectFileName);
	m_projMultiFiles->SetRemote(m_remoteMode);

	wxTreeItemId rootItem = m_projectTree->GetRootItem();
	m_projectTree->SetItemText(rootItem, fileName);
	m_projectTree->SetItemBold(rootItem);

	wxMemoryInputStream projectFileStream(fileContents, fileContents.Len());

	wxFileConfig config(projectFileStream);

	LoadFilesIntoProjectTree("/Sources", FILE_NSCR, m_projectFileFolders[0], config, currentPathFormat);
	LoadFilesIntoProjectTree("/Headers", FILE_NPRC, m_projectFileFolders[1], config, currentPathFormat);
	LoadFilesIntoProjectTree("/Libraries", FILE_DATAFILES, m_projectFileFolders[2], config, currentPathFormat);
	LoadFilesIntoProjectTree("/Other", FILE_NONSOURCE, m_projectFileFolders[3], config, currentPathFormat);

	m_projectTree->ExpandAll();

	m_projectTree->Thaw();
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
		wxLogDebug("No files selected when adding to project");
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

	m_projectTree->AppendItem(m_clickedTreeItem, simpleName, iconIndex, -1, data);
	m_projectTree->SortChildren(m_clickedTreeItem);
	m_projectTree->Refresh();

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
	wxTreeItemId item = event.GetItem();

	if(item == m_projectFileFolders[0]
		|| item == m_projectFileFolders[1]
		|| item == m_projectFileFolders[2]
		|| item == m_projectFileFolders[3])
	{
		return;
	}

	wxTreeItemId rootItem = m_projectTree->GetRootItem();

	if( item != rootItem)
	{
		wxTreeItemId parentItem = m_projectTree->GetItemParent(item);

		//if( (parentItem == m_projectFileFolders[0]) ||
		//	(parentItem == m_projectFileFolders[1]) )

		// allow opening anything but .lib files
		if(parentItem != m_projectFileFolders[0] && parentItem != m_projectFileFolders[1])
		{
			FileNameTreeData* data = static_cast <FileNameTreeData* > (m_projectTree->GetItemData(item));

			wxArrayString filesToOpen;
			wxString pathname = data->filename;
			filesToOpen.Add(pathname);

			OpenSourceFile(filesToOpen);
		}
		else// if(parentItem == m_projectFileFolders[2])
		{
            FileNameTreeData* data = static_cast <FileNameTreeData* > (m_projectTree->GetItemData(item));
            wxString pathname = "append \"" + data->filename + "\"";
            wxArrayString filesToOpen;
            filesToOpen.Add(data->filename);
            if (data->filename.rfind(".txt") != string::npos || data->filename.rfind(".dat") != string::npos)
                OpenSourceFile(filesToOpen);
            else
                m_terminal->pass_command(pathname.ToStdString());
			//wxMessageBox("Only source files can be opened", "Chameleon", wxOK | wxICON_INFORMATION);
			return;
		}
	}
	m_currentEd->Refresh();
	m_book->Refresh();
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
		m_projectTree->DeleteChildren(m_projectFileFolders[i]);
	}

	m_projectTree->SetItemText(m_projectTree->GetRootItem(), "No project");

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

			m_projectTree->AppendItem(treeid, newFileName.GetFullName(), iconNum, -1, data);

			m_projMultiFiles->AddFileToProject(data->filename, fileType);

			int pageNum = GetPageNum(newFileName);

			if(pageNum != -1)
			{
				NumeReEditor* edit = static_cast<NumeReEditor*> (m_book->GetPage(pageNum));
				edit->SetProject(m_projMultiFiles);
			}
		}
	}

	m_projectTree->SortChildren(treeid);
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
    DirTraverser _traverser(m_projectTree, m_iconManager, treeid, fromPath, fileType);
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
	FileNameTreeData* treeData = static_cast<FileNameTreeData*> (m_projectTree->GetItemData(m_clickedTreeItem));

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

		m_projectTree->Delete(m_clickedTreeItem);

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
			wxLogDebug("NET_ERROR_MESSAGE: %s", statusDetails);
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
	bool doCompile = true;

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

	if(doCompile)
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

		/*
		if(!projToCompile->IsRemote())
		{
			wxMessageBox("Local compiling has not been implemented.\nPlease save this file to a network server\nand try again",
						"Can't Compile Locally", wxOK | wxICON_WARNING);
			return;
		}
		*/


		//m_outputPanel->ClearOutput();

		//m_compiler->CompileProject(projToCompile, m_outputPanel);

		/*int outputIndex = m_noteTerm->FindPagePosition(m_outputPanel);
		m_noteTerm->SetSelection(outputIndex);
		m_outputPanel->SetFocus();*/
	}
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

		if (pos >= 0)
			m_currentEd->SetFocus();
		else
		{
			wxMessageBox(_guilang.get("GUI_SEARCH_END", findString.ToStdString()),
				_guilang.get("GUI_SEARCH_END_HEAD"), wxOK | wxICON_EXCLAMATION, this);
		}
	}
	else if (type == wxEVT_COMMAND_FIND_REPLACE)
	{
		if (m_currentEd->GetSelectedText() != findString)
		{
			wxBell();
			return;
		}

		int pos = m_currentEd->GetSelectionStart();
		wxString replaceString = event.GetReplaceString();
		m_currentEd->ReplaceSelection(replaceString);
		m_currentEd->EnsureCaretVisible();
		m_currentEd->SetSelection(pos, pos + replaceString.Length());

		m_currentEd->SetFocus();

		// TODO Do a Find after this for the next item automatically?
	}
	else if (type == wxEVT_COMMAND_FIND_REPLACE_ALL)
	{
		wxString replaceString = event.GetReplaceString();
		if (findString == replaceString)
			return;

		wxBusyCursor busy;
		int count = ReplaceAllStrings(findString, replaceString, flags);

		wxMessageBox(_guilang.get("GUI_REPLACE_END", toString(count), findString.ToStdString(), replaceString.ToStdString()), _guilang.get("GUI_REPLACE_END_HEAD"), wxOK, this);
	}
	else if (type == wxEVT_COMMAND_FIND_CLOSE)
	{
		if (wxDynamicCast(event.GetEventObject(), wxDialog))
		{
			((wxDialog*)event.GetEventObject())->Destroy();
		}
		m_findReplace = NULL;
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
			m_currentEd->EnsureCaretVisible();
			m_currentEd->SetSelection(pos, pos + findString.Length());
		}
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
	if( id == ID_OPEN_SOURCE_LOCAL)
	{
		m_remoteMode = false;
	}
	else if(id == ID_OPEN_SOURCE_REMOTE)
	{
		m_remoteMode = true;
	}

	wxArrayString fnames = OpenFile(FILE_ALLSOURCETYPES);

	if(fnames.Count() > 0)
	{
		OpenSourceFile (fnames);
	}
}

void NumeReWindow::OnSaveSourceFile( int id )
{
	if( id == ID_SAVE_SOURCE_LOCAL)
	{
		m_remoteMode = false;
	}
	else if(id == ID_SAVE_SOURCE_REMOTE)
	{
		m_remoteMode = true;
	}
	SaveFile(true, false, FILE_ALLSOURCETYPES);
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
		wxLogDebug("Connected: %d", m_terminal->IsConnected());

		// Focus on the terminal
		///int terminalIndex = m_noteTerm->FindPagePosition(m_termContainer);
		///m_noteTerm->SetSelection(terminalIndex);
		m_terminal->SetFocus();
	}
	else if(isok == NET_AUTH_FAILED)
	{
		wxLogDebug("User gave us an invalid password");
		wxMessageBox("Your password was not accepted.  Perhaps you typed it wrong?", "Invalid Password",
		wxOK | wxICON_HAND);

		// reset the password to blank so we go into CheckForBlankPassword()
		m_options->SetPassphrase(wxEmptyString);
		goto checkPassword;
	}
	else
	{
		wxLogDebug("Tried to start Terminal with invalid networking status: %d", isok);
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

void NumeReWindow::OnFindReplace(int id)
{
	if(m_findReplace != NULL)
	{
		bool isReplaceDialog = m_findReplace->GetWindowStyle() & wxFR_REPLACEDIALOG;

		if( (isReplaceDialog && (id == ID_REPLACE)) ||
			(!isReplaceDialog && (id == ID_FIND)) )
		{
			return;
		}
		else
		{
			delete m_findReplace;
		}
	}

	bool showFind = (id == ID_FIND);
	int dialogFlags = showFind ? 0 : wxFR_REPLACEDIALOG;
	wxString title = showFind ? _guilang.get("GUI_DLG_FIND") : _guilang.get("GUI_DLG_REPLACE");

	m_findData.SetFlags(wxFR_DOWN);

	m_findReplace = new wxFindReplaceDialog(this,	&m_findData, title, dialogFlags);
	m_findReplace->Show(true);
}

void NumeReWindow::OnOptions()
{
	if(m_terminal->IsConnected())
	{
		m_optionsDialog->DisableServerSettings();
	}
	else
	{
		m_optionsDialog->EnableServerSettings();
	}
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
    }
	m_currentEd->SetFocus();
}

void NumeReWindow::OnProjectIncludeExcludeFile( int id )
{
	bool include = (id == ID_PROJECT_INCLUDE_FILE);
	FileNameTreeData* data = static_cast <FileNameTreeData* > (m_projectTree->GetItemData(m_clickedTreeItem));
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
	m_projectTree->SetItemImage(m_clickedTreeItem, iconIndex);
}

void NumeReWindow::OnPrintPage()
{
	m_currentEd->SetPrintColourMode(m_options->GetPrintStyle());
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
	m_helpController->DisplayContents();
}

void NumeReWindow::OnAbout()
{
	AboutChameleonDialog acd(this, 10000, _guilang.get("GUI_ABOUT_TITLE"));
	acd.ShowModal();
	//m_watchPanel->TestParsing();
}



#if wxUSE_DRAG_AND_DROP

bool ChameleonFileDropTarget::OnDropFiles(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y),
												const wxArrayString& filenames)
{
	wxCHECK_MSG(m_owner, false, wxT("Invalid drop target"));
	const size_t count = filenames.GetCount();
	if (count == 0)
		return false;

	m_owner->SetRemoteMode(false);
	m_owner->OpenSourceFile(filenames);
	return true;
}

#endif //wxUSE_DRAG_AND_DROP
