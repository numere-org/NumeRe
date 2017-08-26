
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


#ifndef VIEWERPANEL_HPP
#define VIEWERPANEL_HPP
#include <wx/wx.h>
#include <wx/panel.h>

class ViewerPanel : public wxPanel
{
    public:
        ViewerPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& position = wxDefaultPosition, const wxSize& size = wxDefaultSize, int style = 0, const wxString& name = wxPanelNameStr)
            : wxPanel(parent, id, position, size, style, name), m_skipFocus(false) {}

        void OnEnter(wxMouseEvent& event);
        inline void toggleSkipFocus()
            {
                m_skipFocus = !m_skipFocus;
            }
    private:
        bool m_skipFocus;

        DECLARE_EVENT_TABLE();
};
#endif // VIEWERPANEL_HPP

