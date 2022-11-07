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

#include "grouppanel.hpp"
#include "../../kernel/core/ui/language.hpp"

extern Language _guilang;
#define ELEMENT_BORDER 5

/////////////////////////////////////////////////
/// \brief Constructor.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param pos const wxPoint&
/// \param size const wxSize&
/// \param style long
///
/////////////////////////////////////////////////
GroupPanel::GroupPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, bool useVerticalSizer) : wxScrolledWindow(parent, id, pos, size, style | wxVSCROLL)
{
    verticalSizer = nullptr;
    horizontalSizer = nullptr;

    // Create the horizontal and the vertical sizers
    horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    verticalSizer = new wxBoxSizer(wxVERTICAL);

    if (useVerticalSizer)
    {
        // Add the vertical sizer as a subsizer
        // of the horizontal sizer
        horizontalSizer->Add(verticalSizer, 1, wxALIGN_TOP | wxEXPAND | wxALL, 0);

        // Set the horizontal sizer as main
        // sizer for the panel
        this->SetSizer(horizontalSizer);

        mainSizer = verticalSizer;
    }
    else
    {
        // Add the horizontal sizer as a subsizer
        // of the vertical sizer
        verticalSizer->Add(horizontalSizer, 1, wxALIGN_TOP | wxEXPAND | wxALL, 0);

        // Set the vertical sizer as main
        // sizer for the panel
        this->SetSizer(verticalSizer);

        mainSizer = horizontalSizer;
    }
}


/////////////////////////////////////////////////
/// \brief Return the pointer to the vertical
/// sizer.
///
/// \return wxBoxSizer*
///
/////////////////////////////////////////////////
wxBoxSizer* GroupPanel::getVerticalSizer()
{
    return verticalSizer;
}


/////////////////////////////////////////////////
/// \brief Return the pointer to the horizontal
/// sizer.
///
/// \return wxBoxSizer*
///
/////////////////////////////////////////////////
wxBoxSizer* GroupPanel::getHorizontalSizer()
{
    return horizontalSizer;
}


/////////////////////////////////////////////////
/// \brief Return the pointer to the current main
/// layout sizer.
///
/// \return wxBoxSizer*
///
/////////////////////////////////////////////////
wxBoxSizer* GroupPanel::getMainSizer()
{
    return mainSizer;
}


/////////////////////////////////////////////////
/// \brief Add extra space between the last added
/// (main) element and the next element to be
/// added.
///
/// \param nSize int
/// \param sizer wxSizer*
/// \return void
///
/////////////////////////////////////////////////
void GroupPanel::AddSpacer(int nSize, wxSizer* sizer)
{
    if (!sizer)
        sizer = mainSizer;

    sizer->AddSpacer(nSize);
}


