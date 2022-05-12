#include "../common/CommonHeaders.h"
#include "NumeReNotebook.h"
#include "NumeReWindow.h"
#include "numeredroptarget.hpp"

#include "../common/vcsmanager.hpp"
#include "editor/editor.h"

#include <wx/wx.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <windows.h>
#include <commctrl.h>

#include "../common/datastructures.h"

BEGIN_EVENT_TABLE(EditorNotebook, wxAuiNotebook)
	EVT_AUINOTEBOOK_BUTTON      	(-1, EditorNotebook::OnButtonClicked)
	EVT_AUINOTEBOOK_TAB_MIDDLE_UP	(-1, EditorNotebook::OnTabMiddleClicked)
	EVT_AUINOTEBOOK_TAB_RIGHT_UP	(-1, EditorNotebook::OnTabRightClicked)
	EVT_AUINOTEBOOK_DRAG_MOTION     (-1, EditorNotebook::OnTabMove)
	//EVT_RIGHT_UP	(EditorNotebook::OnTabRightClicked)
	//EVT_MOUSEWHEEL  (EditorNotebook::OnTabScroll)
	EVT_ENTER_WINDOW(EditorNotebook::OnEnter)
	EVT_LEAVE_WINDOW(EditorNotebook::OnLeave)
	EVT_SIZE        (EditorNotebook::OnSize)
	EVT_SPLITTER_UNSPLIT(-1, EditorNotebook::OnUnsplit)
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
EditorNotebook::EditorNotebook(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) : wxAuiNotebook(parent, id, pos, size, style | wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_WINDOWLIST_BUTTON /*, name*/)
{
    m_mouseFocus = false;
    m_top_parent = nullptr;
    m_showPathsOnTabs = false;
}


/////////////////////////////////////////////////
/// \brief Empty destructor.
/////////////////////////////////////////////////
EditorNotebook::~EditorNotebook()
{
}


/////////////////////////////////////////////////
/// \brief This member function enables/disables
/// the relative paths on the tab and refreshes
/// the tab texts automatically.
///
/// \param showText bool
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::SetShowPathsOnTabs(bool showText)
{
    m_showPathsOnTabs = showText;

    for (size_t i = 0; i < GetPageCount(); i++)
    {
        NumeReEditor* edit = getEditor(i);
        SetTabText(i, edit->GetFileNameAndPath());
    }

    Refresh();
}


/////////////////////////////////////////////////
/// \brief Set a text for the selected editor
/// tab. Note that this function expects a
/// filepath.
///
/// \param nTab size_t
/// \param text const wxString&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::SetTabText(size_t nTab, const wxString& text)
{
    size_t pos = 0;
    std::string path = replacePathSeparator(text.ToStdString());
    std::string ext = path.substr(path.find_last_of("/.")+1);
    std::string filetype = _guilang.get("COMMON_FILETYPE_" + toUpperCase(ext));

    if (filetype == "COMMON_FILETYPE_" + toUpperCase(ext))
        filetype = _guilang.get("GUI_STATUSBAR_UNKNOWN", toUpperCase(ext));

    SetPageToolTip(nTab, path + "\n" + filetype);
    std::vector<std::string> vPaths = m_top_parent->getPathDefs();

    if (m_showPathsOnTabs)
    {
        for (int i = LOADPATH; i < PATH_LAST; i++)
        {
            if (path.substr(0, vPaths[i].length()) == vPaths[i])
            {
                pos = vPaths[i].length();

                while (path[pos] == '/')
                    pos++;

                break;
            }
        }

        // Nothing found-must be an absolute path. We will
        // replace /PATH/ with /../
        if (!pos)
            path = shortenFileName(path);
    }
    else
        pos = path.rfind('/')+1;

    SetPageText(nTab, path.substr(pos));
}


/////////////////////////////////////////////////
/// \brief Create a new editor and add it to a
/// new tab automatically. The passed text is
/// used for the tab name.
///
/// \param text const wxString&
/// \return NumeReEditor*
///
/////////////////////////////////////////////////
NumeReEditor* EditorNotebook::createEditor(const wxString& text)
{
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);
    NumeReEditor* editor = new NumeReEditor(m_top_parent, m_top_parent->getOptions(), splitter,
                                            wxID_ANY, m_top_parent->getTerminal()->getSyntax(), m_top_parent->getTerminal());
    splitter->Initialize(editor);
    AddPage(splitter, text, false);
    return editor;
}


/////////////////////////////////////////////////
/// \brief Returns a pointer to the embedded
/// editor instance. Will return the left
/// or top editor pointer or the secondary editor,
/// if the boolean is set to true. The return
/// value might be a nullptr.
///
/// \param pageNum size_t
/// \param secondary bool
/// \return NumeReEditor*
///
/////////////////////////////////////////////////
NumeReEditor* EditorNotebook::getEditor(size_t pageNum, bool secondary)
{
    if (pageNum >= GetPageCount())
        return nullptr;

    wxSplitterWindow* splitter = static_cast<wxSplitterWindow*>(GetPage(pageNum));

    if (secondary && splitter->GetWindow2())
        return static_cast<NumeReEditor*>(splitter->GetWindow2());
    else if (secondary)
        return nullptr;

    return static_cast<NumeReEditor*>(splitter->GetWindow1());
}


