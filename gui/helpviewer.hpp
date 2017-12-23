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

#ifndef HELPVIEWER_HPP
#define HELPVIEWER_HPP

#include <wx/wxhtml.h>
#include <wx/wx.h>
#include <vector>

using namespace std;

class NumeReWindow;

class HelpViewer : public wxHtmlWindow
{
    public:
        HelpViewer(wxWindow* parent, NumeReWindow* m_main) : wxHtmlWindow(parent), m_mainFrame(m_main), m_nHistoryPointer(0) {SetFonts(wxEmptyString, "Consolas");};
        virtual bool SetPage(const wxString& source);

    private:
        void OnKeyDown(wxKeyEvent& event);
        void OnEnter(wxMouseEvent& event);
        void OnLinkClick(wxHtmlLinkEvent& event);

        NumeReWindow* m_mainFrame;
        vector<wxString> vHistory;
        size_t m_nHistoryPointer;

        DECLARE_EVENT_TABLE();
};

#endif
