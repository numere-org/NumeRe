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
#include <wx/spinctrl.h>

#ifndef GROUPPANEL_HPP
#define GROUPPANEL_HPP

class GroupPanel : public wxScrolledWindow
{
    private:
        wxBoxSizer* verticalSizer;
        wxBoxSizer* horizontalSizer;

    public:
        GroupPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL);

        wxBoxSizer* getVerticalSizer();
        wxBoxSizer* getHorizontalSizer();
        void AddSpacer(int nSize = 10);

        wxStaticBoxSizer* createGroup(const wxString& sGroupName, int orient = wxVERTICAL);

        wxTextCtrl* CreatePathInput(wxWindow* parent, wxSizer* sizer, const wxString& description, int buttonID);
        wxTextCtrl* CreateTextInput(wxWindow* parent, wxSizer* sizer, const wxString& description, const wxString& sDefault = wxEmptyString);
        wxCheckBox* CreateCheckBox(wxWindow* parent, wxSizer* sizer, const wxString& description);
        wxSpinCtrl* CreateSpinControl(wxWindow* parent, wxSizer* sizer, const wxString& description, int nMin, int nMax, int nInitial);

};


#endif // GROUPPANEL_HPP

