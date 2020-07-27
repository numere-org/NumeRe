/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#include "ipc.hpp"
#include "../gui/NumeReWindow.h"
#include <wx/regex.h>


namespace DDE
{
    /////////////////////////////////////////////////
    /// \brief Creates a new DDEConnection, if the
    /// topic equals DDE_TOPIC.
    ///
    /// \param topic const wxString&
    /// \return wxConnectionBase*
    ///
    /////////////////////////////////////////////////
    wxConnectionBase* Server::OnAcceptConnection(const wxString& topic)
    {
        return topic == DDE_TOPIC ? new Connection(m_Frame) : nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Handles the interprocess communication
    /// between two NumeRe instances.
    ///
    /// \param topic const wxString&
    /// \param data const void*
    /// \param size size_t
    /// \param format wxIPCFormat
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Connection::OnExecute(const wxString& topic, const void* data, size_t size, wxIPCFormat format)
    {
        const wxString strData = wxConnection::GetTextFromData(data, size, format);

        if (strData.StartsWith(_T("[IfExec_Open(\"")))
            return false; // let Shell Open handle the request as we *know* that we have registered the Shell Open command, too

        if (strData.StartsWith(_T("[Open(\"")))
        {
            wxRegEx reCmd(_T("\"(.*)\""));

            if (reCmd.Matches(strData))
                m_fileToLoad = reCmd.GetMatch(strData, 1);

            return true;
        }
        /*else if (strData.StartsWith(_T("[OpenLine(\"")))
        {
            wxRegEx reCmd(_T("\"(.*)\""));

            if (reCmd.Matches(strData))
            {
                wxString file = reCmd.GetMatch(strData, 1);
                CodeBlocksApp* cb = (CodeBlocksApp*)wxTheApp;
                cb->SetAutoFile(file);
            }

            return true;
        }*/
        else if (strData.StartsWith(_T("[CmdLine({")))
        {
            int pos = strData.Find(_T("})]"));

            if (pos != wxNOT_FOUND)
            {
                wxString line = strData.Mid(10, pos - 10);
                line.Replace(_T("\\)"), _T(")"));
                line.Replace(_T("\\("), _T("("));

                if (!m_ArgV.size())
                    m_ArgV.Add("numere.exe"); // Function from main frame expects this as first argument

                while (line.length())
                {
                    m_ArgV.Add(line.substr(0, line.find(' ')));
                    line.erase(0, line.find(' ')+1);
                }
            }

            return true;
        }

        wxSafeShowMessage(wxT("Warning"), wxString::Format(wxT("DDE topic %s not handled."), strData.wx_str()));
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Called, when the DDE::Server or the
    /// DDE::Client closes the connection. Will pass
    /// the gathered commands to the NumeRe main
    /// frame.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Connection::OnDisconnect()
    {
        // delayed files will be loaded automatically if MainFrame already exists,
        // otherwise it happens automatically in OnInit after MainFrame is created
        if (m_Frame && m_ArgV.size() > 1)
            m_Frame->EvaluateCommandLine(m_ArgV);

        if (m_Frame && m_fileToLoad.length())
            m_Frame->OpenFileByType(m_fileToLoad);

        return true;
    }


}
