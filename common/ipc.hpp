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
#ifndef IPC_HPP
#define IPC_HPP

#include <wx/wx.h>
#include <wx/ipc.h>

#define DDE_TOPIC "NumeReIPC"
#define DDE_SERVICE "NumeRe"

// Forward declaration
class NumeReWindow;

// Interprocess communication namespace
namespace DDE
{
    /////////////////////////////////////////////////
    /// \brief This class implements the DDE server
    /// for the first (main) NumeRe instance.
    /////////////////////////////////////////////////
    class Server : public wxServer
    {
        public:
            Server(NumeReWindow* frame) : m_Frame(frame) {}
            virtual wxConnectionBase* OnAcceptConnection(const wxString& topic) override;

        private:
            NumeReWindow* m_Frame;
    };


    /////////////////////////////////////////////////
    /// \brief This class is an implementation of a
    /// DDE connection between DDE::Client and
    /// DDE::Server.
    /////////////////////////////////////////////////
    class Connection : public wxConnection
    {
        public:
            Connection(NumeReWindow* frame) : m_Frame(frame) {}
            virtual bool OnExecute(const wxString& topic, const void* data, size_t size, wxIPCFormat format) override;
            virtual bool OnDisconnect() override;

        private:
            NumeReWindow* m_Frame;
            wxArrayString m_ArgV;
            wxString m_fileToLoad;
    };


    /////////////////////////////////////////////////
    /// \brief This is the implementation of the
    /// DDE client of any secondary NumeRe instances.
    /////////////////////////////////////////////////
    class Client: public wxClient
    {
        public:
            Client(void) {}
            virtual wxConnectionBase* OnMakeConnection(void) override
            {
                return new DDE::Connection(nullptr);
            }
    };
}


#endif // IPC_HPP