/////////////////////////////////////////////////
/// \brief Add some static test to the current
/// sizer and window.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param text const wxString&
/// \param id int
/// \return wxStaticText*
///
/////////////////////////////////////////////////
wxStaticText* GroupPanel::AddStaticText(wxWindow* parent, wxSizer* sizer, const wxString& text, int id, int alignment)
{
    wxStaticText* staticText = new wxStaticText(parent, id, text, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(staticText, 0, alignment | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, ELEMENT_BORDER);

    return staticText;
}


/////////////////////////////////////////////////
/// \brief Member function to create a group (a
/// static box with a label) in the panel.
///
/// \param sGroupName const wxString&
/// \param orient int
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param expand int
/// \return wxStaticBoxSizer*
///
/////////////////////////////////////////////////
wxStaticBoxSizer* GroupPanel::createGroup(const wxString& sGroupName, int orient, wxWindow* parent, wxSizer* sizer, int expand)
{
    if (!parent)
    {
        parent = this;
        sizer = mainSizer;
    }

    // Create a new static box sizer
    wxStaticBoxSizer* groupSizer = new wxStaticBoxSizer(orient, parent, sGroupName);

    // Add the group to the main sizer
    sizer->Add(groupSizer, expand, wxEXPAND | wxALL, ELEMENT_BORDER);

    return groupSizer;
}


/////////////////////////////////////////////////
/// \brief Member function to create a group
/// represented as a box sizer.
///
/// \param orient int
/// \param sizer wxSizer*
/// \param expand int
/// \return wxBoxSizer*
///
/////////////////////////////////////////////////
wxBoxSizer* GroupPanel::createGroup(int orient, wxSizer* sizer, int expand)
{
    if (!sizer)
        sizer = mainSizer;

    // Create a new static box sizer
    wxBoxSizer* groupSizer = new wxBoxSizer(orient);

    // Add the group to the main sizer
    sizer->Add(groupSizer, expand, wxEXPAND | wxALL, 0);

    return groupSizer;
}


/////////////////////////////////////////////////
/// \brief Member function to create a
/// collapsible group.
///
/// \param label const wxString&
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \return wxCollapsiblePane*
///
/// \warning This group is currently quite buggy.
/////////////////////////////////////////////////
wxCollapsiblePane* GroupPanel::createCollapsibleGroup(const wxString& label, wxWindow* parent, wxSizer* sizer)
{
    if (!parent)
    {
        parent = this;
        sizer = mainSizer;
    }

    wxCollapsiblePane* collpane = new wxCollapsiblePane(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE | wxCP_DEFAULT_STYLE);

    // add the pane with a zero proportion value to the sizer which contains it
    sizer->Add(collpane, 1, wxEXPAND | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 1);

    return collpane;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a path input dialog including the
/// "choose" button.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param description const wxString&
/// \param buttonID int
/// \param id int
/// \return wxTextCtrl*
///
/////////////////////////////////////////////////
wxTextCtrl* GroupPanel::CreatePathInput(wxWindow* parent, wxSizer* sizer, const wxString& description, int buttonID, int id)
{
    // Create the text above the input line
    wxStaticText* inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(inputStaticText, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, ELEMENT_BORDER);

    // Create a horizontal sizer for the input
    // line and the buttoon
    wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(hSizer, wxALIGN_LEFT);

    // Create the input line
    wxTextCtrl* textCtrl = new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, wxSize(310, -1), wxTE_PROCESS_ENTER);

    // Create the button
    wxButton* button = new wxButton(parent, buttonID, _guilang.get("GUI_OPTIONS_CHOOSE"));

    // Add both to the horizontal sizer
    hSizer->Add(textCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);
    hSizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, ELEMENT_BORDER);

    return textCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a text input.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param description const wxString&
/// \param sDefault const wxString&
/// \param nStyle int
/// \param id int
/// \param size const wxSize&
/// \param alignment int
/// \return TextField*
///
/////////////////////////////////////////////////
TextField* GroupPanel::CreateTextInput(wxWindow* parent, wxSizer* sizer, const wxString& description, const wxString& sDefault, int nStyle, int id, const wxSize& size, int alignment)
{
    wxStaticText* inputStaticText = nullptr;
    // Create the text above the input line, if it exists
    if (description.length())
    {
        inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
        sizer->Add(inputStaticText, 0, alignment | wxLEFT | wxTOP | wxRIGHT | wxADJUST_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    }

    // Create the input line
    TextField* textCtrl = new TextField(parent, id, sDefault, size, nStyle);
    textCtrl->m_label = inputStaticText;
    sizer->Add(textCtrl, 0, alignment | wxALL | wxEXPAND | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return textCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a usual checkbox.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param description const wxString&
/// \param id int
/// \param alignment int
/// \return wxCheckBox*
///
/////////////////////////////////////////////////
wxCheckBox* GroupPanel::CreateCheckBox(wxWindow* parent, wxSizer* sizer, const wxString& description, int id, int alignment)
{
    // Create the checkbox and assign it to the passed sizer
    wxCheckBox* checkBox = new wxCheckBox(parent, id, description, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(checkBox, 0, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return checkBox;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a spin control including the
/// assigned text.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param description const wxString&
/// \param nMin int
/// \param nMax int
/// \param nInitial int
/// \param id int
/// \param alignment int
/// \return SpinBut*
///
/////////////////////////////////////////////////
SpinBut* GroupPanel::CreateSpinControl(wxWindow* parent, wxSizer* sizer, const wxString& description, int nMin, int nMax, int nInitial, int id, int alignment)
{
    // Create a horizontal sizer for the
    // spin control and its assigned text
    wxBoxSizer* spinCtrlSizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(spinCtrlSizer, 0, alignment | wxALL, ELEMENT_BORDER);

    // Create the spin control
    SpinBut* spinCtrl = new SpinBut(parent, id, wxSize(60, -1), nMin, nMax, nInitial);

    // Create the assigned static text
    wxStaticText* spinCtrlStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
    spinCtrl->m_label = spinCtrlStaticText;

    // Add both to the horizontal sizer
    spinCtrlSizer->Add(spinCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 0);
    spinCtrlSizer->Add(spinCtrlStaticText, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxADJUST_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 3);

    return spinCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a listview control.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param wxLC_REPORT int nStyle=
/// \param wxDefaultSize wxSize size=
/// \param id int
/// \return wxListView*
///
/////////////////////////////////////////////////
wxListView* GroupPanel::CreateListView(wxWindow* parent, wxSizer* sizer, int nStyle /*= wxLC_REPORT*/, wxSize size /*= wxDefaultSize*/, int id)
{
    // Create the listview and assign it to the passed sizer
    wxListView* listView = new wxListView(parent, id, wxDefaultPosition, size, nStyle);
    sizer->Add(listView, 1, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND | wxFIXED_MINSIZE, ELEMENT_BORDER);

    return listView;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a treelist control.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param nStyle int
/// \param size wxSize
/// \param id int
/// \param alignment int
/// \return wxTreeListCtrl*
///
/////////////////////////////////////////////////
wxTreeListCtrl* GroupPanel::CreateTreeListCtrl(wxWindow* parent, wxSizer* sizer, int nStyle, wxSize size, int id, int alignment)
{
    // Create the listview and assign it to the passed sizer
    wxTreeListCtrl* listCtrl = new wxTreeListCtrl(parent, id, wxDefaultPosition, size, nStyle);
    listCtrl->SetMinClientSize(wxSize(100,200));
    sizer->Add(listCtrl, 1, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return listCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a treelist control in the wxCode
/// variant.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param nStyle int
/// \param size wxSize
/// \param id int
/// \param alignment int
/// \return wxcode::wxTreeListCtrl*
///
/////////////////////////////////////////////////
wxcode::wxTreeListCtrl* GroupPanel::CreateWxcTreeListCtrl(wxWindow* parent, wxSizer* sizer, int nStyle, wxSize size, int id, int alignment)
{
    // Create the listview and assign it to the passed sizer
    wxcode::wxTreeListCtrl* listCtrl = new wxcode::wxTreeListCtrl(parent, id, wxDefaultPosition, size, nStyle);
    listCtrl->SetMinClientSize(wxSize(100,200));
    sizer->Add(listCtrl, 1, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return listCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a button.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param description const wxString&
/// \param id int
/// \param alignment int
/// \return wxButton*
///
/////////////////////////////////////////////////
wxButton* GroupPanel::CreateButton(wxWindow* parent, wxSizer* sizer, const wxString& description, int id, int alignment)
{
    wxButton* button = new wxButton(parent, id, description);

    if (sizer == mainSizer)
        sizer->Add(button, 0, alignment | wxALL | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        sizer->Add(button, 1, alignment | wxALL | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return button;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a radio box.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param description const wxString&
/// \param choices const wxArrayString&
/// \param style int
/// \param id int
/// \param alignment int
/// \return wxRadioBox*
///
/////////////////////////////////////////////////
wxRadioBox* GroupPanel::CreateRadioBox(wxWindow* parent, wxSizer* sizer, const wxString& description, const wxArrayString& choices, int style, int id, int alignment)
{
    wxRadioBox* box = new wxRadioBox(parent, id, description, wxDefaultPosition, wxDefaultSize, choices, 0, style);
    sizer->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return box;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a dropdown list.
///
/// \param parent wxWindow*
/// \param wxSizer*sizer
/// \param choices const wxArrayString&
/// \param id int
/// \param alignment int
/// \return wxChoice*
///
/////////////////////////////////////////////////
wxChoice* GroupPanel::CreateChoices(wxWindow* parent, wxSizer*sizer, const wxArrayString& choices, int id, int alignment)
{
    wxChoice* box = new wxChoice(parent, id, wxDefaultPosition, wxDefaultSize, choices);

    if (dynamic_cast<wxBoxSizer*>(sizer) && dynamic_cast<wxBoxSizer*>(sizer)->GetOrientation() == wxHORIZONTAL)
        sizer->Add(box, 1, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        sizer->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return box;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a combobox.
///
/// \param parent wxWindow*
/// \param wxSizer*sizer
/// \param choices const wxArrayString&
/// \param id int
/// \param alignment int
/// \return wxComboBox*
///
/////////////////////////////////////////////////
wxComboBox* GroupPanel::CreateComboBox(wxWindow* parent, wxSizer*sizer, const wxArrayString& choices, int id, int alignment)
{
    wxComboBox* box = new wxComboBox(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, choices);

    if (dynamic_cast<wxBoxSizer*>(sizer) && dynamic_cast<wxBoxSizer*>(sizer)->GetOrientation() == wxHORIZONTAL)
        sizer->Add(box, 1, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        sizer->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return box;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a progress bar.
///
/// \param parent wxWindow*
/// \param wxSizer*sizer
/// \param style int
/// \param id int
/// \param alignment int
/// \return wxGauge*
///
/////////////////////////////////////////////////
wxGauge* GroupPanel::CreateGauge(wxWindow* parent, wxSizer*sizer, int style, int id, int alignment)
{
    wxGauge* gauge = new wxGauge(parent, id, 100, wxDefaultPosition, wxDefaultSize, style);
    sizer->Add(gauge, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return gauge;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a static bitmap.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param filename const wxString&
/// \param id int
/// \param alignment int
/// \return wxStaticBitmap*
///
/////////////////////////////////////////////////
wxStaticBitmap* GroupPanel::CreateBitmap(wxWindow* parent, wxSizer* sizer, const wxString& filename, int id, int alignment)
{
    wxStaticBitmap* bitmap = new wxStaticBitmap(parent, id, wxBitmap(filename, wxBITMAP_TYPE_ANY));
    sizer->Add(bitmap, 0, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return bitmap;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a slider.
///
/// \param parent wxWindow*
/// \param sizer wxSizer*
/// \param nMin int
/// \param nMax int
/// \param nInitial int
/// \param style int
/// \param id int
/// \param alignment int
/// \return wxSlider*
///
/////////////////////////////////////////////////
wxSlider* GroupPanel::CreateSlider(wxWindow* parent, wxSizer* sizer, int nMin, int nMax, int nInitial, int style, int id, int alignment)
{
    wxSlider* slider = new wxSlider(parent, id, nInitial, nMin, nMax, wxDefaultPosition, wxDefaultSize, style);
    sizer->Add(slider, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);

    return slider;
}

