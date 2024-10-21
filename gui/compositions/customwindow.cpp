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
#include "../../externals/tinyxml2/tinyxml2.h"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../../kernel/core/io/logger.hpp"
#include "grouppanel.hpp"
#include <wx/tokenzr.h>
#include <wx/dataview.h>
#include <wx/statline.h>
#include "../wx.h"

#include <string>

#define MINCOLSIZE 70

/////////////////////////////////////////////////
/// \brief This static function converts colors
/// to wxStrings.
///
/// \param c const wxColor&
/// \return wxString
///
/////////////////////////////////////////////////
static wxString toWxString(const wxColor& c)
{
    return wxString::Format("{%d,%d,%d}", c.Red(), c.Green(), c.Blue());
}


/////////////////////////////////////////////////
/// \brief This static function converts
/// wxStrings to colors.
///
/// \param s const wxString&
/// \return wxColour
///
/////////////////////////////////////////////////
static wxColour toWxColour(const wxString& s)
{
    wxColour c;
    c.Set("rgb(" + s + ")");

    return c;
}


/////////////////////////////////////////////////
/// \brief Convert states to lamp colours.
///
/// \param s const wxString&
/// \return wxColour
///
/////////////////////////////////////////////////
static wxColour colorFromLampStates(const wxString& s)
{
    if (s == "ok")
        return wxColour(0,255,0);

    if (s == "error")
        return wxColour(255,0,0);

    if (s == "warn")
        return wxColour(255,255,0);

    // if (s == "off")
    return wxColour(64,64,64);
}


/////////////////////////////////////////////////
/// \brief Convert lamp colours to states.
///
/// \param c const wxColour&
/// \return wxString
///
/////////////////////////////////////////////////
static wxString lampStatesFromColor(const wxColour& c)
{
    if (c == wxColour(0,255,0))
        return "ok";

    if (c == wxColour(255,0,0))
        return "error";

    if (c == wxColour(255,255,0))
        return "warn";

    if (c == wxColour(64,64,64))
        return "off";

    return "";
}


/////////////////////////////////////////////////
/// \brief This static function conversts usual
/// strings into "Kernel strings" (i.e. usual
/// NumeRe code strings).
///
/// \param s wxString
/// \return wxString
///
/////////////////////////////////////////////////
static wxString convertToCodeString(wxString s)
{
    s.Replace("\"", "\\\"");
    return "\"" + s + "\"";
}


/////////////////////////////////////////////////
/// \brief Static function to convert a
/// kernel string into a usual string.
///
/// \param sString wxString
/// \return wxString
///
/////////////////////////////////////////////////
wxString removeQuotationMarks(wxString sString)
{
    sString.Trim(false);
    sString.Trim(true);

    if (sString.length() && sString[0] == '"' && sString[sString.length()-1] == '"')
        sString = sString.substr(1, sString.length()-2);

    sString.Replace("\\\"", "\"");

    return sString;
}


/////////////////////////////////////////////////
/// \brief Separates the different item values.
///
/// \param sItem wxString&
/// \return wxString
///
/////////////////////////////////////////////////
static wxString nextItemValue(wxString& sItem)
{
    wxString sValue = sItem.substr(0, sItem.find_first_of("\t\n"));

    if (sItem.length() == sValue.length())
        sItem.clear();
    else if (sItem[sValue.length()] == '\t')
        sItem.erase(0, sValue.length()+1);
    else
        sItem.erase(0, sValue.length());

    return sValue;
}


/////////////////////////////////////////////////
/// \brief A simple tokenizer to separate a list
/// of strings into multiple strings.
///
/// \param choices wxString&
/// \param keepQuotationMarks bool
/// \return wxArrayString
///
/////////////////////////////////////////////////
static wxArrayString getChoices(wxString& choices, bool keepQuotationMarks = false)
{
    wxArrayString choicesArray;
    size_t nQuotes = 0;

    for (int i = 0; i < (int)choices.length(); i++)
    {
        if (choices[i] == '"' && (!i || choices[i-1] != '\\'))
            nQuotes++;

        if (!(nQuotes % 2) && choices[i] == ',')
        {
            if (keepQuotationMarks)
                choicesArray.Add(choices.substr(0, i));
            else
                choicesArray.Add(removeQuotationMarks(choices.substr(0, i)));
            choices.erase(0, i+1);
            i = -1;
        }
    }

    if (choices.length())
    {
        if (keepQuotationMarks)
            choicesArray.Add(choices);
        else
            choicesArray.Add(removeQuotationMarks(choices));
    }

    return choicesArray;
}


