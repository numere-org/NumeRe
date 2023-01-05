/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#ifndef TERMINALCALLTIP_HPP
#define TERMINALCALLTIP_HPP

#include <wx/window.h>
#include <wx/textctrl.h>

/////////////////////////////////////////////////
/// \brief This class represents the calltip in
/// the terminal. The associated window will only
/// be shown and hidden. No new window is created
/// for eachs calltip.
/////////////////////////////////////////////////
class TerminalCallTip : public wxWindow
{
    private:
        wxTextCtrl* m_text;

    public:
        TerminalCallTip(wxWindow* parent, const wxSize& s = wxDefaultSize);

        void PopUp(const wxPoint& pos, const wxString& text);
        void Dismiss();
        void Resize(const wxSize& s);
        void ChangeFont(const wxFont& font);

        void Highlight(size_t start, size_t len);

        virtual bool AcceptsFocus() const override
        {
            return false;
        }

        virtual bool AcceptsFocusFromKeyboard() const override
        {
            return false;
        }

        virtual bool AcceptsFocusRecursively() const override
        {
            return false;
        }
};

#endif // TERMINALCALLTIP_HPP

