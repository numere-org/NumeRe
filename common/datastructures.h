// Datastructures.h - common data structures for the Chameleon project
#ifndef DATASTRUCTURES__H
#define DATASTRUCTURES__H

#define CHAMELEON_VERSION_NUM " 0.1beta4 "

#include <wx/wx.h>
#include <wx/hashmap.h>
#include <wx/string.h>
#include <wx/dynarray.h>
#include <wx/filename.h>
//#include <wx/treectrl.h>

class wxWindow;
class NumeReEditor;
class wxTextOutputStream;

enum VariableNames
{
	VN_NUMPAGES,
	VN_CURRENTPAGE,
	VN_CLICKEDTAB,
};

enum MarkerTypes
{
    MARKER_DUPLICATEINDICATOR_ONE,
    MARKER_DUPLICATEINDICATOR_TWO,
	MARKER_BOOKMARK,
    MARKER_MODIFIED,
    MARKER_SAVED,
	MARKER_BREAKPOINT,
	MARKER_FOCUSEDLINE,
	MARKER_SECTION
};


enum WindowID
{
	// menu items
	EVENTID_MENU_START = 5200,
	ID_MENU_QUIT = EVENTID_MENU_START,

	// File menu
	ID_MENU_NEW_EMPTY,
	ID_MENU_NEW_SCRIPT,
	ID_MENU_NEW_PROCEDURE,
	ID_MENU_NEW_PLUGIN,
	ID_MENU_SAVE,
	ID_MENU_SAVE_SOURCE_LOCAL,
	ID_MENU_SAVE_SOURCE_REMOTE,
	ID_MENU_OPEN_SOURCE_LOCAL,
	ID_MENU_OPEN_SOURCE_REMOTE,
	ID_MENU_OPEN_FILE_FROM_TREE,
	ID_MENU_OPEN_IMAGE_FROM_TREE,
	ID_MENU_DELETE_FILE_FROM_TREE,
	ID_MENU_COPY_FILE_FROM_TREE,
	ID_MENU_INSERT_FILE_INTO_TREE,
	ID_MENU_RENAME_FILE_IN_TREE,
	ID_MENU_NEW_FOLDER_IN_TREE,
	ID_MENU_REMOVE_FOLDER_FROM_TREE,
	ID_MENU_OPEN_IN_EXPLORER,
	ID_MENU_AUTOINDENT,
	ID_MENU_LINEWRAP,
	ID_MENU_DISPCTRLCHARS,
	ID_MENU_USETXTADV,
	ID_MENU_USEANALYZER,
	ID_MENU_INDENTONTYPE,
	ID_MENU_AUTOFORMAT,
	ID_MENU_FOLD_ALL,
	ID_MENU_UNFOLD_ALL,
	ID_MENU_EDIT_FILE_FROM_TREE,
	ID_MENU_USESECTIONS,
	ID_MENU_INSERT_IN_EDITOR_FROM_TREE,
	ID_MENU_INSERT_IN_CONSOLE_FROM_TREE,
	ID_MENU_HELP_ON_ITEM,
	ID_MENU_SHOW_DESCRIPTION,

	ID_MENU_PRINT_PAGE,
	ID_MENU_PRINT_PREVIEW,
	ID_MENU_PRINT_SETUP,

	ID_MENU_CLOSEPAGE,
	ID_MENU_CLOSEALL,
	ID_MENU_CLOSEOTHERS,
	ID_MENU_CLOSETAB,
	ID_MENU_OPEN_FOLDER,

	ID_MENU_TOGGLE_CONSOLE,
	ID_MENU_TOGGLE_FILETREE,
	ID_MENU_TOGGLE_HISTORY,

	ID_MENU_UNDO,
	ID_MENU_REDO,
	ID_MENU_COPY,
	ID_MENU_CUT,
	ID_MENU_PASTE,

	// Search menu
	ID_MENU_FIND,
	ID_MENU_REPLACE,
	ID_MENU_FIND_PROCEDURE,
	ID_MENU_FIND_INCLUDE,
	ID_MENU_GOTOLINE,

