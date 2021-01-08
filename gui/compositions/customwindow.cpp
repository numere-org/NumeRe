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

#include "customwindow.hpp"
#include "../NumeReWindow.h" // Already includes NumeRe::Window
#include "../../kernel/core/utils/tinyxml2.h"
#include "grouppanel.hpp"
#include <wx/tokenzr.h>
#include "../wx.h"

#include <string>

std::string toString(size_t);

static wxString toWxString(const wxColor& c)
{
    return wxString::Format("{%d,%d,%d}", c.Red(), c.Green(), c.Blue());
}


static wxColour toWxColour(const wxString& s)
{
    wxColour c;
    c.Set("rgb(" + s + ")");

    return c;
}


static wxString convertToCodeString(wxString s)
{
    s.Replace("\"", "\\\"");
    return "\"" + s + "\"";
}


BEGIN_EVENT_TABLE(CustomWindow, wxFrame)
    EVT_BUTTON(-1, CustomWindow::OnClick)
    EVT_CHECKBOX(-1, CustomWindow::OnChange)
    EVT_RADIOBOX(-1, CustomWindow::OnChange)
    EVT_CHOICE(-1, CustomWindow::OnChange)
    EVT_TEXT(-1, CustomWindow::OnChange)
    EVT_TEXT_ENTER(-1, CustomWindow::OnChange)
    EVT_SPINCTRL(-1, CustomWindow::OnSpin)
    EVT_CLOSE(CustomWindow::OnClose)
    EVT_GRID_SELECT_CELL(CustomWindow::OnCellSelect)
    EVT_LEFT_DOWN(CustomWindow::OnMouseLeftDown)
END_EVENT_TABLE()

enum WindowState
{
    ENABLED,
    DISABLED,
    HIDDEN
};


CustomWindow::CustomWindow(wxWindow* parent, const NumeRe::Window& windowRef) : wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxFRAME_FLOAT_ON_PARENT | wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxMAXIMIZE_BOX | wxMINIMIZE_BOX), m_windowRef(windowRef)
{
    m_windowRef.connect(this);

    layout();
}


void CustomWindow::layout()
{
    const tinyxml2::XMLElement* layoutGroup = m_windowRef.getLayout()->FirstChild()->ToElement();

    if (layoutGroup->Attribute("color"))
    {
        wxString sColor = layoutGroup->Attribute("color");
        wxColour background;

        background.Set("rgb(" + sColor + ")");

        SetBackgroundColour(background);
    }

    if (layoutGroup->Attribute("title"))
        SetTitle(layoutGroup->Attribute("title"));
    else
        SetTitle("Custom Window");

    if (layoutGroup->Attribute("icon"))
        SetIcon(wxIcon(layoutGroup->Attribute("icon"), wxICON_DEFAULT_TYPE));
    else
        SetIcon(wxIcon(static_cast<NumeReWindow*>(m_parent)->getProgramFolder()+"\\icons\\icon.ico", wxBITMAP_TYPE_ICO));

    GroupPanel* _groupPanel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);
    _groupPanel->Bind(wxEVT_LEFT_DOWN, &CustomWindow::OnMouseLeftDown, this);

    layoutChild(layoutGroup->FirstChildElement(), _groupPanel, _groupPanel->getVerticalSizer(), _groupPanel);

    _groupPanel->SetScrollbars(20, 20, 200, 200);

    CreateStatusBar();

    if (layoutGroup->Attribute("size"))
    {
        wxString sSize = layoutGroup->Attribute("size");
        long int x,y;
        sSize.substr(0, sSize.find(',')).ToLong(&x);
        sSize.substr(sSize.find(',')+1).ToLong(&y);

        SetClientSize(wxSize(x,y));
    }
    else
        SetClientSize(wxSize(800,600));
    //wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    //vsizer->Add(_groupPanel, 1, wxEXPAND, 0);
    //SetSizer(vsizer);
    //vsizer->SetSizeHints(_groupPanel);
}