/////////////////////////////////////////////////
/// \brief Populate the children of the current
/// wxTreeListItem.
///
/// \param listCtrl wxTreeListCtrl*
/// \param values const wxArrayString&
/// \param parentItem wxTreeListItem
/// \param hasChild bool
/// \return void
///
/////////////////////////////////////////////////
static void populateChild(wxTreeListCtrl* listCtrl, const wxArrayString& values, wxTreeListItem parentItem, bool hasChild)
{
    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    size_t nColumns = listCtrl->GetColumnCount();

    for (size_t i = 0; i < values.size(); i++)
    {
        wxString sItem = values[i];
        size_t currCol = 1u;
        bool check = false;

        if (useCheckBoxes)
            check = nextItemValue(sItem) != "0";

        wxTreeListItem item = listCtrl->AppendItem(parentItem, nextItemValue(sItem));

        if (check && useCheckBoxes)
            listCtrl->CheckItem(item);

        while (sItem.length() && currCol < nColumns && !sItem.StartsWith("\n{"))
        {
            listCtrl->SetItemText(item, currCol, nextItemValue(sItem));
            currCol++;
        }

        if (hasChild && sItem.StartsWith("\n{") && sItem.EndsWith("}"))
        {
            sItem.erase(0, 2);
            sItem.RemoveLast();
            wxArrayString childValues = getChoices(sItem);
            populateChild(listCtrl, childValues, item, hasChild);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This static function converts the list
/// of values into items for the passed tree list
/// control and populates it.
///
/// \param listCtrl wxTreeListCtrl*
/// \param values const wxArrayString&
/// \return void
///
/////////////////////////////////////////////////
static void populateTreeListCtrl(wxTreeListCtrl* listCtrl, const wxArrayString& values)
{
    if (!values.size())
    {
        listCtrl->DeleteAllItems();
        return;
    }

    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    size_t nColumns = 1;
    bool hasChild = false;

    for (size_t pos = 0; pos < values[0].length(); pos++)
    {
        if (values[0][pos] == '\t')
            nColumns++;

        if (values[0].substr(pos, 2) == "\n{")
        {
            hasChild = true;
            break;
        }
    }

    if (useCheckBoxes)
        nColumns--;

    listCtrl->DeleteAllItems();

    while (listCtrl->GetColumnCount() < nColumns)
        listCtrl->AppendColumn("");

    populateChild(listCtrl, values, listCtrl->GetRootItem(), hasChild);

    nColumns = listCtrl->GetColumnCount();
    int colSize = std::max(listCtrl->GetClientSize().x / (int)nColumns - 2, MINCOLSIZE);

    for (size_t i = 0; i < listCtrl->GetColumnCount(); i++)
    {
        listCtrl->SetColumnWidth(i, colSize);
    }
}


/////////////////////////////////////////////////
/// \brief This static function returns the
/// current value of the passed tree list
/// control, whereas the value is either the
/// current selection or a vector of the current
/// checkbox states.
///
/// \param listCtrl wxTreeListCtrl*
/// \return wxString
///
/////////////////////////////////////////////////
static wxString getTreeListCtrlValue(wxTreeListCtrl* listCtrl)
{
    wxString values;
    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    wxTreeListItems items;

    // Get selections if any and no checkboxes are used
    if (!useCheckBoxes)
    {
        size_t itemCount = 0;
        /*if (listCtrl->GetSelections(items))
        {
            for (size_t i = 0; i < items.size(); i++)*/
            for (wxTreeListItem item = listCtrl->GetFirstItem(); item.IsOk(); item = listCtrl->GetNextItem(item))
            {
                if (values.length())
                    values += ", ";

                wxString sItem;

                for (size_t j = 0; j < listCtrl->GetColumnCount(); j++)
                {
                    sItem += listCtrl->GetItemText(item, j);

                    if (j+1 < listCtrl->GetColumnCount())
                        sItem += "\t";
                }

                values += convertToCodeString(sItem);
                itemCount++;
            }

            /*if (listCtrl->GetColumnCount() > 1 && itemCount > 1)
                return convertToCodeString(values);*/

            if (!values.length())
                return "\"\"";
        /*}
        else
            values = "\"\"";*/

        return values;
    }

    // Get the complete list or the states of the checkboxes
    for (wxTreeListItem item = listCtrl->GetFirstItem(); item.IsOk(); item = listCtrl->GetNextItem(item))
    {
        if (values.length())
            values += ",";

        values += listCtrl->GetCheckedState(item) == wxCHK_CHECKED ? "1" : "0";
    }

    return "\"{" + values + "}\"";
}


/////////////////////////////////////////////////
/// \brief Finds the depth-first numerical ID of
/// an element in the wxTreeListCtrl.
///
/// \param listCtrl wxTreeListCtrl*
/// \param item const wxTreeListItem&
/// \return int
///
/////////////////////////////////////////////////
static int enumerateListItems(wxTreeListCtrl* listCtrl, const wxTreeListItem& item)
{
    int id = 0;

    for (wxTreeListItem currItem = listCtrl->GetFirstItem(); currItem.IsOk(); currItem = listCtrl->GetNextItem(currItem), id++)
    {
        if (currItem == item)
            return id;
    }

    return wxNOT_FOUND;
}


/////////////////////////////////////////////////
/// \brief Enumeration to define possible states
/// of window items.
/////////////////////////////////////////////////
enum WindowState
{
    READONLY,
    ENABLED,
    DISABLED,
    HIDDEN
};


BEGIN_EVENT_TABLE(CustomWindow, wxFrame)
    EVT_BUTTON(-1, CustomWindow::OnClick)
    EVT_CHECKBOX(-1, CustomWindow::OnChange)
    EVT_RADIOBOX(-1, CustomWindow::OnChange)
    EVT_CHOICE(-1, CustomWindow::OnChange)
    EVT_COMBOBOX(-1, CustomWindow::OnChange)
    EVT_TEXT(-1, CustomWindow::OnChange)
    EVT_TEXT_ENTER(-1, CustomWindow::OnChange)
    EVT_SPINCTRL(-1, CustomWindow::OnSpin)
    EVT_CLOSE(CustomWindow::OnClose)
    EVT_GRID_SELECT_CELL(CustomWindow::OnCellSelect)
    EVT_GRID_CELL_LEFT_DCLICK(CustomWindow::OnCellActivate)
    EVT_LEFT_DOWN(CustomWindow::OnMouseLeftDown)
    EVT_TREELIST_ITEM_CHECKED(-1, CustomWindow::OnTreeListEvent)
    EVT_TREELIST_SELECTION_CHANGED(-1, CustomWindow::OnTreeListEvent)
    EVT_TREELIST_ITEM_ACTIVATED(-1, CustomWindow::OnTreeListActivateEvent)
    EVT_SIZE(CustomWindow::OnSizeEvent)
    EVT_SLIDER(-1, CustomWindow::OnChange)
    EVT_DATE_CHANGED(-1, CustomWindow::OnDateEvent)
    EVT_TIME_CHANGED(-1, CustomWindow::OnDateEvent)
    EVT_MENU(-1, CustomWindow::OnMenuEvent)
    EVT_BOOKCTRL_PAGE_CHANGED(-1, CustomWindow::OnTabChanged)
    cEVT_SET_VALUE(-1, CustomWindow::OnSetValueEvent)
    cEVT_SET_LABEL(-1, CustomWindow::OnSetLabelEvent)
    cEVT_SET_SELECTION(-1, CustomWindow::OnSetSelectionEvent)
    cEVT_SET_FOCUS(-1, CustomWindow::OnSetFocusEvent)
END_EVENT_TABLE()





/////////////////////////////////////////////////
/// \brief CustomWindow constructor. Connects
/// this window with the NumeRe::Window instance
/// and triggers the layouting algorithm.
///
/// \param parent wxWindow*
/// \param windowRef const NumeRe::Window&
/// \param addStyle int
///
/////////////////////////////////////////////////
CustomWindow::CustomWindow(wxWindow* parent, const NumeRe::Window& windowRef, int addStyle) : wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, addStyle | wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxMAXIMIZE_BOX | wxMINIMIZE_BOX), m_windowRef(windowRef), m_dialogLock(nullptr)
{
    m_windowRef.connect(this);

    layout();
}


/////////////////////////////////////////////////
/// \brief This member function evaluates the
/// layout supplied by the user via a
/// tinyxml2::XMLDocument instance.
///
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::layout()
{
    // Get the layout group from the layout script
    const tinyxml2::XMLElement* layoutGroup = m_windowRef.getLayout()->FirstChild()->ToElement();

    // Evaluate possible background color information
    if (layoutGroup->Attribute("color"))
    {
        wxString sColor = layoutGroup->Attribute("color");
        wxColour background;

        background.Set("rgb(" + sColor + ")");

        SetBackgroundColour(background);
    }

    // Evaluate the title information
    if (layoutGroup->Attribute("title"))
        SetTitle(layoutGroup->Attribute("title"));
    else
        SetTitle("NumeRe: Custom Window");

    // Evaluate the user-supplied icon or use
    // the standard icon
    if (layoutGroup->Attribute("icon"))
    {
        wxFileName iconfile(layoutGroup->Attribute("icon"));

        if (iconfile.GetExt() == "ico")
            SetIcon(wxIcon(layoutGroup->Attribute("icon"), wxBITMAP_TYPE_ICO));
        else if (iconfile.GetExt() == "png")
            SetIcon(wxIcon(layoutGroup->Attribute("icon"), wxBITMAP_TYPE_PNG));
        else if (iconfile.GetExt() == "bmp")
            SetIcon(wxIcon(layoutGroup->Attribute("icon"), wxBITMAP_TYPE_BMP));
    }
    else
        SetIcon(static_cast<NumeReWindow*>(m_parent)->getStandardIcon());

    // Create the GroupPanel instance and bind the mouse event handler
    // the frame's event handler
    GroupPanel* _groupPanel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);
    _groupPanel->Bind(wxEVT_LEFT_DOWN, &CustomWindow::OnMouseLeftDown, this);

    // Recursively layout the child elements of the current XMLElement
    // object
    layoutChild(layoutGroup->FirstChildElement(), _groupPanel, _groupPanel->getMainSizer(), _groupPanel);

    // Define the scrollbars
    _groupPanel->SetScrollbars(20, 20, 200, 200);

    // Create a status bar
    CreateStatusBar();

    if (layoutGroup->Attribute("statustext"))
        setStatusText(layoutGroup->Attribute("statustext"));

    // Evaluate the size information
    if (layoutGroup->Attribute("size"))
    {
        wxString sSize = layoutGroup->Attribute("size");
        long int x,y;
        sSize.substr(0, sSize.find(',')).ToLong(&x);
        sSize.substr(sSize.find(',')+1).ToLong(&y);

        if (x == -1 && y == -1)
            Maximize();
        else
            SetClientSize(wxSize(x,y));
    }
    else
        SetClientSize(wxSize(800,600));

    Layout();


    //wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    //vsizer->Add(_groupPanel, 1, wxEXPAND, 0);
    //SetSizer(vsizer);
    //vsizer->SetSizeHints(_groupPanel);
}


/////////////////////////////////////////////////
/// \brief This member function can be called
/// recursively and creates the layout for the
/// current XMLElement's children.
///
/// \param currentChild const tinyxml2::XMLElement*
/// \param currParent wxWindow*
/// \param currSizer wxSizer*
/// \param _groupPanel GroupPanel*
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::layoutChild(const tinyxml2::XMLElement* currentChild, wxWindow* currParent, wxSizer* currSizer, GroupPanel* _groupPanel)
{
    wxNotebook* noteBook = nullptr;

    // As long as another child (i.e. sibling) can be found
    while (currentChild)
    {
        // Evaluate the id attribute
        int id = currentChild->Attribute("id") ?
                    currentChild->DoubleAttribute("id") : m_windowItems.size() + 1000;

        // Get the text used by some controls
        wxString text = currentChild->GetText() ? currentChild->GetText() : "";

        wxFont font = GetFont();
        WindowState state = ENABLED;
        int alignment = wxALIGN_CENTER_VERTICAL;

        // Evaluate the state attribute
        if (currentChild->Attribute("state"))
        {
            if (currentChild->Attribute("state", "disabled"))
                state = DISABLED;
            else if (currentChild->Attribute("state", "hidden"))
                state = HIDDEN;
            else if (currentChild->Attribute("state", "readonly"))
                state = READONLY;
        }

        // evaluate the font attribute
        if (currentChild->Attribute("font"))
        {
            wxString sFont = currentChild->Attribute("font");

            if (sFont.find('i') != std::string::npos)
                font.MakeItalic();

            if (sFont.find('b') != std::string::npos)
                font.MakeBold();
        }

        // Get the alignment of the elements
        if (currentChild->Attribute("align"))
        {
            wxString sAlign = currentChild->Attribute("align");
            alignment = 0;

            // LRCTB
            if (sAlign.find_first_of("TB") == std::string::npos)
                alignment |= wxALIGN_CENTER_VERTICAL;

            if (sAlign.find_first_of("LR") == std::string::npos)
                alignment |= wxALIGN_CENTER_HORIZONTAL;

            if (sAlign.find("L") != std::string::npos)
                alignment |= wxALIGN_LEFT;

            if (sAlign.find("R") != std::string::npos)
                alignment |= wxALIGN_RIGHT;

            if (sAlign.find("T") != std::string::npos)
                alignment |= wxALIGN_TOP;

            if (sAlign.find("B") != std::string::npos)
                alignment |= wxALIGN_BOTTOM;

        }

        // Now check for the current XMLElement's
        // value (i.e. the XML-Tag name) and create
        // a corresponding control (if available)
        std::string sValue = currentChild->Value();

        if (sValue == "button")
        {
            int proportion = currentChild->IntAttribute("prop", 1);
            // Add a button
            wxButton* button = _groupPanel->CreateButton(currParent, currSizer, removeQuotationMarks(text), id, alignment, proportion);
            button->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::BUTTON, button);

            if (currentChild->Attribute("onclick"))
                m_eventTable[id].onclick = currentChild->Attribute("onclick");

            if (currentChild->Attribute("color"))
                button->SetForegroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED || state == READONLY)
                button->Disable();
            else if (state == HIDDEN)
                button->Hide();
        }
        else if (sValue == "checkbox")
        {
            // Add a checkbox
            wxCheckBox* checkbox = _groupPanel->CreateCheckBox(currParent, currSizer, removeQuotationMarks(text), id, alignment);
            checkbox->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::CHECKBOX, checkbox);

            if (currentChild->Attribute("value"))
                checkbox->SetValue(currentChild->Attribute("value", "1"));

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                checkbox->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED || state == READONLY)
                checkbox->Disable();
            else if (state == HIDDEN)
                checkbox->Hide();
        }
        else if (sValue == "radio")
        {
            // Add a radio group
            wxString label;
            int style = wxHORIZONTAL;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "horizontal") ? wxHORIZONTAL : wxVERTICAL;

            wxRadioBox* radiobox = _groupPanel->CreateRadioBox(currParent, currSizer, label, getChoices(text), style, id, alignment);
            radiobox->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::RADIOGROUP, radiobox);

            if (currentChild->Attribute("value"))
                radiobox->SetSelection(currentChild->IntAttribute("value")-1);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (state == DISABLED || state == READONLY)
                radiobox->Disable();
            else if (state == HIDDEN)
                radiobox->Hide();
        }
        else if (sValue == "spinbut")
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

            SpinBut* spinctrl = _groupPanel->CreateSpinControl(currParent, currSizer, label, nMin, nMax, nValue, id, alignment);
            spinctrl->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::SPINCTRL, spinctrl);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                spinctrl->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED || state == READONLY)
                spinctrl->Enable(false);
            else if (state == HIDDEN)
                spinctrl->Show(false);
        }
        else if (sValue == "slider")
        {
            // Add a slider
            int proportion = currentChild->IntAttribute("prop", 0);
            int nMin = 0, nMax = 100, nValue = 0;
            int style = wxHORIZONTAL;

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "horizontal") ? wxHORIZONTAL | wxSL_LABELS | wxSL_AUTOTICKS : wxVERTICAL | wxSL_LABELS | wxSL_AUTOTICKS;

            if (currentChild->Attribute("min"))
                nMin = currentChild->DoubleAttribute("min");

            if (currentChild->Attribute("max"))
                nMax = currentChild->DoubleAttribute("max");

            if (currentChild->Attribute("value"))
                nValue = currentChild->DoubleAttribute("value");

            wxSlider* slider = _groupPanel->CreateSlider(currParent, currSizer, nMin, nMax, nValue, style, id, alignment, proportion);
            slider->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::SLIDER, slider);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                slider->SetForegroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED || state == READONLY)
                slider->Enable(false);
            else if (state == HIDDEN)
                slider->Show(false);
        }
        else if (sValue == "gauge")
        {
            // Add a gauge
            int proportion = currentChild->IntAttribute("prop", 0);
            wxString label;
            int style = wxHORIZONTAL;

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "horizontal") ? wxGA_HORIZONTAL | wxGA_SMOOTH : wxGA_VERTICAL | wxGA_SMOOTH;

            wxGauge* gauge = _groupPanel->CreateGauge(currParent, currSizer, style, id, alignment, proportion);
            m_windowItems[id] = std::make_pair(CustomWindow::GAUGE, gauge);

            if (currentChild->Attribute("value"))
                gauge->SetValue(currentChild->DoubleAttribute("value"));

            if (state == DISABLED || state == READONLY)
                gauge->Disable();
            else if (state == HIDDEN)
                gauge->Hide();
        }
        else if (sValue == "dropdown")
        {
            // Add a dropdown
            int proportion = currentChild->IntAttribute("prop", 1);
            wxArrayString choices;

            if (currentChild->Attribute("label"))
            {
                wxString l = currentChild->Attribute("label");
                choices = getChoices(l);
            }
            else
                choices = getChoices(text);

            wxChoice* choice = _groupPanel->CreateChoices(currParent, currSizer, choices, id, alignment, proportion);
            m_windowItems[id] = std::make_pair(CustomWindow::DROPDOWN, choice);

            if (currentChild->Attribute("value"))
                choice->SetSelection(currentChild->IntAttribute("value")-1);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                choice->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED || state == READONLY)
                choice->Disable();
            else if (state == HIDDEN)
                choice->Hide();
        }
        else if (sValue == "combobox")
        {
            // Add a combobox
            int proportion = currentChild->IntAttribute("prop", 1);
            wxArrayString choices;

            if (currentChild->Attribute("label"))
            {
                wxString l = currentChild->Attribute("label");
                choices = getChoices(l);
            }
            else
                choices = getChoices(text);

            wxComboBox* combo = _groupPanel->CreateComboBox(currParent, currSizer, choices, id, alignment, proportion);
            m_windowItems[id] = std::make_pair(CustomWindow::COMBOBOX, combo);

            if (currentChild->Attribute("value"))
                combo->SetSelection(currentChild->IntAttribute("value")-1);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                combo->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED || state == READONLY)
                combo->Disable();
            else if (state == HIDDEN)
                combo->Hide();
        }
        else if (sValue == "textfield")
        {
            // Add a textctrl
            int proportion = currentChild->IntAttribute("prop", 0);
            int style = wxTE_PROCESS_ENTER;
            wxSize size(310,-1);

            if (currentChild->Attribute("size"))
            {
                wxString sSize = currentChild->Attribute("size");
                long int x,y;
                sSize.substr(0, sSize.find(',')).ToLong(&x);
                sSize.substr(sSize.find(',')+1).ToLong(&y);

                size.x = x;
                size.y = y;
            }

            if (currentChild->Attribute("type", "multiline"))
                style = wxTE_MULTILINE | wxTE_BESTWRAP;

            if (currentChild->Attribute("type", "markup"))
                style = wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_RICH2 | wxTE_AUTO_URL;

            wxString label;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            TextField* textctrl = _groupPanel->CreateTextInput(currParent, currSizer, label, "", style, id, size, alignment, proportion);
            textctrl->SetMarkupText(toInternalString(text.ToStdString())); // Also correctly convert linebreak characters
            textctrl->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::TEXTCTRL, textctrl);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (currentChild->Attribute("color"))
                textctrl->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                textctrl->Enable(false);
            else if (state == HIDDEN)
                textctrl->Show(false);

            textctrl->SetEditable(state != READONLY);
        }
        else if (sValue == "lamp")
        {
            // Add a lamp
            int proportion = currentChild->IntAttribute("prop", 0);
            wxSize size(20,10);

            if (currentChild->Attribute("size"))
            {
                wxString sSize = currentChild->Attribute("size");
                long int x,y;
                sSize.substr(0, sSize.find(',')).ToLong(&x);
                sSize.substr(sSize.find(',')+1).ToLong(&y);

                size.x = x;
                size.y = y;
            }

            wxString label;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            TextField* textctrl = _groupPanel->CreateLamp(currParent, currSizer, label, wxEmptyString, 0, id, size, alignment, proportion);
            m_windowItems[id] = std::make_pair(CustomWindow::LAMP, textctrl);

            //if (currentChild->Attribute("onclick"))
            //    m_eventTable[id].onclick = currentChild->Attribute("onclick");

            if (currentChild->Attribute("color"))
                textctrl->SetBackgroundColour(toWxColour(currentChild->Attribute("color")));
            else if (currentChild->Attribute("value"))
                 textctrl->SetBackgroundColour(colorFromLampStates(currentChild->Attribute("value")));

            if (state == DISABLED || state == READONLY)
                textctrl->Enable(false);
            else if (state == HIDDEN)
                textctrl->Show(false);
        }
        else if (sValue == "datetimepicker")
        {
            // Add a datetimepicker
            int proportion = currentChild->IntAttribute("prop", 1);
            int style = DT_PICKER_DATE | DT_PICKER_TIME;

            if (currentChild->Attribute("type"))
            {
                if (currentChild->Attribute("type", "timepicker"))
                    style = DT_PICKER_TIME;

                if (currentChild->Attribute("type", "datepicker"))
                    style = DT_PICKER_DATE;
            }

            DateTimePicker* dtPicker = _groupPanel->CreateDateTimePicker(currParent, currSizer, wxDefaultDateTime, style, id, alignment, proportion);

            if (currentChild->Attribute("value"))
            {
                wxString time = currentChild->Attribute("value");
                dtPicker->SetValue(removeQuotationMarks(time));
            }

            m_windowItems[id] = std::make_pair(CustomWindow::DATETIMEPICKER, dtPicker);

            if (currentChild->Attribute("onchange"))
                m_eventTable[id].onchange = currentChild->Attribute("onchange");

            if (state == DISABLED || state == READONLY)
                dtPicker->Enable(false);
            else if (state == HIDDEN)
                dtPicker->Show(false);
        }
        else if (sValue == "statictext" || sValue == "text")
        {
            // Add a static test
            wxStaticText* statictext = _groupPanel->AddStaticText(currParent, currSizer, removeQuotationMarks(text), id, alignment);
            statictext->SetFont(font);
            m_windowItems[id] = std::make_pair(CustomWindow::TEXT, statictext);

            if (currentChild->Attribute("color"))
                statictext->SetForegroundColour(toWxColour(currentChild->Attribute("color")));
        }
        else if (sValue == "prop")
        {
            // Create internal variables
            wxArrayString varList = getChoices(text);

            for (size_t i = 0; i < varList.size(); i++)
            {
                if (varList[i].find('=') != std::string::npos)
                {
                    wxString name = varList[i].substr(0, varList[i].find('='));
                    wxString value = varList[i].substr(varList[i].find('=')+1);

                    m_varTable[name.Trim()] = value.Trim(false);
                }
                else
                    m_varTable[varList[i]] = "0";
            }
        }
        else if (sValue == "bitmap")
        {
            // Add an image
            wxStaticBitmap* bitmap = _groupPanel->CreateBitmap(currParent, currSizer, removeQuotationMarks(text), id, alignment);
            m_windowItems[id] = std::make_pair(CustomWindow::IMAGE, bitmap);
        }
        else if (sValue == "separator")
        {
            // Add a separator
            int style = wxHORIZONTAL;
            bool created = false;

            if (currentChild->Attribute("type"))
            {
                if (currentChild->Attribute("type", "space"))
                {
                    // Add a spacer
                    long int space;
                    removeQuotationMarks(text).ToLong(&space);
                    _groupPanel->AddSpacer(space, currSizer);
                    created = true;
                }
                else
                    style = currentChild->Attribute("type", "vertical") ? wxVERTICAL : wxHORIZONTAL;
            }

            if (!created)
            {
                // Add a static line (i.e. the default separator)
                wxStaticLine* line = new wxStaticLine(currParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style);
                currSizer->Add(line, 0, wxEXPAND | wxALL, 5);
            }
        }
        else if (sValue == "grapher")
        {
            // Add a grapher object
            wxMGL* mgl = new wxMGL(currParent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME, true);
            m_windowItems[id] = std::make_pair(CustomWindow::GRAPHER, mgl);

            if (currentChild->Attribute("size"))
            {
                wxString sSize = currentChild->Attribute("size");
                long int x,y;
                sSize.substr(0, sSize.find(',')).ToLong(&x);
                sSize.substr(sSize.find(',')+1).ToLong(&y);

                mgl->SetMinClientSize(wxSize(x,y));
            }
            else
                mgl->SetMinClientSize(wxSize(600,400));

            currSizer->Add(mgl, !currentChild->Attribute("size"), alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5);

            if (currentChild->Attribute("onclick"))
                m_eventTable[id].onclick = currentChild->Attribute("onclick");
        }
        else if (sValue == "tablegrid")
        {
            // Add a table grid
            int proportion = currentChild->IntAttribute("prop", 1);
            TableViewer* table = new TableViewer(currParent, id, nullptr, nullptr, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_THEME);
            currSizer->Add(table, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN | wxFIXED_MINSIZE, 5);
            m_windowItems[id] = std::make_pair(CustomWindow::TABLE, table);
            table->SetTableReadOnly(state == READONLY);

            if (currentChild->Attribute("size"))
            {
                wxString sSize = currentChild->Attribute("size");
                long int x,y;
                sSize.substr(0, sSize.find(',')).ToLong(&x);
                sSize.substr(sSize.find(',')+1).ToLong(&y);
                NumeRe::Table data(x,y);
                table->SetData(data, "", "");
                table->SetMinSize(table->calculateMinSize());
            }
            else
            {
                NumeRe::Table data(1,1);
                table->SetData(data, "", "");
                table->SetMinSize(wxSize(-1, 300));
            }

            if (currentChild->Attribute("onclick"))
                m_eventTable[id].onclick = currentChild->Attribute("onclick");

            if (currentChild->Attribute("onactivate"))
                m_eventTable[id].onactivate = currentChild->Attribute("onactivate");

            if (state == DISABLED)
                table->Enable(false);
            else if (state == HIDDEN)
                table->Show(false);

            //table->SetSize(table->getSize)
        }
        else if (sValue == "treelist")
        {
            // Add a treelist control
            int proportion = currentChild->IntAttribute("prop", 1);
            int style = wxTL_SINGLE;

            if (currentChild->Attribute("type"))
                style |= currentChild->Attribute("type", "checkmark") ? wxTL_CHECKBOX : wxTL_MULTIPLE;

            wxArrayString labels;
            wxArrayString values;
            int colSize = GetClientSize().x;

            wxTreeListCtrl* listCtrl = _groupPanel->CreateTreeListCtrl(currParent, currSizer, style, wxDefaultSize, id, alignment, proportion);
            m_windowItems[id] = std::make_pair(CustomWindow::TREELIST, listCtrl);
            listCtrl->SetFont(font);

            if (currentChild->Attribute("label"))
            {
                wxString label = currentChild->Attribute("label");
                labels = getChoices(label);
            }

            if (currentChild->Attribute("value"))
            {
                wxString value = currentChild->Attribute("value");
                values = getChoices(value);
            }

            if (values.size())
            {
                if (labels.size())
                {
                    colSize = std::max(colSize / (int)labels.size() - 2, MINCOLSIZE);

                    for (size_t j = 0; j < labels.size(); j++)
                    {
                        listCtrl->AppendColumn(labels[j], colSize);
                    }
                }

                populateTreeListCtrl(listCtrl, values);
            }
            else if (currentChild->Attribute("size"))
            {
                wxString sSize = currentChild->Attribute("size");
                long int row,col;
                sSize.substr(0, sSize.find(',')).ToLong(&row);
                sSize.substr(sSize.find(',')+1).ToLong(&col);

                colSize = std::max(colSize / (int)col - 2, MINCOLSIZE);

                for (size_t j = 0; j < (size_t)col; j++)
                {
                    if (labels.size() > j)
                        listCtrl->AppendColumn(labels[j], colSize);
                    else
                        listCtrl->AppendColumn("", colSize);
                }

                for (int i = 0; i < row; i++)
                {
                    listCtrl->AppendItem(listCtrl->GetRootItem(), "ITEM " + wxString::Format("%d", i+1));
                }
            }
            else if (labels.size())
            {
                colSize = std::max(colSize / (int)labels.size() - 2, MINCOLSIZE);

                for (size_t j = 0; j < labels.size(); j++)
                {
                    listCtrl->AppendColumn(labels[j], colSize);
                }
            }

            if (currentChild->Attribute("onclick"))
                m_eventTable[id].onclick = currentChild->Attribute("onclick");

            if (currentChild->Attribute("onactivate"))
                m_eventTable[id].onactivate = currentChild->Attribute("onactivate");

            if (state == DISABLED || state == READONLY)
                listCtrl->Enable(false);
            else if (state == HIDDEN)
                listCtrl->Show(false);
        }
        else if (sValue == "group")
        {
            // Add a group. A group is a recursive control,
            // which contains further controls (including further groups).
            // Will call this function recursively.
            wxString label;
            int style = wxHORIZONTAL;
            bool isCollapsible = false;
            bool isNotebook = false;
            bool isMenu = false;
            int expand = 0;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("type"))
                style = currentChild->Attribute("type", "horizontal") ? wxHORIZONTAL : wxVERTICAL;

            if (currentChild->Attribute("style"))
            {
                isCollapsible = currentChild->Attribute("style", "collapse");
                isNotebook = currentChild->Attribute("style", "tabs");
                isMenu = currentChild->Attribute("style", "menu");
            }

            if (currentChild->Attribute("expand"))
                expand = currentChild->Attribute("expand", "true") ? 1 : 0;

            // A collapsible group is currently very buggy (if used
            // with the current GroupPanel).
