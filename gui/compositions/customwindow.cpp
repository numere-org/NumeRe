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
#include "../terminal/terminal.hpp"
#include "../../externals/tinyxml2/tinyxml2.h"
#include "../../kernel/kernel.hpp"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../../kernel/core/io/logger.hpp"
#include "../../kernel/core/ParserLib/muValueImpl.hpp"
#include "grouppanel.hpp"
#include "cellvalueshader.hpp"
#include <wx/tokenzr.h>
#include <wx/dataview.h>
#include <wx/statline.h>
#include "../wx.h"

#include <string>

#define MINCOLSIZE 70

/////////////////////////////////////////////////
/// \brief This static function converts colors
/// to mu::Array.
///
/// \param c const wxColor&
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array toArray(const wxColor& c)
{
    mu::Array ret;
    ret.push_back(mu::Numerical(c.Red()));
    ret.push_back(mu::Numerical(c.Green()));
    ret.push_back(mu::Numerical(c.Blue()));
    return ret;
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
    size_t nChildBraces = 0;

    for (int i = 0; i < (int)choices.length(); i++)
    {
        if (choices[i] == '"' && !nChildBraces && (!i || choices[i-1] != '\\' || (i > 1 && choices[i-2] == '\\')))
            nQuotes++;

        if (!(nQuotes % 2))
        {
            if (choices[i] == '{' && i && choices[i-1] == '\n')
                nChildBraces++;

            if (nChildBraces && choices[i] == '}')
                nChildBraces--;

            if (!nChildBraces && choices[i] == ',')
            {
                if (keepQuotationMarks)
                    choicesArray.Add(choices.substr(0, i));
                else
                    choicesArray.Add(removeQuotationMarks(choices.substr(0, i)));

                choices.erase(0, i+1);
                i = -1;
            }
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
/// \brief Convert a mu::Array into a
/// wxArrayString.
///
/// \param choices const mu::Array&
/// \return wxArrayString
///
/////////////////////////////////////////////////
static wxArrayString getChoices(const mu::Array& choices)
{
    wxArrayString arr;

    for (const auto& c : choices)
    {
        arr.Add(c.printVal());
    }

    return arr;
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

static void populateChild(wxTreeListCtrl* listCtrl, const mu::Array& values, wxTreeListItem parentItem);
static void populateArrChild(wxTreeListCtrl* listCtrl, const mu::Array& values, wxTreeListItem parentItem);
static void populateChild(wxTreeListCtrl* listCtrl, const mu::DictStruct& dict, wxTreeListItem parentItem);

/////////////////////////////////////////////////
/// \brief Populate the children of the current
/// wxTreeListItem.
///
/// \param listCtrl wxTreeListCtrl*
/// \param values const mu::Array&
/// \param parentItem wxTreeListItem
/// \return void
///
/////////////////////////////////////////////////
static void populateChild(wxTreeListCtrl* listCtrl, const mu::Array& values, wxTreeListItem parentItem)
{
    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    size_t nColumns = listCtrl->GetColumnCount();
    wxTreeListItem item = parentItem;

    for (size_t i = 0; i < values.size(); i++)
    {
        const mu::Value& elem = values.get(i);
        wxString sItem;

        if (elem.isArray())
            sItem = elem.getArray().get(0).printVal();
        else if (elem.isDictStruct())
        {
            while (nColumns < 2)
            {
                listCtrl->AppendColumn("");
                nColumns = listCtrl->GetColumnCount();
            }

            populateChild(listCtrl, elem.getDictStruct(), parentItem);
            continue;
        }
        else
            sItem = elem.printVal();

        size_t currCol = 1u;
        bool check = false;

        if (useCheckBoxes)
            check = nextItemValue(sItem) != "0";

        item = listCtrl->AppendItem(parentItem, nextItemValue(sItem));

        if (check && useCheckBoxes)
            listCtrl->CheckItem(item);

        while (sItem.length() && currCol < nColumns && !sItem.StartsWith("\n{"))
        {
            listCtrl->SetItemText(item, currCol, nextItemValue(sItem));
            currCol++;
        }

        if (sItem.StartsWith("\n{") && sItem.EndsWith("}"))
        {
            sItem.erase(0, 2);
            sItem.RemoveLast();
            wxArrayString childValues = getChoices(sItem);
            populateChild(listCtrl, childValues, item, true);
        }

        if (elem.isArray() && elem.getArray().size() == 2)
        {
            const mu::Value& val = elem.getArray().get(1);

            if (val.isArray())
                populateChild(listCtrl, val.getArray(), item);
            else
                populateChild(listCtrl, val, item);
        }
    }
}


/////////////////////////////////////////////////
/// \brief Populate a child tree based upon the
/// values from a mu::Array. The values are used
/// as-is, i.e. not as encoded string rows.
///
/// \param listCtrl wxTreeListCtrl*
/// \param values const mu::Array&
/// \param parentItem wxTreeListItem
/// \return void
///
/////////////////////////////////////////////////
static void populateArrChild(wxTreeListCtrl* listCtrl, const mu::Array& values, wxTreeListItem parentItem)
{
    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    size_t nColumns = listCtrl->GetColumnCount();
    wxTreeListItem item = parentItem;

    for (size_t i = 0; i < values.size(); i++)
    {
        const mu::Value& elem = values.get(i);
        wxString sItem;

        if (elem.isArray())
            sItem = elem.getArray().get(0).printVal();
        else if (elem.isDictStruct())
        {
            populateChild(listCtrl, elem.getDictStruct(), parentItem);
            continue;
        }
        else
            sItem = elem.printVal();

        //size_t currCol = 1u;
        bool check = false;

        item = listCtrl->AppendItem(parentItem, nextItemValue(sItem));

        if (check && useCheckBoxes)
            listCtrl->CheckItem(item);

        if (elem.isArray() && elem.getArray().size() == nColumns)
        {
            for (size_t j = 1; j < nColumns; j++)
            {
                if (elem.getArray().get(j).isArray())
                    populateArrChild(listCtrl, elem.getArray().get(j).getArray(), item);
                else if (elem.getArray().get(j).isDictStruct())
                    populateChild(listCtrl, elem.getArray().get(j).getDictStruct(), item);
                else
                    listCtrl->SetItemText(item, j, elem.getArray().get(j).printVal());
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Populate a child tree based upon the
/// values from a DictStruct instance. The
/// instance members are expected to be in XML
/// format.
///
/// \param listCtrl wxTreeListCtrl*
/// \param dict const mu::DictStruct&
/// \param parentItem wxTreeListItem
/// \return void
///
/////////////////////////////////////////////////
static void populateXmlChild(wxTreeListCtrl* listCtrl, const mu::DictStruct& dict, wxTreeListItem parentItem)
{
    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    size_t nColumns = listCtrl->GetColumnCount();
    wxTreeListItem item = parentItem;
    std::vector<std::string> fields = dict.getFields();

    for (size_t i = 0; i < fields.size(); i++)
    {
        if (fields[i] == "name")
            continue;

        const mu::BaseValue* elem = dict.read(fields[i]);

        if (fields[i] == "text" && nColumns > 1)
        {
            listCtrl->SetItemText(parentItem, 1, elem->printVal(0, 0));
            continue;
        }

        if (fields[i] == "nodes")
        {
            if (elem->m_type == mu::TYPE_ARRAY)
            {
                populateArrChild(listCtrl, static_cast<const mu::ArrValue*>(elem)->get(), parentItem);
                continue;
            }
            else if (elem->m_type == mu::TYPE_DICTSTRUCT)
            {
                populateChild(listCtrl, static_cast<const mu::DictStructValue*>(elem)->get(), parentItem);
                continue;
            }
        }

        wxString sItem;
        item = listCtrl->AppendItem(parentItem, fields[i]);

        if (elem->m_type == mu::TYPE_ARRAY)
        {
            populateArrChild(listCtrl, static_cast<const mu::ArrValue*>(elem)->get(), item);
            continue;
        }
        else if (elem->m_type == mu::TYPE_DICTSTRUCT)
        {
            populateChild(listCtrl, static_cast<const mu::DictStructValue*>(elem)->get(), item);
            continue;
        }
        else
            sItem = elem->printVal(0, 0);

        //size_t currCol = 1u;
        bool check = false;

        if (check && useCheckBoxes)
            listCtrl->CheckItem(item);
    }
}


/////////////////////////////////////////////////
/// \brief Populate a child tree based upon the
/// values from a DictStruct instance.
///
/// \param listCtrl wxTreeListCtrl*
/// \param dict const mu::DictStruct&
/// \param parentItem wxTreeListItem
/// \return void
///
/////////////////////////////////////////////////
static void populateChild(wxTreeListCtrl* listCtrl, const mu::DictStruct& dict, wxTreeListItem parentItem)
{
    wxTreeListItem item = parentItem;

    if (dict.hasXmlStructure() && dict.isField("name"))
    {
        item = listCtrl->AppendItem(parentItem, dict.read("name")->printVal(0, 0));
        populateXmlChild(listCtrl, dict, item);
        return;
    }

    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    //size_t nColumns = listCtrl->GetColumnCount();
    std::vector<std::string> fields = dict.getFields();

    for (size_t i = 0; i < fields.size(); i++)
    {
        const mu::BaseValue* elem = dict.read(fields[i]);
        wxString sItem;
        item = listCtrl->AppendItem(parentItem, fields[i]);

        if (elem->m_type == mu::TYPE_ARRAY)
        {
            populateArrChild(listCtrl, static_cast<const mu::ArrValue*>(elem)->get(), item);
            continue;
        }
        else if (elem->m_type == mu::TYPE_DICTSTRUCT)
        {
            populateChild(listCtrl, static_cast<const mu::DictStructValue*>(elem)->get(), item);
            continue;
        }
        else
            sItem = elem->printVal(0, 0);

        //size_t currCol = 1u;
        bool check = false;

        if (check && useCheckBoxes)
            listCtrl->CheckItem(item);
    }
}


/////////////////////////////////////////////////
/// \brief This static function converts the list
/// of values into items for the passed tree list
/// control and populates it.
///
/// \param listCtrl wxTreeListCtrl*
/// \param values const mu::Array&
/// \return void
///
/////////////////////////////////////////////////
static void populateTreeListCtrl(wxTreeListCtrl* listCtrl, const mu::Array& values)
{
#warning TODO (numere#1#10/31/24): Method might need some improvements and a corresponding -get value behavior
    if (!values.size())
    {
        listCtrl->DeleteAllItems();
        return;
    }

    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    size_t nColumns = 1;

    if (values.get(0).isDictStruct())
        nColumns = 2;
    else
    {
        std::string elem = values.unWrap().get(0).printVal();

        for (size_t pos = 0; pos < elem.length(); pos++)
        {
            if (elem[pos] == '\t')
                nColumns++;

            if (elem.substr(pos, 2) == "\n{")
                break;
        }

        if (useCheckBoxes)
            nColumns--;
    }


    listCtrl->DeleteAllItems();

    while (listCtrl->GetColumnCount() < nColumns)
        listCtrl->AppendColumn("");

    populateChild(listCtrl, values, listCtrl->GetRootItem());

    nColumns = listCtrl->GetColumnCount();
    int colSize = std::max(listCtrl->GetClientSize().x / (int)nColumns - 2, MINCOLSIZE);

    for (size_t i = 0; i < listCtrl->GetColumnCount(); i++)
    {
        listCtrl->SetColumnWidth(i, colSize);
    }
}


/////////////////////////////////////////////////
/// \brief Get the child values of the selected
/// parent item as a common mu::Array instance.
///
/// \param listCtrl wxTreeListCtrl*
/// \param parent wxTreeListItem
/// \param useCheckBox bool
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array getChildValues(wxTreeListCtrl* listCtrl, wxTreeListItem parent, bool useCheckBox)
{
    mu::Array currentLevel;

    wxTreeListItem next = listCtrl->GetFirstChild(parent);

    while (next.IsOk())
    {
        mu::Value item;

        if (useCheckBox)
            item = mu::Value(listCtrl->GetCheckedState(next) == wxCHK_CHECKED);
        else
        {
            item = mu::Value("");

            for (size_t j = 0; j < listCtrl->GetColumnCount(); j++)
            {
                item.getStr() += listCtrl->GetItemText(next, j).ToStdString();

                if (j+1 < listCtrl->GetColumnCount())
                    item.getStr() += "\t";
            }
        }


        if (listCtrl->GetFirstChild(next).IsOk())
        {
            mu::Array parentChild(item);
            parentChild.push_back(getChildValues(listCtrl, next, useCheckBox));
            currentLevel.push_back(parentChild);
        }
        else
            currentLevel.push_back(item);

        next = listCtrl->GetNextSibling(next);
    }

    return currentLevel;
}


/////////////////////////////////////////////////
/// \brief This static function returns the
/// current value of the passed tree list
/// control, whereas the value is either the
/// current selection or a vector of the current
/// checkbox states.
///
/// \param listCtrl wxTreeListCtrl*
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array getTreeListCtrlValue(wxTreeListCtrl* listCtrl)
{
    mu::Array values;
    bool useCheckBoxes = listCtrl->HasFlag(wxTL_CHECKBOX);
    return getChildValues(listCtrl, listCtrl->GetRootItem(), useCheckBoxes);
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
/// \brief Static helper function to convert a
/// string containing a list of integers into a
/// vector of ints.
///
/// \param strInt const std::string&
/// \return std::vector<int>
///
/////////////////////////////////////////////////
static std::vector<int> strIntToVector(const std::string& strInt)
{
    EndlessVector<std::string> strs = getAllArguments(strInt);
    std::vector<int> ints(strs.size());

    for (size_t i = 0; i < strs.size(); i++)
    {
        ints[i] = StrToInt(strs[i]);
    }

    return ints;
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
    cEVT_SET_OPTIONS(-1, CustomWindow::OnSetOptionsEvent)
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
        SetBackgroundColour(toWxColour(layoutGroup->Attribute("color")));

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
    {
        wxString statusText = layoutGroup->Attribute("statustext");
        setStatusText(getChoices(statusText));
    }

    // Evaluate the size information
    SetClientSize(wxSize(800,600));

    if (layoutGroup->Attribute("size"))
    {
        std::vector<int> s = strIntToVector(layoutGroup->Attribute("size"));

        if (s.size() >= 2)
        {
            if (s[0] == -1 && s[1] == -1)
                Maximize();
            else
                SetClientSize(wxSize(s[0], s[1]));
        }
    }

    Layout();
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
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    size.x = s[0];
                    size.y = s[1];
                }
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
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    size.x = s[0];
                    size.y = s[1];
                }
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
                    std::string name = varList[i].substr(0, varList[i].find('=')).ToStdString();
                    StripSpaces(name);
                    std::string value = varList[i].substr(varList[i].find('=')+1).ToStdString();

                    if (value.front() == '{' && value.back() == '}')
                        value = value.substr(1, value.length()-2);

                    mu::Array v;

                    while (value.length())
                    {
                        std::string cur = getNextArgument(value, true);

                        if (isConvertible(cur, CONVTYPE_VALUE))
                            v.push_back(mu::Value(StrToCmplx(cur)));
                        else if (isConvertible(cur, CONVTYPE_DATE_TIME))
                            v.push_back(mu::Value(StrToTime(cur)));
                        else if (isConvertible(cur, CONVTYPE_LOGICAL))
                            v.push_back(mu::Value(StrToLogical(cur)));
                        else
                            v.push_back(mu::Value(toInternalString(cur)));
                    }

                    m_varTable[name] = v;
                }
                else
                    m_varTable[varList[i].ToStdString()] = mu::Value();
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
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                    mgl->SetMinClientSize(wxSize(s[0], s[1]));
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
            TableViewer* table = new TableViewer(currParent, id, nullptr, nullptr, static_cast<NumeReWindow*>(m_parent),
                                                 wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_THEME);
            currSizer->Add(table, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN | wxFIXED_MINSIZE, 5);
            m_windowItems[id] = std::make_pair(CustomWindow::TABLE, table);
            table->SetTableReadOnly(state == READONLY);

            if (currentChild->Attribute("size"))
            {
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    NumeRe::Table data(s[0], s[1]);
                    table->SetData(data, "", "");
                    table->SetMinSize(table->calculateMinSize());
                }
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
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    colSize = std::max(colSize / (int)s[1] - 2, MINCOLSIZE);

                    for (size_t j = 0; j < (size_t)s[1]; j++)
                    {
                        if (labels.size() > j)
                            listCtrl->AppendColumn(labels[j], colSize);
                        else
                            listCtrl->AppendColumn("", colSize);
                    }

                    for (int i = 0; i < s[0]; i++)
                    {
                        listCtrl->AppendItem(listCtrl->GetRootItem(), "ITEM " + wxString::Format("%d", i+1));
                    }
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
            bool isGrid = false;
            int expand = 0;
            std::vector<int> _rowScl;
            std::vector<int> _colScl;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("type"))
            {
                style = currentChild->Attribute("type", "horizontal") ? wxHORIZONTAL : wxVERTICAL;
                isGrid = currentChild->Attribute("type", "flexgrid");
            }

            if (currentChild->Attribute("style"))
            {
                isCollapsible = currentChild->Attribute("style", "collapse");
                isNotebook = currentChild->Attribute("style", "tabs");
                isMenu = currentChild->Attribute("style", "menu");
            }

            if (currentChild->Attribute("expand"))
                expand = currentChild->Attribute("expand", "true") ? 1 : 0;

            if (currentChild->Attribute("rowscl"))
                _rowScl = strIntToVector(currentChild->Attribute("rowscl"));

            if (currentChild->Attribute("colscl"))
                _colScl = strIntToVector(currentChild->Attribute("colscl"));

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

                    if (isGrid)
                    {
                        wxGridBagSizer* gbSizer = _newPanel->createGridGroup(_newPanel->getMainSizer(), expand);
                        layoutGridChild(currentChild->FirstChildElement(), _newPanel, gbSizer, _newPanel, _rowScl, _colScl);
                    }
                    else
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

                    if (isGrid)
                    {
                        wxGridBagSizer* gbSizer = _groupPanel->createGridGroup(sizer, expand);
                        layoutGridChild(currentChild->FirstChildElement(), sizer->GetStaticBox(), gbSizer, _groupPanel, _rowScl, _colScl);
                    }
                    else
                        layoutChild(currentChild->FirstChildElement(), sizer->GetStaticBox(), sizer, _groupPanel);
                }
            }
            else
            {
                if (isGrid)
                {
                    wxGridBagSizer* gbSizer = _groupPanel->createGridGroup(currSizer, expand);
                    layoutGridChild(currentChild->FirstChildElement(), currParent, gbSizer, _groupPanel, _rowScl, _colScl);
                }
                else
                {
                    wxBoxSizer* sizer = _groupPanel->createGroup(style, currSizer, expand);
                    layoutChild(currentChild->FirstChildElement(), currParent, sizer, _groupPanel);
                }
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
/// recursively and creates the layout for the
/// current XMLElement's children.
///
/// \param currentChild const tinyxml2::XMLElement*
/// \param currParent wxWindow*
/// \param currSizer wxGridBagSizer*
/// \param _groupPanel GroupPanel*
/// \param rowScale const std::vector<int>&
/// \param colScale const std::vector<int>&
/// \return void
///
/////////////////////////////////////////////////
void CustomWindow::layoutGridChild(const tinyxml2::XMLElement* currentChild, wxWindow* currParent, wxGridBagSizer* currSizer, GroupPanel* _groupPanel, const std::vector<int>& rowScale, const std::vector<int>& colScale)
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

        wxGBPosition gbPos;
        wxGBSpan gbSpan;

        if (currentChild->Attribute("fgcell"))
        {
            std::vector<int> s = strIntToVector(currentChild->Attribute("fgcell"));

            if (s.size() >= 2)
            {
                gbPos.SetRow(s[0]-1);
                gbPos.SetCol(s[1]-1);
            }

            if (s.size() == 4)
            {
                gbSpan.SetRowspan(s[2]);
                gbSpan.SetColspan(s[3]);
            }
        }

        // Now check for the current XMLElement's
        // value (i.e. the XML-Tag name) and create
        // a corresponding control (if available)
        std::string sValue = currentChild->Value();

        if (sValue == "button")
        {
            int proportion = currentChild->IntAttribute("prop", 1);
            // Add a button
            wxButton* button = _groupPanel->CreateButton(currParent, SizerWrapper(currSizer, gbPos, gbSpan), removeQuotationMarks(text), id, alignment, proportion);
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
            wxCheckBox* checkbox = _groupPanel->CreateCheckBox(currParent, SizerWrapper(currSizer, gbPos, gbSpan), removeQuotationMarks(text), id, alignment);
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

            wxRadioBox* radiobox = _groupPanel->CreateRadioBox(currParent, SizerWrapper(currSizer, gbPos, gbSpan), label, getChoices(text), style, id, alignment);
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

            SpinBut* spinctrl = _groupPanel->CreateSpinControl(currParent, SizerWrapper(currSizer, gbPos, gbSpan), label, nMin, nMax, nValue, id, alignment);
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

            wxSlider* slider = _groupPanel->CreateSlider(currParent, SizerWrapper(currSizer, gbPos, gbSpan), nMin, nMax, nValue, style, id, alignment, proportion);
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

            wxGauge* gauge = _groupPanel->CreateGauge(currParent, SizerWrapper(currSizer, gbPos, gbSpan), style, id, alignment, proportion);
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

            wxChoice* choice = _groupPanel->CreateChoices(currParent, SizerWrapper(currSizer, gbPos, gbSpan), choices, id, alignment, proportion);
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

            wxComboBox* combo = _groupPanel->CreateComboBox(currParent, SizerWrapper(currSizer, gbPos, gbSpan), choices, id, alignment, proportion);
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
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    size.x = s[0];
                    size.y = s[1];
                }
            }

            if (currentChild->Attribute("type", "multiline"))
                style = wxTE_MULTILINE | wxTE_BESTWRAP;

            if (currentChild->Attribute("type", "markup"))
                style = wxTE_MULTILINE | wxTE_BESTWRAP | wxTE_RICH2 | wxTE_AUTO_URL;

            wxString label;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            TextField* textctrl = _groupPanel->CreateTextInput(currParent, SizerWrapper(currSizer, gbPos, gbSpan), label, "", style, id, size, alignment, proportion);
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
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    size.x = s[0];
                    size.y = s[1];
                }
            }

            wxString label;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            TextField* textctrl = _groupPanel->CreateLamp(currParent, SizerWrapper(currSizer, gbPos, gbSpan), label, wxEmptyString, 0, id, size, alignment, proportion);
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

            DateTimePicker* dtPicker = _groupPanel->CreateDateTimePicker(currParent, SizerWrapper(currSizer, gbPos, gbSpan), wxDefaultDateTime, style, id, alignment, proportion);

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
            wxStaticText* statictext = _groupPanel->AddStaticText(currParent, SizerWrapper(currSizer, gbPos, gbSpan), removeQuotationMarks(text), id, alignment);
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
                    std::string name = varList[i].substr(0, varList[i].find('=')).ToStdString();
                    StripSpaces(name);
                    std::string value = varList[i].substr(varList[i].find('=')+1).ToStdString();

                    if (value.front() == '{' && value.back() == '}')
                        value = value.substr(1, value.length()-2);

                    mu::Array v;

                    while (value.length())
                    {
                        std::string cur = getNextArgument(value, true);

                        if (isConvertible(cur, CONVTYPE_VALUE))
                            v.push_back(mu::Value(StrToCmplx(cur)));
                        else if (isConvertible(cur, CONVTYPE_DATE_TIME))
                            v.push_back(mu::Value(StrToTime(cur)));
                        else if (isConvertible(cur, CONVTYPE_LOGICAL))
                            v.push_back(mu::Value(StrToLogical(cur)));
                        else
                            v.push_back(mu::Value(toInternalString(cur)));
                    }

                    m_varTable[name] = v;
                }
                else
                    m_varTable[varList[i].ToStdString()] = mu::Value();
            }
        }
        else if (sValue == "bitmap")
        {
            // Add an image
            wxStaticBitmap* bitmap = _groupPanel->CreateBitmap(currParent, SizerWrapper(currSizer, gbPos, gbSpan), removeQuotationMarks(text), id, alignment);
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
                    _groupPanel->AddSpacer(space, SizerWrapper(currSizer, gbPos, gbSpan));
                    created = true;
                }
                else
                    style = currentChild->Attribute("type", "vertical") ? wxVERTICAL : wxHORIZONTAL;
            }

            if (!created)
            {
                // Add a static line (i.e. the default separator)
                wxStaticLine* line = new wxStaticLine(currParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style);
                currSizer->Add(line, gbPos, gbSpan, wxEXPAND | wxALL, 5);
            }
        }
        else if (sValue == "grapher")
        {
            // Add a grapher object
            wxMGL* mgl = new wxMGL(currParent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME, true);
            m_windowItems[id] = std::make_pair(CustomWindow::GRAPHER, mgl);

            if (currentChild->Attribute("size"))
            {
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                    mgl->SetMinClientSize(wxSize(s[0],s[1]));
            }
            else
                mgl->SetMinClientSize(wxSize(600,400));

            if (!currSizer->CheckForIntersection(gbPos, gbSpan))
                currSizer->Add(mgl, gbPos, gbSpan, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5);
            else
                wxMessageBox("An element of type \"grapher\" does not fit into its assigned group. The target cell(s) are already occupied.", "Group layout warning", wxOK | wxCENTER | wxICON_WARNING, this);

            if (currentChild->Attribute("onclick"))
                m_eventTable[id].onclick = currentChild->Attribute("onclick");
        }
        else if (sValue == "tablegrid")
        {
            // Add a table grid
            TableViewer* table = new TableViewer(currParent, id, nullptr, nullptr, static_cast<NumeReWindow*>(m_parent),
                                                 wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_THEME);

            if (!currSizer->CheckForIntersection(gbPos, gbSpan))
                currSizer->Add(table, gbPos, gbSpan, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN | wxFIXED_MINSIZE, 5);
            else
                wxMessageBox("An element of type \"tablegrid\" does not fit into its assigned group. The target cell(s) are already occupied.", "Group layout warning", wxOK | wxCENTER | wxICON_WARNING, this);

            m_windowItems[id] = std::make_pair(CustomWindow::TABLE, table);
            table->SetTableReadOnly(state == READONLY);

            if (currentChild->Attribute("size"))
            {
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    NumeRe::Table data(s[0], s[1]);
                    table->SetData(data, "", "");
                    table->SetMinSize(table->calculateMinSize());
                }
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

            wxTreeListCtrl* listCtrl = _groupPanel->CreateTreeListCtrl(currParent, SizerWrapper(currSizer, gbPos, gbSpan), style, wxDefaultSize, id, alignment, proportion);
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
                std::vector<int> s = strIntToVector(currentChild->Attribute("size"));

                if (s.size() >= 2)
                {
                    colSize = std::max(colSize / (int)s[1] - 2, MINCOLSIZE);

                    for (size_t j = 0; j < (size_t)s[1]; j++)
                    {
                        if (labels.size() > j)
                            listCtrl->AppendColumn(labels[j], colSize);
                        else
                            listCtrl->AppendColumn("", colSize);
                    }

                    for (int i = 0; i < s[0]; i++)
                    {
                        listCtrl->AppendItem(listCtrl->GetRootItem(), "ITEM " + wxString::Format("%d", i+1));
                    }
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
            bool isGrid = false;
            int expand = 0;
            std::vector<int> _rowScl;
            std::vector<int> _colScl;

            if (currentChild->Attribute("label"))
                label = currentChild->Attribute("label");

            if (currentChild->Attribute("type"))
            {
                style = currentChild->Attribute("type", "horizontal") ? wxHORIZONTAL : wxVERTICAL;
                isGrid = currentChild->Attribute("type", "flexgrid");
            }

            if (currentChild->Attribute("style"))
            {
                isCollapsible = currentChild->Attribute("style", "collapse");
                isNotebook = currentChild->Attribute("style", "tabs");
                isMenu = currentChild->Attribute("style", "menu");
            }

            if (currentChild->Attribute("expand"))
                expand = currentChild->Attribute("expand", "true") ? 1 : 0;

            if (currentChild->Attribute("rowscl"))
                _rowScl = strIntToVector(currentChild->Attribute("rowscl"));

            if (currentChild->Attribute("colscl"))
                _colScl = strIntToVector(currentChild->Attribute("colscl"));

            // A collapsible group is currently very buggy (if used
            // with the current GroupPanel).
#warning TODO (numere#6#08/15/21): Fix the collapsible group
            if (label.length())
            {
                if (isCollapsible)
                {
                    wxCollapsiblePane* pane = _groupPanel->createCollapsibleGroup(label, currParent, SizerWrapper(currSizer, gbPos, gbSpan));
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
                        currSizer->Add(noteBook, gbPos, gbSpan, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN | wxALL, 5);

                        if (currentChild->Attribute("onchange"))
                            m_eventTable[id].onchange = currentChild->Attribute("onchange");
                    }

                    GroupPanel* _newPanel = new GroupPanel(noteBook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, style == wxHORIZONTAL ? false : true);

                    if (isGrid)
                    {
                        wxGridBagSizer* gbSizer = _newPanel->createGridGroup(_newPanel->getMainSizer(), expand);
                        layoutGridChild(currentChild->FirstChildElement(), _newPanel, gbSizer, _newPanel, _rowScl, _colScl);
                    }
                    else
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
                    wxStaticBoxSizer* sizer = _groupPanel->createGroup(label, style, currParent, SizerWrapper(currSizer, gbPos, gbSpan), expand);

                    if (isGrid)
                    {
                        wxGridBagSizer* gbSizer = _groupPanel->createGridGroup(sizer, expand);
                        layoutGridChild(currentChild->FirstChildElement(), sizer->GetStaticBox(), gbSizer, _groupPanel, _rowScl, _colScl);
                    }
                    else
                        layoutChild(currentChild->FirstChildElement(), sizer->GetStaticBox(), sizer, _groupPanel);
                }
            }
            else
            {
                if (isGrid)
                {
                    wxGridBagSizer* gbSizer = _groupPanel->createGridGroup(SizerWrapper(currSizer, gbPos, gbSpan), expand);
                    layoutGridChild(currentChild->FirstChildElement(), currParent, gbSizer, _groupPanel, _rowScl, _colScl);
                }
                else
                {
                    wxBoxSizer* sizer = _groupPanel->createGroup(style, SizerWrapper(currSizer, gbPos, gbSpan), expand);
                    layoutChild(currentChild->FirstChildElement(), currParent, sizer, _groupPanel);
                }
            }
        }

        // Find the next sibling from the current
        // child element
        if (currentChild->NextSibling())
            currentChild = currentChild->NextSibling()->ToElement();
        else
            break;
    }

    // Apply the flex effect after all embedded elements have been created
    for (int i = 0; i < currSizer->GetRows(); i++)
    {
        if (rowScale.size() > i)
            currSizer->AddGrowableRow(i, rowScale[i]);
        else
            currSizer->AddGrowableRow(i, 1);
    }

    for (int j = 0; j < currSizer->GetCols(); j++)
    {
        if (colScale.size() > j)
            currSizer->AddGrowableCol(j, colScale[j]);
        else
            currSizer->AddGrowableCol(j, 1);
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
    // Ensure that any table editor is closed
    for (auto& item : m_windowItems)
    {
        if (item.second.first == CustomWindow::TABLE)
            static_cast<TableViewer*>(item.second.second)->finalize();
    }

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
                params.value = mu::Value();

            // Create the corresponding key-value-list
            // syntax
            if (event.GetEventType() == wxEVT_GRID_SELECT_CELL
                || event.GetEventType() == wxEVT_GRID_CELL_LEFT_DCLICK)
            {
                TableViewer* table = static_cast<TableViewer*>(m_windowItems[event.GetId()].second);
                params.value = table->get(static_cast<wxGridEvent&>(event).GetRow(),
                                          static_cast<wxGridEvent&>(event).GetCol());
            }

            static NumeReKernel& _kernel = mainWindow->getTerminal()->getKernel();
            mu::Variable* kvl_event = _kernel.getParser().CreateVar("_~event", mu::TYPE_CLUSTER);
            kvl_event->clear();
            kvl_event->push_back("event");
            kvl_event->push_back(sEventType.ToStdString());
            kvl_event->push_back("object");
            kvl_event->push_back(params.type);
            kvl_event->push_back("value");

            if (params.value.size() > 1)
                kvl_event->push_back(params.value);
            else
                kvl_event->push_back(params.value.front());

            kvl_event->push_back("state");
            kvl_event->push_back(params.state);

            wxString p = pos.serialize();

            if (p.length())
            {
                kvl_event->push_back("position");

                if (pos.y > -2)
                {
                    mu::Array posVal(mu::Value(pos.x+1));
                    posVal.push_back(mu::Value(pos.y+1));
                    kvl_event->push_back(posVal);
                }
                else
                    kvl_event->push_back(mu::Value(pos.x+1));
            }

            // Call the procedure with the following syntax:
            // $PROCEDURE(winid, objectid, event{})
            mainWindow->pass_command(sEventHandler + "(" + toString(m_windowRef.getId()) + ","
                                                         + toString(event.GetId()) + ",_~event{})", true);
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
                val.val = params.value;
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
                val.val = params.value;
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
    params.value.push_back(GetClientSize().x);
    params.value.push_back(GetClientSize().y);
    params.state = "running";
    params.color = toArray(GetBackgroundColour());
    params.label = mu::Value(GetTitle().ToStdString());

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
            params.value = mu::Value(static_cast<wxButton*>(object.second)->GetLabel().ToStdString());
            params.label = params.value;
            params.color = toArray(static_cast<wxButton*>(object.second)->GetForegroundColour());

            break;
        case CustomWindow::CHECKBOX:
            params.type = "checkbox";
            params.value = mu::Value(static_cast<wxCheckBox*>(object.second)->IsChecked());
            params.label = mu::Value(static_cast<wxCheckBox*>(object.second)->GetLabel().ToStdString());
            params.color = toArray(static_cast<wxCheckBox*>(object.second)->GetBackgroundColour());

            break;
        case CustomWindow::TEXT:
            params.type = "statictext";
            params.value = mu::Value(static_cast<wxStaticText*>(object.second)->GetLabel().ToStdString());
            params.label = params.value;
            params.color = toArray(static_cast<wxStaticText*>(object.second)->GetForegroundColour());

            break;
        case CustomWindow::TEXTCTRL:
            params.type = "textfield";
            params.value = mu::Value(static_cast<TextField*>(object.second)->GetValue().ToStdString());
            params.label = mu::Value(static_cast<TextField*>(object.second)->GetLabel().ToStdString());
            params.color = toArray(static_cast<TextField*>(object.second)->GetBackgroundColour());

            if (!static_cast<wxTextCtrl*>(object.second)->IsEditable())
                params.state = "readonly";

            break;
        case CustomWindow::LAMP:
            params.type = "lamp";
            params.label = mu::Value(static_cast<TextField*>(object.second)->GetLabel().ToStdString());
            params.color = toArray(static_cast<TextField*>(object.second)->GetBackgroundColour());
            params.value = mu::Value(lampStatesFromColor(static_cast<TextField*>(object.second)->GetBackgroundColour()).ToStdString());

            break;
        case CustomWindow::DATETIMEPICKER:
            params.type = "datetimepicker";
            params.value = mu::Value(StrToTime(static_cast<DateTimePicker*>(object.second)->GetValue().ToStdString()));

            break;
        case CustomWindow::RADIOGROUP:
        {
            params.type = "radio";
            wxRadioBox* box = static_cast<wxRadioBox*>(object.second);
            params.value = mu::Value(box->GetString(box->GetSelection()).ToStdString());
            params.label = mu::Value(box->GetLabel().ToStdString());
            params.color = toArray(static_cast<wxRadioBox*>(object.second)->GetBackgroundColour());

            break;
        }
        case CustomWindow::DROPDOWN:
        {
            params.type = "dropdown";
            wxChoice* choices = static_cast<wxChoice*>(object.second);
            params.value = mu::Value(choices->GetString(choices->GetSelection()).ToStdString());

            for (size_t i = 0; i < choices->GetCount(); i++)
            {
                params.label.push_back(choices->GetString(i).ToStdString());
            }

            params.color = toArray(static_cast<wxChoice*>(object.second)->GetBackgroundColour());

            break;
        }
        case CustomWindow::COMBOBOX:
        {
            params.type = "combobox";
            wxComboBox* combo = static_cast<wxComboBox*>(object.second);

            if (combo->GetSelection() != wxNOT_FOUND)
                params.value = mu::Value(combo->GetString(combo->GetSelection()).ToStdString());
            else
                params.value = mu::Value(combo->GetValue().ToStdString());

            for (size_t i = 0; i < combo->GetCount(); i++)
            {
                params.label.push_back(combo->GetString(i).ToStdString());
            }

            params.color = toArray(static_cast<wxComboBox*>(object.second)->GetBackgroundColour());

            break;
        }
        case CustomWindow::GAUGE:
            params.type = "gauge";
            params.value = mu::Value(static_cast<wxGauge*>(object.second)->GetValue());
            params.label = params.value;
            params.color = toArray(static_cast<wxGauge*>(object.second)->GetBackgroundColour());

            break;
        case CustomWindow::SPINCTRL:
            params.type = "spinbut";
            params.value = mu::Value(static_cast<SpinBut*>(object.second)->GetValue());
            params.label = mu::Value(static_cast<SpinBut*>(object.second)->GetLabel().ToStdString());
            params.color = toArray(static_cast<SpinBut*>(object.second)->GetBackgroundColour());

            break;
        case CustomWindow::SLIDER:
            params.type = "slider";
            params.value = mu::Value(static_cast<wxSlider*>(object.second)->GetValue());

            break;
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            params.table = table->GetDataCopy();
            params.value = table->getSelectedValues();
            params.type = "tablegrid";

            if (!table->IsEditable())
                params.state = "readonly";

            break;
        }
        case CustomWindow::GRAPHER:
        {
            wxMGL* grapher = static_cast<wxMGL*>(object.second);
            mglPoint pos = grapher->getClickedCoords();
            params.value.push_back(pos.x);
            params.value.push_back(pos.y);
            params.type = "grapher";

            break;
        }
        case CustomWindow::TREELIST:
        {
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);

            for (size_t i = 0; i < listCtrl->GetColumnCount(); i++)
            {
                params.label.push_back(listCtrl->GetDataView()->GetColumn(i)->GetTitle().ToStdString());
            }

            params.value = getTreeListCtrlValue(listCtrl);
            params.type = "treelist";

            break;
        }
        case CustomWindow::NOTEBOOK:
        {
            wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);
            params.type = "tabs";

            for (size_t page = 0; page < noteBook->GetPageCount(); page++)
            {
                params.label.push_back(noteBook->GetPageText(page).ToStdString());
            }

            params.value = mu::Value(noteBook->GetPageText(noteBook->GetSelection()).ToStdString());

            break;
        }
        case CustomWindow::MENUITEM:
        {
            wxMenuItem* item = static_cast<wxMenuItem*>(object.second);
            params.type = "menuitem";
            params.label = mu::Value(item->GetItemLabel().ToStdString());

            if (item->IsCheckable())
                params.value = mu::Value(item->IsChecked());
            else
                params.value = mu::Value(item->GetLabelText(item->GetItemLabel()).ToStdString());

            params.color = toArray(item->GetTextColour());
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
        value.val = params.value;
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
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array CustomWindow::getItemLabel(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
        return params.label;

    return mu::Array();
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
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array CustomWindow::getItemColor(int windowItemID) const
{
    WindowItemParams params;

    if (getItemParameters(windowItemID, params))
        return params.color;

    return mu::Array();
}


/////////////////////////////////////////////////
/// \brief Get the current selection of the
/// selected item.
///
/// \param windowItemID int
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array CustomWindow::getItemSelection(int windowItemID) const
{
    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return mu::Array();

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::TEXTCTRL:
        {
            TextField* field = static_cast<TextField*>(object.second);

            if (field->HasSelection())
            {
                mu::Array ret;
                long int from;
                long int to;
                field->GetSelection(&from, &to);
                ret.push_back(mu::Value((int32_t)(from+1)));
                ret.push_back(mu::Value((int32_t)(to-from)));
                return ret;
            }

            return mu::Value((uint64_t)field->GetInsertionPoint()+1);
        }
        case CustomWindow::DROPDOWN:
        {
            wxChoice* choices = static_cast<wxChoice*>(object.second);
            return mu::Value(choices->GetSelection()+1);
        }
        case CustomWindow::COMBOBOX:
        {
            wxComboBox* combo = static_cast<wxComboBox*>(object.second);
            return mu::Value(combo->GetSelection()+1);
        }
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            mu::Array ret;
            ret.push_back(mu::Value(table->GetInternalRows(table->GetGridCursorRow())+1));
            ret.push_back(mu::Value(table->GetGridCursorCol()+1));
            return ret;
        }
        case CustomWindow::TREELIST:
        {
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);
            int selection = enumerateListItems(listCtrl, listCtrl->GetSelection());
            return mu::Value(selection+1);
        }
        case CustomWindow::NOTEBOOK:
        {
            wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);
            int selection = noteBook->GetSelection();
            return mu::Value(selection+1);
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

    return mu::Array();
}