void CustomWindow::layoutChild(const tinyxml2::XMLElement* currentChild, wxWindow* currParent, wxSizer* currSizer, GroupPanel* _groupPanel)
{
    while (currentChild)
    {
        int id = currentChild->Attribute("id") ?
                    currentChild->DoubleAttribute("id") : m_windowItems.size() + 1000;

        wxString text = currentChild->GetText() ? currentChild->GetText() : "";

        wxFont font = GetFont();
        WindowState state = ENABLED;

        if (currentChild->Attribute("state"))
            state = currentChild->Attribute("state", "disabled") ? DISABLED : (currentChild->Attribute("state", "hidden") ? HIDDEN : ENABLED);

        if (currentChild->Attribute("font"))
        {
            wxString sFont = currentChild->Attribute("font");

            if (sFont.find('i') != std::string::npos)
                font.MakeItalic();

            if (sFont.find('b') != std::string::npos)
                font.MakeBold();
        }


        // Add an element
        if (string(currentChild->Value()) == "button")
        {
            // Add a button
            wxButton* button = _groupPanel->CreateButton(currParent, currSizer, removeQuotationMarks(text), id);
            button->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::BUTTON, button);

            if (currentChild->Attribute("onclick"))
                m_eventTable[id] = currentChild->Attribute("onclick");

            if (currentChild->Attribute("color"))
                button->SetForegroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                button->Disable();
            else if (state == HIDDEN)
                button->Hide();
        }
        else if (string(currentChild->Value()) == "checkbox")
        {
            // Add a checkbox
            wxCheckBox* checkbox = _groupPanel->CreateCheckBox(currParent, currSizer, removeQuotationMarks(text), id);
            checkbox->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::CHECKBOX, checkbox);

            if (currentChild->Attribute("value"))
                checkbox->SetValue(currentChild->Attribute("value", "1"));

            if (currentChild->Attribute("onchange"))
                m_eventTable[id] = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                checkbox->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                checkbox->Disable();
            else if (state == HIDDEN)
                checkbox->Hide();
        }
        else if (string(currentChild->Value()) == "radio")
        {
            // Add a radio group
            wxString label;
            int style = wxHORIZONTAL;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "horizontal") ? wxHORIZONTAL : wxVERTICAL;

            wxRadioBox* radiobox = _groupPanel->CreateRadioBox(currParent, currSizer, label, getChoices(text), style, id);
            radiobox->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::RADIOGROUP, radiobox);

            if (currentChild->Attribute("value"))
                radiobox->SetSelection(currentChild->IntAttribute("value")-1);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id] = currentChild->Attribute("onchange");

            if (state == DISABLED)
                radiobox->Disable();
            else if (state == HIDDEN)
                radiobox->Hide();
        }
        else if (string(currentChild->Value()) == "spinbut")
        {
            // Add a spincontrol group
            wxString label;
            int nMin = 0, nMax = 100, nValue = 0;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("min"))
                nMin = currentChild->DoubleAttribute("min");

            if (currentChild->Attribute("max"))
                nMax = currentChild->DoubleAttribute("max");

            if (currentChild->Attribute("value"))
                nValue = currentChild->DoubleAttribute("value");

            SpinBut* spinctrl = _groupPanel->CreateSpinControl(currParent, currSizer, label, nMin, nMax, nValue, id);
            spinctrl->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::SPINCTRL, spinctrl);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id] = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                spinctrl->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                spinctrl->Enable(false);
            else if (state == HIDDEN)
                spinctrl->Show(false);
        }
        else if (string(currentChild->Value()) == "gauge")
        {
            // Add a gauge
            wxString label;
            int style = wxHORIZONTAL;

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "horizontal") ? wxGA_HORIZONTAL | wxGA_SMOOTH : wxGA_VERTICAL | wxGA_SMOOTH;

            wxGauge* gauge = _groupPanel->CreateGauge(currParent, currSizer, style, id);
            m_windowItems[id] = std::make_pair(CustomWindow::GAUGE, gauge);

            if (currentChild->Attribute("value"))
                gauge->SetValue(currentChild->DoubleAttribute("value"));

            if (state == DISABLED)
                gauge->Disable();
            else if (state == HIDDEN)
                gauge->Hide();
        }
        else if (string(currentChild->Value()) == "dropdown")
        {
            // Add a dropdown
            wxChoice* choice = _groupPanel->CreateChoices(currParent, currSizer, getChoices(text), id);
            m_windowItems[id] = std::make_pair(CustomWindow::DROPDOWN, choice);

            if (currentChild->Attribute("value"))
                choice->SetSelection(currentChild->IntAttribute("value")-1);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id] = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                choice->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                choice->Disable();
            else if (state == HIDDEN)
                choice->Hide();
        }
        else if (string(currentChild->Value()) == "textfield")
        {
            // Add a textctrl
            int style = wxTE_PROCESS_ENTER;

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "multiline") ? wxTE_MULTILINE | wxTE_BESTWRAP : wxTE_PROCESS_ENTER;

            wxString label;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            TextField* textctrl = _groupPanel->CreateTextInput(currParent, currSizer, label, removeQuotationMarks(text), style, id);
            textctrl->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::TEXTCTRL, textctrl);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id] = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                textctrl->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                textctrl->Enable(false);
            else if (state == HIDDEN)
                textctrl->Show(false);
        }
        else if (string(currentChild->Value()) == "text")
        {
            // Add a static test
            wxStaticText* statictext = _groupPanel->AddStaticText(currParent, currSizer, removeQuotationMarks(text), id);
            statictext->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::TEXT, statictext);

            if (currentChild->Attribute("color"))
                statictext->SetForegroundColour(toWxColour(currentChild->Attribute("color")));
        }
        else if (string(currentChild->Value()) == "image")
        {
            // Add an image
            wxStaticBitmap* bitmap = _groupPanel->CreateBitmap(currParent, currSizer, removeQuotationMarks(text), id);
            m_windowItems[id] = std::make_pair(CustomWindow::IMAGE, bitmap);
        }
        else if (string(currentChild->Value()) == "grapher")
        {
            // Add a grapher object
            wxMGL* mgl = new wxMGL(currParent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME, true);
            mgl->SetMinClientSize(wxSize(640,480));
            currSizer->Add(mgl, 1, wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5);
            m_windowItems[id] = std::make_pair(CustomWindow::GRAPHER, mgl);

            if (currentChild->Attribute("onclick"))
                m_eventTable[id] = currentChild->Attribute("onclick");
        }
        else if (string(currentChild->Value()) == "tablegrid")
        {
            // Add a table grid
            TableViewer* table = new TableViewer(currParent, id, nullptr, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_THEME);
            currSizer->Add(table, 1, wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5);
            m_windowItems[id] = std::make_pair(CustomWindow::TABLE, table);
            table->SetTableReadOnly(false);

            if (currentChild->Attribute("size"))
            {
                wxString sSize = currentChild->Attribute("size");
                long int x,y;
                sSize.substr(0, sSize.find(',')).ToLong(&x);
                sSize.substr(sSize.find(',')+1).ToLong(&y);
                NumeRe::Table data(x,y);
                table->SetData(data);
            }
            else
            {
                NumeRe::Table data(1,1);
                table->SetData(data);
            }

            if (currentChild->Attribute("onclick"))
                m_eventTable[id] = currentChild->Attribute("onclick");
        }
        else if (string(currentChild->Value()) == "group")
        {
            // Add a group
            wxString label;
            int style = wxHORIZONTAL;
            bool isCollapsible = false;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "horizontal") ? wxHORIZONTAL : wxVERTICAL;

            if (currentChild->Attribute("style"))
                isCollapsible = currentChild->Attribute("style", "collapse");

            if (label.length() && isCollapsible)
            {
                wxCollapsiblePane* pane = _groupPanel->createCollapsibleGroup(label, currParent, currSizer);
                wxBoxSizer* sizer = new wxBoxSizer(style);

                layoutChild(currentChild->FirstChildElement(), pane->GetPane(), sizer, _groupPanel);

                pane->GetPane()->SetSizer(sizer);
                sizer->SetSizeHints(pane->GetPane());
            }
            else if (label.length())
            {
                wxStaticBoxSizer* sizer = _groupPanel->createGroup(label, style, currParent, currSizer);
                layoutChild(currentChild->FirstChildElement(), sizer->GetStaticBox(), sizer, _groupPanel);
            }
            else
            {
                wxBoxSizer* sizer = _groupPanel->createGroup(style, currSizer);
                layoutChild(currentChild->FirstChildElement(), currParent, sizer, _groupPanel);
            }

        }

        if (currentChild->NextSibling())
            currentChild = currentChild->NextSibling()->ToElement();
        else
            break;
    }
}