#warning TODO (numere#6#08/15/21): Fix the collapsible group
            if (label.length())
            {
                if (isCollapsible)
                {
                    wxCollapsiblePane* pane = _groupPanel->createCollapsibleGroup(label, currParent, currSizer);
                    wxBoxSizer* sizer = new wxBoxSizer(style);

                    layoutChild(currentChild->FirstChildElement(), pane->GetPane(), sizer, _groupPanel);

                    pane->GetPane()->SetSizer(sizer);
                    sizer->SetSizeHints(pane->GetPane());
                }
                else if (isNotebook)
                {
                    if (!noteBook)
                    {
                        noteBook = new wxNotebook(currParent, id);
                        m_windowItems[id] = std::make_pair(CustomWindow::NOTEBOOK, noteBook);
                        currSizer->Add(noteBook, 1, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN | wxALL, 5);

                        if (currentChild->Attribute("onchange"))
                            m_eventTable[id].onchange = currentChild->Attribute("onchange");
                    }

                    GroupPanel* _newPanel = new GroupPanel(noteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, style == wxHORIZONTAL ? false : true);

                    layoutChild(currentChild->FirstChildElement(), _newPanel, _newPanel->getMainSizer(), _newPanel);

                    noteBook->AddPage(_newPanel, label);
                }
                else if (isMenu)
                {
                    wxMenuBar* menuBar = GetMenuBar();

                    if (!menuBar)
                    {
                        menuBar = new wxMenuBar();
                        SetMenuBar(menuBar);
                    }

                    wxMenu* currMenu = new wxMenu();
                    menuBar->Append(currMenu, label);
                    layoutMenu(currentChild->FirstChildElement(), currMenu);
                }
                else
                {
                    wxStaticBoxSizer* sizer = _groupPanel->createGroup(label, style, currParent, currSizer, expand);
                    layoutChild(currentChild->FirstChildElement(), sizer->GetStaticBox(), sizer, _groupPanel);
                }
            }
            else
            {
                wxBoxSizer* sizer = _groupPanel->createGroup(style, currSizer, expand);
                layoutChild(currentChild->FirstChildElement(), currParent, sizer, _groupPanel);
            }

        }

        // Find the next sibling from the current
        // child element
        if (currentChild->NextSibling())
            currentChild = currentChild->NextSibling()->ToElement();
        else
            break;
    }
}