	ID_MENU_SELECTION_UP,
	ID_MENU_SELECTION_DOWN,
	ID_MENU_SORT_SELECTION_ASC,
	ID_MENU_SORT_SELECTION_DESC,
	ID_MENU_FIND_DUPLICATES,
	ID_MENU_STRIP_SPACES_BOTH,
	ID_MENU_STRIP_SPACES_FRONT,
	ID_MENU_STRIP_SPACES_BACK,
	ID_MENU_CREATE_LATEX_FILE,
	ID_MENU_RUN_LATEX,
	ID_MENU_COMPILE_LATEX,

	// tools menu items
	ID_MENU_OPTIONS,
	ID_MENU_TOGGLE_DEBUGGER,
	ID_MENU_TOGGLE_COMMENT_LINE,
	ID_MENU_TOGGLE_COMMENT_SELECTION,
	ID_MENU_TOGGLE_NOTEBOOK_MULTIROW,
	ID_MENU_EXECUTE,
	ID_MENU_STOP_EXECUTION,
	ID_MENU_RUN_FROM_TAB,
	ID_MENU_RENAME_SYMBOL,
	ID_MENU_ABSTRAHIZE_SECTION,

	ID_MENU_ABOUT,
	ID_MENU_HELP,

	ID_MENU_ADDEDITORBREAKPOINT,
	ID_MENU_REMOVEEDITORBREAKPOINT,
	ID_MENU_CLEAREDITORBREAKPOINTS,

    ID_MENU_BOOKMARK_NEXT,
    ID_MENU_BOOKMARK_PREVIOUS,
    ID_MENU_BOOKMARK_TOGGLE,
    ID_MENU_BOOKMARK_CLEARMENU,

	EVENTID_MENU_END,

	ID_NEW_PROJECT,
	ID_OPEN_PROJECT_LOCAL,
	ID_OPEN_PROJECT_REMOTE,
	ID_CLOSE_PROJECT,
	ID_REFACTORING_MENU,

	// major widgets
	ID_NOTEBOOK_ED,
	ID_NOTEBOOK_TERM,
	ID_CONTAINER_TERM,
	ID_CONTAINER_DEBUGTERM,
	ID_TAB_ED,
	ID_PANEL_ED,
	ID_ED,
	ID_TERMINAL,
	ID_SPLITEDITOROUTPUT,
	ID_SPLITPROJECTEDITOR,
	ID_PROJECTTREE,
	ID_FUNCTIONTREE,
	ID_OPTIONSDIALOG,
	ID_REMOTEFILEDIALOG,
	ID_COMPILERTEXTBOX,
	ID_COMPILERLISTCTRL,
	ID_COMPILEROUTPUT,
	ID_VARWATCHPANEL,
	ID_DEBUGTERMINAL,

	// edit menu items
	ID_INDENT,
	ID_UNINDENT,
	ID_SELECTALL,
	ID_SELECTLINE,

	ID_STARTCONNECT,
	ID_DISCONNECT,

	ID_COMPILE,
	ID_COMPILE_PROJECT,

	ID_STATUSTIMER,

	ID_TEST,

	ID_PROJECT_ADDFILE,
	ID_PROJECT_REMOVEFILE,
	ID_PROJECT_SETTINGS,

	ID_PROJECT_ADD_SOURCEFILE,
	ID_PROJECT_ADD_HEADERFILE,
	ID_PROJECT_ADD_LIBRARYFILE,
	ID_PROJECT_REMOVE_SOURCEFILE,
	ID_PROJECT_REMOVE_HEADERFILE,
	ID_PROJECT_REMOVE_LIBRARYFILE,

	ID_PROJECT_EXCLUDE_FILE,
	ID_PROJECT_INCLUDE_FILE,

	// debugging command IDs
	ID_DEBUG_IDS_FIRST,
	ID_DEBUG_CONTINUE,
	ID_DEBUG_STEPNEXT,
	ID_DEBUG_STEPOVER,
	ID_DEBUG_STEPOUT,
	ID_DEBUG_IDS_LAST,
	ID_DEBUG_MODIFYBREAKPOINTS,

