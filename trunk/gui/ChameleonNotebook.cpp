#include "../common/CommonHeaders.h"
#include "NumeReNotebook.h"
#include "NumeReWindow.h"

#include "../common/debug.h"

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

BEGIN_EVENT_TABLE(NumeReNotebook, wxNotebook)
//EVT_CONTEXT_MENU(ChameleonNotebook::TestMenu)
	EVT_MIDDLE_UP	(NumeReNotebook::OnTabMiddleClicked)
	EVT_RIGHT_UP	(NumeReNotebook::OnTabRightClicked)
	EVT_MOUSEWHEEL  (NumeReNotebook::OnTabScroll)
	EVT_ENTER_WINDOW(NumeReNotebook::OnEnter)
	EVT_LEAVE_WINDOW(NumeReNotebook::OnLeave)
	EVT_SIZE        (NumeReNotebook::OnSize)
END_EVENT_TABLE()

NumeReNotebook::NumeReNotebook(wxWindow* parent, wxWindowID id,
									 const wxPoint& pos /* = wxDefaultPosition */,
									 const wxSize& size /* = wxDefaultSize */,
									 long style /* = 0 */,
									 const wxString& name /* = "notebook" */)
:wxNotebook(parent, id, pos, size, style, name)
{
    m_mouseFocus = false;
	//m_top_parent = static_cast<NumeReWindow*>(parent);//(NumeReWindow*)wxTheApp->GetTopWindow();
}

NumeReNotebook::~NumeReNotebook ()
{
}

/*
int ChameleonNotebook::HitTest(const wxPoint& pt, long& flags)
{
	TC_HITTESTINFO hitTestInfo;
	hitTestInfo.pt.x = pt.x;
	hitTestInfo.pt.y = pt.y;
	int item = TabCtrl_HitTest( (HWND) GetHWND(), & hitTestInfo ) ;
	flags = 0;

	if ((hitTestInfo.flags & TCHT_NOWHERE) == TCHT_NOWHERE)
		flags |= wxTAB_HITTEST_NOWHERE;
	if ((hitTestInfo.flags & TCHT_ONITEMICON) == TCHT_ONITEMICON)
		flags |= wxTAB_HITTEST_ONICON;
	if ((hitTestInfo.flags & TCHT_ONITEMLABEL) == TCHT_ONITEMLABEL)
		flags |= wxTAB_HITTEST_ONLABEL;

	return item;
}
*/

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
void NumeReNotebook::OnTabRightClicked (wxMouseEvent &event)
{
	wxPoint pt;
	pt.x = event.GetX();
	pt.y = event.GetY();

	long flags = 0;
	int pageNum = this->HitTest (pt, &flags);
	if (pageNum < 0)
	{
		return;
	}

	m_top_parent->SetIntVar(VN_CLICKEDTAB, pageNum);
	wxMenu popupMenu;
	popupMenu.Append(ID_CLOSETAB, _guilang.get("GUI_EDITOR_TAB_CLOSE"));
	popupMenu.AppendSeparator();
	popupMenu.Append(ID_DEBUG_START_TAB, _guilang.get("GUI_TB_RUN"));
	PopupMenu(&popupMenu, pt);
}

void NumeReNotebook::OnTabScroll(wxMouseEvent &event)
{
	wxPoint pt;
	pt.x = event.GetX();
	pt.y = event.GetY();

	long flags = 0;
	int pageNum = this->HitTest (pt, &flags);
	if (pageNum < 0 || GetPageCount() <= 1)
	{
		return;
	}
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

void NumeReNotebook::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    m_mouseFocus = true;
    event.Skip();
}

void NumeReNotebook::OnLeave(wxMouseEvent& event)
{
    m_mouseFocus = false;
    event.Skip();
}

/*
void ChameleonNotebook::CreateBookMenus () {

	wxMenuBar *menuBar = m_parent->GetMenuBar();
	wxString label;


	//m_nonActiveMenu->Append(-1, "Non-active menu");
	m_activeMenu->Append(ID_CLOSETAB, "Close");
	//m_activeMenu->Append(9997, "Active menu");
	//m_activeMenu->Append(9998, "Active menu - 2");

	// create menu 1

	if (menuBar->FindItem (wxID_SAVE)) {
		label = menuBar->GetLabel (wxID_SAVE);
		m_bookMenu1->Append (wxID_SAVE, label);
	}
	if (menuBar->FindItem (wxID_SAVEAS)) {
		label = menuBar->GetLabel (wxID_SAVEAS);
		m_bookMenu1->Append (wxID_SAVEAS, label);
	}
	if (menuBar->FindItem (wxID_CLOSE)) {
		label = menuBar->GetLabel (wxID_CLOSE);
		m_bookMenu1->Append (wxID_CLOSE, label);
	}
	m_bookMenu1->AppendSeparator();
	if (menuBar->FindItem (wxID_PRINT)) {
		label = menuBar->GetLabel (wxID_PRINT);
		m_bookMenu1->Append (wxID_PRINT, label);
	}

	// create menu 2
	m_bookMenu2->Append (myID_PAGEACTIVE, _("Activate"));
#if NOTEBOOK_DELETEPAGE
	m_bookMenu2->Append (myID_PAGECLOSE, _("Close"));
#endif


}
*/

void NumeReNotebook::OnSize(wxSizeEvent &event)
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
int NumeReNotebook::FindPagePosition(wxNotebookPage* page)
{
	int nPageCount = GetPageCount();
	int nPage;
	for ( nPage = 0; nPage < nPageCount; nPage++ )
		if (GetPage(nPage) == page)
			return nPage;
	return -1;
}

void NumeReNotebook::OnTabMiddleClicked( wxMouseEvent &event )
{
	wxPoint pt;
	pt.x = event.GetX();
	pt.y = event.GetY();

	long flags = 0;
	int pageNum = this->HitTest (pt, &flags);
	if (pageNum < 0)
	{
		return;
	}

	m_top_parent->SetIntVar(VN_CLICKEDTAB, pageNum);


	wxCommandEvent command;
	command.SetId(ID_CLOSETAB);
	command.SetEventType(wxEVT_MENU); //10019//wxEVT_MIDDLE_UP
	/*m_parent->GetEventHandler()->*/m_top_parent->GetEventHandler()->ProcessEvent(command);

}