/////////////////////////////////////////////////
/// \brief This member function can be called
/// recursively and creates menus and submenus
/// for the current window layout.
///
/// \param currentChild const tinyxml2::XMLElement*
/// \param currMenu wxMenu*
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::layoutMenu(const tinyxml2::XMLElement* currentChild, wxMenu* currMenu)
{
    while (currentChild)
    {
        // Evaluate the id attribute
        int id = currentChild->Attribute("id") ?
                    currentChild->DoubleAttribute("id") : m_windowItems.size() + 1000;

        // Get the text used by some controls
        wxString text = currentChild->GetText() ? currentChild->GetText() : "";

        wxFont font = GetFont();
        WindowState state = ENABLED;

        // Evaluate the state attribute
        if (currentChild->Attribute("state"))
            state = (currentChild->Attribute("state", "disabled") || currentChild->Attribute("state", "hidden")) ? DISABLED : ENABLED;

        // evaluat the font attribute
        if (currentChild->Attribute("font"))
        {
            wxString sFont = currentChild->Attribute("font");

            if (sFont.find('i') != std::string::npos)
                font.MakeItalic();

            if (sFont.find('b') != std::string::npos)
                font.MakeBold();
        }

        std::string sValue = currentChild->Value();

        if (sValue == "menuitem")
        {
            // Create a menu item
            bool isCheck = false;
            bool isChecked = false;

            if (currentChild->Attribute("type"))
                isCheck = currentChild->Attribute("type", "checkmark") ? true : false;

            if (currentChild->Attribute("value"))
                isChecked = currentChild->Attribute("value", "0") ? false : true;

            wxMenuItem* item = currMenu->Append(id, removeQuotationMarks(text), wxEmptyString, isCheck ? wxITEM_CHECK : wxITEM_NORMAL);
            item->SetFont(font);

            if (isCheck && isChecked)
                item->Check(true);

            m_windowItems[id] = std::make_pair(CustomWindow::MENUITEM, item);

            if (currentChild->Attribute("onclick"))
                m_eventTable[id].onclick = currentChild->Attribute("onclick");

            if (currentChild->Attribute("color"))
                item->SetTextColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                item->Enable(false);
        }
        else if (sValue == "checkbox")
        {
            // Create a checkable menu item
            bool isChecked = false;

            if (currentChild->Attribute("value"))
                isChecked = currentChild->Attribute("value", "0") ? false : true;

            wxMenuItem* item = currMenu->Append(id, removeQuotationMarks(text), wxEmptyString, wxITEM_CHECK);
            item->SetFont(font);

            if (isChecked)
                item->Check(true);

            m_windowItems[id] = std::make_pair(CustomWindow::MENUITEM, item);

            if (currentChild->Attribute("onclick"))
                m_eventTable[id].onclick = currentChild->Attribute("onclick");

            if (currentChild->Attribute("color"))
                item->SetTextColour(toWxColour(currentChild->Attribute("color")));

            if (state == DISABLED)
                item->Enable(false);
        }
        else if (sValue == "separator")
            currMenu->AppendSeparator();
        else if (sValue == "group")
        {
            // Add a group. A group is a recursive control,
            // which contains further controls (including further groups).
            // Will call this function recursively.
            wxString label;
            bool isMenu = false;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("style"))
                isMenu = currentChild->Attribute("style", "menu");

            if (label.length() && isMenu)
            {
                wxMenu* subMenu = new wxMenu();
                currMenu->AppendSubMenu(subMenu, label);
                layoutMenu(currentChild->FirstChildElement(), subMenu);
            }

        }

        // Find the next sibling from the current
        // child element
        if (currentChild->NextSibling())
            currentChild = currentChild->NextSibling()->ToElement();
        else
            break;

    }
}