	ID_DEBUG_ADD_BREAKPOINT,
	ID_DEBUG_REMOVE_BREAKPOINT,
	ID_DEBUG_CLEAR_ALL_BREAKPOINTS,
	ID_DEBUG_RUNTOCURSOR,
	ID_DEBUG_ADD_WATCH,
	ID_DEBUG_REMOVE_WATCH,
	ID_DEBUG_WATCH_SELECTION,
	ID_DEBUG_DISPLAY_SELECTION,
	ID_FIND_PROCEDURE,
	ID_FIND_INCLUDE,
	ID_UPPERCASE,
	ID_LOWERCASE,
	ID_FOLD_CURRENT_BLOCK,
	ID_RENAME_SYMBOLS,
	ID_ABSTRAHIZE_SECTION,

    ID_BOOKMARK_ADD,
    ID_BOOKMARK_REMOVE,
    ID_BOOKMARK_CLEAR,

	ID_DELETE_FROM_HISTORY,
	ID_COPY_FROM_HISTORY,

	ID_DEBUG_VARINFO,

	ID_DEBUG_BREAKPOINT,
	ID_DEBUG_EXIT_NORMAL,
	ID_DEBUG_EXIT_ERROR,

	// ImagePanel Ids
	ID_SAVEIMAGE,
	ID_COPYIMAGE,
	ID_NEXTIMAGE,
	ID_PREVIOUSIMAGE,

	// Graph window IDs
	ID_GRAPH_EXPORT,
	ID_GRAPH_COPY,
	ID_GRAPH_LIGHT,
	ID_GRAPH_ALPHA,
	ID_GRAPH_ZOOM,
	ID_GRAPH_ROTATE,
	ID_GRAPH_RESET,
	ID_GRAPH_NEXT,
	ID_GRAPH_PREVIOUS,
	ID_GRAPH_RUN,
	ID_GRAPH_STOP,

	// Graph draw tool ids
	ID_GRAPH_DRAW_FIRST,
	ID_GRAPH_LINE,
	ID_GRAPH_CIRCLE,
	ID_GRAPH_RECT,
	ID_GRAPH_TEXT,
	ID_GRAPH_DRAW_LAST
};

// IMPORTANT!!!  Any changes to this enum need to be reflected in the
//				 PermStrings array in p.cpp.
enum modules
{
	PERM_FIRST = 0,
	PERM_SYNTAXHIGHLIGHT = PERM_FIRST,
	PERM_AUTOINDENT,
	PERM_DEBUG,
	PERM_TERMINAL,
	PERM_LOCALMODE,
	PERM_PROJECTS,
	PERM_COMPILE,
	//PERM_ADVANCEDCOMPILE,
	PERM_LAST
};

// This enum should match the order of items in the wxImageList
// used for the project tree, as created in RemoteFileDialog's constructor
enum IconIndex
{
	ICON_DEFAULT,
	ICON_PROJECT = ICON_DEFAULT,
	ICON_FOLDERCLOSED,
	ICON_FOLDEROPEN,
	ICON_SOURCE_C,
	ICON_SOURCE_CPP,
	ICON_SOURCE_H,
	ICON_LIBRARY,
	ICON_DISABLED_SOURCE_C,
	ICON_DISABLED_SOURCE_CPP,
	ICON_DISABLED_SOURCE_H,
};

enum CompileResult {
	CR_ERROR = -1,
	CR_OK = 0,
	CR_TERMINATED = 1,
};

// IMPORTANT!!! Any changes to this enum need to be reflected in the
//				PermStrings array in p.cpp.


WX_DEFINE_ARRAY(wxWindow*, WindowPointerArray);
WX_DEFINE_ARRAY(NumeReEditor*, EditorPointerArray);
WX_DEFINE_ARRAY_INT(bool, BoolArray);


enum PathID {
    EXEPATH,
    WORKPATH,
    LOADPATH,
    SAVEPATH,
    SCRIPTPATH,
    PROCPATH,
    PLOTPATH
};


