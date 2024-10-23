/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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


#include "numere.hpp"

#include <wx/display.h>
#include <wx/stdpaths.h>
#include <wx/splash.h>

#include "NumeReWindow.h"
#include "../common/ipc.hpp"
#include "../common/Options.h"

#include "../kernel/core/io/logger.hpp"
#include "../kernel/core/ParserLib/muParser.h"
#include "../kernel/core/ui/error.hpp"
#include "../kernel/core/version.h"

IMPLEMENT_APP(NumeReApp)

/////////////////////////////////////////////////
/// \brief Static helper function to determine
/// the window size and position if necessary.
///
/// \param sWinSize std::string&
/// \return wxRect
///
/////////////////////////////////////////////////
static wxRect determineWindowSize(std::string& sWinSize)
{
    wxRect winRect;

    if (sWinSize == "{}")
        return winRect;

    EndlessVector<StringView> vArgs = getAllArguments(StringView(sWinSize, 1, sWinSize.length()-2));

    if (vArgs.size() < 4)
        sWinSize = "{}";
    else
    {
        winRect.x = StrToInt(vArgs[0].subview(2).to_string());
        winRect.y = StrToInt(vArgs[1].subview(2).to_string());
        winRect.width = StrToInt(vArgs[2].subview(2).to_string());
        winRect.height = StrToInt(vArgs[3].subview(2).to_string());

        int currDisplay = wxNOT_FOUND;
        wxRect dispdim;

        // Find the correct display
        for (size_t i = 0; i < wxDisplay::GetCount(); i++)
        {
            dispdim = wxDisplay(i).GetGeometry();

            if (dispdim.Contains(winRect.x+10, winRect.y+10))
            {
                currDisplay = i;
                break;
            }
        }

        g_logger.info("User-selected display: " + toString(currDisplay) + " @ {x=" + toString(winRect.x) + " y=" + toString(winRect.y)+"}");

        if (currDisplay == wxNOT_FOUND)
        {
            sWinSize = "{}";
            return winRect;
        }

        winRect.width = std::min(dispdim.width-winRect.x+dispdim.x, winRect.width);
        winRect.height = std::min(dispdim.height-winRect.y+dispdim.y, winRect.height);

        g_logger.info("Final window dimensions: w=" + toString(winRect.width) + " h=" + toString(winRect.height));

        if (winRect.width < 200 || winRect.height < 200)
            sWinSize = "{}";
    }

    return winRect;
}



