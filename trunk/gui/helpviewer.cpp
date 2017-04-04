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
#include "NumeReWindow.h"

BEGIN_EVENT_TABLE(HelpViewer, wxHtmlWindow)
    EVT_KEY_DOWN        (HelpViewer::OnKeyDown)
    EVT_ENTER_WINDOW    (HelpViewer::OnEnter)
    EVT_HTML_LINK_CLICKED(-1, HelpViewer::OnLinkClick)
END_EVENT_TABLE()

bool HelpViewer::SetPage(const wxString& source)
{
    if (!vHistory.size())
        vHistory.push_back(source);
    return this->wxHtmlWindow::SetPage(source);
}

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

void HelpViewer::OnLinkClick(wxHtmlLinkEvent& event)
{
    wxString linkadress = event.GetLinkInfo().GetHref();
    wxString pageContent = "";
    bool openself = true;
    if (linkadress.substr(0,10) == "history://")
    {
        if (linkadress.find("?frame=new") != string::npos)
            openself = false;

        linkadress.erase(0,10);
        if (linkadress.find('?') != string::npos)
            linkadress.erase(linkadress.find('?'));

        if (linkadress == "back" && m_nHistoryPointer)
        {
            pageContent = vHistory[m_nHistoryPointer-1];
            if (openself)
                m_nHistoryPointer--;
        }
        else if (linkadress == "forward" && m_nHistoryPointer+1 < vHistory.size())
        {
            pageContent = vHistory[m_nHistoryPointer+1];
            if (openself)
                m_nHistoryPointer++;
        }
        else
            return;
        if (pageContent.substr(0,15) == "<!DOCTYPE html>")
        {
            if (openself)
            {
                this->SetPage(pageContent);
            }
            else
                m_mainFrame->openHTML(pageContent);
        }
        else if (pageContent.length())
        {
            if (openself)
                this->SetPage(m_mainFrame->GetDocContent(pageContent));
            else
                m_mainFrame->openHTML(m_mainFrame->GetDocContent(pageContent));
        }
    }
    else if (linkadress.substr(0,7) == "nhlp://")
    {

        if (linkadress.find("?frame=new") != string::npos)
            openself = false;

        linkadress.erase(0,7);
        if (linkadress.find('?') != string::npos)
            linkadress.erase(linkadress.find('?'));

        pageContent = m_mainFrame->GetDocContent(linkadress);

        if (!pageContent.length())
            return;
        if (openself)
        {
            if (m_nHistoryPointer+1 != vHistory.size())
            {
                // erase the obsolete history
                vHistory.erase(vHistory.begin()+1+m_nHistoryPointer, vHistory.end());
            }
            vHistory.push_back(linkadress);
            m_nHistoryPointer++;
            this->SetPage(pageContent);
        }
        else
        {
            m_mainFrame->openHTML(pageContent);
        }
    }
}