typedef struct
{
	wxArrayInt gdbNumbers;
	wxArrayInt lineNumbers;
} DebugBreakInfo;

//typedef struct
//{
//	bool functionHasBeenVisited;
//	BoolArray variableDisplayed;
//	wxArrayString variableNames;
//} FunctionVariablesInfo;

typedef struct
{
	wxString type;
	wxString name;
	wxString functionName;
	wxString value;
	wxString regexKey;
} VariableInfo;

typedef struct
{
	wxArrayString dirNames;
	wxArrayString fileNames;
} DirListing;

WX_DECLARE_HASH_MAP( int,
					int,
					wxIntegerHash,
					wxIntegerEqual,
					IntIntHashmap );

WX_DECLARE_HASH_MAP(wxString,
					int,
					wxStringHash,
					wxStringEqual,
					StringIntHashmap);

WX_DECLARE_HASH_MAP(wxString,
					DebugBreakInfo,
					wxStringHash,
					wxStringEqual,
					DebugBreakHash);

WX_DECLARE_HASH_MAP(wxString,
					wxArrayInt,
					wxStringHash,
					wxStringEqual,
					FileBreakpointHash);

WX_DECLARE_HASH_MAP(int,
					VariableInfo,
					wxIntegerHash,
					wxIntegerEqual,
					VariableInfoHash);

//WX_DECLARE_HASH_MAP(wxString,
//					FunctionVariablesInfo,
//					wxStringHash,
//					wxStringEqual,
//					FunctionVariablesHash);

WX_DECLARE_STRING_HASH_MAP(wxString,
						   StringStringHashmap);

WX_DECLARE_STRING_HASH_MAP(wxFileName, StringFilenameHash);

WX_DECLARE_OBJARRAY(VariableInfo, VariableInfoArray);

enum PCConnState { // this is used exclusively inside ProcessInfo

	PC_STARTING,	// Waiting for Login confirmation
	PC_READY,		// Connection has been established. Nothing going
					//   on.  Just waiting to be used.
	PC_EXECUTING,	// Very short phase between sending the command
					//   and getting the starttoken
	PC_BUSY,		// Have received the starttoken, so all output is
					//   sent to the "owner"
	PC_ENDING		// Have received the endtoken.  Now in doing term
					//   sequence
};

typedef struct
{
	long pid;
	wxProcess* proc;
	PCConnState state;
	wxTextOutputStream* stdinStream;
	wxString outputBuf;
	wxEvtHandler* owner;
	bool isRunSynch;
} ProcessInfo;

WX_DECLARE_LIST(ProcessInfo, ProcessInfoList);
WX_DEFINE_ARRAY(ProcessInfo*, ProcessInfoArray);

enum NetworkStatus
{
	NET_STARTING = -1, // NEVER used externally
	NET_GOOD = 0,
	NET_UNKNOWN_HOST,  // host finger print not in cache
	NET_CONN_REFUSED,  // host wouldn't allow a connection
	NET_AUTH_FAILED,   // user+pass did not work on host
	NET_ERROR_MESSAGE, // default/catch-all
};

enum NetworkCallResult
{
	NETCALL_WORKED,
	NETCALL_FAILED,
	NETCALL_REDO,
};

enum FileFilterType
{
    FILE_NOTYPE = -1,
	FILE_ALLSOURCETYPES,
	FILE_SUPPORTEDFILES,
	FILE_NUMERE,
	FILE_DATAFILES,
	FILE_IMAGEFILES,
	FILE_NSCR,
	FILE_NPRC,
	FILE_PLUGIN,
	FILE_TEXSOURCE,
	FILE_MATLAB,
	FILE_CPP,
	FILE_ALLFILES,
	FILE_NONSOURCE,
	FILE_LASTTYPE,
};

enum ModifiedFileAction
{
	MODIFIEDFILE_CLOSE,
	MODIFIEDFILE_RELOAD,
	MODIFIEDFILE_COMPILE,
};

#endif
