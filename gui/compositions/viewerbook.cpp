/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "viewerbook.hpp"
#include "tabartprovider.hpp"
#include "../../common/Options.h"
#include <wx/aui/dockart.h>

BEGIN_EVENT_TABLE(ViewerBook, wxAuiNotebook)
	EVT_AUINOTEBOOK_DRAG_MOTION     (-1, ViewerBook::OnTabMove)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief ViewerBook constructor. Sets
/// automatically the correct tab art provider.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param activeCol const wxColour&
/// \param position const wxPoint&
/// \param size const wxSize&
/// \param style int
/// \param name const wxString&
///
/////////////////////////////////////////////////
ViewerBook::ViewerBook(wxWindow* parent, wxWindowID id, const wxColour& activeCol, const wxPoint& position, const wxSize& size, int style, const wxString& name)
            : wxAuiNotebook(parent, id, position, size, style | wxAUI_NB_TAB_MOVE | wxNB_TOP), m_skipFocus(false)
{
    SetArtProvider(new TabArtProvider(activeCol));
    // This is the way, the internal sash can be coloured!
    m_mgr.GetArtProvider()->SetColor(wxAUI_DOCKART_SASH_COLOUR, activeCol.ChangeLightness(Options::PANEL));
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
void ViewerBook::OnTabMove(wxAuiNotebookEvent& event)
{
    int tab = GetTabFromPoint(wxGetMousePosition());

    if (tab != wxNOT_FOUND)
        ChangeSelection(tab);

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
int ViewerBook::GetTabFromPoint(const wxPoint& pt)
{
    wxPoint client_pt = ScreenToClient(pt);
    wxAuiTabCtrl* tabCtrl = GetTabCtrlFromPoint(client_pt);

    if (!tabCtrl)
    {
        return wxNOT_FOUND;
    }

    wxPoint tl = tabCtrl->GetScreenRect().GetTopLeft();
    wxWindow* win;
    bool success = tabCtrl->TabHitTest(pt.x-tl.x, pt.y-tl.y, &win);

	if (!success)
        return wxNOT_FOUND;

	return m_tabs.GetIdxFromWindow(win);
}

