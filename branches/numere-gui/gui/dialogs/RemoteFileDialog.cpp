/////////////////////////////////////////////////////////////////////////////
// Name:        RemoteFileDialog.cpp
// Purpose:
// Author:
// Modified by:
// Created:     01/12/04 16:32:27
// RCS-ID:
// Copyright:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "RemoteFileDialog.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "../../common/CommonHeaders.h"

////@begin includes
////@end includes

#include <wx/filename.h>
#include <wx/toolbar.h>
#include <wx/imaglist.h>
#include <wx/tokenzr.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>

#include "RemoteFileDialog.h"
#include "../NumeReWindow.h"
#include "../IconManager.h"
#include "../../network/networking.h"
#include "../../common/datastructures.h"



#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(TwoDStringVector)
WX_DEFINE_OBJARRAY(ThreeDStringVector)



//#include "../../editor/editor.h"

////@begin XPM images
////@end XPM images

#include "cpp.xpm"
#include "c.xpm"
#include "h.xpm"
#include "lib.xpm"
//#include "folder256.xpm"
#include "closedfolder16x1632bpp.xpm"
#include "openfolder16x1632bpp.xpm"
#include "defaultfile.xpm"
#include "doc.xpm"

#include "upfolder.xpm"
#include "refresh.xpm"
#include "HomeHS.xpm"


#include "../../common/debug.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
/*!
 * RemoteFileDialog type definition
 */

IMPLEMENT_CLASS( RemoteFileDialog, wxDialog )

/*!
 * RemoteFileDialog event table definition
 */

BEGIN_EVENT_TABLE( RemoteFileDialog, wxDialog )

////@begin RemoteFileDialog event table entries
    EVT_LIST_ITEM_SELECTED( ID_LISTCTRL, RemoteFileDialog::OnItemSelected )
    EVT_LIST_ITEM_ACTIVATED( ID_LISTCTRL, RemoteFileDialog::OnItemActivated )

    EVT_BUTTON( ID_BUTTONOPEN, RemoteFileDialog::OnButtonOpenClick )

    EVT_COMBOBOX( ID_COMBOBOX1, RemoteFileDialog::OnFileTypeSelected )

    EVT_BUTTON( ID_BUTTONCANCEL, RemoteFileDialog::OnButtonCancelClick )

////@end RemoteFileDialog event table entries

	EVT_MENU(ID_UPFOLDER, RemoteFileDialog::OnButtonUpFolder)
	EVT_MENU(ID_REFRESHFOLDER, RemoteFileDialog::OnButtonRefresh)
	EVT_MENU(ID_HOMEFOLDER, RemoteFileDialog::OnButtonHome)
	EVT_TEXT_ENTER(ID_TXTFILENAME, RemoteFileDialog::OnEnter)
END_EVENT_TABLE()

/*!
 * RemoteFileDialog constructors
 */

RemoteFileDialog::RemoteFileDialog( )
{

}

RemoteFileDialog::RemoteFileDialog( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);

	m_parentFrame = (NumeReWindow*)parent;

	//m_currentPath.AssignDir("~");
	SetUserHomeDir("~");
	m_openMode = true;

	m_currentFilterIndex = 0;
	// "All files (*.*) will always be the last item in the dropdown box
	//m_filterAllFilesIndex = m_comboFiletypes->GetCount() - 1;
}

/*!
 * RemoteFileDialog creator
 */

//////////////////////////////////////////////////////////////////////////////
///  public Create
///  Does the usual window creation, but also creates the image list used by the dialog and the main project tree.
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool RemoteFileDialog::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin RemoteFileDialog member initialisation
    m_sizToolbar = NULL;
    m_pathBox = NULL;
    m_list = NULL;
    m_txtFilename = NULL;
    m_buttonOpen = NULL;
    m_comboFiletypes = NULL;
    m_buttonCancel = NULL;
////@end RemoteFileDialog member initialisation

