/////////////////////////////////////////////////////////////////////////////
// Name:        RemoteFileDialog.h
// Purpose:
// Author:
// Modified by:
// Created:     01/12/04 16:32:27
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _REMOTEFILEDIALOG_H_
#define _REMOTEFILEDIALOG_H_

#ifdef __GNUG__
#pragma interface "RemoteFileDialog.h"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/listctrl.h"
////@end includes







#include <wx/dynarray.h>


// each wxArrayString will hold the extensions for that file class (ie, .C and .CPP for "C++ source files")
WX_DECLARE_OBJARRAY(wxArrayString, TwoDStringVector);

// each TwoDStringVector holds the information for a file grouping (FILE_SOURCECODE, FILE_PROJECT)
WX_DECLARE_OBJARRAY(TwoDStringVector, ThreeDStringVector);



/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxListCtrl;
////@end forward declarations

class NumeReEditor;
class NumeReWindow;
class Networking;
class wxListEvent;
class wxFileConfig;
class NumeReNotebook;
class wxTreeCtrl;
class wxTextCtrl;
class wxLogWindow;
class wxTabCtrl;
class IntIntHashmap;
class WindowPointerArray;
class wxPanel;
class wxSplitterWindow;
class RemoteFileDialog;
class OptionsDialog;
class Permission;
class wxFileConfig;
class wxTelnet;
class wxMenu;
class wxTreeItemId;
class wxImageList;
class wxFileName;
class IconManager;


#include "../../common/datastructures.h"

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define ID_PATHBOX 10002
#define ID_LISTCTRL 10001
#define ID_TXTFILENAME 10008
#define ID_BUTTONOPEN 10003
#define ID_COMBOBOX1 10004
#define ID_BUTTONCANCEL 10005
#define SYMBOL_REMOTEFILEDIALOG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_REMOTEFILEDIALOG_TITLE _("Open/Save")
#define SYMBOL_REMOTEFILEDIALOG_IDNAME ID_DIALOG
#define SYMBOL_REMOTEFILEDIALOG_SIZE wxSize(600, 330)
#define SYMBOL_REMOTEFILEDIALOG_POSITION wxDefaultPosition
////@end control identifiers


#define ID_HOMEFOLDER 10096
#define ID_UPFOLDER 10097
#define ID_REFRESHFOLDER 10098
#define ID_DIALOGTOOLBAR 10099

/*!
 * RemoteFileDialog class declaration
 */

class RemoteFileDialog: public wxDialog
{
    DECLARE_CLASS( RemoteFileDialog )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    RemoteFileDialog( );
    RemoteFileDialog( wxWindow* parent,
						wxWindowID id = -1,
						const wxString& caption = _("Open/Save"),
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize,
						long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );


    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Open/Save"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin RemoteFileDialog event handler declarations

    /// wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LISTCTRL
    void OnItemSelected( wxListEvent& event );

    /// wxEVT_COMMAND_LIST_ITEM_ACTIVATED event handler for ID_LISTCTRL
    void OnItemActivated( wxListEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTONOPEN
    void OnButtonOpenClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX1
    void OnFileTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTONCANCEL
    void OnButtonCancelClick( wxCommandEvent& event );

////@end RemoteFileDialog event handler declarations

	void OnButtonUpFolder(wxCommandEvent& event );
	void OnButtonRefresh(wxCommandEvent &event);
	void OnButtonHome(wxCommandEvent &event);
	void OnEnter(wxCommandEvent &event);

////@begin RemoteFileDialog member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end RemoteFileDialog member function declarations

	void SetNetworking(Networking* network);
	//wxString GetLocalFileNameAndPath();
	wxString GetRemoteFileName() { return m_remoteFileNamePath.GetFullName();}
	wxString GetRemotePath() { return m_remoteFileNamePath.GetPath(false, wxPATH_UNIX);}

	wxString GetRemoteFileNameAndPath();

	//void OpenRemoteFile();
	void StoreFileName(wxString filename);
	bool Prepare(bool open, wxString filterString);//FileDisplayType displayType);

	bool ShowDirectory(wxString dirname, bool refresh = false, bool showHidden = false);
	void FillListView();
	void LoadTestData();

	void SetIconManager(IconManager* iconManager);

	//int GetIconIndex(wxString extension);

    /// Should we show tooltips?
    static bool ShowToolTips();

private:

	//wxPathFormat GetCurrentPathFormat();
	void ItemActivated();

	void SetUserHomeDir(wxString path);
	//void

////@begin RemoteFileDialog member variables
    wxBoxSizer* m_sizToolbar;
    wxTextCtrl* m_pathBox;
    wxListCtrl* m_list;
    wxTextCtrl* m_txtFilename;
    wxButton* m_buttonOpen;
    wxComboBox* m_comboFiletypes;
    wxButton* m_buttonCancel;
////@end RemoteFileDialog member variables

	wxToolBar* m_toolbar;

	wxFileName m_currentPath;

	bool m_openMode;

	int m_currentFilterIndex;
	int m_filterAllFilesIndex;
	FileFilterType m_currentFilterType;
	DirListing m_currentDirListing;

	wxFileName m_localFileNamePath;
	wxFileName m_remoteFileNamePath;
	wxFileName m_userHomeDir;
	wxSortedArrayString m_currentDirs;
	wxSortedArrayString m_currentFiles;
	wxArrayString m_filterList;
	//ThreeDStringVector m_fileExtensionList;
	TwoDStringVector m_fileExtensionList;

	StringIntHashmap m_iconExtensionMapping;

	NumeReWindow* m_parentFrame;
	Networking* m_network;
	IconManager* m_iconManager;
};

#endif
    // _REMOTEFILEDIALOG_H_