/////////////////////////////////////////////////
/// \brief This member function is the central
/// kernel interaction event handler. Will be
/// called from the wxWidgets' event handler and
/// all the registered procedures (if any).
///
/// \param event wxEvent&
/// \param sEventType const wxString&
/// \param pos const EventPosition&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::handleEvent(wxEvent& event, const wxString& sEventType, const EventPosition& pos)
{
#warning NOTE (numere#1#08/15/21): The "onclose" event is still undefined
    if (sEventType == "onclose")
    {
        if (isDialog())
            endDialog("");

        return;
    }

    // Try to find a defined event handler for
    // the current window item element
    auto iter = m_eventTable.find(event.GetId());

    if (iter != m_eventTable.end())
    {
        wxString sEventHandler;

        if (sEventType == "onchange")
            sEventHandler = iter->second.onchange;
        else if (sEventType == "onclick")
            sEventHandler = iter->second.onclick;
        else if (sEventType == "onactivate")
            sEventHandler = iter->second.onactivate;

        if (sEventHandler[0] == '$' && sEventHandler.length() > 1 && (wxIsalnum(sEventHandler[1]) || sEventHandler[1] == '_') && !isDialog())
        {
            // If the event handler starts with an
            // dollar, it must be a procedure
            NumeReWindow* mainWindow = static_cast<NumeReWindow*>(m_parent);
            WindowItemParams params;

            // Get the parameters for the selected
            // window item
            getItemParameters(event.GetId(), params);

            // Do not return the values of a tree list here
            if (params.type == "treelist")
                params.value = "nan";

            wxString kvl_event;

            // Create the corresponding key-value-list
            // syntax
            if (event.GetEventType() == wxEVT_GRID_SELECT_CELL
                || event.GetEventType() == wxEVT_GRID_CELL_LEFT_DCLICK)
            {
                TableViewer* table = static_cast<TableViewer*>(m_windowItems[event.GetId()].second);
                params.value = convertToCodeString(table->GetCellValue(static_cast<wxGridEvent&>(event).GetRow(),
                                                                       static_cast<wxGridEvent&>(event).GetCol()));
            }

            kvl_event = "{\"event\",\"" + sEventType
                        + "\",\"object\",\"" + params.type
                        + "\",\"value\"," + params.value
                        + ",\"state\",\"" + params.state + "\"";

            wxString p = pos.serialize();

            if (p.length())
                kvl_event += ",\"position\", " + p;

            // Call the procedure with the following syntax:
            // $PROCEDURE(winid, objectid, event{})
            mainWindow->pass_command(sEventHandler + "(" + toString(m_windowRef.getId()) + ","
                                                         + toString(event.GetId()) + ","
                                                         + kvl_event + "})", true);
        }
        else if (sEventHandler.find('(') != std::string::npos)
        {
            wxArrayString funcDef = decodeEventHandlerFunction(sEventHandler);

            if (funcDef.front() == "evt_close")
                closeWindow();
            else if (funcDef.front() == "evt_sendvaltoitem" && funcDef.size() >= 2)
            {
                long int targetID;
                WindowItemParams params;

                getItemParameters(event.GetId(), params);
                WindowItemValue val;
                val.stringValue = params.value;
                val.tableValue = params.table;
                val.type = params.type;

                for (size_t i = 1; i < funcDef.size(); i++)
                {
                    funcDef[i].ToLong(&targetID);
                    setItemValue(val, targetID);
                }
            }
            else if (funcDef.front() == "evt_copyvalues" && funcDef.size() >= 3)
            {
                long int sourceID, targetID;
                funcDef[1].ToLong(&sourceID);
                WindowItemParams params;
                getItemParameters(sourceID, params);

                WindowItemValue val;
                val.stringValue = params.value;
                val.tableValue = params.table;
                val.type = params.type;

                for (size_t i = 2; i < funcDef.size(); i++)
                {
                    funcDef[i].ToLong(&targetID);
                    setItemValue(val, targetID);
                }
            }
            else if (funcDef.front() == "evt_changestate" && funcDef.size() >= 3)
            {
                long int id;
                funcDef[1].ToLong(&id);
                setItemState(funcDef[2], id);
            }
            else if (funcDef.front() == "evt_enddialog" && funcDef.size() >= 2)
            {
                if (isDialog())
                    endDialog(funcDef[1]);
                else
                    closeWindow();
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Returns the parameters of this window.
///
/// \param params WindowItemParams&
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::getWindowParameters(WindowItemParams& params) const
{
    params.type = "window";
    params.value = wxString::Format("{%d,%d}", GetClientSize().x, GetClientSize().y);
    params.state = "running";
    params.color = toWxString(GetBackgroundColour());
    params.label = "\"" + GetTitle() + "\"";

    return true;
}


/////////////////////////////////////////////////
/// \brief Returns the parameters of the selected
/// window item.
///
/// \param windowItemID int
/// \param params WindowItemParams&
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::getItemParameters(int windowItemID, WindowItemParams& params) const
{
    // If the ID equals -1, return the window parameters instead
    if (windowItemID == -1)
        return getWindowParameters(params);

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

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
            params.type = "statictext";
            params.value = convertToCodeString(static_cast<wxStaticText*>(object.second)->GetLabel());
            params.label = params.value;
            params.color = toWxString(static_cast<wxStaticText*>(object.second)->GetForegroundColour());

            break;
        case CustomWindow::TEXTCTRL:
            params.type = "textfield";
            params.value = convertToCodeString(static_cast<TextField*>(object.second)->GetValue());
            params.label = convertToCodeString(static_cast<TextField*>(object.second)->GetLabel());
            params.color = toWxString(static_cast<TextField*>(object.second)->GetBackgroundColour());

            if (!static_cast<wxTextCtrl*>(object.second)->IsEditable())
                params.state = "readonly";

            break;
        case CustomWindow::LAMP:
            params.type = "lamp";
            params.label = convertToCodeString(static_cast<TextField*>(object.second)->GetLabel());
            params.color = toWxString(static_cast<TextField*>(object.second)->GetBackgroundColour());
            params.value = "\"" + lampStatesFromColor(static_cast<TextField*>(object.second)->GetBackgroundColour()) + "\"";

            break;
        case CustomWindow::DATETIMEPICKER:
            params.type = "datetimepicker";
            params.value = convertToCodeString(static_cast<DateTimePicker*>(object.second)->GetValue());

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

            for (size_t i = 0; i < choices->GetCount(); i++)
            {
                if (params.label.length())
                {
                    if (params.label[0] != '{')
                        params.label.insert(0, '{');

                    params.label += ", ";
                }

                params.label += convertToCodeString(choices->GetString(i));
            }

            if (params.label[0] == '{')
                params.label += '}';

            params.color = toWxString(static_cast<wxChoice*>(object.second)->GetBackgroundColour());

            break;
        }
        case CustomWindow::COMBOBOX:
        {
            params.type = "combobox";
            wxComboBox* combo = static_cast<wxComboBox*>(object.second);

            if (combo->GetSelection() != wxNOT_FOUND)
                params.value = convertToCodeString(combo->GetString(combo->GetSelection()));
            else
                params.value = convertToCodeString(combo->GetValue());

            for (size_t i = 0; i < combo->GetCount(); i++)
            {
                if (params.label.length())
                {
                    if (params.label[0] != '{')
                        params.label.insert(0, '{');

                    params.label += ", ";
                }

                params.label += convertToCodeString(combo->GetString(i));
            }

            if (params.label[0] == '{')
                params.label += '}';

            params.color = toWxString(static_cast<wxComboBox*>(object.second)->GetBackgroundColour());

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
        case CustomWindow::SLIDER:
            params.type = "slider";
            params.value = wxString::Format("%d", static_cast<wxSlider*>(object.second)->GetValue());

            break;
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            params.table = table->GetDataCopy();
            params.value = convertToCodeString(table->getSelectedValues());
            params.type = "tablegrid";

            if (!table->IsEditable())
                params.state = "readonly";

            break;
        }
        case CustomWindow::GRAPHER:
        {
            wxMGL* grapher = static_cast<wxMGL*>(object.second);
            params.value = convertToCodeString(grapher->getClickedCoords());
            params.type = "grapher";

            break;
        }
        case CustomWindow::TREELIST:
        {
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);

            for (size_t i = 0; i < listCtrl->GetColumnCount(); i++)
            {
                if (params.label.length())
                {
                    if (params.label[0] != '{')
                        params.label.insert(0, '{');

                    params.label += ", ";
                }

                params.label += convertToCodeString(listCtrl->GetDataView()->GetColumn(i)->GetTitle());
            }

            if (params.label[0] == '{')
                params.label += '}';

            params.value = getTreeListCtrlValue(listCtrl);
            params.type = "treelist";

            break;
        }
        case CustomWindow::NOTEBOOK:
        {
            wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);
            params.type = "tabs";

            for (int page = 0; page < noteBook->GetPageCount(); page++)
            {
                if (params.label.length())
                {
                    if (params.label[0] != '{')
                        params.label.insert(0, '{');

                    params.label += ", ";
                }

                params.label += convertToCodeString(noteBook->GetPageText(page));
            }

            if (params.label[0] == '{')
                params.label += '}';

            params.value = convertToCodeString(noteBook->GetPageText(noteBook->GetSelection()));

            break;
        }
        case CustomWindow::MENUITEM:
        {
            wxMenuItem* item = static_cast<wxMenuItem*>(object.second);
            params.type = "menuitem";
            params.label = convertToCodeString(item->GetItemLabel());

            if (item->IsCheckable())
                params.value = item->IsChecked() ? "true" : "false";
            else
                params.value = convertToCodeString(item->GetLabelText(item->GetItemLabel()));

            params.color = toWxString(item->GetTextColour());
            params.state = item->IsEnabled() ? "enabled" : "disabled";

            break;
        }
        case CustomWindow::IMAGE:
            break;
    }

    if (object.first != CustomWindow::MENUITEM && !params.state.length())
        params.state = !static_cast<wxWindow*>(object.second)->IsShown() ? "hidden" : static_cast<wxWindow*>(object.second)->IsEnabled() ? "enabled" : "disabled";

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function decodes the
/// arguments of a event handler function and
/// returns them as a wxArrayString.
///
/// \param sEventHandler const wxString&
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString CustomWindow::decodeEventHandlerFunction(const wxString& sEventHandler) const
{
    wxArrayString funcDef;
    wxString sParams = sEventHandler.substr(sEventHandler.find('(')+1);
    sParams.erase(sParams.rfind(')'));

    funcDef.Add(sEventHandler.substr(0, sEventHandler.find('(')));

    if (sParams.length())
    {
        wxArrayString choices = getChoices(sParams, true);

        for (size_t i = 0; i < choices.size(); i++)
            funcDef.Add(choices[i]);
    }

    return funcDef;
}


/////////////////////////////////////////////////
/// \brief Wrapper for wxWindow::Refresh to use
/// it together with CallAfter().
///
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::Refresh()
{
    wxWindow::Refresh();
}


/////////////////////////////////////////////////
/// \brief Returns a list of all window item IDs,
/// which correspond to the selected
/// WindowItemType.
///
/// \param _type CustomWindow::WindowItemType
/// \return std::vector<int>
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Close this window.
///
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::closeWindow()
{
    return Close();
}


/////////////////////////////////////////////////
/// \brief Get the value of the selected item.
///
/// \param windowItemID int
/// \return WindowItemValue
///
/////////////////////////////////////////////////
WindowItemValue CustomWindow::getItemValue(int windowItemID) const
{
    WindowItemParams params;
    WindowItemValue value;

    if (getItemParameters(windowItemID, params))
    {
        if (params.value.substr(0, 2) == "\"{" && params.value.substr(params.value.length()-2) == "}\"")
            params.value = params.value.substr(1, params.value.length()-2);

        value.stringValue = params.value;
        value.tableValue = params.table;
        value.type = params.type;
        return value;
    }

    return value;
}


/////////////////////////////////////////////////
/// \brief Get the label of the selected item.
///
/// \param windowItemID int
/// \return wxString
///
/////////////////////////////////////////////////
wxString CustomWindow::getItemLabel(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
    {
        if (params.label.substr(0, 2) == "\"{" && params.label.substr(params.label.length()-2) == "}\"")
            params.label = params.label.substr(1, params.label.length()-2);

        return params.label;
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Get the state of the selected item.
///
/// \param windowItemID int
/// \return wxString
///
/////////////////////////////////////////////////
wxString CustomWindow::getItemState(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
        return params.state;

    return "";
}


/////////////////////////////////////////////////
/// \brief Get the color of the selected item.
///
/// \param windowItemID int
/// \return wxString
///
/////////////////////////////////////////////////
wxString CustomWindow::getItemColor(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
        return params.color;

    return "";
}


/////////////////////////////////////////////////
/// \brief Get the current selection of the
/// selected item.
///
/// \param windowItemID int
/// \return wxString
///
/////////////////////////////////////////////////
wxString CustomWindow::getItemSelection(int windowItemID) const
{
    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return "";

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::TEXTCTRL:
        {
            TextField* field = static_cast<TextField*>(object.second);

            if (field->HasSelection())
            {
                long int from;
                long int to;
                field->GetSelection(&from, &to);
                return "{" + toString((long long)from+1) + "," + toString((long long)to-from) + "}";
            }

            return toString((long long)field->GetInsertionPoint()+1);
        }
        case CustomWindow::DROPDOWN:
        {
            wxChoice* choices = static_cast<wxChoice*>(object.second);
            return toString(choices->GetSelection()+1);
        }
        case CustomWindow::COMBOBOX:
        {
            wxComboBox* combo = static_cast<wxComboBox*>(object.second);
            return toString(combo->GetSelection()+1);
        }
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            return "{" + toString(table->GetInternalRows(table->GetGridCursorRow())+1) + "," + toString(table->GetGridCursorCol()+1) + "}";
        }
        case CustomWindow::TREELIST:
        {
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);
            int selection = enumerateListItems(listCtrl, listCtrl->GetSelection());
            return toString(selection+1);
        }
        case CustomWindow::NOTEBOOK:
        {
            wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);
            int selection = noteBook->GetSelection();
            return toString(selection+1);
        }
        case CustomWindow::GAUGE:
        case CustomWindow::SPINCTRL:
        case CustomWindow::SLIDER:
        case CustomWindow::RADIOGROUP:
        case CustomWindow::BUTTON:
        case CustomWindow::CHECKBOX:
        case CustomWindow::TEXT:
        case CustomWindow::MENUITEM:
        case CustomWindow::GRAPHER:
        case CustomWindow::IMAGE:
        case CustomWindow::DATETIMEPICKER:
        case CustomWindow::LAMP:
            break;
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the value of the selected
/// window property.
///
/// \param varName const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString CustomWindow::getPropValue(const wxString& varName) const
{
    auto iter = m_varTable.find(varName);

    if (iter != m_varTable.end())
        return iter->second;

    return "nan";
}


/////////////////////////////////////////////////
/// \brief Returns a list of all available window
/// properties.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString CustomWindow::getProperties() const
{
    wxString sProperties;

    for (auto iter : m_varTable)
    {
        if (sProperties.length())
            sProperties += ",";

        sProperties += "\"" + iter.first + "\"";
    }

    if (!sProperties.length())
        return "\"\"";

    return sProperties;
}


/////////////////////////////////////////////////
/// \brief Returns the contents of the status bar
/// if any available.
///
/// \return wxString
///
/////////////////////////////////////////////////
wxString CustomWindow::getStatusText() const
{
    const wxStatusBar* bar = GetStatusBar();
    int numFields = bar->GetFieldsCount();
    wxString sStatusText;

    for (int i = 0; i < numFields; i++)
    {
        if (sStatusText.length())
        {
            if (sStatusText[0] != '{')
                sStatusText.insert(0, '{');

            sStatusText += ",";
        }

        sStatusText += "\"" + bar->GetStatusText(i) + "\"";
    }

    if (!sStatusText.length())
        return "\"\"";

    if (sStatusText[0] == '{')
        sStatusText += '}';

    return sStatusText;
}


/////////////////////////////////////////////////
/// \brief Push an item value change to the
/// internal event handler.
///
/// \param _value WindowItemValue&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::pushItemValue(WindowItemValue& _value, int windowItemID)
{
    GetEventHandler()->QueueEvent(new SetValueEvent(SET_WINDOW_VALUE, GetId(), windowItemID, _value));
    return true;
}


/////////////////////////////////////////////////
/// \brief Push an item label change to the
/// internal event handler.
///
/// \param _label const wxString&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::pushItemLabel(const wxString& _label, int windowItemID)
{
    GetEventHandler()->QueueEvent(new SetLabelEvent(SET_WINDOW_LABEL, GetId(), windowItemID, _label));
    return true;
}


/////////////////////////////////////////////////
/// \brief Push an item selection change to the
/// internal event handler.
///
/// \param selectionID int
/// \param selectionID2 int
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::pushItemSelection(int selectionID, int selectionID2, int windowItemID)
{
    GetEventHandler()->QueueEvent(new SetSelectionEvent(SET_WINDOW_SELECTION, GetId(), windowItemID, selectionID, selectionID2));
    return true;
}


/////////////////////////////////////////////////
/// \brief Push an item focus change to the
/// internal event handler.
///
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::pushItemFocus(int windowItemID)
{
    GetEventHandler()->QueueEvent(new SetFocusEvent(SET_WINDOW_FOCUS, GetId(), windowItemID));
    return true;
}


/////////////////////////////////////////////////
/// \brief Change the value of the selected item.
///
/// \param _value WindowItemValue&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemValue(WindowItemValue& _value, int windowItemID)
{
    if (windowItemID == -1)
    {
        long int x,y;
        _value.stringValue.substr(0, _value.stringValue.find(',')).ToLong(&x);
        _value.stringValue.substr(_value.stringValue.find(',')+1).ToLong(&y);

        if (x == -1 && y == -1)
            Maximize();
        else
            SetClientSize(wxSize(x,y));

        Refresh();

        return true;
    }

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

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
            static_cast<TextField*>(object.second)->SetMarkupText(removeQuotationMarks(_value.stringValue));
            break;
        case CustomWindow::LAMP:
        {
            wxColour color = colorFromLampStates(_value.stringValue);
            static_cast<wxTextCtrl*>(object.second)->SetBackgroundColour(color);
            Refresh();
            break;
        }
        case CustomWindow::DATETIMEPICKER:
        {
            //wxDateTime dt;
            //wxString::const_iterator end;
            //
            //if (!dt.ParseDateTime(removeQuotationMarks(_value.stringValue), &end))
            //    dt.ParseDate(removeQuotationMarks(_value.stringValue), &end);

            static_cast<DateTimePicker*>(object.second)->SetValue(removeQuotationMarks(_value.stringValue));
            break;
        }
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
        case CustomWindow::SLIDER:
        {
            long int nVal;
            removeQuotationMarks(_value.stringValue).ToLong(&nVal);
            static_cast<wxSlider*>(object.second)->SetValue(nVal);
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
        case CustomWindow::COMBOBOX:
        {
            wxComboBox* combo = static_cast<wxComboBox*>(object.second);
            int sel = combo->FindString(removeQuotationMarks(_value.stringValue), true);

            if (sel != wxNOT_FOUND)
                combo->SetSelection(sel);
            else
                combo->SetValue(removeQuotationMarks(_value.stringValue));

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
            table->SetData(_value.tableValue, "", "");
            break;
        }
        case CustomWindow::TREELIST:
        {
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);
            populateTreeListCtrl(listCtrl, getChoices(_value.stringValue));
            break;
        }
        case CustomWindow::NOTEBOOK:
        {
            wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);
            noteBook->SetPageText(noteBook->GetSelection(), removeQuotationMarks(_value.stringValue));

            break;
        }
        case CustomWindow::MENUITEM:
        {
            wxMenuItem* item = static_cast<wxMenuItem*>(object.second);

            if (item->IsCheckable())
                item->Check(removeQuotationMarks(_value.stringValue) != "0");
            else
                item->SetItemLabel(removeQuotationMarks(_value.stringValue));

            break;
        }
        case CustomWindow::GRAPHER:
            break;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Change the label of the selected item.
///
/// \param _label const wxString&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
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

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

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
        case CustomWindow::LAMP:
            static_cast<TextField*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::SPINCTRL:
            static_cast<SpinBut*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::RADIOGROUP:
            static_cast<wxRadioBox*>(object.second)->SetLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::TREELIST:
        {
            wxString lab = _label;
            wxArrayString labels = getChoices(lab);
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);

            for (size_t i = 0; i < labels.size(); i++)
            {
                if (listCtrl->GetColumnCount() <= i)
                    listCtrl->AppendColumn(labels[i]);
                else
                    listCtrl->GetDataView()->GetColumn(i)->SetTitle(labels[i]);
            }

            break;
        }
        case CustomWindow::NOTEBOOK:
        {
            wxString lab = _label;
            wxArrayString labels = getChoices(lab);
            wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);

            for (size_t i = 0; i < labels.size(); i++)
            {
                if (i >= (size_t)noteBook->GetPageCount())
                    break;

                noteBook->SetPageText(i, labels[i]);
            }

            break;
        }
        case CustomWindow::MENUITEM:
            static_cast<wxMenuItem*>(object.second)->SetItemLabel(removeQuotationMarks(_label));
            break;
        case CustomWindow::DROPDOWN:
        {
            wxString lab = _label;
            wxArrayString labels = getChoices(lab);
            static_cast<wxChoice*>(object.second)->Set(labels);
            break;
        }
        case CustomWindow::COMBOBOX:
        {
            wxString lab = _label;
            wxArrayString labels = getChoices(lab);
            static_cast<wxComboBox*>(object.second)->Set(labels);
            break;
        }
        case CustomWindow::GAUGE:
        case CustomWindow::IMAGE:
        case CustomWindow::TABLE:
        case CustomWindow::GRAPHER:
        case CustomWindow::SLIDER:
        case CustomWindow::DATETIMEPICKER:
            break;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Change the state of the selected item.
