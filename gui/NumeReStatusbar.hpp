/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include <wx/statusbr.h>
#include <wx/gauge.h>

#ifndef NUMERESTATUSBAR_HPP
#define NUMERESTATUSBAR_HPP

// Specialization of a status bar for the main window
// will handle a wxGauge as busy indicator
class NumeReStatusbar : public wxStatusBar
{
    private:
        // The busy indicator
        wxGauge* m_busyIndicator;

    public:
        // Constructor
        NumeReStatusbar(wxWindow* parent, wxWindowID id = wxID_ANY, long style = wxSTB_DEFAULT_STYLE);

        // Definition of possible status fields
        enum StatusField
        {
            STATUS_BUSYINDICATOR,
            STATUS_PATH,
            STATUS_FILETYPE,
            STATUS_RWMODE,
            STATUS_CARETPOSITION,
            STATUS_DEBUGGER
        };

        // Switch the indicator from busy to ready
        void Ready();

        // Switch the indicator from ready to busy
        void Busy();

        // Update a status field with a new value
        void SetStatus(StatusField field, const wxString& sStatusText);

        // Return the contents of the status field
        wxString GetStatus(StatusField field);

};

#endif // NUMERESTATUSBAR_HPP