/////////////////////////////////////////////////
/// \brief Returns a pointer to the current
/// viewed editor in this notebook. Will return
/// the left or top editor or the secondary one,
/// if the boolean is set to true. The return
/// value might be a nullptr. This is a
/// convenience wrapper around
/// EditorNotebook::getEditor.
///
/// \param secondary bool
/// \return NumeReEditor*
///
/////////////////////////////////////////////////
NumeReEditor* EditorNotebook::getCurrentEditor(bool secondary)
{
    return getEditor(GetSelection(), secondary);
}


/////////////////////////////////////////////////
/// \brief Returns a pointer to the current
/// focused editor. This will automatically
/// return the secondary editor, it that is the
/// focused one. This handling might be weak. In
/// doubt, the primary editor is returned.
///
/// \return NumeReEditor*
///
/////////////////////////////////////////////////
NumeReEditor* EditorNotebook::getFocusedEditor()
{
    NumeReEditor* first = getCurrentEditor(false);
    NumeReEditor* second = getCurrentEditor(true);

    if (second && second->HasFocus())
        return second;

    return first;
}


/////////////////////////////////////////////////
/// \brief Split the current editor horizontally
/// or vertically (depending on the flag), if it
/// is not already splitted.
///
/// \param pageNum size_t
/// \param horizontal bool
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::split(size_t pageNum, bool horizontal)
{
    if (pageNum >= GetPageCount())
        return;

    wxSplitterWindow* splitter = static_cast<wxSplitterWindow*>(GetPage(pageNum));

    if (!splitter->IsSplit())
    {
        NumeReEditor* edit = static_cast<NumeReEditor*>(splitter->GetWindow1());
        NumeReEditor* secEdit = new NumeReEditor(m_top_parent, m_top_parent->getOptions(), splitter,
                                                 wxID_ANY, m_top_parent->getTerminal()->getSyntax(), m_top_parent->getTerminal());
        secEdit->SetDocPointer(edit->GetDocPointer());
        secEdit->SetFilename(edit->GetFileName(), false);
        secEdit->ToggleSettings(edit->getSettings());
        secEdit->UpdateSyntaxHighlighting();

        if (m_top_parent->getOptions()->GetFoldDuringLoading())
            secEdit->FoldAll();

        secEdit->GotoLine(edit->GetCurrentLine());
        secEdit->EnsureVisible(edit->GetCurrentLine());

        #if wxUSE_DRAG_AND_DROP
        secEdit->SetDropTarget(new NumeReDropTarget(m_top_parent, secEdit, NumeReDropTarget::EDITOR));
        #endif


        if (horizontal)
            splitter->SplitHorizontally(splitter->GetWindow1(), secEdit);
        else
            splitter->SplitVertically(splitter->GetWindow1(), secEdit);

        splitter->SetSashGravity(0.5);
    }
}


/////////////////////////////////////////////////
/// \brief Remove the splitted view.
///
/// \param pageNum size_t
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::unsplit(size_t pageNum)
{
    if (pageNum >= GetPageCount())
        return;

    wxSplitterWindow* splitter = static_cast<wxSplitterWindow*>(GetPage(pageNum));

    if (splitter->IsSplit())
    {
        wxWindow* sec = splitter->GetWindow2();
        splitter->Unsplit(sec);
        Refresh();
        sec->Destroy();
    }
}


/////////////////////////////////////////////////
/// \brief Check, whether the editor is currently
/// splitted.
///
/// \param pageNum size_t
/// \return bool
///
/////////////////////////////////////////////////
bool EditorNotebook::isSplit(size_t pageNum) const
{
    if (pageNum >= GetPageCount())
        return false;

    wxSplitterWindow* splitter = static_cast<wxSplitterWindow*>(GetPage(pageNum));

    return splitter->IsSplit();
}


/////////////////////////////////////////////////
/// \brief Event handler, if the user drags the
/// sash to the outermost edge.
///
/// \param event wxSplitterEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnUnsplit(wxSplitterEvent& event)
{
    event.GetWindowBeingRemoved()->Destroy();
}


