/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef CUSTOMWINDOW_HPP
#define CUSTOMWINDOW_HPP

#include <wx/wx.h>
#include <map>
#include <vector>
#include <utility>
#include <wx/treelist.h>

#include "../../kernel/windowmanager.hpp"
#include "../../kernel/core/datamanagement/table.hpp"
#include "tableviewer.hpp"

// Forward declarations
class GroupPanel;
class GraphHelper;

namespace tinyxml2
{
    class XMLElement;
}


/////////////////////////////////////////////////
/// \brief A structure to simplify the reading of
/// all parameter values of each window item.
/////////////////////////////////////////////////
struct WindowItemParams
{
    wxString type, value, state, label, color;
    NumeRe::Table table;
};


/////////////////////////////////////////////////
/// \brief A structure to simplify reading and
/// updating window item values.
/////////////////////////////////////////////////
struct WindowItemValue
{
    wxString type;
    wxString stringValue;
    NumeRe::Table tableValue;
};


/////////////////////////////////////////////////
/// \brief This class represents a window, which
/// can be created by the user during run-time by
/// using a layout script and the \c window
/// command.
/////////////////////////////////////////////////
class CustomWindow : public wxFrame
{
    public:
        enum WindowItemType
        {
            CHECKBOX,   // OK
            TEXT,       // OK
            TEXTCTRL,   // OK
            TREELIST,
            SPINCTRL,   // OK
            BUTTON,     // OK
            RADIOGROUP, // OK
            DROPDOWN,   // OK
            GAUGE,      // OK
            IMAGE,      // OK
            TABLE,      // OK
            GRAPHER     // OK
        };

    private:
        std::map<int, std::pair<WindowItemType,wxWindow*>> m_windowItems;
        std::map<int, wxString> m_eventTable;
        NumeRe::Window m_windowRef;

        void layout();
        void layoutChild(const tinyxml2::XMLElement* currentChild, wxWindow* currParent, wxSizer* currSizer, GroupPanel* _groupPanel);
        void handleEvent(wxEvent& event, const wxString& sEventType);
        bool getWindowParameters(WindowItemParams& params) const;
        bool getItemParameters(int windowItemID, WindowItemParams& params) const;
        wxArrayString getChoices(wxString& choices);
        wxString removeQuotationMarks(wxString sString);
        void Refresh();

    public:
        CustomWindow(wxWindow* parent, const NumeRe::Window& windowRef);

        std::vector<int> getWindowItems(WindowItemType _type) const;
        bool closeWindow();
        WindowItemValue getItemValue(int windowItemID) const;
        wxString getItemLabel(int windowItemID) const;
        wxString getItemState(int windowItemID) const;
        wxString getItemColor(int windowItemID) const;
        bool setItemValue(WindowItemValue& _value, int windowItemID);
        bool setItemLabel(const wxString& _label, int windowItemID);
        bool setItemState(const wxString& _state, int windowItemID);
        bool setItemColor(const wxString& _color, int windowItemID);
        bool setItemGraph(GraphHelper* _helper, int windowItemID);

        void OnClick(wxCommandEvent& event);
        void OnChange(wxCommandEvent& event);
        void OnSpin(wxSpinEvent& event);
        void OnCellSelect(wxGridEvent& event);
        void OnClose(wxCloseEvent& event);
        void OnMouseLeftDown(wxMouseEvent& event);
        void OnTreeListEvent(wxTreeListEvent& event);
        void OnSizeEvent(wxSizeEvent& event);

        DECLARE_EVENT_TABLE();
};


#endif // CUSTOMWINDOW_HPP