///
/// \param _state const wxString&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemState(const wxString& _state, int windowItemID)
{
    if (windowItemID == -1)
        return false;

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

    if (object.first == CustomWindow::MENUITEM)
    {
        if (_state == "hidden" || _state == "disabled")
            static_cast<wxMenuItem*>(object.second)->Enable(false);
        else
            static_cast<wxMenuItem*>(object.second)->Enable(true);

        return true;
    }

    wxWindow* window = static_cast<wxWindow*>(object.second);

    if (_state == "hidden")
        window->Show(false);
    else if (_state == "readonly")
    {
        window->Show(true);

        if (object.first == CustomWindow::TABLE)
        {
            window->Enable(true);
            static_cast<TableViewer*>(window)->SetTableReadOnly(true);
        }
        else if (object.first == CustomWindow::TEXTCTRL)
        {
            window->Enable(true);
            static_cast<TextField*>(window)->SetEditable(false);
        }
        else
            window->Enable(false);
    }
    else if (_state == "disabled")
    {
        window->Show(true);
        window->Enable(false);
    }
    else if (_state == "enabled")
    {
        window->Show(true);
        window->Enable(true);

        if (object.first == CustomWindow::TABLE)
            static_cast<TableViewer*>(window)->SetTableReadOnly(false);
        else if (object.first == CustomWindow::TEXTCTRL)
            static_cast<TextField*>(window)->SetEditable(true);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Change the color of the selected item.
///
/// \param _color const wxString&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
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

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

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
        case CustomWindow::LAMP:
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
        case CustomWindow::COMBOBOX:
            static_cast<wxComboBox*>(object.second)->SetBackgroundColour(color);
            break;
        case CustomWindow::MENUITEM:
            static_cast<wxMenuItem*>(object.second)->SetTextColour(color);
            break;
        case CustomWindow::TREELIST:
        case CustomWindow::GAUGE:
        case CustomWindow::IMAGE:
        case CustomWindow::TABLE:
        case CustomWindow::GRAPHER:
        case CustomWindow::SLIDER:
        case CustomWindow::DATETIMEPICKER:
            break;
    }

    Refresh();

    return true;
}