/////////////////////////////////////////////////
/// \brief Shows the context menu for the current
/// tab.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnTabRightClicked(wxAuiNotebookEvent& event)
{
    wxPoint pt = wxGetMousePosition();
	int pageNum = GetTabFromPoint(pt);
	pt -= GetScreenRect().GetTopLeft();

    if (pageNum < 0)
		return;

    g_logger.debug("Clicked on page " + toString(pageNum) + " with title " + GetPageText(pageNum).ToStdString());

    VersionControlSystemManager manager(m_top_parent);
    wxString filename = getEditor(pageNum)->GetFileNameAndPath();

	m_top_parent->SetIntVar(VN_CLICKEDTAB, pageNum);
	wxMenu popupMenu;
	popupMenu.Append(ID_MENU_CLOSETAB, _guilang.get("GUI_EDITOR_TAB_CLOSE"));
	popupMenu.Append(ID_MENU_CLOSEALL, _guilang.get("GUI_EDITOR_TAB_CLOSEALL"));
	popupMenu.Append(ID_MENU_CLOSEOTHERS, _guilang.get("GUI_EDITOR_TAB_CLOSEOTHERS"));
	popupMenu.AppendSeparator();
	popupMenu.Append(ID_MENU_OPEN_FOLDER, _guilang.get("GUI_EDITOR_TAB_OPENFOLDER"));
	popupMenu.AppendSeparator();
	popupMenu.Append(ID_MENU_RUN_FROM_TAB, _guilang.get("GUI_TB_RUN"));
	popupMenu.AppendSeparator();

    if (isSplit(pageNum))
        popupMenu.Append(ID_MENU_UNSPLIT_TAB, _guilang.get("GUI_EDITOR_TAB_UNSPLIT"));
    else
    {
        wxMenu* splitMenu = new wxMenu();
        splitMenu->Append(ID_MENU_SPLIT_TAB_H, _guilang.get("GUI_EDITOR_TAB_SPLIT_H"));
        splitMenu->Append(ID_MENU_SPLIT_TAB_V, _guilang.get("GUI_EDITOR_TAB_SPLIT_V"));
        popupMenu.Append(wxID_ANY, _guilang.get("GUI_EDITOR_TAB_SPLITTING"), splitMenu);
    }

	// Append the file revision menu, if the file has revisions
	if (manager.hasRevisions(filename))
    {
        popupMenu.AppendSeparator();
        popupMenu.Append(ID_MENU_SHOW_REVISIONS_FROM_TAB, _guilang.get("GUI_TREE_PUP_SHOWREVISIONS"));
    }

	PopupMenu(&popupMenu, pt);
}


/////////////////////////////////////////////////
/// \brief This event handler fixes the issue
/// that the control does not correctly update
/// the selected page during moving a page.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnTabMove(wxAuiNotebookEvent& event)
{
    int tab = GetTabFromPoint(wxGetMousePosition());

    if (tab != wxNOT_FOUND)
        ChangeSelection(tab);

    event.Skip();
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


/////////////////////////////////////////////////
/// \brief Returns the tab at the defined
/// absolute coordinates. Actually more or less
/// the same than the generic HitTest() member
/// functions.
///
/// \param pt const wxPoint&
/// \return int
///
/////////////////////////////////////////////////
int EditorNotebook::GetTabFromPoint(const wxPoint& pt)
{
    wxPoint client_pt = ScreenToClient(pt);
    wxAuiTabCtrl* tabCtrl = GetTabCtrlFromPoint(client_pt);

    if (!tabCtrl)
    {
        g_logger.debug("Found page: false (out of bounds)");
        return wxNOT_FOUND;
    }

    wxPoint tl = tabCtrl->GetScreenRect().GetTopLeft();
    wxWindow* win;
    bool success = tabCtrl->TabHitTest(pt.x-tl.x, pt.y-tl.y, &win);
	g_logger.debug("Found page: " + toString(success));

	if (!success)
        return wxNOT_FOUND;

	return m_tabs.GetIdxFromWindow(win);
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
		if (getEditor(nPage) == page)
			return nPage;
	return -1;
}


/////////////////////////////////////////////////
/// \brief Executes the closing command, if the
/// user clickes with the middle mouse button on
/// a tab.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnButtonClicked(wxAuiNotebookEvent& event)
{
    int pageNum = GetTabFromPoint(wxGetMousePosition());

	if (pageNum < 0)
		return;

	m_top_parent->SetIntVar(VN_CLICKEDTAB, pageNum);

	wxCommandEvent command;
	command.SetId(ID_MENU_CLOSETAB);
	command.SetEventType(wxEVT_MENU); //10019//wxEVT_MIDDLE_UP
	m_top_parent->GetEventHandler()->ProcessEvent(command);
}


/////////////////////////////////////////////////
/// \brief Executes the closing command, if the
/// user clickes on the button of a tab.
///
/// \param event wxAuiNotebookEvent&
/// \return void
///
/////////////////////////////////////////////////
void EditorNotebook::OnTabMiddleClicked(wxAuiNotebookEvent& event)
{
    int pageNum = GetTabFromPoint(wxGetMousePosition());

	if (pageNum < 0)
		return;

	m_top_parent->SetIntVar(VN_CLICKEDTAB, pageNum);

	wxCommandEvent command;
	command.SetId(ID_MENU_CLOSETAB);
	command.SetEventType(wxEVT_MENU); //10019//wxEVT_MIDDLE_UP
	m_top_parent->GetEventHandler()->ProcessEvent(command);
}


