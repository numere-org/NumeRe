#include "../common/CommonHeaders.h"
#include "NumeReNotebook.h"
#include "NumeReWindow.h"

#include "../common/debug.h"
#include "../common/vcsmanager.hpp"
#include "editor/editor.h"

#include <wx/wx.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <windows.h>
#include <commctrl.h>
//#include <wx/tabctrl.h>

#include "../common/datastructures.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(EditorNotebook, wxNotebook)
	EVT_MIDDLE_UP	(EditorNotebook::OnTabMiddleClicked)
	EVT_RIGHT_UP	(EditorNotebook::OnTabRightClicked)
	EVT_MOUSEWHEEL  (EditorNotebook::OnTabScroll)
	EVT_ENTER_WINDOW(EditorNotebook::OnEnter)
	EVT_LEAVE_WINDOW(EditorNotebook::OnLeave)
	EVT_SIZE        (EditorNotebook::OnSize)
END_EVENT_TABLE()

/////////////////////////////////////////////////
/// \brief Constructor.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param pos const wxPoint&
/// \param size const wxSize&
/// \param style long
/// \param name const wxString&
///
/////////////////////////////////////////////////
EditorNotebook::EditorNotebook(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) : wxNotebook(parent, id, pos, size, style, name)
{
    m_mouseFocus = false;
	//m_top_parent = static_cast<NumeReWindow*>(parent);//(NumeReWindow*)wxTheApp->GetTopWindow();
}


/////////////////////////////////////////////////
/// \brief Empty destructor.
/////////////////////////////////////////////////
EditorNotebook::~EditorNotebook()
{
}


//////////////////////////////////////////////////////////////////////////////
///  public OnTabActivate
///  Pops up a menu when the user right-clicks a tab
///
///  @param  event wxMouseEvent & The generated menu event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void EditorNotebook::OnTabRightClicked(wxMouseEvent &event)
{
	wxPoint pt;
	pt.x = event.GetX();
	pt.y = event.GetY();

	long flags = 0;
	int pageNum = this->HitTest(pt, &flags);

	if (pageNum < 0)
		return;

    VersionControlSystemManager manager(m_top_parent);
    wxString filename = static_cast<NumeReEditor*>(GetPage(pageNum))->GetFileNameAndPath();

	m_top_parent->SetIntVar(VN_CLICKEDTAB, pageNum);
	wxMenu popupMenu;
	popupMenu.Append(ID_MENU_CLOSETAB, _guilang.get("GUI_EDITOR_TAB_CLOSE"));
	popupMenu.Append(ID_MENU_CLOSEALL, _guilang.get("GUI_EDITOR_TAB_CLOSEALL"));
	popupMenu.Append(ID_MENU_CLOSEOTHERS, _guilang.get("GUI_EDITOR_TAB_CLOSEOTHERS"));
	popupMenu.AppendSeparator();
	popupMenu.Append(ID_MENU_OPEN_FOLDER, _guilang.get("GUI_EDITOR_TAB_OPENFOLDER"));
	popupMenu.AppendSeparator();
	popupMenu.Append(ID_MENU_RUN_FROM_TAB, _guilang.get("GUI_TB_RUN"));

	// Append the file revision menu, if the file has revisions
	if (manager.hasRevisions(filename))
    {
        popupMenu.AppendSeparator();
        popupMenu.Append(ID_MENU_SHOW_REVISIONS_FROM_TAB, _guilang.get("GUI_TREE_PUP_SHOWREVISIONS"));
    }

	PopupMenu(&popupMenu, pt);
}


/////////////////////////////////////////////////
/// \brief Scrolls through the notebook pages, if
/// the user hovers over the tabs and uses the
/// mouse wheel.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnTabScroll(wxMouseEvent &event)
{
	wxPoint pt;
	pt.x = event.GetX();
	pt.y = event.GetY();

	long flags = 0;
	int pageNum = this->HitTest (pt, &flags);

	if (pageNum < 0 || GetPageCount() <= 1)
		return;

	size_t currentPage = GetSelection();

	if (event.GetWheelRotation() < 0)
	{
        if (currentPage + 1 == GetPageCount())
            SetSelection(0);
        else
            SetSelection(currentPage+1);
	}
	else
	{
        if (!currentPage)
            SetSelection(GetPageCount()-1);
        else
            SetSelection(currentPage-1);
	}
}


/////////////////////////////////////////////////
/// \brief Focuses the tabs, if the user hovers
/// over them.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    m_mouseFocus = true;
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Deactivates the internal mouse focus
/// state.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnLeave(wxMouseEvent& event)
{
    m_mouseFocus = false;
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Size event handling function.
///
/// \param event wxSizeEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnSize(wxSizeEvent &event)
{
	event.Skip();
}


//////////////////////////////////////////////////////////////////////////////
///  public FindPagePosition
///  Finds the index of a given notebook page
///
///  @param  page wxNotebookPage * The generated notebook event
///
///  @return int  The index of the requested page (-1 if not found)
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
int EditorNotebook::FindPagePosition(wxNotebookPage* page)
{
	int nPageCount = GetPageCount();
	int nPage;
	for ( nPage = 0; nPage < nPageCount; nPage++ )
		if (GetPage(nPage) == page)
			return nPage;
	return -1;
}


/////////////////////////////////////////////////
/// \brief Executes the closing command, if the
/// user clickes with the middle mouse button on
/// a tab.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnTabMiddleClicked( wxMouseEvent &event )
{
	wxPoint pt;
	pt.x = event.GetX();
	pt.y = event.GetY();

	long flags = 0;
	int pageNum = this->HitTest (pt, &flags);

	if (pageNum < 0)
		return;

	m_top_parent->SetIntVar(VN_CLICKEDTAB, pageNum);

	wxCommandEvent command;
	command.SetId(ID_MENU_CLOSETAB);
	command.SetEventType(wxEVT_MENU); //10019//wxEVT_MIDDLE_UP
	m_top_parent->GetEventHandler()->ProcessEvent(command);
}


