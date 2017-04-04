#ifndef CHAMELEONWINDOW__H
#define CHAMELEONWINDOW__H

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "../common/datastructures.h"
#include "../common/DebugEvent.h"
#include "../common/globals.hpp"
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include "../common/filewatcher.hpp"
#include "wx/dnd.h"
#include "../kernel/core/language.hpp"
#include "../kernel/core/tools.hpp"
#include "dialogs/tipdialog.hpp"
#include "viewerframe.hpp"
#include "viewerbook.hpp"
#include "filetree.hpp"

// forward declarations

class wxFileConfig;
class NumeReNotebook;
class wxTreeCtrl;
class wxTextCtrl;
class NumeReEditor;
class NumeReHistory;
class wxLogWindow;
class wxTabCtrl;
class IntIntHashmap;
class WindowPointerArray;
class wxPanel;
class wxSplitterWindow;
class RemoteFileDialog;
class OptionsDialog;
class Permission;
class Networking;
class Compiler;
class wxFileConfig;
class wxTelnet;
class wxMenu;
class wxTreeItemId;
class wxImageList;
class wxNotebookEvent;
class wxSplitterEvent;
class wxTreeEvent;
class wxStatusBar;
class ProjectInfo;
class wxSSH;
class Options;
class wxTermContainer;
class Debugger;
class VariableWatchPanel;
class wxTimer;
class wxListCtrl;
class CompilerOutputPanel;
class IconManager;
class wxProportionalSplitterWindow;
class wxCHMHelpController;
class DebugManager;
class DebugViewer;
class ChameleonProjectManager;




//----------------------------------------------------------------------

class MyApp : public wxApp
{
    public:
        ~MyApp();
        virtual bool OnInit();
        //virtual int OnRun();
        //virtual void OnUnhandledException();
};

//----------------------------------------------------------------------


extern Language _guilang;


// Define a new frame type: this is going to be our main frame
class NumeReWindow : public wxFrame
{
    public:
        NumeReWindow(const wxString& title, const wxPoint& pos, const wxSize& size);

        ~NumeReWindow();

        void SetIntVar(int variableName, int value);
        int GetIntVar(int variableName);

        bool IsEnabled(int permission);
        bool IsDebugging();
        bool IsDebuggerPaused();

        bool GetRemoteMode()
        {
            return m_remoteMode;
        }
        void SetRemoteMode(bool remote)
        {
            m_remoteMode = remote;
        }

        void FocusOnLine(wxString filename, int linenumber, bool showMarker = true, wxString linecontents = wxEmptyString);

        NetworkCallResult CheckNetworkStatus();

        void NewFile(FileFilterType _filetype = FILE_NONSOURCE, const wxString& defaultfilename = "");
        void OpenSourceFile(wxArrayString fnames, unsigned int nLine = 0);
        void openImage(wxFileName filename);
        void openHTML(wxString HTMLcontent);
        void openTable(const vector<vector<string> >& sTable, const string& sTableName);
        void editTable(const vector<vector<string> >& sTable, const string& sTableName);
        void evaluateDebugInfo(const vector<string>& vDebugInfo);

        Networking* GetNetworking();

        void PassImageList(wxImageList* imagelist);

        MyTipProvider* tipProvider;
        bool showTipAtStartup;
        void updateTipAtStartupSetting(bool bTipAtStartup);
        void EvaluateOptions();
        void EvaluateCommandLine(wxArrayString& wxArgV);
        wxString getProgramFolder();
        void AddToHistory(const wxString& sCommand);
        wxString GetDocContent(wxString docid);

        string m_UnrecoverableFiles;

    private:
        void InitializeProgramOptions();

        void CheckSize();

        void OnMenuEvent(wxCommandEvent &event);

        void OnHelp();
        void OnAbout();
        void OnPrintSetup();
        void OnPrintPreview();
        void OnPrintPage();
        void OnProjectIncludeExcludeFile( int id );
        void OnOptions();
        void OnFindReplace(int id );
        void OnCopy();
        void OnStartConnect();
        void OnSaveSourceFile( int id );
        void OnOpenSourceFile( int id );
        void OnDebugCommand(wxCommandEvent &event);
        void OnDebugBreakpointCommand(wxCommandEvent &event);

