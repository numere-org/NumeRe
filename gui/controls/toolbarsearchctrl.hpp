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

class NumeReWindow;
class NumeReTerminal;


/////////////////////////////////////////////////
/// \brief This class specializes the generic
/// search control to be placed in the toolbar
/// and to use the internal NumeRe::DataBase as
/// data source.
/////////////////////////////////////////////////
class ToolBarSearchCtrl : public SearchCtrl
{
    private:
        NumeRe::DataBase searchDB;
        NumeReWindow* m_mainframe;
        NumeReTerminal* m_terminal;

    protected:
        // Interaction functions with the wxTreeCtrl
        virtual bool selectItem(const wxString& value) override;
        virtual wxString getDragDropText(const wxString& value) override;
        virtual wxArrayString getCandidates(const wxString& enteredText) override;

    public:
        ToolBarSearchCtrl(wxWindow* parent, wxWindowID id, const NumeRe::DataBase& db, NumeReWindow* _mainframe, NumeReTerminal* _term, const wxString& hint = wxEmptyString, const wxString& calltip = wxEmptyString, const wxString& calltiphighlight = wxEmptyString, int width = 300, int extension = 200) : SearchCtrl(parent, id, wxEmptyString, 0)
        {
            // Provide a neat hint to the user, what he
            // may expect from this control
            SetHint(hint);
            searchDB = db;
            m_mainframe = _mainframe;
            m_terminal = _term;

            // Define size and additional extent
            // (20 additional pixels for the scroll bar)
            SetSize(wxSize(width, -1));
            SetPopupExtents(0, extension+20);

            // Define the number of columns
            wxArrayInt sizes;
            sizes.Add(130, 1);
            sizes.Add(width + extension - 130, 1);

            popUp->SetColSizes(sizes);
            popUp->EnableDragDrop(true);
            popUp->EnableColors(true);
            popUp->SetCallTips(calltip, calltiphighlight);
        }
};



#endif // TOOLBARSEARCHCTRL_HPP



