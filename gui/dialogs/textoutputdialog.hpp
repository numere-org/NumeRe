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

#ifndef TEXTOUTPUTDIALOG_HPP
#define TEXTOUTPUTDIALOG_HPP

#include <wx/wx.h>

class TextOutputDialog : public wxDialog
{
    private:
        wxTextCtrl* m_textctrl;

    public:
        TextOutputDialog(wxWindow* parent, wxWindowID id, const wxString& title, long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

        void push_text(const wxString& text);
};


#endif // TEXTOUTPUTDIALOG_HPP