/////////////////////////////////////////////////
/// \brief Set the selection in the selected
/// window item.
///
/// \param selectionID int
/// \param selectionID2 int
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemSelection(int selectionID, int selectionID2, int windowItemID)
{
    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::TEXTCTRL:
        {
            TextField* field = static_cast<TextField*>(object.second);

            if (selectionID2)
                field->SetSelection(std::max(std::min((long)selectionID-1, (long)field->GetLastPosition()), 0L),
                                    std::max(std::min((long)selectionID-1+selectionID2, (long)field->GetLastPosition()), 0L));
            else
                field->SetInsertionPoint(std::max(std::min((long)selectionID-1, (long)field->GetLastPosition()), 0L));

            break;
        }
        case CustomWindow::DROPDOWN:
        {
            wxChoice* choices = static_cast<wxChoice*>(object.second);

            if (selectionID > 0 && selectionID <= choices->GetCount())
                choices->SetSelection(selectionID-1);

            break;
        }
        case CustomWindow::COMBOBOX:
        {
            wxComboBox* combo = static_cast<wxComboBox*>(object.second);

            if (selectionID > 0 && selectionID <= combo->GetCount())
                combo->SetSelection(selectionID-1);

            break;
        }
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            table->SetGridCursorSilent(std::min(std::max(0, table->GetExternalRows(selectionID-1)), table->GetNumberRows()-1),
                                       std::min(std::max(0, selectionID2-1), table->GetNumberCols()-1));
            break;
        }
        case CustomWindow::TREELIST:
        {
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);

            for (wxTreeListItem item = listCtrl->GetFirstItem(); item.IsOk(); item = listCtrl->GetNextItem(item))
            {
                if (selectionID <= 1)
                {
                    listCtrl->Select(item);
                    break;
                }

                selectionID--;
            }

            break;
        }
        case CustomWindow::NOTEBOOK:
        {
            wxNotebook* notebook = static_cast<wxNotebook*>(object.second);

            if (selectionID > 0 && selectionID <= notebook->GetPageCount())
                notebook->ChangeSelection(selectionID-1);

            break;
        }
        case CustomWindow::GAUGE:
        case CustomWindow::SPINCTRL:
        case CustomWindow::SLIDER:
        case CustomWindow::RADIOGROUP:
        case CustomWindow::BUTTON:
        case CustomWindow::CHECKBOX:
        case CustomWindow::TEXT:
        case CustomWindow::MENUITEM:
        case CustomWindow::GRAPHER:
        case CustomWindow::IMAGE:
        case CustomWindow::DATETIMEPICKER:
        case CustomWindow::LAMP:
            break;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Set the keyboard focus to the selected