void CustomWindow::handleEvent(wxEvent& event, const wxString& sEventType)
{
    if (sEventType == "onclose")
        return;

    auto iter = m_eventTable.find(event.GetId());

    if (iter != m_eventTable.end())
    {
        if (iter->second == "close")
            closeWindow();
        else if (iter->second[0] == '$')
        {
            NumeReWindow* mainWindow = static_cast<NumeReWindow*>(m_parent);
            WindowItemParams params;

            getItemParameters(event.GetId(), params);

            wxString kvl_event;

            if (event.GetEventType() == wxEVT_GRID_SELECT_CELL)
            {
                kvl_event = "{\"event\",\"onclick\",\"object\",\"" + params.type
                            + "\",\"value\"," + sEventType
                            + ",\"state\",\"" + params.state + "\"}";
            }
            else
            {
                kvl_event = "{\"event\",\"" + sEventType
                            + "\",\"object\",\"" + params.type
                            + "\",\"value\"," + params.value
                            + ",\"state\",\"" + params.state + "\"}";
            }

            // $PROCEDURE(winid, objectid, event{})
            mainWindow->pass_command(iter->second + "(" + toString(m_windowRef.getId()) + ","
                                                        + toString(event.GetId()) + ","
                                                        + kvl_event + ")", true);
        }
    }
}


