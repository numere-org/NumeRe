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

#include <wx/treectrl.h>
#include <wx/aui/auibook.h>
#include <wx/snglinst.h>
#include <vector>
#include <utility>

#include "NumeReStatusbar.hpp"
#include "globals.hpp"
#include "wx/dnd.h"
#include "../common/datastructures.h"
#include "../common/filewatcher.hpp"
#include "../kernel/windowmanager.hpp"
#include "../kernel/core/ui/language.hpp"
#include "../kernel/core/utils/tools.hpp"
#include "../kernel/core/plotting/graph_helper.hpp"
#include "../kernel/core/datamanagement/container.hpp"
#include "../kernel/core/datamanagement/table.hpp"
#include "dialogs/tipdialog.hpp"
#include "compositions/treepanel.hpp"
#include "compositions/viewerframe.hpp"
#include "compositions/viewerbook.hpp"
#include "compositions/filetree.hpp"
#include "compositions/variableviewer.hpp"
#include "compositions/procedureviewer.hpp"

#define OPENFILE_NOTHING 0
#define OPENFILE_BLACKLIST_ADD 1
#define OPENFILE_BLACKLIST_REMOVE 2
#define OPENFILE_FORCE 4

/** \mainpage NumeRe: Framework fuer Numerische Rechnungen
 *
 * \section intro Introduction
 * NumeRe: Framework fuer Numerische Rechnungen is a framework written for
 * Microsoft Windows(R) and released under the GNU GPL v3 for solving and
 * visualizing mathematical and physical problems numerically.
 *
 * NumeRe is based upon muParser, MathGL, BasicExcel, TinyXML-2, the Boost
 * C++-Library & the GNU Scientific Library
 *
 * \section license License
 *
 * NumeRe: Framework fuer Numerische Rechnungen
 * Copyright (C) 2017  Erik Haenel et al.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 *
 * \section depend Dependencies
 *
 * This section lists the dependencies of this application
 *
 * \subsection compiler Compiler
 *
 * This application was developed with TDM-GCC 4.9.2, which was derived
 * from the MinGW GCC with the identical version number. In principle,
 * this source code should compile with each gcc port.
 *
 * \subsection gui GUI-Framework
 *
 * This application uses wxWidgets v3.0.3 as GUI framework.
 *
 * \subsection external External Libraries
 *
 * You'll need the boost libaries and MathGL to compile this application.
 *
 */



// forward declarations

class wxFileConfig;
class EditorNotebook;
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
class OptionsDialog;
class wxFileConfig;
class wxTelnet;
class wxMenu;
class wxTreeItemId;
class wxImageList;
class wxNotebookEvent;
class wxSplitterEvent;
class wxTreeEvent;
class wxStatusBar;
class Options;
class NumeReTerminal;
class wxTermContainer;
class VariableWatchPanel;
class wxTimer;
class wxListCtrl;
class wxBitmap;
class CompilerOutputPanel;
class IconManager;
class ProportionalSplitterWindow;
class wxCHMHelpController;
class DebugViewer;

namespace DDE
{
    class Server;
}




//----------------------------------------------------------------------

/////////////////////////////////////////////////
/// \brief This class represents the application
/// controller, which will create the main frame
/// of the actual application. It will also
/// include the single instance checking and the
/// necessary DDE command interface handling.
/////////////////////////////////////////////////
class MyApp : public wxApp
{
    public:
        ~MyApp();
        virtual bool OnInit() override;
        virtual int OnExit() override;
        virtual bool OnExceptionInMainLoop() override;

    private:
        wxSingleInstanceChecker* m_singlinst;
        DDE::Server* m_DDEServer;
        //virtual int OnRun();
        //virtual void OnUnhandledException();
};

//----------------------------------------------------------------------


extern Language _guilang;


/////////////////////////////////////////////////
/// \brief This class is the actual NumeRe main
/// frame. The application's logic is implemented
/// here.
/////////////////////////////////////////////////
class NumeReWindow : public wxFrame
{
    public:
        NumeReWindow(const wxString& title, const wxPoint& pos, const wxSize& size);

        ~NumeReWindow();

        void SetIntVar(int variableName, int value);
        int GetIntVar(int variableName);

        void NewFile(FileFilterType _filetype = FILE_NONSOURCE, const wxString& defaultfilename = "");
        void ShowRevision(const wxString& revisionName, const wxString& revisionContent);
        void DefaultPage();
        void OpenFileByType(const wxFileName& filename);
        void OpenFilesFromList(const wxArrayString& filenameslist);
        void OpenSourceFile(wxArrayString fnames, unsigned int nLine = 0, int nOpenFileFlag = OPENFILE_NOTHING);
        void openImage(wxFileName filename);
        void openPDF(wxFileName filename);
        bool ShowHelp(const wxString& sDocId);
        void openHTML(wxString HTMLcontent);
        void openTable(NumeRe::Container<std::string> _stringTable, const std::string& tableDisplayName, const std::string& sIntName);
        void openTable(NumeRe::Table _table, const std::string& tableDisplayName, const std::string& sIntName);
        void editTable(NumeRe::Container<std::string> _stringTable, const std::string& tableDisplayName);
        void editTable(NumeRe::Table _table, const std::string& tableDisplayName);
        void showTable(const wxString& tableName, const wxString& tableDisplayName);
        void showWindow(NumeRe::Window& window);
        void pass_command(const wxString& command, bool isEvent = false);
        void evaluateDebugInfo(const std::vector<std::string>& vDebugInfo);
        void createLaTeXFile();
        void runLaTeX();
        void compileLaTeX();
        void runFromLine(wxCommandEvent &event);