/// item.
///
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemFocus(int windowItemID)
{
    if (windowItemID == -1)
    {
        SetFocus();
        return true;
    }

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;
    static_cast<wxWindow*>(object.second)->SetFocus();
    return true;
}


/////////////////////////////////////////////////
/// \brief Updates the selected grapher item.
///
/// \param _helper GraphHelper*
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemGraph(GraphHelper* _helper, int windowItemID)
{
    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end() || iter->second.first != CustomWindow::GRAPHER)
        return false;

    wxMGL* mgl = static_cast<wxMGL*>(iter->second.second);
    wxSize s = mgl->GetSize();
    mgl->Animation(false);

    mgl->SetDraw(_helper);
    mgl->SetGraph(_helper->setGrapher());

    for (size_t i = 0; i < 4; i++)
    {
        mgl->timeAxes[i] = _helper->timeAxes[i];
    }

    mgl->SetSize(s.x, s.y);
    mgl->Refresh();

    if (mgl->getNumFrames() > 1)
        mgl->AnimateAsynch();

    return true;
}


/////////////////////////////////////////////////
/// \brief Sets the value of the selected window
/// property.
///
/// \param _value const wxString&
/// \param varName const wxString&
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setPropValue(const wxString& _value, const wxString& varName)
{
    auto iter = m_varTable.find(varName);

    if (iter != m_varTable.end())
    {
        iter->second = _value;
        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Sets the text into the status bar.
///
/// \param _value wxString
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setStatusText(wxString _value)
{
    wxArrayString values = getChoices(_value);

    wxStatusBar* bar = GetStatusBar();

    if (bar->GetFieldsCount() != values.size())
        bar->SetFieldsCount(values.size());

    for (size_t i = 0; i < values.size(); i++)
    {
        bar->SetStatusText(values[i], i);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Menu event handler.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnMenuEvent(wxCommandEvent& event)
{
    handleEvent(event, "onclick");
}


/////////////////////////////////////////////////
/// \brief Button click event handler.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnClick(wxCommandEvent& event)
{
    handleEvent(event, "onclick");
}


/////////////////////////////////////////////////
/// \brief Generic onchange event handler.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnChange(wxCommandEvent& event)
{
    auto iter = m_windowItems.find(event.GetId());

    if (iter != m_windowItems.end() && iter->second.first != CustomWindow::SPINCTRL)
        handleEvent(event, "onchange");
}


/////////////////////////////////////////////////
/// \brief wxSpinCtrl event handler.
///
/// \param event wxSpinEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnSpin(wxSpinEvent& event)
{
    handleEvent(event, "onchange");
}


/////////////////////////////////////////////////
/// \brief wxNoteBook event handler.
///
/// \param event wxBookCtrlEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnTabChanged(wxBookCtrlEvent& event)
{
    handleEvent(event, "onchange");
}


/////////////////////////////////////////////////
/// \brief wxGrid event handler.
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnCellSelect(wxGridEvent& event)
{
    TableViewer* table = static_cast<TableViewer*>(m_windowItems[event.GetId()].second);

    if (table->isSilentSelection())
    {
        event.Skip();
        return;
    }

    handleEvent(event,
                "onclick",
                EventPosition(table->GetInternalRows(event.GetRow()), event.GetCol()));
}


/////////////////////////////////////////////////
/// \brief wxGrid event handler for activation.
///
/// \param event wxGridEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnCellActivate(wxGridEvent& event)
{
    TableViewer* table = static_cast<TableViewer*>(m_windowItems[event.GetId()].second);
    handleEvent(event,
                "onactivate",
                EventPosition(table->GetInternalRows(event.GetRow()), event.GetCol()));
}


/////////////////////////////////////////////////
/// \brief OnClose event handler.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnClose(wxCloseEvent& event)
{
    handleEvent(event, "onclose");
    m_windowRef.updateWindowInformation(NumeRe::STATUS_CANCEL, "");
    static_cast<NumeReWindow*>(m_parent)->unregisterWindow(this);
    Destroy();
}


/////////////////////////////////////////////////
/// \brief Mouse event handler.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnMouseLeftDown(wxMouseEvent& event)
{
    handleEvent(event, "onclick", EventPosition(event.GetX(), event.GetY()));
}


/////////////////////////////////////////////////
/// \brief Tree list control event handler.
///
/// \param event wxTreeListEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnTreeListEvent(wxTreeListEvent& event)
{
    wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(m_windowItems[event.GetId()].second);

    if (listCtrl->HasFlag(wxTL_CHECKBOX)
        && event.GetEventType() == wxEVT_TREELIST_SELECTION_CHANGED)
        return;

    handleEvent(event, "onclick", EventPosition(enumerateListItems(listCtrl, event.GetItem())));
}


/////////////////////////////////////////////////
/// \brief Tree list control event handler.
///
/// \param event wxTreeListEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnTreeListActivateEvent(wxTreeListEvent& event)
{
    wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(m_windowItems[event.GetId()].second);

    if (listCtrl->HasFlag(wxTL_CHECKBOX)
        && event.GetEventType() == wxEVT_TREELIST_SELECTION_CHANGED)
        return;

    handleEvent(event, "onactivate", EventPosition(enumerateListItems(listCtrl, event.GetItem())));
}


/////////////////////////////////////////////////
/// \brief Date and time picker event handler.
///
/// \param event wxDateEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnDateEvent(wxDateEvent& event)
{
    handleEvent(event, "onchange");
}


/////////////////////////////////////////////////
/// \brief On size event handler.
///
/// \param event wxSizeEvent&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::OnSizeEvent(wxSizeEvent& event)
{
    // Inform the window to refresh its contents
    // asynchronously
    CallAfter(&CustomWindow::Refresh);
    event.Skip();
}