bool CustomWindow::getWindowParameters(WindowItemParams& params) const
{
    params.type = "window";
    params.value = wxString::Format("{%d,%d}", GetClientSize().x, GetClientSize().y);
    params.state = "running";
    params.color = toWxString(GetBackgroundColour());
    params.label = "\"" + GetTitle() + "\"";

    return true;
}


bool CustomWindow::getItemParameters(int windowItemID, WindowItemParams& params) const
{
    if (windowItemID == -1)
        return getWindowParameters(params);

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxWindow*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::BUTTON:
            params.type = "button";
            params.value = convertToCodeString(static_cast<wxButton*>(object.second)->GetLabel());
            params.label = params.value;
            params.color = toWxString(static_cast<wxButton*>(object.second)->GetForegroundColour());

            break;
        case CustomWindow::CHECKBOX:
            params.type = "checkbox";
            params.value = static_cast<wxCheckBox*>(object.second)->IsChecked() ? "true" : "false";
            params.label = convertToCodeString(static_cast<wxCheckBox*>(object.second)->GetLabel());
            params.color = toWxString(static_cast<wxCheckBox*>(object.second)->GetBackgroundColour());

            break;
        case CustomWindow::TEXT:
            params.type = "text";
            params.value = convertToCodeString(static_cast<wxStaticText*>(object.second)->GetLabel());
            params.label = params.value;
            params.color = toWxString(static_cast<wxStaticText*>(object.second)->GetForegroundColour());

            break;
        case CustomWindow::TEXTCTRL:
            params.type = "textfield";
            params.value = convertToCodeString(static_cast<TextField*>(object.second)->GetValue());
            params.label = convertToCodeString(static_cast<TextField*>(object.second)->GetLabel());
            params.color = toWxString(static_cast<TextField*>(object.second)->GetBackgroundColour());

            break;
        case CustomWindow::RADIOGROUP:
        {
            params.type = "radio";
            wxRadioBox* box = static_cast<wxRadioBox*>(object.second);
            params.value = convertToCodeString(box->GetString(box->GetSelection()));
            params.label = convertToCodeString(box->GetLabel());
            params.color = toWxString(static_cast<wxRadioBox*>(object.second)->GetBackgroundColour());

            break;
        }
        case CustomWindow::DROPDOWN:
        {
            params.type = "dropdown";
            wxChoice* choices = static_cast<wxChoice*>(object.second);
            params.value = convertToCodeString(choices->GetString(choices->GetSelection()));
            params.label = params.value;
            params.color = toWxString(static_cast<wxChoice*>(object.second)->GetBackgroundColour());

            break;
        }
        case CustomWindow::GAUGE:
            params.type = "gauge";
            params.value = wxString::Format("%d", static_cast<wxGauge*>(object.second)->GetValue());
            params.label = params.value;
            params.color = toWxString(static_cast<wxGauge*>(object.second)->GetBackgroundColour());

            break;
        case CustomWindow::SPINCTRL:
            params.type = "spinbut";
            params.value = wxString::Format("%d", static_cast<SpinBut*>(object.second)->GetValue());
            params.label = convertToCodeString(static_cast<SpinBut*>(object.second)->GetLabel());
            params.color = toWxString(static_cast<SpinBut*>(object.second)->GetBackgroundColour());

            break;
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            params.table = table->GetDataCopy();
            params.value = convertToCodeString(table->getSelectedValues());
            params.type = "tablegrid";
        }
        case CustomWindow::GRAPHER:
        {
            wxMGL* grapher = static_cast<wxMGL*>(object.second);
            params.value = convertToCodeString(grapher->getClickedCoords());
            params.type = "grapher";
        }


    //LISTVIEW
    }

    params.state = !object.second->IsShown() ? "hidden" : object.second->IsEnabled() ? "enabled" : "disabled";

    return true;
}


