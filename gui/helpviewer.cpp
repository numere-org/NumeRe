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

bool HelpViewer::ShowPageOnItem(wxString docID)
{
    wxString pageContent = "";
    bool openself = true;

    if (docID.substr(0,10) == "history://")
    {
        if (docID.find("?frame=new") != string::npos)
            openself = false;

        docID.erase(0,10);

        if (docID.find('?') != string::npos)
            docID.erase(docID.find('?'));

        if (docID == "back" && m_nHistoryPointer)
        {
            pageContent = vHistory[m_nHistoryPointer-1];
            if (openself)
                m_nHistoryPointer--;
        }
        else if (docID == "forward" && m_nHistoryPointer+1 < vHistory.size())
        {
            pageContent = vHistory[m_nHistoryPointer+1];
            if (openself)
                m_nHistoryPointer++;
        }
        else
            return false;

        if (pageContent.substr(0,15) == "<!DOCTYPE html>")
        {
            if (openself)
            {
                this->SetPage(pageContent);
            }
            else
                m_mainFrame->ShowHelp(pageContent);
        }
        else if (pageContent.length())
        {
            if (openself)
                this->SetPage(m_mainFrame->GetDocContent(pageContent));
            else
                m_mainFrame->ShowHelp(pageContent);
        }
    }
    else if (docID.substr(0,7) == "nhlp://")
    {

        if (docID.find("?frame=new") != string::npos)
            openself = false;

        docID.erase(0,7);

        if (docID.find('?') != string::npos)
            docID.erase(docID.find('?'));

        if (!openself)
            return m_mainFrame->ShowHelp(docID);

        pageContent = m_mainFrame->GetDocContent(docID);

        if (!pageContent.length())
            return false;

        if (openself)
        {
            if (m_nHistoryPointer+1 != vHistory.size())
            {
                // erase the obsolete history
                vHistory.erase(vHistory.begin()+1+m_nHistoryPointer, vHistory.end());
            }

            vHistory.push_back(docID);
            m_nHistoryPointer++;
            this->SetPage(pageContent);
        }
        else
        {
            m_mainFrame->ShowHelp(pageContent);
        }
    }
    else if (docID.find("://") == string::npos)
    {
        pageContent = m_mainFrame->GetDocContent(docID);

        if (!pageContent.length())
            return false;

        if (m_nHistoryPointer+1 != vHistory.size() && vHistory.size())
        {
            // erase the obsolete history
            vHistory.erase(vHistory.begin()+1+m_nHistoryPointer, vHistory.end());
        }

        if (vHistory.size())
        {
            vHistory.push_back(docID);
            m_nHistoryPointer++;
        }

        this->SetPage(pageContent);
    }
    else if (docID.substr(0,15) == "<!DOCTYPE html>")
    {
        if (openself)
        {
            this->SetPage(docID);
        }
        else
            m_mainFrame->ShowHelp(docID);
    }
    else
        return false;

    return true;
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
    ShowPageOnItem(linkadress);
}

