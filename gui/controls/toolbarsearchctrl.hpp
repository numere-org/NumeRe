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

#ifndef TOOLBARSEARCHCTRL_HPP
#define TOOLBARSEARCHCTRL_HPP

#include "searchctrl.hpp"
#include "../../kernel/core/datamanagement/database.hpp"

class ToolBarSearchCtrl : public SearchCtrl
{
    private:
        NumeRe::DataBase searchDB;

    protected:
        // Interaction functions with the wxTreeCtrl
        virtual void selectItem(const wxString& value) override;
        virtual wxArrayString getCandidates(const wxString& enteredText) override;

    public:
        ToolBarSearchCtrl(wxWindow* parent, wxWindowID id, const NumeRe::DataBase& db, const wxString& hint = wxEmptyString) : SearchCtrl(parent, id, wxEmptyString, 0)
        {
            // Provide a neat hint to the user, what he
            // may expect from this control
            SetHint(hint);
            searchDB = db;
            SetSize(wxSize(300, -1));
        }
};



#endif // TOOLBARSEARCHCTRL_HPP



