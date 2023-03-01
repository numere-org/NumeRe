/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2013  Erik Haenel et al.

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

#ifndef LISTEDITDIALOG_HPP
#define LISTEDITDIALOG_HPP

#include <wx/wx.h>
#include <wx/listctrl.h>

/////////////////////////////////////////////////
/// \brief Declares a dialog for editing the
/// elements of a list.
/////////////////////////////////////////////////
class ListEditDialog : public wxDialog
{
    private:
        wxListView* m_list;

        void OnLabelEdit(wxListEvent& event);

    public:
        ListEditDialog(wxWindow* parent, const wxArrayString& listEntries, const wxString& title, const wxString& desc = wxEmptyString);

        wxArrayString getEntries() const;

        DECLARE_EVENT_TABLE()
};


#endif // LISTEDITDIALOG_HPP

