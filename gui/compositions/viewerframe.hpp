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

#include <wx/wx.h>

#ifndef VIEWERFRAME_HPP
#define VIEWERFRAME_HPP

/////////////////////////////////////////////////
/// \brief This class generalizes a set of basic
/// floating window functionalities like being
/// closable by pressing ESC or to get the
/// keyboard focus automatically.
/////////////////////////////////////////////////
class ViewerFrame : public wxFrame
{
    public:
        ViewerFrame(wxWindow* parent, const wxString& title) : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxFRAME_FLOAT_ON_PARENT | wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxMAXIMIZE_BOX | wxMINIMIZE_BOX) {};

        void OnKeyDown(wxKeyEvent& event);
        void OnFocus(wxFocusEvent& event);
        void OnEnter(wxMouseEvent& event);
        void OnClose(wxCloseEvent& event);

        DECLARE_EVENT_TABLE();
};

#endif