//----------------------------------------------------------------------
/////////////////////////////////////////////////
/// \brief "Main program" equivalent: the program
/// execution "starts" here. If we detect an
/// already instance of NumeRe, we will send the
/// command line contents to the existing
/// instance and cancel the start up here.
///
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReApp::OnInit()
{
    #ifdef DO_LOG
    g_logger.setLoggingLevel(Logger::LVL_DEBUG);
    #endif // DO_LOG

    g_logger.push_info(LOGGER_STARTUP_LINE);
    g_logger.push_info("NumeRe v " + sVersion + " (var. " + AutoVersion::UBUNTU_VERSION_STYLE + ")");
    g_logger.write_system_information();
    g_logger.info("Starting up.");

    wxString sInstanceLocation = wxStandardPaths::Get().GetDataDir();
    sInstanceLocation.Replace(":\\", "~");
    sInstanceLocation.Replace("\\", "~");

    g_logger.debug("Creating single instance checker.");
    m_singlinst = new wxSingleInstanceChecker("NumeRe::" + sInstanceLocation + "::" + wxGetUserId(), wxGetHomeDir());
    m_DDEServer = nullptr;

    // Avoid starting up a second instance
    if (m_singlinst->IsAnotherRunning())
    {
        // Create a new client
        wxLogNull ln; // own error checking implemented -> avoid debug warnings
        DDE::Client* client = new DDE::Client;
        DDE::Connection* connection = (DDE::Connection*)client->MakeConnection("localhost", DDE_SERVICE, DDE_TOPIC);

        if (connection)
        {
            // don't eval here just forward the whole command line to the other instance
            wxString cmdLine;

            for (int i = 1 ; i < argc; ++i)
                cmdLine += wxString(argv[i]) + ' ';

            if (!cmdLine.IsEmpty())
            {
                // escape openings and closings so it is easily possible to find the end on the rx side
                cmdLine.Replace(_T("("), _T("\\("));
                cmdLine.Replace(_T(")"), _T("\\)"));
                connection->Execute(_T("[CmdLine({") + cmdLine + _T("})]"));
            }

            connection->Disconnect();
            delete connection;
        }

        // free memory DDE-/IPC-clients
        delete client;
        delete m_singlinst;
        return false;
    }


    std::setlocale(LC_ALL, "C");
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());

    wxInitAllImageHandlers();
    wxBitmap splashImage;
    wxSplashScreen* splash = nullptr;
    g_logger.debug("Loading splash image.");
    wxString splashPath = f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR)+"icons\\splash.png";

    if (splashImage.LoadFile(splashPath, wxBITMAP_TYPE_PNG))
        splash = new wxSplashScreen(splashImage, wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT, 0, nullptr,
                                    wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
    else
        g_logger.error("The splash at '" + splashPath.ToStdString() + "' could not be found or loaded.");

    g_findReplace = nullptr;

    g_logger.info("Creating graphical user interface.");

    // Create and initialize the main frame. Will also
    // include loading the configuration, loading existing
    // caches and preparing the editor.
    m_mainWindow = new NumeReWindow("NumeRe (v" + sVersion + ")", wxDefaultPosition, wxDefaultSize);

    // The logger should now know, if we start from
    // a crash
    g_logger.debug("Starting DDE server instance.");

    // Create the DDE server for the first (main)
    // instance of the application
    m_DDEServer = new DDE::Server(m_mainWindow);
    m_DDEServer->Create(DDE_SERVICE);

    g_logger.debug("Determining window size.");

    if (m_mainWindow->getOptions()->getSetting(SETTING_B_SAVEWINDOWSIZE).active())
    {
        std::string sWinSize = m_mainWindow->getOptions()->getSetting(SETTING_S_WINDOWSIZE).stringval();
        g_logger.info("User-selected window size: " + sWinSize);
        wxRect winRect = determineWindowSize(sWinSize);

        if (sWinSize != "{}")
            m_mainWindow->SetSize(winRect);
        else
            m_mainWindow->Maximize();
    }
    else
        m_mainWindow->Maximize();

    g_logger.debug("Showing main frame.");
    m_mainWindow->Show(true);

    // Passed commandline items
    wxArrayString wxArgV;
    for (int i = 0; i < argc; i++)
    {
        wxArgV.Add(argv[i]);
    }

    // force the history window to perform a
    // page down to scroll to its actually
    // last position
    m_mainWindow->forceHistoryPageDown();
    m_mainWindow->EvaluateCommandLine(wxArgV);
    m_mainWindow->Ready();
    g_logger.info("NumeRe ready.");

    // Destroy the splash image once the window
    // is ready
    if (splash)
        splash->Destroy();

    m_mainWindow->ShowEditorCarets();

    // Any files, which could not be loaded
    if (m_mainWindow->m_UnrecoverableFiles.length())
    {
        g_logger.warning("The files '" + m_mainWindow->m_UnrecoverableFiles + "' could not be restored.");
        m_mainWindow->Refresh();
        m_mainWindow->Update();
        wxMessageBox(_guilang.get("GUI_DLG_SESSION_RECREATIONERROR", m_mainWindow->m_UnrecoverableFiles),
                     _guilang.get("GUI_DLG_SESSION_ERROR"), wxICON_ERROR, m_mainWindow);
    }

    // Do we recover from a crash?
    if (g_logger.startFromCrash())
    {
        m_mainWindow->Refresh();
        m_mainWindow->Update();

        if (wxMessageBox(_guilang.get("GUI_DLG_START_AFTER_CRASH_QUESTION"),
                         _guilang.get("GUI_DLG_START_AFTER_CRASH"), wxICON_QUESTION | wxCENTER | wxYES | wxNO, m_mainWindow) == wxYES)
            m_mainWindow->OnReportIssue(ERR_CRASH);
    }

    // Did we already experience some errors during startup?
    if (g_logger.hasLoggedError())
    {
        m_mainWindow->Refresh();
        m_mainWindow->Update();

        if (wxMessageBox("We are sorry, we found problems during application start-up indicating missing or corrupted language or definition files.\nWe recommend that you perform a new and clean install into the pre-selected directory 'C:/Software/NumeRe'.\n\nDo you want to send us an error report?",
                         "NumeRe: Issue Reporter", wxICON_QUESTION | wxCENTER | wxYES | wxNO, m_mainWindow) == wxYES)
            m_mainWindow->OnReportIssue(ERR_STARTUP);
    }

    // Tip of the day
    if (m_mainWindow->showTipAtStartup)
    {
        g_logger.debug("Showing tip of the day.");
        wxArrayString DialogText;
        DialogText.Add(_guilang.get("GUI_TIPOTD_HEAD"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_DYK"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_NEXT"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_STAS"));
        DialogText.Add(_guilang.get("GUI_TIPOTD_CLOSE"));
        m_mainWindow->updateTipAtStartupSetting(ShowTip(m_mainWindow, m_mainWindow->tipProvider, DialogText));
    }

    delete m_mainWindow->tipProvider;
    m_mainWindow->tipProvider = nullptr;

    return true;
}


/////////////////////////////////////////////////
/// \brief Empty destructor.
/////////////////////////////////////////////////
NumeReApp::~NumeReApp()
{
}


/////////////////////////////////////////////////
/// \brief Called on application shut down. Will
/// free the memory of the IPC class instances.
///
/// \return int
///
/////////////////////////////////////////////////
int NumeReApp::OnExit()
{
    if (m_singlinst)
        delete m_singlinst;

    if (m_DDEServer)
        delete m_DDEServer;

    return 0;
}


/////////////////////////////////////////////////
/// \brief This handler should be called, if an
/// unhandled exception propagated through the
/// event loop. We'll try to recover from this
/// issue here. However, it might not be possible.
///
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReApp::OnExceptionInMainLoop()
{
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        g_logger.error(std::string("Unexpected exception: ") + e.what());
        wxMessageBox(std::string("An unexpected exception was caught. If it is reproducable, consider informing us about this issue. Message: ") + e.what(), "Exception caught");
    }
    catch (SyntaxError& e)
    {
        g_logger.error("Unexpected exception: " + toString((int)e.errorcode) + ", message: " + _guilang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3])) + ", expression: " + e.getExpr() + ", token: " + e.getToken());
        wxMessageBox("An unexpected exception was caught. If it is reproducable, consider informing us about this issue. Code: " + toString((int)e.errorcode) + ", message: " + _guilang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]), toString(e.getIndices()[3])) + ", expression: " + e.getExpr() + ", token: " + e.getToken(), "Exception caught");
    }
    catch (mu::Parser::exception_type& e)
    {
        g_logger.error("Unexpected exception: " + e.GetMsg());
        wxMessageBox("An unexpected exception was caught. If it is reproducable, consider informing us about this issue. Message: " + e.GetMsg(), "Exception caught");
    }
    catch ( ... )
    {
        throw;
    }

    return true;
}

//----------------------------------------------------------------------



