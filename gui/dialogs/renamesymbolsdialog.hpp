/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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
#include <wx/listctrl.h>
#include <vector>

#ifndef RENAMESYMBOLSDIALOG_HPP
#define RENAMESYMBOLSDIALOG_HPP

class RenameSymbolsDialog : public wxDialog
{
    private:
        wxCheckBox* m_replaceInComments;
        wxCheckBox* m_replaceInWholeFile;
        wxCheckBox* m_replaceBeforeCursor;
        wxCheckBox* m_replaceAfterCursor;
        wxTextCtrl* m_replaceName;

        void fillChangesLog(wxListView* listView, const std::vector<wxString>& vChangeLog);

    public:
        RenameSymbolsDialog(wxWindow* parent, const std::vector<wxString>& vChangeLog, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxString& defaultval = wxEmptyString);

        wxString GetValue()
        {
            return m_replaceName->GetValue();
        }
        bool replaceInWholeFile()
        {
            return m_replaceInWholeFile->GetValue();
        }
        bool replaceInComments()
        {
            return m_replaceInComments->GetValue();
        }
        bool replaceBeforeCursor()
        {
            return m_replaceBeforeCursor->GetValue();
        }
        bool replaceAfterCursor()
        {
            return m_replaceAfterCursor->GetValue();
        }
};

#endif // RENAMESYMBOLSDIALOG_HPP



