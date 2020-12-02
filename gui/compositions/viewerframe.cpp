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

#include "viewerframe.hpp"
#include "../NumeReWindow.h"


BEGIN_EVENT_TABLE(ViewerFrame, wxFrame)
    EVT_KEY_DOWN        (ViewerFrame::OnKeyDown)
    EVT_SET_FOCUS       (ViewerFrame::OnFocus)
    EVT_ENTER_WINDOW    (ViewerFrame::OnEnter)
    EVT_CLOSE           (ViewerFrame::OnClose)
END_EVENT_TABLE()

/////////////////////////////////////////////////
/// \brief This event handler closes the frame,
/// if the user presses ESC.
///
/// \param event wxKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void ViewerFrame::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        this->Close();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This event handler passes the keyboard
/// focus down to the first child in the window
/// list, if the frame gets keyboard focus.
///
/// \param event wxFocusEvent&
/// \return void
///
/////////////////////////////////////////////////
void ViewerFrame::OnFocus(wxFocusEvent& event)
{
    this->SetTransparent(wxIMAGE_ALPHA_OPAQUE);
    if (this->GetChildren().size())
        this->GetChildren().front()->SetFocus();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This event handler get the keyboard
/// focus, if the user enters with the mouse.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void ViewerFrame::OnEnter(wxMouseEvent& event)
{
    if (this->GetChildren().size())
        this->GetChildren().front()->SetFocus();
    else
        this->SetFocus();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This event handler informs all child
/// windows that this frame will now close. It
/// furthermore automatically unregisters itself
/// in the opened window list.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void ViewerFrame::OnClose(wxCloseEvent& event)
{
    auto children = this->GetChildren();

    if (children.size())
    {
        for (auto iter = children.begin(); iter != children.end(); iter++)
            static_cast<wxWindow*>(*iter)->Close();
    }

    // Unregister this window to avoid multiple
    // destroys of this instance
    static_cast<NumeReWindow*>(GetParent())->unregisterWindow(this);

    event.Skip();
}