/////////////////////////////////////////////////
/// \brief Returns the value of the selected
/// window property.
///
/// \param varName const std::string&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array CustomWindow::getPropValue(const std::string& varName) const
{
    auto iter = m_varTable.find(varName);

    if (iter != m_varTable.end())
        return iter->second;

    return mu::Array();
}


/////////////////////////////////////////////////
/// \brief Returns a list of all available window
/// properties.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array CustomWindow::getProperties() const
{
    mu::Array props;

    for (auto iter : m_varTable)
    {
        props.push_back(iter.first);
    }

    return props;
}


/////////////////////////////////////////////////
/// \brief Returns the contents of the status bar
/// if any available.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array CustomWindow::getStatusText() const
{
    const wxStatusBar* bar = GetStatusBar();
    int numFields = bar->GetFieldsCount();
    mu::Array statusText;

    for (int i = 0; i < numFields; i++)
    {
        statusText.push_back(bar->GetStatusText(i).ToStdString());
    }

    return statusText;
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
/// \param _label const mu::Array&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::pushItemLabel(const mu::Array& _label, int windowItemID)
{
    GetEventHandler()->QueueEvent(new SetLabelEvent(SET_WINDOW_LABEL, GetId(), windowItemID, _label));
    return true;
}


/////////////////////////////////////////////////
/// \brief Push an item options change to the
/// internal event handler.
///
/// \param _opts const mu::Array&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::pushItemOptions(const mu::Array& _opts, int windowItemID)
{
    GetEventHandler()->QueueEvent(new SetOptionsEvent(SET_WINDOW_OPTIONS, GetId(), windowItemID, _opts));
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
    try
    {
        if (windowItemID == -1)
        {
            long int x = -1, y = -1;

            if (_value.val.size())
                x = _value.val.get(0).getNum().asI64();

            if (_value.val.size() > 1)
                y = _value.val.get(1).getNum().asI64();

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
                static_cast<wxButton*>(object.second)->SetLabel(_value.val.get(0).printVal());
                break;
            case CustomWindow::CHECKBOX:
                static_cast<wxCheckBox*>(object.second)->SetValue((bool)_value.val.get(0));
                break;
            case CustomWindow::TEXT:
                static_cast<wxStaticText*>(object.second)->SetLabel(_value.val.get(0).printVal());
                break;
            case CustomWindow::TEXTCTRL:
            {
                TextField* field = static_cast<TextField*>(object.second);
                field->SetMarkupText(_value.val.get(0).printVal());
                field->ShowPosition(0);

                break;
            }
            case CustomWindow::LAMP:
            {
                wxColour color = colorFromLampStates(_value.val.get(0).printVal());
                static_cast<wxTextCtrl*>(object.second)->SetBackgroundColour(color);
                Refresh();
                break;
            }
            case CustomWindow::DATETIMEPICKER:
            {
                static_cast<DateTimePicker*>(object.second)->ChangeValue(_value.val.get(0).printVal());
                break;
            }
            case CustomWindow::GAUGE:
            {
                long int nVal = _value.val.get(0).getNum().asI64();

                if (nVal == -1)
                    static_cast<wxGauge*>(object.second)->Pulse();
                else
                    static_cast<wxGauge*>(object.second)->SetValue(nVal);

                break;
            }
            case CustomWindow::SPINCTRL:
            {
                long int nVal = _value.val.get(0).getNum().asI64();
                static_cast<SpinBut*>(object.second)->SetValue(nVal);
                break;
            }
            case CustomWindow::SLIDER:
            {
                long int nVal = _value.val.get(0).getNum().asI64();
                static_cast<wxSlider*>(object.second)->SetValue(nVal);
                break;
            }
            case CustomWindow::RADIOGROUP:
            {
                wxRadioBox* box = static_cast<wxRadioBox*>(object.second);
                int sel = box->FindString(_value.val.get(0).printVal(), true);

                if (sel != wxNOT_FOUND)
                    box->SetSelection(sel);

                break;
            }
            case CustomWindow::DROPDOWN:
            {
                wxChoice* choices = static_cast<wxChoice*>(object.second);
                int sel = choices->FindString(_value.val.get(0).printVal(), true);

                if (sel != wxNOT_FOUND)
                    choices->SetSelection(sel);

                break;
            }
            case CustomWindow::COMBOBOX:
            {
                wxComboBox* combo = static_cast<wxComboBox*>(object.second);
                int sel = combo->FindString(_value.val.get(0).printVal(), true);

                if (sel != wxNOT_FOUND)
                    combo->SetSelection(sel);
                else
                    combo->ChangeValue(_value.val.get(0).printVal());

                break;
            }
            case CustomWindow::IMAGE:
            {
                wxStaticBitmap* bitmap = static_cast<wxStaticBitmap*>(object.second);
                bitmap->SetBitmap(wxBitmap(_value.val.get(0).printVal(), wxBITMAP_TYPE_ANY));
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
                populateTreeListCtrl(listCtrl, _value.val);
                break;
            }
            case CustomWindow::NOTEBOOK:
            {
                wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);
                noteBook->SetPageText(noteBook->GetSelection(), _value.val.get(0).printVal());

                break;
            }
            case CustomWindow::MENUITEM:
            {
                wxMenuItem* item = static_cast<wxMenuItem*>(object.second);

                if (item->IsCheckable())
                    item->Check((bool)_value.val.get(0));
                else
                    item->SetItemLabel(_value.val.get(0).printVal());

                break;
            }
            case CustomWindow::GRAPHER:
                break;
        }
    }
    catch (...)
    {
        wxMessageBox(_guilang.get("GUI_CUSTOMWINDOW_CATCH", "window ID -set value=VAL item=ID"),
                     _guilang.get("GUI_CUSTOMWINDOW_CATCH_HEAD"), wxCENTER | wxICON_ERROR | wxOK, this);
        return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Change the label of the selected item.
///
/// \param _label const mu::Array&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemLabel(const mu::Array& _label, int windowItemID)
{
    if (windowItemID == -1)
    {
        SetTitle(_label.get(0).printVal());
        return true;
    }

    auto iter = m_windowItems.find(windowItemID);

    if (iter == m_windowItems.end())
        return false;

    std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

    switch (object.first)
    {
        case CustomWindow::BUTTON:
            static_cast<wxButton*>(object.second)->SetLabel(_label.get(0).printVal());
            break;
        case CustomWindow::CHECKBOX:
            static_cast<wxCheckBox*>(object.second)->SetLabel(_label.get(0).printVal());
            break;
        case CustomWindow::TEXT:
            static_cast<wxStaticText*>(object.second)->SetLabel(_label.get(0).printVal());
            break;
        case CustomWindow::TEXTCTRL:
        case CustomWindow::LAMP:
            static_cast<TextField*>(object.second)->SetLabel(_label.get(0).printVal());
            break;
        case CustomWindow::SPINCTRL:
            static_cast<SpinBut*>(object.second)->SetLabel(_label.get(0).printVal());
            break;
        case CustomWindow::RADIOGROUP:
            static_cast<wxRadioBox*>(object.second)->SetLabel(_label.get(0).printVal());
            break;
        case CustomWindow::TREELIST:
        {
            wxTreeListCtrl* listCtrl = static_cast<wxTreeListCtrl*>(object.second);

            for (size_t i = 0; i < _label.size(); i++)
            {
                if (listCtrl->GetColumnCount() <= i)
                    listCtrl->AppendColumn(_label.get(i).printVal());
                else
                    listCtrl->GetDataView()->GetColumn(i)->SetTitle(_label.get(i).printVal());
            }

            break;
        }
        case CustomWindow::NOTEBOOK:
        {
            wxNotebook* noteBook = static_cast<wxNotebook*>(object.second);

            for (size_t i = 0; i < _label.size(); i++)
            {
                if (i >= (size_t)noteBook->GetPageCount())
                    break;

                noteBook->SetPageText(i, _label.get(i).printVal());
            }

            break;
        }
        case CustomWindow::MENUITEM:
            static_cast<wxMenuItem*>(object.second)->SetItemLabel(_label.get(0).printVal());
            break;
        case CustomWindow::DROPDOWN:
        {
            static_cast<wxChoice*>(object.second)->Set(getChoices(_label));
            break;
        }
        case CustomWindow::COMBOBOX:
        {
            static_cast<wxComboBox*>(object.second)->Set(getChoices(_label));
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
/// \param _color const mu::Array&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemColor(const mu::Array& _color, int windowItemID)
{
    try
    {
        wxColour color(_color.get(0).getNum().asUI64(),
                       _color.get(1).getNum().asUI64(),
                       _color.get(2).getNum().asUI64());

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
            case CustomWindow::NOTEBOOK:
                break;
        }
    }
    catch (...)
    {
        wxMessageBox(_guilang.get("GUI_CUSTOMWINDOW_CATCH", "window ID -set color={R,G,B} item=ID"),
                     _guilang.get("GUI_CUSTOMWINDOW_CATCH_HEAD"), wxCENTER | wxICON_ERROR | wxOK, this);
        return false;
    }

    Refresh();

    return true;
}


/////////////////////////////////////////////////
/// \brief Set additional options for the
/// selected item.
///
/// \param _options const mu::Array&
/// \param windowItemID int
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setItemOptions(const mu::Array& _options, int windowItemID)
{
    try
    {
        if (windowItemID == -1)
            return false;

        auto iter = m_windowItems.find(windowItemID);

        if (iter == m_windowItems.end())
            return false;

        std::pair<CustomWindow::WindowItemType, wxObject*> object = iter->second;

        switch (object.first)
        {
            case CustomWindow::TABLE:
            {
                TableViewer* table = static_cast<TableViewer*>(object.second);

                bool fitSize = false;

                if (_options.size() == 1 && _options.get(0).isDictStruct())
                {
                    const mu::DictStruct& dict = _options.get(0).getDictStruct();

                    if (dict.isField("min-cols") && dict.read("min-cols")->m_type == mu::TYPE_NUMERICAL)
                    {
                        int64_t c = table->GetNumberCols();
                        int64_t t = static_cast<const mu::NumValue*>(dict.read("min-cols"))->get().asI64();

                        if (c < t+1)
                            table->AppendCols(t-c+1);
                    }

                    if (dict.isField("min-rows") && dict.read("min-rows")->m_type == mu::TYPE_NUMERICAL)
                    {
                        int64_t r = table->GetNumberRows();
                        int64_t t = static_cast<const mu::NumValue*>(dict.read("min-rows"))->get().asI64();

                        if (r < t+1)
                            table->AppendRows(t-r+1);
                    }

                    if (dict.isField("use-qmarks"))
                        table->enableQuotationMarks((bool)*dict.read("use-qmarks"));

                    if (dict.isField("fitsize"))
                        fitSize = (bool)*dict.read("fitsize");

                    if (dict.isField("fitsize-cols"))
                    {
                        wxArrayInt cols;
                        int64_t currentCols = table->GetNumberCols()-1;
                        const mu::BaseValue* val = dict.read("fitsize-cols");

                        if (val->m_type == mu::TYPE_ARRAY)
                        {
                            const mu::Array& selectedCols = static_cast<const mu::ArrValue*>(val)->get();

                            for (size_t k = 0; k < selectedCols.size(); k++)
                            {
                                int64_t selected = selectedCols.get(k).getNum().asI64();

                                if (currentCols < selected || selected < 1)
                                    continue;

                                cols.Add(selected-1);
                            }
                        }
                        else if (val->m_type == mu::TYPE_NUMERICAL)
                        {
                            int64_t c = static_cast<const mu::NumValue*>(val)->get().asI64();

                            if (c > 0 && c <= currentCols)
                                cols.Add(c-1);
                        }

                        if (!cols.size())
                            return false;

                        for (size_t i = 0; i < cols.size(); i++)
                        {
                            table->AutoSizeColumn(i);
                        }
                    }

                    if (dict.isField("cond-format"))
                    {
                        mu::Array formatting = mu::Value(dict.read("cond-format")->clone());
                        int64_t currentCols = table->GetNumberCols()-1;

                        for (size_t j = 0; j < formatting.size(); j++)
                        {
                            wxArrayInt cols;

                            if (!formatting.get(j).isDictStruct())
                                return false;

                            const mu::DictStruct& condFormat = formatting.get(j).getDictStruct();

                            if (!condFormat.isField("cols")
                                || !condFormat.isField("format")
                                || condFormat.read("format")->m_type != mu::TYPE_ARRAY)
                                return false;

                            if (condFormat.read("cols")->m_type == mu::TYPE_ARRAY)
                            {
                                const mu::Array& selectedCols = static_cast<const mu::ArrValue*>(condFormat.read("cols"))->get();

                                for (size_t k = 0; k < selectedCols.size(); k++)
                                {
                                    int64_t selected = selectedCols.get(k).getNum().asI64();

                                    if (currentCols < selected || selected < 1)
                                        continue;

                                    cols.Add(selected-1);
                                }
                            }
                            else if (condFormat.read("cols")->m_type == mu::TYPE_NUMERICAL)
                            {
                                int64_t c = static_cast<const mu::NumValue*>(condFormat.read("cols"))->get().asI64();

                                if (c > 0 && c <= currentCols)
                                    cols.Add(c-1);
                            }

                            if (!cols.size())
                                return false;

                            wxGridCellCoordsContainer cells(cols, table->GetRows()-1, false);
                            std::vector<wxColour> colors;

                            const mu::Array& definition = static_cast<const mu::ArrValue*>(condFormat.read("format"))->get();

                            if (definition.get(0) == mu::Value("if-true"))
                            {
                                // {"if-true", "<", 42, "#FF0000"}
                                std::string cond = definition.get(1).getStr();
                                const mu::Array vals = definition.get(2);
                                colors.push_back(wxColour(definition.get(3).getStr()));

                                CellFilterCondition condition;

                                for (size_t k = 0; k < vals.size(); k++)
                                {
                                    if (vals.get(k).isString())
                                        condition.m_strs.push_back(vals.get(k).getStr());
                                    else
                                        condition.m_vals.push_back(vals.get(k).as_cmplx());
                                }

                                if (cond == "<")
                                    condition.m_type = CellFilterCondition::CT_LESS_THAN;
                                else if (cond == "<=")
                                    condition.m_type = CellFilterCondition::CT_LESS_EQ_THAN;
                                else if (cond == ">")
                                    condition.m_type = CellFilterCondition::CT_GREATER_THAN;
                                else if (cond == ">=")
                                    condition.m_type = CellFilterCondition::CT_GREATER_EQ_THAN;
                                else if (cond == "==")
                                    condition.m_type = vals.front().isString() ?
                                        CellFilterCondition::CT_EQUALS_STR : CellFilterCondition::CT_EQUALS_VAL;
                                else if (cond == "!=")
                                    condition.m_type = vals.front().isString() ?
                                        CellFilterCondition::CT_NOT_EQUALS_STR : CellFilterCondition::CT_NOT_EQUALS_VAL;
                                else if (cond == "strfnd" && vals.front().isString())
                                    condition.m_type = CellFilterCondition::CT_FIND_STR;
                                else if (cond == "!strfnd" && vals.front().isString())
                                    condition.m_type = CellFilterCondition::CT_NOT_FIND_STR;

                                table->conditionalFormat(cells, CellValueShader(colors, condition));
                            }
                            else if (definition.get(0) == mu::Value("map"))
                            {
                                // {"map", {1,2,3,4},{"#FF0000", "#00FF00", "#0000FF", "#FFFFFF"}}
                                const mu::Array& vals = definition.get(1);
                                const mu::Array& colorStrings = definition.get(2);

                                if (colorStrings.getCommonType() != mu::TYPE_STRING)
                                    return false;

                                CellFilterCondition condition;
                                condition.m_type = CellFilterCondition::CT_EQUALS_ARRAY;

                                for (size_t k = 0; k < vals.size(); k++)
                                {
                                    if (vals.get(k).isNumerical())
                                        condition.m_vals.push_back(vals.get(k).as_cmplx());
                                    else
                                        condition.m_vals.push_back(NAN);

                                    condition.m_strs.push_back(vals.get(k).printVal());
                                }

                                for (size_t k = 0; k < colorStrings.size(); k++)
                                {
                                    colors.push_back(wxColour(colorStrings.get(k).getStr()));
                                }

                                table->conditionalFormat(cells, CellValueShader(colors, condition));
                            }
                            else if (definition.get(0) == mu::Value("cscale"))
                            {
                                // {"cscale", {0,42},{"#FF0000", "#00FF00"}}
                                const mu::Array& vals = definition.get(1);
                                const mu::Array& colorStrings = definition.get(2);

                                if (colorStrings.size() < 2 || colorStrings.getCommonType() != mu::TYPE_STRING)
                                    return false;

                                CellFilterCondition condition;
                                condition.m_type = CellFilterCondition::CT_INTERVAL_RE;

                                for (size_t k = 0; k < vals.size(); k++)
                                {
                                    condition.m_vals.push_back(vals.get(k).as_cmplx());
                                }

                                for (size_t k = 0; k < colorStrings.size(); k++)
                                {
                                    colors.push_back(wxColour(colorStrings.get(k).getStr()));
                                }

                                table->conditionalFormat(cells, CellValueShader(colors, condition));
                            }
                            else if (definition.get(0) == mu::Value("cscale-limited"))
                            {
                                // {"cscale-limited", {0,42},{"#FF0000", "#00FF00"}}
                                const mu::Array& vals = definition.get(1);
                                const mu::Array& colorStrings = definition.get(2);

                                if (colorStrings.size() < 4 || colorStrings.getCommonType() != mu::TYPE_STRING)
                                    return false;

                                CellFilterCondition condition;
                                condition.m_type = CellFilterCondition::CT_INTERVAL_RE_EXCL;

                                for (size_t k = 0; k < vals.size(); k++)
                                {
                                    condition.m_vals.push_back(vals.get(k).as_cmplx());
                                }

                                for (size_t k = 0; k < colorStrings.size(); k++)
                                {
                                    colors.push_back(wxColour(colorStrings.get(k).getStr()));
                                }

                                table->conditionalFormat(cells, CellValueShader(colors, condition));
                            }
                        }
                    }

                    if (dict.isField("cell-format"))
                    { // "cell-format", {CELLS, FORMATTING, CELLS, FORMATTING, ...}
                        mu::Array formatting = mu::Value(dict.read("cell-format")->clone());
                        int64_t currentRows = table->GetNumberRows()-1;
                        int64_t currentCols = table->GetNumberCols()-1;

                        for (size_t j = 0; j < formatting.size(); j++)
                        {
                            wxGridCellCoordsArray cells;

                            // CELLS = {r, c} or {{r}, {c}}
                            if (!formatting.get(j).isDictStruct())
                                return false;

                            const mu::DictStruct& cellFormat = formatting.get(j).getDictStruct();

                            if (!cellFormat.isField("cells")
                                || !cellFormat.isField("format")
                                || cellFormat.read("cells")->m_type != mu::TYPE_ARRAY
                                || cellFormat.read("format")->m_type != mu::TYPE_DICTSTRUCT)
                                return false;

                            const mu::Array& cellsDef = static_cast<const mu::ArrValue*>(cellFormat.read("cells"))->get();

                            mu::Array rows = cellsDef.get(0);
                            mu::Array cols = cellsDef.get(1);

                            for (size_t k = 0; k < std::max(rows.size(), cols.size()); k++)
                            {
                                int64_t row = rows.get(k).getNum().asI64();
                                int64_t col = cols.get(k).getNum().asI64();

                                if (row > 0 && row <= currentRows && col > 0 && col <= currentCols)
                                    cells.Add(wxGridCellCoords(row-1, col-1));
                            }

                            if (!cells.size())
                                return false;

                            wxGridCellAttr* attr = new wxGridCellAttr();

                            if (cellFormat.isField(mu::Path("format/font")))
                            {
                                wxFont font = table->GetDefaultCellFont();
                                std::string sFormattingString = cellFormat.read(mu::Path("format/font"))->printVal(0, 0);

                                for (const char c : sFormattingString)
                                {
                                    switch (c)
                                    {
                                    case 'i':
                                        font.MakeItalic();
                                        break;
                                    case 'b':
                                        font.MakeBold();
                                        break;
                                    case 'u':
                                        font.MakeUnderlined();
                                        break;
                                    case 'x':
                                        font.MakeStrikethrough();
                                        break;
                                    case 's':
                                        font.MakeSmaller();
                                        break;
                                    case 'l':
                                        font.MakeLarger();
                                        break;
                                    }
                                }

                                attr->SetFont(font);
                            }

                            if (cellFormat.isField(mu::Path("format/color")))
                                attr->SetTextColour(wxColour(cellFormat.read(mu::Path("format/color"))->printVal(0, 0)));

                            if (cellFormat.isField(mu::Path("format/bgcolor")))
                                attr->SetBackgroundColour(wxColour(cellFormat.read(mu::Path("format/bgcolor"))->printVal(0, 0)));

                            for (size_t n = 0; n < cells.size(); n++)
                            {
                                table->SetAttr(table->GetExternalRows(cells[n].GetRow()),
                                               cells[n].GetCol(),
                                               attr->Clone());
                            }

                            attr->DecRef();
                            table->Refresh();
                        }
                    }

                    if (dict.isField("col-labels") && dict.read("col-labels")->m_type == mu::TYPE_DICTSTRUCT)
                    {// "col-labels", {{COLS},{LABELS}}, ...
                        const mu::DictStruct& labelDef = static_cast<const DictStructValue*>(dict.read("col-labels"))->get();
                        int64_t currentCols = table->GetNumberCols()-1;

                        mu::Array cols = mu::Value(labelDef.read("cols")->clone());
                        mu::Array labels = mu::Value(labelDef.read("labels")->clone());

                        for (size_t j = 0; j < std::max(cols.size(), labels.size()); j++)
                        {
                            int64_t col = cols.get(j).getNum().asI64();

                            if (col <= 0 || col > currentCols)
                                continue;

                            table->SetColLabelValue(col-1, labels.get(j).getStr());
                        }
                    }

                    if (dict.isField("hide-rows"))
                    {// "hide-rows", {COLS}, ...
                        mu::Array rows = mu::Value(dict.read("hide-rows")->clone());
                        int64_t currentRows = table->GetNumberRows()-1;

                        for (size_t j = 0; j < rows.size(); j++)
                        {
                            int64_t row = table->GetExternalRows(rows.get(j).getNum().asI64()-1);

                            if (row < 0 || row >= currentRows)
                                continue;

                            table->HideRow(row);
                        }
                    }

                    if (dict.isField("hide-cols"))
                    {// "hide-cols", {COLS}, ...
                        mu::Array cols = mu::Value(dict.read("hide-cols")->clone());
                        int64_t currentCols = table->GetNumberCols()-1;

                        for (size_t j = 0; j < cols.size(); j++)
                        {
                            int64_t col = cols.get(j).getNum().asI64();

                            if (col <= 0 || col > currentCols)
                                continue;

                            table->HideCol(col-1);
                        }
                    }
                }
                else
                {
                    for (size_t i = 0; i < _options.size(); i+=2)
                    {
                        if (_options.get(i) == mu::Value("min-cols"))
                        {
                            int64_t c = table->GetNumberCols();

                            if (c < _options.get(i+1).getNum().asI64()+1)
                                table->AppendCols(_options.get(i+1).getNum().asI64()-c+1);
                        }

                        if (_options.get(i) == mu::Value("min-rows"))
                        {
                            int64_t r = table->GetInternalRows(table->GetNumberRows());

                            if (r < _options.get(i+1).getNum().asI64()+1)
                                table->AppendRows(_options.get(i+1).getNum().asI64()-r+1);
                        }

                        if (_options.get(i) == mu::Value("use-qmarks"))
                            table->enableQuotationMarks((bool)_options.get(i+1));

                        if (_options.get(i) == mu::Value("fitsize"))
                            fitSize = (bool)_options.get(i+1);

                        if (_options.get(i) == mu::Value("fitsize-cols"))
                        {
                            wxArrayInt cols;
                            int64_t currentCols = table->GetNumberCols()-1;

                            if (_options.get(i+1).isArray())
                            {
                                const mu::Array& selectedCols = _options.get(i+1).getArray();

                                for (size_t k = 0; k < selectedCols.size(); k++)
                                {
                                    int64_t selected = selectedCols.get(k).getNum().asI64();

                                    if (currentCols < selected || selected < 1)
                                        continue;

                                    cols.Add(selected-1);
                                }
                            }
                            else if (_options.get(i+1).getNum().asI64() > 0 && _options.get(i+1).getNum().asI64() <= currentCols)
                                cols.Add(_options.get(i+1).getNum().asI64()-1);

                            if (!cols.size())
                                return false;

                            for (size_t i = 0; i < cols.size(); i++)
                            {
                                table->AutoSizeColumn(i);
                            }
                        }

                        if (_options.get(i) == mu::Value("cond-format"))
                        {
                            const mu::Array& formatting = _options.get(i+1).getArray();
                            int64_t currentCols = table->GetNumberCols()-1;

                            for (size_t j = 0; j < formatting.size(); j+=2)
                            {
                                wxArrayInt cols;

                                if (formatting.get(j).isArray())
                                {
                                    const mu::Array& selectedCols = formatting.get(j).getArray();

                                    for (size_t k = 0; k < selectedCols.size(); k++)
                                    {
                                        int64_t selected = selectedCols.get(k).getNum().asI64();

                                        if (currentCols < selected || selected < 1)
                                            continue;

                                        cols.Add(selected-1);
                                    }
                                }
                                else if (formatting.get(j).getNum().asI64() > 0 && formatting.get(j).getNum().asI64() <= currentCols)
                                    cols.Add(formatting.get(j).getNum().asI64()-1);

                                if (!cols.size())
                                    return false;

                                wxGridCellCoordsContainer cells(cols, table->GetRows()-1, false);
                                std::vector<wxColour> colors;

                                const mu::Array& definition = formatting.get(j+1).getArray();

                                if (definition.get(0) == mu::Value("if-true"))
                                {
                                    // {"if-true", "<", 42, "#FF0000"}
                                    std::string cond = definition.get(1).getStr();
                                    const mu::Array vals = definition.get(2);
                                    colors.push_back(wxColour(definition.get(3).getStr()));

                                    CellFilterCondition condition;

                                    for (size_t k = 0; k < vals.size(); k++)
                                    {
                                        if (vals.get(k).isString())
                                            condition.m_strs.push_back(vals.get(k).getStr());
                                        else
                                            condition.m_vals.push_back(vals.get(k).as_cmplx());
                                    }

                                    if (cond == "<")
                                        condition.m_type = CellFilterCondition::CT_LESS_THAN;
                                    else if (cond == "<=")
                                        condition.m_type = CellFilterCondition::CT_LESS_EQ_THAN;
                                    else if (cond == ">")
                                        condition.m_type = CellFilterCondition::CT_GREATER_THAN;
                                    else if (cond == ">=")
                                        condition.m_type = CellFilterCondition::CT_GREATER_EQ_THAN;
                                    else if (cond == "==")
                                        condition.m_type = vals.front().isString() ?
                                            CellFilterCondition::CT_EQUALS_STR : CellFilterCondition::CT_EQUALS_VAL;
                                    else if (cond == "!=")
                                        condition.m_type = vals.front().isString() ?
                                            CellFilterCondition::CT_NOT_EQUALS_STR : CellFilterCondition::CT_NOT_EQUALS_VAL;
                                    else if (cond == "strfnd" && vals.front().isString())
                                        condition.m_type = CellFilterCondition::CT_FIND_STR;
                                    else if (cond == "!strfnd" && vals.front().isString())
                                        condition.m_type = CellFilterCondition::CT_NOT_FIND_STR;

                                    table->conditionalFormat(cells, CellValueShader(colors, condition));
                                }
                                else if (definition.get(0) == mu::Value("map"))
                                {
                                    // {"map", {1,2,3,4},{"#FF0000", "#00FF00", "#0000FF", "#FFFFFF"}}
                                    const mu::Array& vals = definition.get(1);
                                    const mu::Array& colorStrings = definition.get(2);

                                    if (colorStrings.getCommonType() != mu::TYPE_STRING)
                                        return false;

                                    CellFilterCondition condition;
                                    condition.m_type = CellFilterCondition::CT_EQUALS_ARRAY;

                                    for (size_t k = 0; k < vals.size(); k++)
                                    {
                                        if (vals.get(k).isNumerical())
                                            condition.m_vals.push_back(vals.get(k).as_cmplx());
                                        else
                                            condition.m_vals.push_back(NAN);

                                        condition.m_strs.push_back(vals.get(k).printVal());
                                    }

                                    for (size_t k = 0; k < colorStrings.size(); k++)
                                    {
                                        colors.push_back(wxColour(colorStrings.get(k).getStr()));
                                    }

                                    table->conditionalFormat(cells, CellValueShader(colors, condition));
                                }
                                else if (definition.get(0) == mu::Value("cscale"))
                                {
                                    // {"cscale", {0,42},{"#FF0000", "#00FF00"}}
                                    const mu::Array& vals = definition.get(1);
                                    const mu::Array& colorStrings = definition.get(2);

                                    if (colorStrings.size() < 2 || colorStrings.getCommonType() != mu::TYPE_STRING)
                                        return false;

                                    CellFilterCondition condition;
                                    condition.m_type = CellFilterCondition::CT_INTERVAL_RE;

                                    for (size_t k = 0; k < vals.size(); k++)
                                    {
                                        condition.m_vals.push_back(vals.get(k).as_cmplx());
                                    }

                                    for (size_t k = 0; k < colorStrings.size(); k++)
                                    {
                                        colors.push_back(wxColour(colorStrings.get(k).getStr()));
                                    }

                                    table->conditionalFormat(cells, CellValueShader(colors, condition));
                                }
                                else if (definition.get(0) == mu::Value("cscale-limited"))
                                {
                                    // {"cscale-limited", {0,42},{"#FF0000", "#00FF00"}}
                                    const mu::Array& vals = definition.get(1);
                                    const mu::Array& colorStrings = definition.get(2);

                                    if (colorStrings.size() < 4 || colorStrings.getCommonType() != mu::TYPE_STRING)
                                        return false;

                                    CellFilterCondition condition;
                                    condition.m_type = CellFilterCondition::CT_INTERVAL_RE_EXCL;

                                    for (size_t k = 0; k < vals.size(); k++)
                                    {
                                        condition.m_vals.push_back(vals.get(k).as_cmplx());
                                    }

                                    for (size_t k = 0; k < colorStrings.size(); k++)
                                    {
                                        colors.push_back(wxColour(colorStrings.get(k).getStr()));
                                    }

                                    table->conditionalFormat(cells, CellValueShader(colors, condition));
                                }
                            }
                        }

                        if (_options.get(i) == mu::Value("cell-format"))
                        { // "cell-format", {CELLS, FORMATTING, CELLS, FORMATTING, ...}
                            const mu::Array& formatting = _options.get(i+1).getArray();
                            int64_t currentRows = table->GetNumberRows()-1;
                            int64_t currentCols = table->GetNumberCols()-1;

                            for (size_t j = 0; j < formatting.size(); j+=2)
                            {
                                wxGridCellCoordsArray cells;

                                // CELLS = {r, c} or {{r}, {c}}
                                if (!formatting.get(j).isArray())
                                    return false;

                                mu::Array rows = formatting.get(j).getArray().get(0);
                                mu::Array cols = formatting.get(j).getArray().get(1);

                                for (size_t k = 0; k < std::max(rows.size(), cols.size()); k++)
                                {
                                    int64_t row = rows.get(k).getNum().asI64();
                                    int64_t col = cols.get(k).getNum().asI64();

                                    if (row > 0 && row <= currentRows && col > 0 && col <= currentCols)
                                        cells.Add(wxGridCellCoords(row-1, col-1));
                                }

                                if (!cells.size())
                                    return false;

                                wxGridCellAttr* attr = new wxGridCellAttr();

                                const mu::Array& definition = formatting.get(j+1).getArray();

                                for (size_t n = 0; n < definition.size(); n+=2)
                                {
                                    if (definition.get(n) == mu::Value("font"))
                                    {
                                        wxFont font = table->GetDefaultCellFont();
                                        std::string sFormattingString = definition.get(n+1).getStr();

                                        for (const char c : sFormattingString)
                                        {
                                            switch (c)
                                            {
                                            case 'i':
                                                font.MakeItalic();
                                                break;
                                            case 'b':
                                                font.MakeBold();
                                                break;
                                            case 'u':
                                                font.MakeUnderlined();
                                                break;
                                            case 'x':
                                                font.MakeStrikethrough();
                                                break;
                                            case 's':
                                                font.MakeSmaller();
                                                break;
                                            case 'l':
                                                font.MakeLarger();
                                                break;
                                            }
                                        }

                                        attr->SetFont(font);
                                    }

                                    if (definition.get(n) == mu::Value("color"))
                                        attr->SetTextColour(wxColour(definition.get(n+1).getStr()));

                                    if (definition.get(n) == mu::Value("bgcolor"))
                                        attr->SetBackgroundColour(wxColour(definition.get(n+1).getStr()));
                                }

                                for (size_t n = 0; n < cells.size(); n++)
                                {
                                    table->SetAttr(table->GetExternalRows(cells[n].GetRow()),
                                                   cells[n].GetCol(),
                                                   attr->Clone());
                                }

                                attr->DecRef();
                                table->Refresh();
                            }
                        }

                        if (_options.get(i) == mu::Value("col-labels"))
                        {// "col-labels", {{COLS},{LABELS}}, ...
                            const mu::Array& labelDef = _options.get(i+1).getArray();
                            int64_t currentCols = table->GetNumberCols()-1;

                            mu::Array cols = labelDef.get(0);
                            mu::Array labels = labelDef.get(1);

                            for (size_t j = 0; j < std::max(cols.size(), labels.size()); j++)
                            {
                                int64_t col = cols.get(j).getNum().asI64();

                                if (col <= 0 || col > currentCols)
                                    continue;

                                table->SetColLabelValue(col-1, labels.get(j).getStr());
                            }
                        }

                        if (_options.get(i) == mu::Value("hide-rows"))
                        {// "hide-rows", {COLS}, ...
                            mu::Array rows = _options.get(i+1);
                            int64_t currentRows = table->GetNumberRows()-1;

                            for (size_t j = 0; j < rows.size(); j++)
                            {
                                int64_t row = table->GetExternalRows(rows.get(j).getNum().asI64()-1);

                                if (row < 0 || row >= currentRows)
                                    continue;

                                table->HideRow(row);
                            }
                        }

                        if (_options.get(i) == mu::Value("hide-cols"))
                        {// "hide-cols", {COLS}, ...
                            mu::Array cols = _options.get(i+1);
                            int64_t currentCols = table->GetNumberCols()-1;

                            for (size_t j = 0; j < cols.size(); j++)
                            {
                                int64_t col = cols.get(j).getNum().asI64();

                                if (col <= 0 || col > currentCols)
                                    continue;

                                table->HideCol(col-1);
                            }
                        }
                    }
                }

                // Resizing at the end
                if (fitSize)
                {
                    table->AutoSize();
                    table->GetParent()->Layout();
                }

                break;
            }
            case CustomWindow::BUTTON:
            case CustomWindow::CHECKBOX:
            case CustomWindow::TEXT:
            case CustomWindow::TEXTCTRL:
            case CustomWindow::LAMP:
            case CustomWindow::SPINCTRL:
            case CustomWindow::RADIOGROUP:
            case CustomWindow::DROPDOWN:
            case CustomWindow::COMBOBOX:
            case CustomWindow::MENUITEM:
            case CustomWindow::TREELIST:
            case CustomWindow::GAUGE:
            case CustomWindow::IMAGE:
            case CustomWindow::GRAPHER:
            case CustomWindow::SLIDER:
            case CustomWindow::DATETIMEPICKER:
            case CustomWindow::NOTEBOOK:
                break;
        }
    }
    catch (...)
    {
        wxMessageBox(_guilang.get("GUI_CUSTOMWINDOW_CATCH", "window ID -set options={KEY-VAL-LIST} item=ID"),
                     _guilang.get("GUI_CUSTOMWINDOW_CATCH_HEAD"), wxCENTER | wxICON_ERROR | wxOK, this);
        return false;
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
            int pos = std::max(std::min((long)selectionID-1, (long)field->GetLastPosition()), 0L);

            if (selectionID2)
                field->SetSelection(pos,
                                    std::max(std::min((long)selectionID-1+selectionID2, (long)field->GetLastPosition()), 0L));
            else
                field->SetInsertionPoint(pos);

            field->ShowPosition(pos);

            break;
        }
        case CustomWindow::DROPDOWN:
        {
            wxChoice* choices = static_cast<wxChoice*>(object.second);

            if (selectionID > 0 && selectionID <= (int)choices->GetCount())
                choices->SetSelection(selectionID-1);

            break;
        }
        case CustomWindow::COMBOBOX:
        {
            wxComboBox* combo = static_cast<wxComboBox*>(object.second);

            if (selectionID > 0 && selectionID <= (int)combo->GetCount())
                combo->SetSelection(selectionID-1);

            break;
        }
        case CustomWindow::TABLE:
        {
            TableViewer* table = static_cast<TableViewer*>(object.second);
            int row = std::min(std::max(0, table->GetExternalRows(selectionID-1)), table->GetNumberRows()-1);
            int col = std::min(std::max(0, selectionID2-1), table->GetNumberCols()-1);
            table->SetGridCursorSilent(row, col);
            table->MakeCellVisible(row, col);
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

            if (selectionID > 0 && selectionID <= (int)notebook->GetPageCount())
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
/// \param _value const mu::Array&
/// \param varName const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setPropValue(const mu::Array& _value, const std::string& varName)
{
    auto iter = m_varTable.find(varName);

    if (iter != m_varTable.end())
    {
        iter->second = _value;
        iter->second.dereference();
        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Sets the text into the status bar.
///
/// \param _values const wxArrayString&
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setStatusText(const wxArrayString& _values)
{
    wxStatusBar* bar = GetStatusBar();

    if (bar->GetFieldsCount() != (int)_values.size())
        bar->SetFieldsCount(_values.size());

    for (size_t i = 0; i < _values.size(); i++)
    {
        bar->SetStatusText(_values[i], i);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Sets the text into the status bar.
///
/// \param _value const mu::Array&
/// \return bool
///
/////////////////////////////////////////////////
bool CustomWindow::setStatusText(const mu::Array& _value)
{
    return setStatusText(getChoices(_value));
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