////@begin RemoteFileDialog creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
////@end RemoteFileDialog creation

	/*
	wxImageList* images = new wxImageList(16, 16);
	wxImageList* transferImageList = new wxImageList(16, 16);

	// put together the file icons for C++ file types

	// TODO add bitmap for project files here
	// until then, use default image
	m_iconExtensionMapping["cpj"] = images->GetImageCount();
	m_iconExtensionMapping["DEFAULTFILEEXTENSION"] = images->GetImageCount();
	wxBitmap defaultfile(defaultfile_xpm);
	images->Add(defaultfile);
	transferImageList->Add(defaultfile);

	wxBitmap closedfolder(closedfolder16x1632bpp_xpm);
	images->Add(closedfolder);
	transferImageList->Add(closedfolder);

	wxBitmap openfolder(openfolder16x1632bpp_xpm);
	images->Add(openfolder);
	transferImageList->Add(openfolder);


	m_iconExtensionMapping["c"] = images->GetImageCount();
	wxBitmap bmStandardC(c_xpm);
	images->Add(bmStandardC);
	transferImageList->Add(bmStandardC);

	m_iconExtensionMapping["cpp"] = images->GetImageCount();
	wxBitmap bmCpp(cpp_xpm);
	images->Add(bmCpp);
	transferImageList->Add(bmCpp);

	m_iconExtensionMapping["hpp"] = images->GetImageCount();
	m_iconExtensionMapping["h"] = images->GetImageCount();
	wxBitmap bmH(h_xpm);
	images->Add(bmH);
	transferImageList->Add(bmH);

	m_iconExtensionMapping["lib"] = images->GetImageCount();
	wxBitmap bmLib(lib_xpm);
	images->Add(bmLib);
	transferImageList->Add(bmLib);

	m_iconExtensionMapping["txt"] = images->GetImageCount();
	wxBitmap bmText(doc_xpm);
	images->Add(bmText);
	transferImageList->Add(bmText);
	*/


	m_toolbar = new wxToolBar(this, ID_DIALOGTOOLBAR, wxDefaultPosition, wxDefaultSize,
								wxTB_HORIZONTAL | wxTB_FLAT | wxNO_BORDER | wxTB_NODIVIDER);

	m_sizToolbar->Add(m_toolbar, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

	m_toolbar->SetToolBitmapSize(wxSize(16, 16));


    wxBitmap upfolder(upfolder_xpm);
	m_toolbar->AddTool(ID_UPFOLDER, upfolder, "Go up a folder");

	wxBitmap home(HomeHS_xpm);
	m_toolbar->AddTool(ID_HOMEFOLDER, home, "Home folder");


	wxBitmap refresh(refresh_xpm);
	m_toolbar->AddTool(ID_REFRESHFOLDER, refresh, "Refresh this folder");



	m_toolbar->Realize();


	/*
	m_list->AssignImageList(images, wxIMAGE_LIST_SMALL);


	wxMemoryDC dc;


	wxPen pen(wxColour("navy"), 2);
	dc.SetPen(pen);

	dc.SelectObject(bmStandardC);
	dc.DrawLine(0, 0, 15, 15);
	dc.DrawLine(15, 0, 0, 15);
	dc.SelectObject(wxNullBitmap);
	transferImageList->Add(bmStandardC);


	dc.SelectObject(bmCpp);
	dc.DrawLine(0, 0, 15, 15);
	dc.DrawLine(15, 0, 0, 15);
	dc.SelectObject(wxNullBitmap);
	transferImageList->Add(bmCpp);


	dc.SelectObject(bmH);
	dc.DrawLine(0, 0, 15, 15);
	dc.DrawLine(15, 0, 0, 15);
	dc.SelectObject(wxNullBitmap);
	transferImageList->Add(bmH);


	dc.SelectObject(bmLib);
	dc.DrawLine(0, 0, 15, 15);
	dc.DrawLine(15, 0, 0, 15);
	dc.SelectObject(wxNullBitmap);
	transferImageList->Add(bmLib);



	dc.SetPen(wxNullPen);

	((ChameleonWindow*) parent)->PassImageList(transferImageList);
	*/


    return TRUE;
}

/*!
 * Control creation for RemoteFileDialog
 */