        void refreshFunctionTree();

        NumeReEditor* GetCurrentEditor();

        void PassImageList(wxImageList* imagelist);

        MyTipProvider* tipProvider;
        bool showTipAtStartup;

        void updateTipAtStartupSetting(bool bTipAtStartup);
        void EvaluateOptions();
        void EvaluateCommandLine(wxArrayString& wxArgV);
        wxString getProgramFolder();
        void AddToHistory(const wxString& sCommand);
        wxString GetDocContent(wxString docid);
        std::vector<std::string> GetDocIndex();
        void forceHistoryPageDown();

        std::string m_UnrecoverableFiles;

        NumeReTerminal* getTerminal() {return m_terminal;}
        std::vector<std::string> getPathDefs();

        void addToReloadBlackList(const wxString& sFilename);
        void removeFromReloadBlackList(const wxString& sFilename);
        bool isOnReloadBlackList(wxString sFilename);
        bool GetFileContents(wxString fileToLoad, wxString &fileContents, wxString &fileName);

        wxTreeItemId getDragDropSourceItem();
        wxString getTreePath(const wxTreeItemId& itemId);
        wxString getTreeFolderPath(const wxTreeItemId& itemId);

        void Ready();
        void Busy();

        void OnPrintSetup();
        wxRect DeterminePrintSize();
        Options* getOptions() {return m_options;}
        void FindAndOpenProcedure(const wxString& procedureName);

        void UpdateLocationIfOpen(const wxFileName& fname, const wxFileName& newFName);
        void reloadFileIfOpen(const wxString& fname, bool force = false);

        void registerWindow(wxWindow* window, WindowType type);
        void unregisterWindow(wxWindow* window);
        wxWindow* findWindow(WindowType type);
        void closeWindows(WindowType type = WT_ALL);
        wxIcon getStandardIcon();
        wxBitmap getToolbarIcon(const wxString& iconName);

        void notifyInstallationDone();


    private:
        void InitializeProgramOptions();
        void prepareSession();

        int CopyEditorSettings(FileFilterType _fileType);

        void OnMenuEvent(wxCommandEvent& event);
        void OnPluginMenuEvent(wxCommandEvent& event);
        void OnAskForNewFile();
        void OnHelp();
        void OnAbout();
        void OnPrintPreview();
        void OnPrintPage();
        void OnOptions();
        void OnFindReplace(int id );
        void OnCopyAsPath();
        void OnOpenInExplorer();
        void OnShowRevisions();
        void OnShowRevisionsFromTab();
        void OnTagCurrentRevision();
        void OnCreateNewFolder();
        void OnRemoveFolder();
        void OnSaveSourceFile( int id );
        void OnOpenSourceFile( int id );
        void OnExecuteFile(const std::string& sFileName, int id);
        void OnCalculateDependencies();
        void OnCreatePackage(const wxString& projectFile);
        void OnCompareFiles();

        void OnClose(wxCloseEvent& event);
        void Test(wxCommandEvent& event);

        void OnUpdateSaveUI();
        void ToolbarStatusUpdate();
        void OnIdle(wxIdleEvent &event);
        void UpdateStatusBar();

        void OnStatusTimer(wxTimerEvent &event);
        void OnFileEventTimer(wxTimerEvent& event);

        void OnSplitterDoubleClick(wxSplitterEvent &event);

        void OnTreeItemRightClick(wxTreeEvent& event);
        void OnTreeItemActivated(wxTreeEvent& event);
        void OnTreeItemToolTip(wxTreeEvent& event);
        void OnTreeDragDrop(wxTreeEvent& event);

        wxString addLinebreaks(const wxString& sLine);
        wxString getFileDetails(const wxFileName& filename);

        void OnPageChange(wxAuiNotebookEvent& event);

        void OnFindEvent(wxFindDialogEvent& event);

        void OnFileSystemEvent(wxFileSystemWatcherEvent& event);
        void CreateProcedureTree(const std::string& sProcedurePath);

        void showGraph(NumeRe::Window& window);
        void showFileDialog(NumeRe::Window& window);
        void showDirDialog(NumeRe::Window& window);
        void showTextEntry(NumeRe::Window& window);
        void showMessageBox(NumeRe::Window& window);
        void showListDialog(NumeRe::Window& window);
        void showSelectionDialog(NumeRe::Window& window);
        void showListEditDialog(NumeRe::Window& window);