        void OnDebugEvent(wxDebugEvent &event);

        void OnClose(wxCloseEvent& event);
        void Test(wxCommandEvent& event);

        void OnUpdateSaveUI();//wxUpdateUIEvent &event);
        void OnUpdateDebugUI();//wxUpdateUIEvent &event);
        void OnUpdateConnectionUI();//wxUpdateUIEvent &event);
        void OnUpdateCompileUI();//wxUpdateUIEvent &event);
        void OnUpdateProjectUI();//wxUpdateUIEvent &event);
        void OnIdle(wxIdleEvent &event);
        void UpdateStatusBar();

        void OnStatusTimer(wxTimerEvent &event);

        void OnSplitterDoubleClick(wxSplitterEvent &event);
        void OnTermResize(wxSplitterEvent &event);
        void OnSize(wxSizeEvent &event);

        void OnTreeItemRightClick(wxTreeEvent& event);
        void OnTreeItemActivated(wxTreeEvent& event);
        void OnTreeItemToolTip(wxTreeEvent& event);
        void OnTreeDragDrop(wxTreeEvent& event);

        wxString addLinebreaks(const wxString& sLine);

        void OnPageChange(wxBookCtrlEvent& event);

        void OnFindEvent(wxFindDialogEvent& event);
        /*void addFileToTree(const std::string& sFilePath);
        void removeFileFromTree(const std::string& sFilePath);*/

        void OnFileSystemEvent(wxFileSystemWatcherEvent& event);

        wxArrayString OpenFile(FileFilterType filterType );
        bool SaveFile(bool saveas, bool askLocalRemote, FileFilterType filterType);
        bool GetFileContents(wxString fileToLoad, wxString &fileContents, wxString &fileName);
        void CloseFile(int pageNr = -1, bool askforsave = true);
        void CloseAllFiles();
        void CloseTab();
        void PageHasChanged (int pageNr = -1);
        int GetPageNum(wxFileName fn, bool compareWholePath = true, int startingTab = 0);
        int HandleModifiedFile(int pageNr, ModifiedFileAction fileAction);

        void OpenProjectFile(bool isRemote);
        void AddFileToProject();
        void RemoveFileFromProject();
        void SaveProjectFile();
        void CloseProjectFile(bool canUserCancel = true);
        void LoadFilesIntoProjectTree(wxString configPath, FileFilterType fileType, wxTreeItemId treeid,
                                      wxFileConfig& config, wxPathFormat currentPathFormat);
        void LoadFilesToTree(wxString fromPath, FileFilterType fileType, wxTreeItemId treeid);

        void Compile();

        wxRect DeterminePrintSize();

        wxString ConstructFilterString(FileFilterType filterType);
        int FindString(const wxString &findString, int start_pos = -1, int flags = -1, bool highlight = TRUE);
        int ReplaceAllStrings(const wxString &findString, const wxString &replaceString, int flags = -1);

        void deleteFile();
        void insertCopiedFile();
        void renameFile();

        bool CheckForBlankPassword();
        bool AskUserForPassword();

        void UpdateMenuBar();
        void UpdateToolbar();
        void UpdateTerminalNotebook();
        void UpdateWindowTitle(const wxString& filename);
        void toggleConsole();
        void toggleFiletree();
        void toggleHistory();
        void showConsole();
        void gotoLine();
        void setEditorFocus();
        void setViewerFocus();

        void prepareFunctionTree();

        string prepareTooltip(const string& sTooltiptext);


        //void CleanupDropMenu();

        int* SelectIntVar(int variableName);

        /*! Displays the output from wxLogDebug calls */
        wxLogWindow* logWindow;

        // main widgets
        /*! Main editor notebook */
        NumeReNotebook* m_book;
        /*! Notebook for the terminal and debug-related widgets */
        NumeReNotebook*  m_noteTerm;
        /*! Notebook for the tree controls on the right side */
        ViewerBook* m_treeBook;
        /*! Pointer to the currently active editor */
        NumeReEditor* m_currentEd;

        NumeReHistory* m_history;

