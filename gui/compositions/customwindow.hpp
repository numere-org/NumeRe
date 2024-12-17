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
#include <wx/notebook.h>
#include <map>
#include <vector>
#include <utility>
#include <wx/treelist.h>
#include <wx/dateevt.h>

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
    std::string type, state;
    mu::Array value, label, color;
    NumeRe::Table table;
};


/////////////////////////////////////////////////
/// \brief A structure to simplify reading and
/// updating window item values.
/////////////////////////////////////////////////
struct WindowItemValue
{
    std::string type;
    mu::Array val;
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
            return wxString::Format("{%d,%d}", x+1, y+1);
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
    SetLabelEvent(wxEventType eventType, int winid, int winItem, const mu::Array& label)
        : wxEvent(winid, eventType), m_label(label), m_item(winItem) {}
    virtual wxEvent* Clone() const {return new SetLabelEvent(*this); }

    mu::Array m_label;
    int m_item;
};


/////////////////////////////////////////////////
/// \brief This event is used to asynchronously
/// change the element options in the window.
/////////////////////////////////////////////////
class SetOptionsEvent : public wxEvent
{
public:
    SetOptionsEvent(wxEventType eventType, int winid, int winItem, const mu::Array& opts)
        : wxEvent(winid, eventType), m_options(opts), m_item(winItem) {}
    virtual wxEvent* Clone() const {return new SetOptionsEvent(*this); }

    mu::Array m_options;
    int m_item;
};


/////////////////////////////////////////////////
/// \brief This event is used to asynchronously
/// change the element selection in the window.
/////////////////////////////////////////////////
class SetSelectionEvent : public wxEvent
{
public:
    SetSelectionEvent(wxEventType eventType, int winid, int winItem, int selId1, int selId2)
        : wxEvent(winid, eventType), m_selectionId1(selId1), m_selectionId2(selId2), m_item(winItem) {}
    virtual wxEvent* Clone() const {return new SetSelectionEvent(*this); }

    int m_selectionId1;
    int m_selectionId2;
    int m_item;
};


/////////////////////////////////////////////////
/// \brief This event is used to asynchronously
/// change the elements' focus in the window.
/////////////////////////////////////////////////
class SetFocusEvent : public wxEvent
{
public:
    SetFocusEvent(wxEventType eventType, int winid, int winItem)
        : wxEvent(winid, eventType), m_item(winItem) {}
    virtual wxEvent* Clone() const {return new SetFocusEvent(*this); }

    int m_item;
};

wxDEFINE_EVENT(SET_WINDOW_VALUE, SetValueEvent);
wxDEFINE_EVENT(SET_WINDOW_LABEL, SetLabelEvent);
wxDEFINE_EVENT(SET_WINDOW_OPTIONS, SetOptionsEvent);
wxDEFINE_EVENT(SET_WINDOW_SELECTION, SetSelectionEvent);
wxDEFINE_EVENT(SET_WINDOW_FOCUS, SetFocusEvent);

#define cEVT_SET_VALUE(id, func) \
    wx__DECLARE_EVT1(SET_WINDOW_VALUE, id, &func)
#define cEVT_SET_LABEL(id, func) \
    wx__DECLARE_EVT1(SET_WINDOW_LABEL, id, &func)
#define cEVT_SET_OPTIONS(id, func) \
    wx__DECLARE_EVT1(SET_WINDOW_OPTIONS, id, &func)
#define cEVT_SET_SELECTION(id, func) \
    wx__DECLARE_EVT1(SET_WINDOW_SELECTION, id, &func)
