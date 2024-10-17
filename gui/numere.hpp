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

#include <wx/wx.h>
#include <wx/snglinst.h>

class NumeReWindow;

namespace DDE
{
    class Server;
}

/////////////////////////////////////////////////
/// \brief This class represents the application
/// controller, which will create the main frame
/// of the actual application. It will also
/// include the single instance checking and the
/// necessary DDE command interface handling.
/////////////////////////////////////////////////
class NumeReApp : public wxApp
{
    public:
        ~NumeReApp();
        virtual bool OnInit() override;
        virtual int OnExit() override;
        virtual bool OnExceptionInMainLoop() override;

        NumeReWindow* getMainWindow()
        {
            return m_mainWindow;
        }

    private:
        wxSingleInstanceChecker* m_singlinst;
        DDE::Server* m_DDEServer;
        NumeReWindow* m_mainWindow;
        //virtual int OnRun();
        //virtual void OnUnhandledException();
};

