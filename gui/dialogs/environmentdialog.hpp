/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#ifndef ENVIRONMENTDIALOG_HPP
#define ENVIRONMENTDIALOG_HPP


#include <wx/wx.h>
#include "../compositions/grouppanel.hpp"
#include "../../kernel/environment.hpp"

/////////////////////////////////////////////////
/// \brief Provides a dialog to and and modify
/// the declared environment variables.
/////////////////////////////////////////////////
class EnvironmentDialog : public wxDialog
{
    private:
        wxTreeListCtrl* m_envVars;
        TextField* m_varName;
        TextField* m_varValue;

        Environment m_environment;

        void populate();

        void OnItemSelect(wxTreeListEvent& event);
        void OnButtonClick(wxCommandEvent& event);

    public:
        EnvironmentDialog(wxWindow* parent, const wxString& envFilePath);

        DECLARE_EVENT_TABLE();
};


#endif // ENVIRONMENTDIALOG_HPP