#define cEVT_SET_FOCUS(id, func) \
    wx__DECLARE_EVT1(SET_WINDOW_FOCUS, id, &func)


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
            CHECKBOX,
            TEXT,
            TEXTCTRL,
            TREELIST,
            SPINCTRL,
            BUTTON,
            RADIOGROUP,
            DROPDOWN,
            COMBOBOX,
            GAUGE,
            IMAGE,
            TABLE,
            GRAPHER,
            SLIDER,
            DATETIMEPICKER,
            LAMP,
            NOTEBOOK,
            MENUITEM
        };

    private:
        std::map<int, std::pair<WindowItemType,wxObject*>> m_windowItems;
        std::map<int, EventHandler> m_eventTable;
        std::map<std::string, mu::Array> m_varTable;

        NumeRe::Window m_windowRef;
        wxWindowDisabler* m_dialogLock;
        wxString m_dialogResult;

        void layout();
        void layoutChild(const tinyxml2::XMLElement* currentChild, wxWindow* currParent, wxSizer* currSizer, GroupPanel* _groupPanel);
        void layoutMenu(const tinyxml2::XMLElement* currentChild, wxMenu* currMenu);
        void handleEvent(wxEvent& event, const wxString& sEventType, const EventPosition& pos = EventPosition());
        bool getWindowParameters(WindowItemParams& params) const;
        bool getItemParameters(int windowItemID, WindowItemParams& params) const;
        wxArrayString decodeEventHandlerFunction(const wxString& sEventHandler) const;
        void Refresh();

    public:
        CustomWindow(wxWindow* parent, const NumeRe::Window& windowRef, int addStyle);

        std::vector<int> getWindowItems(WindowItemType _type) const;
        bool closeWindow();
        WindowItemValue getItemValue(int windowItemID) const;
        mu::Array getItemLabel(int windowItemID) const;
        wxString getItemState(int windowItemID) const;
        mu::Array getItemColor(int windowItemID) const;
        mu::Array getItemSelection(int windowItemID) const;
        mu::Array getPropValue(const std::string& varName) const;
        mu::Array getProperties() const;
        mu::Array getStatusText() const;

        bool pushItemValue(WindowItemValue& _value, int windowItemID);
        bool pushItemLabel(const mu::Array& _label, int windowItemID);
        bool pushItemOptions(const mu::Array& _opts, int windowItemID);
        bool pushItemSelection(int selectionID, int selectionID2, int windowItemID);
        bool pushItemFocus(int windowItemID);

        bool setItemValue(WindowItemValue& _value, int windowItemID);
        bool setItemLabel(const mu::Array& _label, int windowItemID);
        bool setItemState(const wxString& _state, int windowItemID);
        bool setItemColor(const mu::Array& _color, int windowItemID);
        bool setItemOptions(const mu::Array& _options, int windowItemID);
        bool setItemSelection(int selectionID, int selectionID2, int windowItemID);
        bool setItemFocus(int windowItemID);
        bool setItemGraph(GraphHelper* _helper, int windowItemID);
        bool setPropValue(const mu::Array& _value, const std::string& varName);
        bool setStatusText(const wxArrayString& _values);
        bool setStatusText(const mu::Array& _value);

        void asDialog()
        {
            if (!m_dialogLock)
                m_dialogLock = new wxWindowDisabler(this);
        }

        bool isDialog() const
        {
            return m_dialogLock != nullptr;
        }

        void endDialog(const wxString& dialogResult)
        {
            if (m_dialogLock)
            {
                m_dialogResult = dialogResult;
                delete m_dialogLock;
                m_dialogLock = nullptr;
            }
        }

        wxString getDialogResult() const
        {
            return m_dialogResult;
        }

        void OnMenuEvent(wxCommandEvent& event);
        void OnClick(wxCommandEvent& event);
        void OnChange(wxCommandEvent& event);
        void OnSpin(wxSpinEvent& event);
        void OnTabChanged(wxBookCtrlEvent& event);
        void OnCellSelect(wxGridEvent& event);
        void OnCellActivate(wxGridEvent& event);
        void OnClose(wxCloseEvent& event);
        void OnMouseLeftDown(wxMouseEvent& event);
        void OnTreeListEvent(wxTreeListEvent& event);
        void OnTreeListActivateEvent(wxTreeListEvent& event);
        void OnDateEvent(wxDateEvent& event);
        void OnSizeEvent(wxSizeEvent& event);

        void OnSetValueEvent(SetValueEvent& event)
        {
            setItemValue(event.m_val, event.m_item);
        }

        void OnSetLabelEvent(SetLabelEvent& event)
        {
            setItemLabel(event.m_label, event.m_item);
        }

        void OnSetOptionsEvent(SetOptionsEvent& event)
        {
            setItemOptions(event.m_options, event.m_item);
        }

        void OnSetSelectionEvent(SetSelectionEvent& event)
        {
            setItemSelection(event.m_selectionId1, event.m_selectionId2, event.m_item);
        }

        void OnSetFocusEvent(SetFocusEvent& event)
        {
            setItemFocus(event.m_item);
        }

        DECLARE_EVENT_TABLE();
};


#endif // CUSTOMWINDOW_HPP