wxArrayString CustomWindow::getChoices(wxString& choices)
{
    wxArrayString choicesArray;
    size_t nQuotes = 0;

    for (int i = 0; i < (int)choices.length(); i++)
    {
        if (choices[i] == '"' && (!i || choices[i-1] != '\\'))
            nQuotes++;

        if (!(nQuotes % 2) && choices[i] == ',')
        {
            choicesArray.Add(removeQuotationMarks(choices.substr(0, i)));
            choices.erase(0, i+1);
            i = -1;
        }
    }

    if (choices.length())
        choicesArray.Add(removeQuotationMarks(choices));

    return choicesArray;
}


wxString CustomWindow::removeQuotationMarks(wxString sString)
{
    sString.Trim(false);
    sString.Trim(true);

    if (sString.length() && sString[0] == '"' && sString[sString.length()-1] == '"')
        sString = sString.substr(1, sString.length()-2);

    sString.Replace("\\\"", "\"");

    return sString;
}


std::vector<int> CustomWindow::getWindowItems(CustomWindow::WindowItemType _type) const
{
    std::vector<int> vIDs;

    for (auto iter : m_windowItems)
    {
        if (iter.second.first == _type)
            vIDs.push_back(iter.first);
    }

    return vIDs;
}


bool CustomWindow::closeWindow()
{
    return Close();
}


WindowItemValue CustomWindow::getItemValue(int windowItemID) const
{
    WindowItemParams params;
    WindowItemValue value;

    if (getItemParameters(windowItemID, params))
    {
        value.stringValue = params.value;
        value.tableValue = params.table;
        value.type = params.type;
        return value;
    }

    return value;
}


wxString CustomWindow::getItemLabel(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
        return params.label;

    return "";
}


wxString CustomWindow::getItemState(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
        return params.state;

    return "";
}


wxString CustomWindow::getItemColor(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
        return params.color;

    return "";
}


bool CustomWindow::setItemValue(WindowItemValue& _value, int windowItemID)
{
    if (windowItemID == -1)
    {
        long int x,y;
        _value.stringValue.substr(0, _value.stringValue.find(',')).ToLong(&x);
        _value.stringValue.substr(_value.stringValue.find(',')+1).ToLong(&y);

        SetClientSize(wxSize(x,y));
        Refresh();

        return true;
    }

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxWindow*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::BUTTON:
            static_cast<wxButton*>(object.second)->SetLabel(removeQuotationMarks(_value.stringValue));
            break;
        case CustomWindow::CHECKBOX:
            static_cast<wxCheckBox*>(object.second)->SetValue(removeQuotationMarks(_value.stringValue) == "1");
            break;
        case CustomWindow::TEXT:
            static_cast<wxStaticText*>(object.second)->SetLabel(removeQuotationMarks(_value.stringValue));
            break;
        case CustomWindow::TEXTCTRL:
            static_cast<TextField*>(object.second)->ChangeValue(removeQuotationMarks(_value.stringValue));
            break;
        case CustomWindow::GAUGE:
        {
            long int nVal;
            removeQuotationMarks(_value.stringValue).ToLong(&nVal);

            if (nVal == -1)
                static_cast<wxGauge*>(object.second)->Pulse();
            else
                static_cast<wxGauge*>(object.second)->SetValue(nVal);

            break;
        }
        case CustomWindow::SPINCTRL:
        {
            long int nVal;
            removeQuotationMarks(_value.stringValue).ToLong(&nVal);
            static_cast<SpinBut*>(object.second)->SetValue(nVal);
            break;
        }
        case CustomWindow::RADIOGROUP:
        {
            wxRadioBox* box = static_cast<wxRadioBox*>(object.second);
            int sel = box->FindString(removeQuotationMarks(_value.stringValue), true);

            if (sel != wxNOT_FOUND)
                box->SetSelection(sel);

            break;
        }
        case CustomWindow::DROPDOWN:
        {
            wxChoice* choices = static_cast<wxChoice*>(object.second);
            int sel = choices->FindString(removeQuotationMarks(_value.stringValue), true);

            if (sel != wxNOT_FOUND)
                choices->SetSelection(sel);

            break;
        }
        case CustomWindow::IMAGE:
        {
            wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>(object.second);
            bitmap->SetBitmap(wxBitmap(removeQuotationMarks(_value.stringValue), wxBITMAP_TYPE_ANY));
            break;
        }
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            table->SetData(_value.tableValue);
            break;
        }

    //LISTVIEW,
    }

    return true;
}


