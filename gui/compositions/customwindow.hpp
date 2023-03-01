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
/// \brief Simple structure to store event
/// positions (either one- or two-dimensional).
/////////////////////////////////////////////////
struct EventPosition
{
    int x;
    int y;

    EventPosition(int _x = -2, int _y = -2) : x(_x), y(_y) {}
    wxString serialize() const
    {
        if (x != -2 && y != -2)
            return wxString::Format("\"{%d,%d}\"", x+1, y+1);
        else if (x != -2)
            return wxString::Format("%d", x+1);

        return "";
    }
};


/////////////////////////////////////////////////
/// \brief This event is used to asynchronously
/// change the element values in the window.
/////////////////////////////////////////////////
class SetValueEvent : public wxEvent
{
public:
    SetValueEvent(wxEventType eventType, int winid, int winItem, const WindowItemValue& val)
        : wxEvent(winid, eventType), m_val(val), m_item(winItem) {}
    virtual wxEvent* Clone() const {return new SetValueEvent(*this); }

    WindowItemValue m_val;
    int m_item;
};


/////////////////////////////////////////////////
/// \brief This event is used to asynchronously
/// change the element labels in the window.
/////////////////////////////////////////////////
class SetLabelEvent : public wxEvent
{
public:
    SetLabelEvent(wxEventType eventType, int winid, int winItem, const wxString& label)
        : wxEvent(winid, eventType), m_label(label) , m_item(winItem) {}
    virtual wxEvent* Clone() const {return new SetLabelEvent(*this); }

    wxString m_label;
    int m_item;
};

wxDEFINE_EVENT(SET_WINDOW_VALUE, SetValueEvent);
wxDEFINE_EVENT(SET_WINDOW_LABEL, SetLabelEvent);

#define cEVT_SET_VALUE(id, func) \
    wx__DECLARE_EVT1(SET_WINDOW_VALUE, id, &func)
#define cEVT_SET_LABEL(id, func) \
    wx__DECLARE_EVT1(SET_WINDOW_LABEL, id, &func)


/////////////////////////////////////////////////
/// \brief This struct lists all defined event
/// handlers for a single UI element.
/////////////////////////////////////////////////
struct EventHandler
{
    wxString onchange;
    wxString onclick;
    wxString onactivate;

    EventHandler() {}
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
            TREELIST,   // OK
            SPINCTRL,   // OK
            BUTTON,     // OK
            RADIOGROUP, // OK
            DROPDOWN,   // OK
            COMBOBOX,
            GAUGE,      // OK
            IMAGE,      // OK
            TABLE,      // OK
            GRAPHER,    // OK
            SLIDER,
            MENUITEM
        };

    private:
        std::map<int, std::pair<WindowItemType,wxObject*>> m_windowItems;
        std::map<int, EventHandler> m_eventTable;
        std::map<wxString, wxString> m_varTable;

        NumeRe::Window m_windowRef;

        void layout();
        void layoutChild(const tinyxml2::XMLElement* currentChild, wxWindow* currParent, wxSizer* currSizer, GroupPanel* _groupPanel);
        void layoutMenu(const tinyxml2::XMLElement* currentChild, wxMenu* currMenu);
        void handleEvent(wxEvent& event, const wxString& sEventType, const EventPosition& pos = EventPosition());
        bool getWindowParameters(WindowItemParams& params) const;
        bool getItemParameters(int windowItemID, WindowItemParams& params) const;
        wxArrayString getChoices(wxString& choices) const;
        wxArrayString decodeEventHandlerFunction(const wxString& sEventHandler) const;
        wxString removeQuotationMarks(wxString sString) const;
        void Refresh();

    public:
        CustomWindow(wxWindow* parent, const NumeRe::Window& windowRef, int addStyle);

        std::vector<int> getWindowItems(WindowItemType _type) const;
        bool closeWindow();
        WindowItemValue getItemValue(int windowItemID) const;
        wxString getItemLabel(int windowItemID) const;
        wxString getItemState(int windowItemID) const;
        wxString getItemColor(int windowItemID) const;
        wxString getPropValue(const wxString& varName) const;
        wxString getProperties() const;

        bool pushItemValue(WindowItemValue& _value, int windowItemID);
        bool pushItemLabel(const wxString& _label, int windowItemID);

        bool setItemValue(WindowItemValue& _value, int windowItemID);
        bool setItemLabel(const wxString& _label, int windowItemID);
        bool setItemState(const wxString& _state, int windowItemID);
        bool setItemColor(const wxString& _color, int windowItemID);
        bool setItemGraph(GraphHelper* _helper, int windowItemID);
        bool setPropValue(const wxString& _value, const wxString& varName);

        void OnMenuEvent(wxCommandEvent& event);
        void OnClick(wxCommandEvent& event);
        void OnChange(wxCommandEvent& event);
        void OnSpin(wxSpinEvent& event);
        void OnCellSelect(wxGridEvent& event);
        void OnCellActivate(wxGridEvent& event);
        void OnClose(wxCloseEvent& event);
        void OnMouseLeftDown(wxMouseEvent& event);
        void OnTreeListEvent(wxTreeListEvent& event);
        void OnTreeListActivateEvent(wxTreeListEvent& event);
        void OnSizeEvent(wxSizeEvent& event);

        void OnSetValueEvent(SetValueEvent& event)
        {
            setItemValue(event.m_val, event.m_item);
        }

        void OnSetLabelEvent(SetLabelEvent& event)
        {
            setItemLabel(event.m_label, event.m_item);
        }

        DECLARE_EVENT_TABLE();
};


#endif // CUSTOMWINDOW_HPP

