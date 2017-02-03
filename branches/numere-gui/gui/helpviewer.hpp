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


#include <wx/wxhtml.h>
#include <wx/wx.h>

class HelpViewer : public wxHtmlWindow
{
    public:
        HelpViewer(wxWindow* parent) : wxHtmlWindow(parent) {};

    private:
        void OnKeyDown(wxKeyEvent& event);
        void OnEnter(wxMouseEvent& event);

        DECLARE_EVENT_TABLE();
};

