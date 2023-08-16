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
#include "../documentationbrowser.hpp"
#include "../NumeReWindow.h"
#include <wx/html/htmprint.h>

extern wxPrintData* g_printData;
extern wxPageSetupData* g_pageSetupData;


BEGIN_EVENT_TABLE(HelpViewer, wxHtmlWindow)
    EVT_KEY_DOWN        (HelpViewer::OnKeyDown)
//    EVT_ENTER_WINDOW    (HelpViewer::OnEnter)
    EVT_HTML_LINK_CLICKED(-1, HelpViewer::OnLinkClick)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Set the contents to be displayed in
/// this window.
///
/// \param source const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool HelpViewer::SetPage(const wxString& source)
{
    if (!vHistory.size())
        vHistory.push_back(source);

    bool res = wxHtmlWindow::SetPage(source);
    m_browser->setCurrentTabText(GetOpenedPageTitle());

    return res;
}


/////////////////////////////////////////////////
/// \brief Public member function to display a
/// content in the viewer window. The type of the
/// content is determined in this function and
/// handled correspondingly.
///
/// \param docID wxString
/// \return bool
///
/////////////////////////////////////////////////
bool HelpViewer::ShowPageOnItem(wxString docID)
{
    wxString pageContent;
    bool openself = true;

    // Determine the type of the content: is it a link, a html page
    // or a search keyword
    if (docID.substr(0,10) == "history://")
    {
        // History link: only for legacy reasons
        //if (docID.find("?frame=new") != string::npos)
        //    openself = false;

        docID.erase(0,10);

        if (docID.find('?') != std::string::npos)
            docID.erase(docID.find('?'));

        // Redirect to the corresponding public interface functions
        if (docID == "back")
        {
            return HistoryGoBack();
        }
        else if (docID == "forward")
        {
            return HistoryGoForward();
        }
        else
            return false;
    }
    else if (docID.substr(0,7) == "nhlp://")
    {
        // Regular link
        //
        // Determine first, if the link shall be opened in the current
        // window or in a new one
        if (docID.find("?frame=new") != std::string::npos)
            openself = false;

        // Extract the search keyword from the link
        docID.erase(0,7);

        if (docID.find('?') != std::string::npos)
            docID.erase(docID.find('?'));

        // If the target is not the current window, redirect the the
        // link to the main window, which will create a new window
        if (!openself)
            return m_browser->createNewPage(docID);

        // Get the page content from the kernel
        pageContent = m_mainFrame->GetDocContent(docID);

        if (!pageContent.length())
            return false;

        // Open the page
		if (m_nHistoryPointer+1 != vHistory.size())
		{
			// erase the obsolete history
			vHistory.erase(vHistory.begin()+1+m_nHistoryPointer, vHistory.end());
		}

		vHistory.push_back(docID);
		m_nHistoryPointer++;
		this->SetPage(pageContent);
    }
    else if (docID.substr(0,15) == "<!DOCTYPE html>")
    {
        // This is a html page, display it directly
		this->SetPage(docID);
    }
    else if (docID.find("://") == std::string::npos)
    {
        // This is a search keyword, get the contents from
        // the kernel
        pageContent = m_mainFrame->GetDocContent(docID);

        if (!pageContent.length())
            return false;

        // Display the page
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
    else
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Public member function to go one step
/// back in the history.
///
/// \return bool
///
/////////////////////////////////////////////////
bool HelpViewer::HistoryGoBack()
{
    if (m_nHistoryPointer)
    {
        // Get the content of the history and decrement the pointer
        wxString pageContent = vHistory[m_nHistoryPointer-1];
        m_nHistoryPointer--;

        // Depending on the type of the content, assign it directly
        // or ask the kernel for the page contents
        if (pageContent.substr(0,15) == "<!DOCTYPE html>")
        {
            return this->SetPage(pageContent);
        }
        else if  (pageContent.length())
        {
            return this->SetPage(m_mainFrame->GetDocContent(pageContent));
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Public member function to go one step
/// forward in the history.
///
/// \return bool
///
/////////////////////////////////////////////////
bool HelpViewer::HistoryGoForward()
{
    if (m_nHistoryPointer+1 < vHistory.size())
    {
        // Get the content of the history and increment the pointer
        wxString pageContent = vHistory[m_nHistoryPointer+1];
        m_nHistoryPointer++;

        // Depending on the type of the content, assign it directly
        // or ask the kernel for the page contents
        if (pageContent.substr(0,15) == "<!DOCTYPE html>")
        {
            return this->SetPage(pageContent);
        }
        else if  (pageContent.length())
        {
            return this->SetPage(m_mainFrame->GetDocContent(pageContent));
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Public member function to return to
/// the home page.
///
/// \return bool
///
/////////////////////////////////////////////////
bool HelpViewer::GoHome()
{
    return ShowPageOnItem("numere");
}


/////////////////////////////////////////////////
/// \brief Public member function to display the
/// index page.
///
/// \return bool
///
/////////////////////////////////////////////////
bool HelpViewer::GoIndex()
{
    return ShowPageOnItem("idx");
}


/////////////////////////////////////////////////
/// \brief Static helper function to create a
/// basic printout object.
///
/// \return wxHtmlPrintout*
///
/////////////////////////////////////////////////
static wxHtmlPrintout* createPrintout()
{
    wxHtmlPrintout* printout = new wxHtmlPrintout();
    printout->SetFonts(wxEmptyString, "Consolas");
    printout->SetMargins(12.6f, 12.6f, 12.6f, 12.6f, 2.5f);
    printout->SetFooter("<div align=\"center\">@PAGENUM@ / @PAGESCNT@</div>");

    return printout;
}


/////////////////////////////////////////////////
/// \brief Public member function to open the
/// print preview page.
///
/// \return bool
///
/////////////////////////////////////////////////
bool HelpViewer::Print()
{
    // If the printing setup was not filled correctly,
    // ask the user to provide the necessary data
    if (!g_printData->IsOk())
        m_mainFrame->OnPrintSetup();

    wxPrintDialogData printDialogData( *g_printData);

    // Create a new html printout class and apply the
    // necessary settings
    wxHtmlPrintout* printout = createPrintout();
    wxHtmlPrintout* printoutForPrinting = createPrintout();

    // Obtain the content of the page from the history
    wxString htmlText = vHistory[m_nHistoryPointer];

    // Depending on the type of the content of the history,
    // assign it directly or obtain the page data from the
    // kernel
    if (htmlText.substr(0,15) == "<!DOCTYPE html>")
    {
        printout->SetHtmlText(htmlText);
        printoutForPrinting->SetHtmlText(htmlText);
    }
    else
    {
        printout->SetHtmlText(m_mainFrame->GetDocContent(htmlText));
        printoutForPrinting->SetHtmlText(m_mainFrame->GetDocContent(htmlText));
    }

    // Create a new preview object
    wxPrintPreview *preview = new wxPrintPreview(printout, printoutForPrinting, g_printData);

    // Ensure that the preview is filled correctly
    if (!preview->Ok())
    {
        delete preview;
        wxMessageBox(_guilang.get("GUI_PREVIEW_ERROR"), _guilang.get("GUI_PREVIEW_ERROR_HEAD"), wxOK | wxICON_WARNING);
        return false;
    }

    // Obtain the screen size and create a new preview frame
    wxRect rect = m_mainFrame->DeterminePrintSize();
    wxPreviewFrame *frame = new wxPreviewFrame (preview, this, _guilang.get("GUI_PREVIEW_HEAD"));

    // Start the frame and display it
    frame->SetSize(rect);
    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
    frame->Maximize();

    return true;
}


/////////////////////////////////////////////////
/// \brief Event handler, which gets fired when
/// the user presses a key.
///
/// \param event wxKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void HelpViewer::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_browser->Close();
}


/////////////////////////////////////////////////
/// \brief Event handler to automatically focus
/// the window below the mouse pointer.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void HelpViewer::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Event handler for the case that the
/// user clicks on a link in the document.
///
/// \param event wxHtmlLinkEvent&
/// \return void
///
/////////////////////////////////////////////////
void HelpViewer::OnLinkClick(wxHtmlLinkEvent& event)
{
    wxString linkadress = event.GetLinkInfo().GetHref();
    ShowPageOnItem(linkadress);
}