        wxArrayString OpenFile(FileFilterType filterType );
        bool SaveCurrentFile(bool saveas);
        bool SaveTab(int tab);
        bool SaveAll(bool refreshLibrary);
        void CloseFile(int pageNr = -1, bool askforsave = true);
        bool CloseAllFiles();
        void CloseTab(bool force);
        void CloseOtherTabs();
        void OpenContainingFolder();
        void EvaluateTab();
        void PageHasChanged (int pageNr = -1);
        int GetPageNum(wxFileName fn, bool compareWholePath = true, int startingTab = 0);
        int HandleModifiedFile(int pageNr, ModifiedFileAction fileAction);

        void LoadFilesToTree(wxString fromPath, FileFilterType fileType, wxTreeItemId treeid);

        std::vector<std::string> getFileForInstaller(const std::string& sLayoutFileName);
        wxString ConstructFilterString(FileFilterType filterType);
        int FindString(const wxString &findString, int start_pos = -1, int flags = -1, bool highlight = TRUE);
        int ReplaceAllStrings(const wxString &findString, const wxString &replaceString, int flags = -1);

        void deleteFile();
        void insertCopiedFile();
        void renameFile();

        void UpdateMenuBar();
        void UpdatePackageMenu();
        void UpdateToolbar();
        void UpdateTerminalNotebook();
        void UpdateVarViewer();
        void UpdateWindowTitle(const wxString& filename);
        void toggleConsole();
        void toggleFiletree();
        void toggleHistory();
        void showConsole();
        void gotoLine();
        void setEditorFocus();
        void setViewerFocus();

        void prepareFunctionTree();
        std::string prepareTooltip(const std::string& sTooltiptext);

        wxPrintData* setDefaultPrinterSettings();
        int* SelectIntVar(int variableName);

        /*! Displays the output from wxLogDebug calls */
        wxLogWindow* logWindow;

        // main widgets
        /*! Main editor notebook */
        EditorNotebook* m_book;
        /*! Notebook for the terminal and debug-related widgets */
        ViewerBook*  m_noteTerm;
        /*! Notebook for the tree controls on the right side */
        ViewerBook* m_treeBook;
        /*! Pointer to the currently active editor */
        //NumeReEditor* m_currentEd;

        NumeReHistory* m_history;
        VariableViewer* m_varViewer;
        ProcedureViewer* m_procedureViewer;

        ViewerFrame* m_currentView;
        /*! Displays the files in the current project */
        FileTree* m_fileTree;
        Filewatcher* m_watcher;
        TreePanel* m_filePanel;
        TreePanel* m_functionPanel;

        // Blacklist for suppressing the reloading and the corresponding dialog
        std::vector<wxString> vReloadBlackList;

        /*! Displays functions and commands */
        FileTree* m_functionTree;
        /*! The status bar */
        NumeReStatusbar* m_statusBar;
        //wxPanel* panelEd;
        /*! Holds the editor/project group and the output notebook */
        ProportionalSplitterWindow* m_splitEditorOutput;
        /*! Holds the command window and the history window*/
        ProportionalSplitterWindow* m_splitCommandHistory;
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
        NumeReTerminal* m_terminal;

        wxPanel* m_watchPanelContainer;
        wxPanel* m_container1;
        wxPanel* m_container2;
        ProportionalSplitterWindow* m_watchPanelSplitter;

        // dialogs
        OptionsDialog*  m_optionsDialog;

        /*! Used to initiate UI updates */
        wxTimer* m_updateTimer;
        wxTimer* m_fileEventTimer;

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
        wxTreeItemId m_dragDropSourceItem;

        Options* m_options;
        DebugViewer* m_debugViewer;
        IconManager* m_iconManager;

        int m_numPages;
        /*! Tracks the active editor tab */
        int m_currentPage;
        /*! Tracks the number of new files created - used to identify <untitled> files */
        int m_fileNum;
        /*! The last clicked tab in the editor notebook */
        int m_clickedTabNum;
        int m_splitterPos;

        bool m_appClosing;
        bool m_sessionSaved;
        bool m_setSelection;
        bool m_remoteMode;
        bool m_appStarting;
        bool m_currentlyDebugging;
        bool m_multiRowState;
        bool m_loadingFilesDuringStartup;
        std::map<int, wxMenuItem*> m_menuItems;
        std::vector<std::pair<int, wxString> > m_modifiedFiles;
        std::map<wxWindow*, WindowType> m_openedWindows;
        std::map<size_t, std::string> m_pluginMenuMap;


        wxString m_filterNSCRFiles;
        wxString m_filterNPRCFiles;
        wxString m_filterNLYTFiles;
        wxString m_filterNumeReFiles;
        wxString m_filterExecutableFiles;
        wxString m_filterDataFiles;
        wxString m_filterImageFiles;
        wxString m_filterAllFiles;
        wxString m_filterTeXSource;
        wxString m_filterNonsource;
        wxString m_filterSupportedFiles;

        std::string m_lastIconStyle;
        bool m_lastToolbarStretchState;

        std::map<wxString, __time64_t> m_filesLastSaveTime;
        wxString m_fileToRefresh;


        DECLARE_EVENT_TABLE()


};



#endif
