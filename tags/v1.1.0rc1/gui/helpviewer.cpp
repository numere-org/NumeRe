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



#include "helpviewer.hpp"

BEGIN_EVENT_TABLE(HelpViewer, wxHtmlWindow)
    EVT_KEY_DOWN        (HelpViewer::OnKeyDown)
    EVT_ENTER_WINDOW    (HelpViewer::OnEnter)
END_EVENT_TABLE()

void HelpViewer::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
}


void HelpViewer::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}