        ViewerFrame* m_currentView;
        /*! Displays the files in the current project */
        FileTree* m_projectTree;
        Filewatcher* m_watcher;

        /*! Displays functions and commands */
        FileTree* m_functionTree;
        /*! The status bar */
        wxStatusBar* m_statusBar;
        //wxPanel* panelEd;
        /*! Holds the editor/project group and the output notebook */
        wxProportionalSplitterWindow* m_splitEditorOutput;
        /*! Holds the command window and the history window*/
        wxProportionalSplitterWindow* m_splitCommandHistory;
        /*! Holds the project tree and the editor notebook */
        wxSplitterWindow* m_splitProjectEditor;
        /*! Displays the output from the compiler */
        CompilerOutputPanel* m_outputPanel;
        /*! The main terminal widget container*/
        wxTermContainer* m_termContainer;
        /*! The debug I/O widget container*/
        wxTermContainer* m_debugTermContainer;
        /*! Displays watched variables */
        VariableWatchPanel* m_watchPanel;
        /*! The actual terminal widget */
        wxSSH* m_terminal;
        /*! The actual debug I/O widget */
        wxSSH* m_debugTerminal;

        wxPanel* m_watchPanelContainer;
        wxPanel* m_container1;
        wxPanel* m_container2;
        wxProportionalSplitterWindow* m_watchPanelSplitter;

        // dialogs
        OptionsDialog*  m_optionsDialog;
        RemoteFileDialog* m_remoteFileDialog;


        /* Responsible for writing the Chameleon settings to the INI file */
        wxFileConfig* m_config;

        //wxImageList* m_tempImageList;
        /*! Used to initiate UI updates */
        wxTimer* m_updateTimer;

        //IntIntHashmap m_permNumMap;
        /*! Maps a file extension to the image list index for that extension's icon */
        StringIntHashmap m_extensionMappings;
        //WindowPointerArray m_infoTabTracker;

        /*! Used to hold data for the find/replace dialogs */
        wxFindReplaceData m_findData;

        /*! The IDs for the file type folders in the project tree */
        wxTreeItemId m_projectFileFolders[5];
        /*! The last clicked item in the project tree */
        wxTreeItemId m_clickedTreeItem;
        /*! The last copied item in the project tree */
        wxTreeItemId m_copiedTreeItem;
        /*! The file type that corresponds to the last clicked project folder */
        FileFilterType m_projectSelectedFolderType;

        Networking* m_network;
        Compiler* m_compiler;
        Options* m_options;
        /*! The currently open multi-file project */
        ProjectInfo* m_projMultiFiles;
        Debugger* m_debugger;
        DebugManager* m_debugManager;
        DebugViewer* m_debugViewer;
        IconManager* m_iconManager;
        ChameleonProjectManager* m_projectManager;

        int m_numPages;
        /*! Tracks the active editor tab */
        int m_currentPage;
        /*! Tracks the number of new files created - used to identify <untitled> files */
        int m_fileNum;
        /*! The last clicked tab in the editor notebook */
        int m_clickedTabNum;
        int m_splitterPos;

        float fSplitPercentage;
        float fVerticalSplitPercentage;

        bool m_appClosing;
        bool m_sessionSaved;
        bool m_setSelection;
        bool m_remoteMode;
        bool m_appStarting;
        bool m_currentlyDebugging;
        bool m_compileProject;
        bool m_multiRowState;


        wxString m_filterNSCRFiles;
        wxString m_filterNPRCFiles;
        wxString m_filterNumeReFiles;
        wxString m_filterExecutableFiles;
        wxString m_filterDataFiles;
        wxString m_filterImageFiles;
        wxString m_filterAllFiles;
        wxString m_filterTeXSource;
        wxString m_filterNonsource;

        wxString m_currentSavedFile;


        DECLARE_EVENT_TABLE()


};




#if wxUSE_DRAG_AND_DROP
class  ChameleonFileDropTarget : public wxFileDropTarget
{
    public:
        ChameleonFileDropTarget(NumeReWindow *owner) : m_owner(owner) {}
        virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);

        NumeReWindow *m_owner;
};
#endif //wxUSE_DRAG_AND_DROP


#endif