void RemoteFileDialog::CreateControls()
{
////@begin RemoteFileDialog content construction
    RemoteFileDialog* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    m_sizToolbar = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(m_sizToolbar, 0, wxGROW|wxLEFT|wxRIGHT, 5);

    wxStaticText* itemStaticText4 = new wxStaticText( itemDialog1, wxID_STATIC, _("Current path:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_sizToolbar->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    m_pathBox = new wxTextCtrl( itemDialog1, ID_PATHBOX, _T(""), wxDefaultPosition, wxSize(300, -1), wxTE_READONLY );
    m_sizToolbar->Add(m_pathBox, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxFIXED_MINSIZE, 5);

    m_list = new wxListCtrl( itemDialog1, ID_LISTCTRL, wxDefaultPosition, wxSize(480, 280), wxLC_LIST  );
    itemBoxSizer2->Add(m_list, 1, wxGROW|wxALL|wxFIXED_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText9 = new wxStaticText( itemDialog1, wxID_STATIC, _("File &name:"), wxDefaultPosition, wxSize(70, -1), 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    m_txtFilename = new wxTextCtrl( itemDialog1, ID_TXTFILENAME, _T(""), wxDefaultPosition, wxSize(246, -1), wxTE_PROCESS_ENTER );
    itemBoxSizer8->Add(m_txtFilename, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxFIXED_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer8->Add(itemBoxSizer11, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxFIXED_MINSIZE, 10);

    m_buttonOpen = new wxButton( itemDialog1, ID_BUTTONOPEN, _("Open"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(m_buttonOpen, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer13, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText14 = new wxStaticText( itemDialog1, wxID_STATIC, _("Files of &type:"), wxDefaultPosition, wxSize(70, -1), 0 );
    itemBoxSizer13->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxString m_comboFiletypesStrings[] = {
        _("C++ source files (*.c, *.cpp)"),
        _("C++ header files (*.h, *.hpp)"),
        _("All files (*.*)")
    };
    m_comboFiletypes = new wxComboBox( itemDialog1, ID_COMBOBOX1, _("C++ source files (*.c, *.cpp)"), wxDefaultPosition, wxSize(246, -1), 3, m_comboFiletypesStrings, wxCB_READONLY );
    m_comboFiletypes->SetStringSelection(_("C++ source files (*.c, *.cpp)"));
    itemBoxSizer13->Add(m_comboFiletypes, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer13->Add(itemBoxSizer16, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxFIXED_MINSIZE, 10);

    m_buttonCancel = new wxButton( itemDialog1, ID_BUTTONCANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(m_buttonCancel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end RemoteFileDialog content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

void RemoteFileDialog::OnButtonOpenClick( wxCommandEvent& event )
{
    // Insert custom code here

	ItemActivated();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON1
 */

void RemoteFileDialog::OnButtonCancelClick( wxCommandEvent& event )
{
    // Insert custom code here
    EndModal(wxCANCEL);
}

/*!
 * Should we show tooltips?
 */

bool RemoteFileDialog::ShowToolTips()
{
  return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON2
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnButtonUpFolder
///  Displays the appropriate directory when the user clicks the "Up folder" button
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::OnButtonUpFolder( wxCommandEvent& event )
{
    event.Skip();

	//wxPathFormat pathFormat = GetCurrentPathFormat();

	int numDirs = m_currentPath.GetDirCount();

	if(numDirs > 0)
	{
		wxString currPath = m_currentPath.GetPath(false, wxPATH_UNIX);


		// if we're not at a user's home directory, get rid of the last
		// directory in the path
		if(currPath != "~")
		{
			m_currentPath.RemoveDir(numDirs - 1);
		}
	}
	wxString path = m_currentPath.GetPath();
	ShowDirectory(m_currentPath.GetPath(false, wxPATH_UNIX));
}

void RemoteFileDialog::OnButtonHome( wxCommandEvent& event )
{
	event.Skip();

	ShowDirectory(m_userHomeDir.GetPath(false, wxPATH_UNIX));
}

//////////////////////////////////////////////////////////////////////////////
///  public GetRemoteFileNameAndPath
///  Gets the selected filename and path
///
///  @return wxString The selected filename and path
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString RemoteFileDialog::GetRemoteFileNameAndPath()
{
	return m_remoteFileNamePath.GetFullPath(wxPATH_UNIX);
}

/*
wxString RemoteFileDialog::GetLocalFileNameAndPath()
{
	return m_localFileNamePath.GetFullPath();
}
*/


void RemoteFileDialog::SetNetworking(Networking* network)
{
	m_network = network;
	//m_currentPath.AssignDir("~");
	SetUserHomeDir("~");
}

//////////////////////////////////////////////////////////////////////////////
///  public ShowDirectory
///  Displays the contents of a given directory
///
///  @param  dirname    wxString  The directory to display
///  @param  refresh    bool      [=false] Whether or not to force a refresh (in case caching is on)
///  @param  showHidden bool      [=false] Whether or not to show hidden files
///
///  @return bool       Whether or not the call succeeded
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool RemoteFileDialog::ShowDirectory(wxString dirname, bool refresh, bool showHidden)
{
	DirListing dl;
	if(!m_network->GetDirListing(dirname, dl, refresh, showHidden))
	{
		return false;
	}

	m_list->ClearAll();
	m_pathBox->SetValue(dirname);
	m_currentPath.AssignDir(dirname);

	m_currentDirListing = dl;
	FillListView();

	return true;
}

//////////////////////////////////////////////////////////////////////////////
///  public FillListView
///  Inserts file items into the main listview
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::FillListView()
{
	// disables VS's 4018 warning: "signed/unsigned mismatch" relating to the GetCount() comparison
#pragma warning( disable : 4018)

	m_list->ClearAll();

	wxSortedArrayString sortedDirs(m_currentDirListing.dirNames);
	wxSortedArrayString sortedFiles(m_currentDirListing.fileNames);

	m_currentDirs.Clear();
	m_currentFiles.Clear();

	// should be getting just the name of the directory itself from Networking, so I
	// can insert it directly into the listview and give it the folder icon
	for(int i = 0; i < sortedDirs.GetCount(); i++)
	{
		m_list->InsertItem(m_list->GetItemCount(), sortedDirs[i], ICON_FOLDERCLOSED);
		m_currentDirs.Add(sortedDirs[i]);
	}


	//TwoDStringVector dsv = m_fileExtensionList[m_currentFilterType];

	wxArrayString currentFilterExtensions = m_fileExtensionList[m_currentFilterIndex];

	wxSortedArrayString currentFileSet;

	int currentFileIconIndex = 0;

	for(int i = 0; i < sortedFiles.GetCount(); i++)
	{
		wxLogNull disableLogging;

		wxFileName currentName(sortedFiles[i]);
		wxString currentFileExtension = currentName.GetExt();

		if(	(currentFilterExtensions[0] == "*.*") ||
			(currentFilterExtensions.Index(currentFileExtension) != wxNOT_FOUND))
		{
			wxString file = currentName.GetFullName();
			m_currentFiles.Add(file);
			currentFileIconIndex = m_iconManager->GetIconIndex(currentFileExtension);
			m_list->InsertItem(m_list->GetItemCount(), file, currentFileIconIndex);
		}
	}


	/*
	for(int j = 0; j < currentFilterExtensions.Count(); j++)
	{
		wxString currentExtensionType = currentFilterExtensions[j];

		int currentExtensionIconNumber = 0; //m_iconManager->GetIconIndex(currentExtensionType)

		for(int i = 0; i < sortedFiles.GetCount(); i++)
		{
			wxFileName currentName(sortedFiles[i]);
			wxString currentFileExtension = currentName.GetExt();
			if( (currentFileExtension == currentExtensionType) ||
				(currentExtensionType == "*.*"))
			{
				currentExtensionIconNumber = m_iconManager->GetIconIndex(currentFileExtension);
				wxString file = currentName.GetFullName();
				m_currentFiles.Add(file);
				currentFileSet.Add(file);
			}
		}

		for(int k = 0; k < currentFileSet.Count(); k++)
		{
			m_list->InsertItem(m_list->GetItemCount(), currentFileSet[k], currentExtensionIconNumber);
		}
		currentFileSet.Clear();
	}
	*/

	m_pathBox->SetSelection(0,0);
	m_list->SetFocus();

	// turn the size_t warning back on
#pragma warning( default : 4018 )
}

/*
void RemoteFileDialog::LoadTestData()
{
	ShowDirectory("C:\\Projects\\cs4810\\gears\\chameleon\\gui\\");
}


// save the information for opening a file
void RemoteFileDialog::OpenRemoteFile()
{
	wxString remoteFileName = m_txtFilename->GetValue();
	m_remoteFileNamePath.Assign(m_currentPath.GetPath(false, wxPATH_UNIX), remoteFileName);

	EndModal(wxOK);

}
*/

// save the information for saving a file
void RemoteFileDialog::StoreFileName(wxString filename)
{
	m_remoteFileNamePath.Assign(m_currentPath.GetPath(false, wxPATH_UNIX), filename);

	EndModal(wxOK);
}



/*!
 * wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LISTCTRL
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnItemSelected
///  Sets the filename field with the selected item's name
///
///  @param  event wxListEvent & The generated listcontrol event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::OnItemSelected( wxListEvent& event )
{
    // Insert custom code here
    event.Skip();

	wxString itemText = event.GetText();
	m_txtFilename->SetValue(itemText);
}

/*!
 * wxEVT_COMMAND_LIST_ITEM_ACTIVATED event handler for ID_LISTCTRL
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnItemActivated
///  Called whenever the user activates an item in the listview
///
///  @param  event wxListEvent & The generated listcontrol event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::OnItemActivated( wxListEvent& event )
{
    // Insert custom code here
    event.Skip();

	wxString itemText = event.GetText();

	m_txtFilename->SetValue(itemText);

	ItemActivated();
}

/*
wxPathFormat RemoteFileDialog::GetCurrentPathFormat()
{
	wxPathFormat pathFormat = wxPATH_UNIX;
	return pathFormat;
}
*/

//////////////////////////////////////////////////////////////////////////////
///  public Prepare
///  Initializes the dialog before it's displayed
///
///  @param  open         bool      True if it's an Open dialog, false if it's a Save dialog
///  @param  filterString wxString  The file filter string to use
///
///  @return bool         Whether or not the function succeeded
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool RemoteFileDialog::Prepare(bool open, wxString filterString)//FileDisplayType displayType)
{
	m_openMode = open;


	m_comboFiletypes->Clear();


	wxStringTokenizer filterTokenizer;
	filterTokenizer.SetString(filterString, "|");

	wxStringTokenizer fileTypeTokenizer;

	wxArrayString as;

	wxFileName extGrabber;

	m_fileExtensionList.Clear();

	m_currentFilterIndex = 0;

	while(filterTokenizer.HasMoreTokens())
	{
		wxString comboText = filterTokenizer.GetNextToken();
		m_comboFiletypes->Append(comboText);
		wxString fileTypeString = filterTokenizer.GetNextToken();

		fileTypeTokenizer.SetString(fileTypeString, ";");

		while(fileTypeTokenizer.HasMoreTokens())
		{
			wxString typeString = fileTypeTokenizer.GetNextToken();
			if(typeString == "*.*")
			{
				as.Add(typeString);
			}
			else
			{
				extGrabber.Assign(typeString);
				as.Add(extGrabber.GetExt());
			}

			extGrabber.Clear();
		}

		m_fileExtensionList.Add(as);
		as.Clear();
	}

	if(m_openMode)
	{
		SetTitle("Open File");
		m_buttonOpen->SetLabel("Open");
	}
	else
	{
		wxString title = "Save File As";

		if(filterString.Find("*.cpj") != wxNOT_FOUND)
		{
			title = "Save New Project";
		}

		SetTitle(title);
		m_buttonOpen->SetLabel("Save");
	}


	m_comboFiletypes->SetSelection(0);

	m_filterAllFilesIndex = m_comboFiletypes->FindString("All files (*.*)");


	//wxPathFormat format = GetCurrentPathFormat();



	wxString userHomeDir;
	if(!m_network->GetHomeDirPath(userHomeDir))
	{
		return false;
	}
	SetUserHomeDir(userHomeDir);

	wxString path = m_currentPath.GetPath(false, wxPATH_UNIX);
	m_txtFilename->SetValue(wxEmptyString);
	return ShowDirectory(path);
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTONOPEN
 */

//////////////////////////////////////////////////////////////////////////////
///  private ItemActivated
///  Handles showing directories and saving filenames when the user activates an item
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::ItemActivated()
{
	wxString userFileName = m_txtFilename->GetValue();

	wxFileName fn;

	int dirResult = m_currentDirListing.dirNames.Index(userFileName); //m_currentDirs.Index(text);

	// the item name isn't a directory
	if(dirResult == wxNOT_FOUND)
	{

		// if we're trying to save a file and the filter isn't "All files (*.*)
		if(!m_openMode && (m_currentFilterIndex != m_filterAllFilesIndex))
		{
			fn.SetFullName(userFileName);
			wxString ext = fn.GetExt();

			if(ext == wxEmptyString)
			{
				// retrieves the appropriate set of extensions for the
				// selected filter and uses the first one as the default extension
				fn.SetExt(m_fileExtensionList[m_currentFilterIndex][0]);

				userFileName = fn.GetFullName();
			}

		}

		int fileResult = m_currentDirListing.fileNames.Index(userFileName);//m_currentFiles.Index(text);

		// and it's not a file either
		if(fileResult == wxNOT_FOUND)
		{
			// trying to open a non-existent file
			if(m_openMode)
			{
				wxString errorMessage = "\"" + userFileName + "\" was not found.  Please enter or select a valid file name.";

				wxMessageBox(errorMessage, "Open", wxOK | wxICON_EXCLAMATION);
				return;
			}
			// trying to save a file and it's a new file
			else
			{
				StoreFileName(userFileName);
			}
		}
		else
		{
			// filename found, open it
			if(m_openMode)
			{
				StoreFileName(userFileName);
			}
			// file already exists - confirm overwrite
			else
			{
				wxString confirmMessage = userFileName + " already exists.  Do you want to replace it?";
				int result = wxMessageBox(confirmMessage, "Save As", wxYES_NO | wxICON_QUESTION);

				if(result == wxYES)
				{
					StoreFileName(userFileName);
				}
				else
				{
					return;
				}
			}
		}
	}
	else
	{
		// call ShowDirectory w/ new directory

		// m_currentPath gets overwritten in ShowDirectory iff the call succeeds
		//m_currentPath.AppendDir(userFileName);

		wxFileName tempDirName(m_currentPath);
		tempDirName.AppendDir(userFileName);

		if(!ShowDirectory(tempDirName.GetPath(false, wxPATH_UNIX)))
		{
			wxString errorMessage = wxString::Format("The operation failed.  Error message: \r\n%s",
				m_network->GetStatusDetails());
			wxMessageBox(errorMessage, "Network Operation Failed", wxOK | wxCENTRE | wxICON_HAND);
		}
	}
	m_txtFilename->Clear();
}/*!
 * wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX1
 */

//////////////////////////////////////////////////////////////////////////////
///  public OnFileTypeSelected
///  Updates the listview contents when the user changes the selected file type
///
///  @param  event wxCommandEvent & The generated event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::OnFileTypeSelected( wxCommandEvent& event )
{
    // Insert custom code here
    event.Skip();

	int selectionIndex = m_comboFiletypes->GetSelection();

	if(selectionIndex != m_currentFilterIndex)
	{
		m_currentFilterIndex = selectionIndex;
		FillListView();
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public OnButtonRefresh
///  Refreshes the listview's contents from the server
///
///  @param  event wxCommandEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::OnButtonRefresh(wxCommandEvent &event)
{
	wxString path = m_currentPath.GetPath(false, wxPATH_UNIX);
	ShowDirectory(path, true);
}

//////////////////////////////////////////////////////////////////////////////
///  public OnEnter
///  Allows the user to close the dialog by pressing Enter
///
///  @param  event wxCommandEvent & The generated command event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void RemoteFileDialog::OnEnter(wxCommandEvent &event)
{
	ItemActivated();
}

void RemoteFileDialog::SetUserHomeDir( wxString newHomeDir )
{
	wxString currentHomeDir = m_userHomeDir.GetPath(false, wxPATH_UNIX);

	if(newHomeDir != currentHomeDir)
	{
		m_currentPath.SetPath(newHomeDir, wxPATH_UNIX);
	}

	m_userHomeDir.SetPath(newHomeDir, wxPATH_UNIX);
}

/*!
 * Get bitmap resources
 */

wxBitmap RemoteFileDialog::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin RemoteFileDialog bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end RemoteFileDialog bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon RemoteFileDialog::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin RemoteFileDialog icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end RemoteFileDialog icon retrieval
}

void RemoteFileDialog::SetIconManager(IconManager* iconManager)
{
	m_iconManager = iconManager;
	m_list->SetImageList(m_iconManager->GetImageList(), wxIMAGE_LIST_SMALL);
}