bool CustomWindow::setItemLabel(const wxString& _label, int windowItemID)
{
    if (windowItemID == -1)
    {
        SetTitle(removeQuotationMarks(_label));
        return true;
    }

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxWindow*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::BUTTON:
            static_cast<wxButton*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::CHECKBOX:
            static_cast<wxCheckBox*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::TEXT:
            static_cast<wxStaticText*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::TEXTCTRL:
            static_cast<TextField*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::SPINCTRL:
            static_cast<SpinBut*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::RADIOGROUP:
            static_cast<wxRadioBox*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;

    //LISTVIEW,
    }

    return true;
}


bool CustomWindow::setItemState(const wxString& _state, int windowItemID)
{
    if (windowItemID == -1)
        return false;

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxWindow*> object = iter->second;

    if (_state == "hidden")
        object.second->Show(false);
    else if (_state == "disabled")
    {
        object.second->Show(true);
        object.second->Enable(false);
    }
    else if (_state == "enabled")
    {
        object.second->Show(true);
        object.second->Enable(true);
    }

    return true;
}


bool CustomWindow::setItemColor(const wxString& _color, int windowItemID)
{
    wxColour color = toWxColour(_color);

    if (windowItemID == -1)
    {
        SetBackgroundColour(color);
        Refresh();

        return true;
    }

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxWindow*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::BUTTON:
            static_cast<wxButton*>(object.second)->SetForegroundColour(color);
            break;
        case CustomWindow::CHECKBOX:
            static_cast<wxCheckBox*>(object.second)->SetBackgroundColour(color);
            break;
        case CustomWindow::TEXT:
            static_cast<wxStaticText*>(object.second)->SetForegroundColour(color);
            break;
        case CustomWindow::TEXTCTRL:
            static_cast<wxTextCtrl*>(object.second)->SetBackgroundColour(color);
            break;
        case CustomWindow::SPINCTRL:
            static_cast<wxSpinCtrl*>(object.second)->SetBackgroundColour(color);
            break;
        case CustomWindow::RADIOGROUP:
            static_cast<wxRadioBox*>(object.second)->SetBackgroundColour(color);
            break;
        case CustomWindow::DROPDOWN:
            static_cast<wxChoice*>(object.second)->SetBackgroundColour(color);
            break;

    //LISTVIEW,
    }

    Refresh();

    return true;
}


bool CustomWindow::setItemGraph(GraphHelper* _helper, int windowItemID)
{
    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end() || iter->second.first != CustomWindow::GRAPHER)
        return false;

    wxMGL* mgl = static_cast<wxMGL*>(iter->second.second);

    mgl->SetDraw(_helper);
    mgl->SetGraph(_helper->setGrapher());

    double dHeight = sqrt(640.0*480.0 / _helper->getAspect());

    if (_helper->getHires())
        dHeight = sqrt(1280.0*960.0 / _helper->getAspect());

    mgl->SetSize((int)lrint(_helper->getAspect()*dHeight), (int)lrint(dHeight));

    mgl->Refresh();

    return true;
}


void CustomWindow::OnClick(wxCommandEvent& event)
{
    handleEvent(event, "onclick");
}


void CustomWindow::OnChange(wxCommandEvent& event)
{
    auto iter = m_windowItems.find(event.GetId());

    if (iter != m_windowItems.end() && iter->second.first != CustomWindow::SPINCTRL)
        handleEvent(event, "onchange");
}


void CustomWindow::OnSpin(wxSpinEvent& event)
{
    handleEvent(event, "onchange");
}


void CustomWindow::OnCellSelect(wxGridEvent& event)
{
    TableViewer* table = static_cast<TableViewer*>(m_windowItems[event.GetId()].second);
    handleEvent(event, convertToCodeString(table->GetCellValue(event.GetRow(), event.GetCol())));
}


void CustomWindow::OnClose(wxCloseEvent& event)
{
    handleEvent(event, "onclose");
    m_windowRef.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    static_cast<NumeReWindow*>(m_parent)->unregisterWindow(this);
    Destroy();
}


void CustomWindow::OnMouseLeftDown(wxMouseEvent& event)
{
    event.GetPosition();
    handleEvent(event, "onclick");
}



