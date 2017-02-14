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


BEGIN_EVENT_TABLE(ViewerFrame, wxFrame)
    EVT_KEY_DOWN        (ViewerFrame::OnKeyDown)
    EVT_SET_FOCUS       (ViewerFrame::OnFocus)
    EVT_ENTER_WINDOW    (ViewerFrame::OnEnter)
    EVT_CLOSE           (ViewerFrame::OnClose)
END_EVENT_TABLE()

void ViewerFrame::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        this->Close();
}

void ViewerFrame::OnFocus(wxFocusEvent& event)
{
    this->SetTransparent(wxIMAGE_ALPHA_OPAQUE);
    if (this->GetChildren().size())
        this->GetChildren().front()->SetFocus();
    event.Skip();
}

void ViewerFrame::OnEnter(wxMouseEvent& event)
{
    if (this->GetChildren().size())
        this->GetChildren().front()->SetFocus();
    else
        this->SetFocus();
    event.Skip();
}

void ViewerFrame::OnClose(wxCloseEvent& event)
{
    if (this->GetChildren().size())
        this->GetChildren().front()->Close();
    event.Skip();
}

