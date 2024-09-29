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

#ifndef HISTORYSEARCHCTRL_HPP
#define HISTORYSEARCHCTRL_HPP

#include "../controls/searchctrl.hpp"
#include "history.hpp"


/////////////////////////////////////////////////
/// \brief This class specializes the generic
/// search control to interact with a
/// NumeReHistory instance.
/////////////////////////////////////////////////
class HistorySearchCtrl : public SearchCtrl
{
    private:
        NumeReHistory* m_history;

    protected:
        virtual bool selectItem(const wxString& value) override;
        virtual wxArrayString getCandidates(const wxString& enteredText) override;

    public:
        HistorySearchCtrl(wxWindow* parent, wxWindowID id, const wxString& hint = wxEmptyString, const wxString& calltip = wxEmptyString, NumeReHistory* history = nullptr) : SearchCtrl(parent, id, wxEmptyString), m_history(history)
        {
            // Provide a neat hint to the user, what he
            // may expect from this control
            SetHint(hint);
            popUp->SetCallTips(calltip);
            wxArrayInt sizes;
            sizes.Add(450, 1);
            popUp->SetColSizes(sizes);
            popUp->EnableDragDrop(true);
        }

        void OnSizeEvent(wxSizeEvent& event);
        DECLARE_EVENT_TABLE();
};



#endif // HISTORYSEARCHCTRL_HPP

