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
#include "../../common/markup.hpp"

BEGIN_EVENT_TABLE(TextField, wxTextCtrl)
    EVT_TEXT_URL(-1, TextField::OnUrlClick)
END_EVENT_TABLE()

/////////////////////////////////////////////////
/// \brief Handle simple markup styled text in
/// this control. Implemented as simple state
/// machine.
///
/// \param text const wxString&
/// \return void
///
/////////////////////////////////////////////////
void TextField::SetMarkupText(const wxString& text)
{
    if (!HasFlag(wxTE_RICH) && !HasFlag(wxTE_RICH2))
    {
        ChangeValue(text);
        return;
    }

    // Clear first (Clear() will generate an obsolete event)
    ChangeValue("");

    static const wxFont defaultFont = GetFont();
    wxTextAttr attr;
    attr.SetFont(defaultFont);
    attr.SetTextColour(*wxBLACK);
    attr.SetBackgroundColour(*wxWHITE);
    attr.SetLeftIndent(0);
    SetDefaultStyle(attr);

    std::vector<Markup::Token<wxString>> markupTokens = Markup::decode(text);

    for (size_t i = 0; i < markupTokens.size(); i++)
    {
        attr.SetTextColour(*wxBLACK);
        attr.SetBackgroundColour(*wxWHITE);
        attr.SetFont(defaultFont);

        if (markupTokens[i].line == Markup::PARAGRAPH)
            attr.SetLeftIndent(0);
        else if (markupTokens[i].line == Markup::H1)
        {
            attr.SetFontUnderlined(true);
            attr.SetFontSize(defaultFont.GetPointSize()+6);
        }
        else if (markupTokens[i].line == Markup::H2)
        {
            attr.SetFontSize(defaultFont.GetPointSize()+4);
            SetDefaultStyle(attr);
        }
        else if (markupTokens[i].line == Markup::H3)
        {
            attr.SetFont(defaultFont.Italic());
            attr.SetFontSize(defaultFont.GetPointSize()+2);
        }
        else if (markupTokens[i].line == Markup::UL && (!i || markupTokens[i-1].line != Markup::UL))
        {
            attr.SetLeftIndent(30, 20);
            SetDefaultStyle(attr);
            AppendText(L"\u2022");
        }

        if (markupTokens[i].inLine & Markup::ITALICS && markupTokens[i].inLine & Markup::BOLD)
            attr.SetFont(defaultFont.Italic().Bold());
        else if (markupTokens[i].inLine & Markup::ITALICS)
            attr.SetFont(defaultFont.Italic());
        else if (markupTokens[i].inLine & Markup::BOLD)
            attr.SetFont(defaultFont.Bold());

        if (markupTokens[i].inLine & Markup::BRCKT)
            attr.SetTextColour(m_highlightColour);

        if (markupTokens[i].inLine & Markup::EMPH)
            attr.SetBackgroundColour(wxColor(255,255,164));

        if (markupTokens[i].inLine & Markup::CODE)
        {
            wxFont font;
            font.SetNativeFontInfoUserDesc("consolas");
            attr.SetFont(font);
            attr.SetTextColour(wxColour(0,0,100));
            attr.SetBackgroundColour(wxColour(240,240,240));
        }

        SetDefaultStyle(attr);
        AppendText(markupTokens[i].text);
    }
}


/////////////////////////////////////////////////
/// \brief Handle the event, if the user clicks
/// on a selected URL within the control.
///
/// \param event wxTextUrlEvent&
/// \return void
///
/////////////////////////////////////////////////
void TextField::OnUrlClick(wxTextUrlEvent& event)
{
    wxMouseEvent mouseEvent = event.GetMouseEvent();

    if (mouseEvent.IsButton())
    {
        wxString url = GetRange(event.GetURLStart(), event.GetURLEnd());
        wxLaunchDefaultBrowser(url);
    }
}


/////////////////////////////////////////////////
/// \brief Simple static helper to simplify the
/// layout error messaging.
///
/// \param uiElement const wxString&
/// \param parent wxWindow*
/// \return void
///
/////////////////////////////////////////////////
static void layoutError(const wxString& uiElement, wxWindow* parent)
{
    wxMessageBox("An element of type \"" + uiElement + "\" does not fit into its assigned group. If the assigned group is of type \"flexgrid\", then the target cell(s) may already be occupied.", "Group layout warning", wxOK | wxCENTER | wxICON_WARNING, parent);
}


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
/// \brief Member function to create a group (a
/// static box with a label) in the panel.
///
/// \param sGroupName const wxString&
/// \param orient int
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param expand int
/// \return wxStaticBoxSizer*
///
/////////////////////////////////////////////////
wxStaticBoxSizer* GroupPanel::createGroup(const wxString& sGroupName, int orient, wxWindow* parent, const SizerWrapper& sizer, int expand)
{
    if (!parent)
        parent = this;

    // Create a new static box sizer
    wxStaticBoxSizer* groupSizer = new wxStaticBoxSizer(orient, parent, sGroupName);

    // Add the group to the main sizer
    if (!sizer)
        mainSizer->Add(groupSizer, expand, wxEXPAND | wxALL, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(groupSizer, expand, wxEXPAND | wxALL, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(groupSizer, sizer.pos, sizer.span, wxEXPAND | wxALL, ELEMENT_BORDER);
    else
        wxMessageBox("A labeled group does not fit into its assigned group. If the assigned group is of type \"flexgrid\", then the target cell(s) may already be occupied.", "Group layout warning", wxOK | wxCENTER | wxICON_WARNING, this);

    return groupSizer;
}


/////////////////////////////////////////////////
/// \brief Member function to create a group
/// represented as a box sizer.
///
/// \param orient int
/// \param sizer const SizerWrapper&
/// \param expand int
/// \return wxBoxSizer*
///
/////////////////////////////////////////////////
wxBoxSizer* GroupPanel::createGroup(int orient, const SizerWrapper& sizer, int expand)
{
    // Create a new static box sizer
    wxBoxSizer* groupSizer = new wxBoxSizer(orient);

    // Add the group to the main sizer
    if (!sizer)
        mainSizer->Add(groupSizer, expand, wxEXPAND | wxALL, 0);
    else if (sizer.isBox())
        sizer.s->Add(groupSizer, expand, wxEXPAND | wxALL, 0);
    else if (sizer.canEmplace())
        sizer.gbs->Add(groupSizer, sizer.pos, sizer.span, expand, wxEXPAND | wxALL, 0);
    else
        wxMessageBox("A group does not fit into its assigned group. If the assigned group is of type \"flexgrid\", then the target cell(s) may already be occupied.", "Group layout warning", wxOK | wxCENTER | wxICON_WARNING, this);

    return groupSizer;
}


/////////////////////////////////////////////////
/// \brief Member function to create a grid group
/// represented as a gridbag sizer.
///
/// \param sizer const SizerWrapper&
/// \param expand int
/// \return wxGridBagSizer*
///
/////////////////////////////////////////////////
wxGridBagSizer* GroupPanel::createGridGroup(const SizerWrapper& sizer, int expand)
{
    // Create a new static box sizer
    wxGridBagSizer* groupSizer = new wxGridBagSizer();

    if (expand)
        groupSizer->SetFlexibleDirection(wxBOTH);

    // Add the group to the main sizer
    if (!sizer)
        mainSizer->Add(groupSizer, expand, wxEXPAND | wxALL, 0);
    else if (sizer.isBox())
        sizer.s->Add(groupSizer, expand, wxEXPAND | wxALL, 0);
    else if (sizer.canEmplace())
        sizer.gbs->Add(groupSizer, sizer.pos, sizer.span, expand, wxEXPAND | wxALL, 0);
    else
        wxMessageBox("A group of type \"flexgrid\" does not fit into its assigned group. If the assigned group is of type \"flexgrid\", then the target cell(s) may already be occupied.", "Group layout warning", wxOK | wxCENTER | wxICON_WARNING, this);

    return groupSizer;
}


/////////////////////////////////////////////////
/// \brief Member function to create a
/// collapsible group.
///
/// \param label const wxString&
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \return wxCollapsiblePane*
///
/// \warning This group is currently quite buggy.
/////////////////////////////////////////////////
wxCollapsiblePane* GroupPanel::createCollapsibleGroup(const wxString& label, wxWindow* parent, const SizerWrapper& sizer)
{
    if (!parent)
        parent = this;

    wxCollapsiblePane* collpane = new wxCollapsiblePane(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE | wxCP_DEFAULT_STYLE);

    // add the pane with a zero proportion value to the sizer which contains it
    if (!sizer)
        mainSizer->Add(collpane, 1, wxEXPAND | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 1);
    else if (sizer.isBox())
        sizer.s->Add(collpane, 1, wxEXPAND | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 1);
    else if (sizer.canEmplace())
        sizer.gbs->Add(collpane, sizer.pos, sizer.span, wxEXPAND | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 1);

    return collpane;
}


/////////////////////////////////////////////////
/// \brief Add extra space between the last added
/// (main) element and the next element to be
/// added.
///
/// \param nSize int
/// \param sizer const SizerWrapper&
/// \return void
///
/////////////////////////////////////////////////
void GroupPanel::AddSpacer(int nSize, const SizerWrapper& sizer)
{
    if (!sizer)
        mainSizer->AddSpacer(nSize);
    else if (sizer.isBox())
        sizer.s->AddSpacer(nSize);
    else if (sizer.canEmplace())
        sizer.gbs->Add(nSize, nSize, sizer.pos, sizer.span);
    else
        layoutError("spacer", this);
}


/////////////////////////////////////////////////
/// \brief Add some static test to the current
/// sizer and window.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param text const wxString&
/// \param id int
/// \return wxStaticText*
///
/////////////////////////////////////////////////
wxStaticText* GroupPanel::AddStaticText(wxWindow* parent, const SizerWrapper& sizer, const wxString& text, int id, int alignment)
{
    wxStaticText* staticText = new wxStaticText(parent, id, text, wxDefaultPosition, wxDefaultSize, 0);

    if (!sizer)
        mainSizer->Add(staticText, 0, alignment | wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(staticText, 0, alignment | wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(staticText, sizer.pos, sizer.span, alignment | wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);
    else
        layoutError("statictext", this);

    return staticText;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a path input dialog including the
/// "choose" button.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param description const wxString&
/// \param buttonID int
/// \param id int
/// \return wxTextCtrl*
///
/////////////////////////////////////////////////
wxTextCtrl* GroupPanel::CreatePathInput(wxWindow* parent, const SizerWrapper& sizer, const wxString& description, int buttonID, int id)
{
    wxBoxSizer* groupSizer = new wxBoxSizer(wxVERTICAL);

    // Create the text above the input line
    if (description.length())
    {
        wxStaticText* inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
        groupSizer->Add(inputStaticText, 0, wxALIGN_LEFT | wxBOTTOM | wxADJUST_MINSIZE, ELEMENT_BORDER);
    }

    // Create a horizontal sizer for the input
    // line and the buttoon
    wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
    groupSizer->Add(hSizer, wxALIGN_LEFT);

    // Create the input line
    wxTextCtrl* textCtrl = new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, wxSize(310, -1), wxTE_PROCESS_ENTER);

    // Create the button
    wxButton* button = new wxButton(parent, buttonID, _guilang.get("GUI_OPTIONS_CHOOSE"));

    // Add both to the horizontal sizer
    hSizer->Add(textCtrl, 1, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxRIGHT, 2*ELEMENT_BORDER);
    hSizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT, 0);

    if (!sizer)
        mainSizer->Add(groupSizer, 0, wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(groupSizer, 0, wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(groupSizer, sizer.pos, sizer.span, wxALL | wxADJUST_MINSIZE, ELEMENT_BORDER);
    else
        wxMessageBox("A path selector element does not fit into its assigned group. If the group is of type \"flexgrid\", then the target cell(s) may already be occupied.", "Group layout warning", wxOK | wxCENTER | wxICON_WARNING, this);

    return textCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a text input.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param description const wxString&
/// \param sDefault const wxString&
/// \param nStyle int
/// \param id int
/// \param size const wxSize&
/// \param alignment int
/// \param proportion int
/// \return TextField*
///
/////////////////////////////////////////////////
TextField* GroupPanel::CreateTextInput(wxWindow* parent, const SizerWrapper& sizer, const wxString& description, const wxString& sDefault, int nStyle, int id, const wxSize& size, int alignment, int proportion)
{
    wxStaticText* inputStaticText = nullptr;
    int margin = wxBOTTOM;
    wxOrientation orient = wxVERTICAL;

    if (sizer.isBox() && static_cast<wxBoxSizer*>(sizer.s)->GetOrientation() != wxVERTICAL)
    {
        margin = wxRIGHT;
        orient = wxHORIZONTAL;
    }

    wxBoxSizer* groupSizer = new wxBoxSizer(orient);

    // Create the text above the input line, if it exists
    if (description.length())
    {
        inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
        groupSizer->Add(inputStaticText, 0, alignment | margin | wxADJUST_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    }

    // Create the input line
    TextField* textCtrl = new TextField(parent, id, sDefault, size, nStyle);
    textCtrl->m_label = inputStaticText;
    groupSizer->Add(textCtrl, proportion, alignment | wxEXPAND | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 0);

    if (!sizer)
        mainSizer->Add(groupSizer, proportion, alignment | wxALL | wxEXPAND | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(groupSizer, proportion, alignment | wxALL | wxEXPAND | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(groupSizer, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("textfield", this);

    return textCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a "lamp".
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param description const wxString&
/// \param sDefault const wxString&
/// \param nStyle int
/// \param id int
/// \param size const wxSize&
/// \param alignment int
/// \param proportion int
/// \return TextField*
///
/////////////////////////////////////////////////
TextField* GroupPanel::CreateLamp(wxWindow* parent, const SizerWrapper& sizer, const wxString& description, const wxString& sDefault, int nStyle, int id, const wxSize& size, int alignment, int proportion)
{
    wxStaticText* inputStaticText = nullptr;
    int margin = wxBOTTOM;
    wxOrientation orient = wxVERTICAL;

    if (sizer.isBox() && static_cast<wxBoxSizer*>(sizer.s)->GetOrientation() != wxVERTICAL)
    {
        margin = wxRIGHT;
        orient = wxHORIZONTAL;
    }

    wxBoxSizer* groupSizer = new wxBoxSizer(orient);

    // Create the text above the input line, if it exists
    if (description.length())
    {
        inputStaticText = new wxStaticText(parent, wxID_STATIC, description, wxDefaultPosition, wxDefaultSize, 0);
        groupSizer->Add(inputStaticText, 0, alignment | margin | wxADJUST_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    }

    // Create the "lamp"
    TextField* textCtrl = new TextField(parent, id, sDefault, size, nStyle | wxBORDER_RAISED | wxTE_READONLY);
    textCtrl->m_label = inputStaticText;
    textCtrl->SetBackgroundColour(wxColour(64,64,64));
    groupSizer->Add(textCtrl, proportion, alignment | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 0);

    if (!sizer)
        mainSizer->Add(groupSizer, proportion, alignment | wxALL | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(groupSizer, proportion, alignment | wxALL | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(groupSizer, sizer.pos, sizer.span, alignment | wxALL | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("lamp", this);

    return textCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a usual checkbox.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param description const wxString&
/// \param id int
/// \param alignment int
/// \return wxCheckBox*
///
/////////////////////////////////////////////////
wxCheckBox* GroupPanel::CreateCheckBox(wxWindow* parent, const SizerWrapper& sizer, const wxString& description, int id, int alignment)
{
    // Create the checkbox and assign it to the passed sizer
    wxCheckBox* checkBox = new wxCheckBox(parent, id, description, wxDefaultPosition, wxDefaultSize, 0);

    if (!sizer)
        mainSizer->Add(checkBox, 0, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(checkBox, 0, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(checkBox, sizer.pos, sizer.span, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("checkbox", this);;

    return checkBox;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a spin control including the
/// assigned text.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param description const wxString&
/// \param nMin int
/// \param nMax int
/// \param nInitial int
/// \param id int
/// \param alignment int
/// \return SpinBut*
///
/////////////////////////////////////////////////
SpinBut* GroupPanel::CreateSpinControl(wxWindow* parent, const SizerWrapper& sizer, const wxString& description, int nMin, int nMax, int nInitial, int id, int alignment)
{
    // Create a horizontal sizer for the
    // spin control and its assigned text
    wxBoxSizer* spinCtrlSizer = new wxBoxSizer(wxHORIZONTAL);

    if (!sizer)
        mainSizer->Add(spinCtrlSizer, 0, alignment | wxALL, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(spinCtrlSizer, 0, alignment | wxALL, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(spinCtrlSizer, sizer.pos, sizer.span, alignment | wxALL, ELEMENT_BORDER);
    else
        layoutError("spinbut", this);

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
/// \param sizer const SizerWrapper&
/// \param wxLC_REPORT int nStyle=
/// \param wxDefaultSize wxSize size=
/// \param id int
/// \param proportion int
/// \return wxListView*
///
/////////////////////////////////////////////////
wxListView* GroupPanel::CreateListView(wxWindow* parent, const SizerWrapper& sizer, int nStyle, wxSize size, int id, int proportion)
{
    // Create the listview and assign it to the passed sizer
    wxListView* listView = new wxListView(parent, id, wxDefaultPosition, size, nStyle);

    if (!sizer)
        mainSizer->Add(listView, proportion, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND | wxFIXED_MINSIZE, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(listView, proportion, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND | wxFIXED_MINSIZE, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(listView, sizer.pos, sizer.span, wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND | wxFIXED_MINSIZE, ELEMENT_BORDER);
    else
        layoutError("wxListView", this);

    return listView;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a treelist control.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param nStyle int
/// \param size wxSize
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return wxTreeListCtrl*
///
/////////////////////////////////////////////////
wxTreeListCtrl* GroupPanel::CreateTreeListCtrl(wxWindow* parent, const SizerWrapper& sizer, int nStyle, wxSize size, int id, int alignment, int proportion)
{
    // Create the listview and assign it to the passed sizer
    wxTreeListCtrl* listCtrl = new wxTreeListCtrl(parent, id, wxDefaultPosition, size, nStyle);
    listCtrl->SetMinClientSize(wxSize(100,200));

    if (!sizer)
        mainSizer->Add(listCtrl, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(listCtrl, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(listCtrl, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("treelist", this);

    return listCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a treelist control in the wxCode
/// variant.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param nStyle int
/// \param size wxSize
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return wxcode::wxTreeListCtrl*
///
/////////////////////////////////////////////////
wxcode::wxTreeListCtrl* GroupPanel::CreateWxcTreeListCtrl(wxWindow* parent, const SizerWrapper& sizer, int nStyle, wxSize size, int id, int alignment, int proportion)
{
    // Create the listview and assign it to the passed sizer
    wxcode::wxTreeListCtrl* listCtrl = new wxcode::wxTreeListCtrl(parent, id, wxDefaultPosition, size, nStyle);
    listCtrl->SetMinClientSize(wxSize(100,200));

    if (!sizer)
        mainSizer->Add(listCtrl, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(listCtrl, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(listCtrl, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("wxCode::TreeListCtrl", this);

    return listCtrl;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a button.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param description const wxString&
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return wxButton*
///
/////////////////////////////////////////////////
wxButton* GroupPanel::CreateButton(wxWindow* parent, const SizerWrapper& sizer, const wxString& description, int id, int alignment, int proportion)
{
    wxButton* button = new wxButton(parent, id, description);

    if (sizer.s == mainSizer || !sizer)
        mainSizer->Add(button, 0, alignment | wxALL | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(button, proportion, alignment | wxALL | wxEXPAND | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(button, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxFIXED_MINSIZE | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("button", this);

    return button;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a radio box.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param description const wxString&
/// \param choices const wxArrayString&
/// \param style int
/// \param id int
/// \param alignment int
/// \return wxRadioBox*
///
/////////////////////////////////////////////////
wxRadioBox* GroupPanel::CreateRadioBox(wxWindow* parent, const SizerWrapper& sizer, const wxString& description, const wxArrayString& choices, int style, int id, int alignment)
{
    wxRadioBox* box = new wxRadioBox(parent, id, description, wxDefaultPosition, wxDefaultSize, choices, 0, style);

    if (!sizer)
        mainSizer->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(box, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("radio", this);

    return box;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a dropdown list.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param choices const wxArrayString&
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return wxChoice*
///
/////////////////////////////////////////////////
wxChoice* GroupPanel::CreateChoices(wxWindow* parent, const SizerWrapper& sizer, const wxArrayString& choices, int id, int alignment, int proportion)
{
    wxChoice* box = new wxChoice(parent, id, wxDefaultPosition, wxDefaultSize, choices);

    if (!sizer)
        mainSizer->Add(box, 0, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox() && dynamic_cast<wxBoxSizer*>(sizer.s) && dynamic_cast<wxBoxSizer*>(sizer.s)->GetOrientation() == wxHORIZONTAL)
        sizer.s->Add(box, proportion, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(box, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("dropdown", this);

    return box;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a combobox.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param choices const wxArrayString&
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return wxComboBox*
///
/////////////////////////////////////////////////
wxComboBox* GroupPanel::CreateComboBox(wxWindow* parent, const SizerWrapper& sizer, const wxArrayString& choices, int id, int alignment, int proportion)
{
    wxComboBox* box = new wxComboBox(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, choices);

    if (!sizer)
        mainSizer->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox() && dynamic_cast<wxBoxSizer*>(sizer.s) && dynamic_cast<wxBoxSizer*>(sizer.s)->GetOrientation() == wxHORIZONTAL)
        sizer.s->Add(box, proportion, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(box, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(box, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("combobox", this);

    return box;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a progress bar.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param style int
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return wxGauge*
///
/////////////////////////////////////////////////
wxGauge* GroupPanel::CreateGauge(wxWindow* parent, const SizerWrapper& sizer, int style, int id, int alignment, int proportion)
{
    wxGauge* gauge = new wxGauge(parent, id, 100, wxDefaultPosition, wxDefaultSize, style);

    if (!sizer)
        mainSizer->Add(gauge, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(gauge, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(gauge, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("gauge", this);

    return gauge;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a static bitmap.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param filename const wxString&
/// \param id int
/// \param alignment int
/// \return wxStaticBitmap*
///
/////////////////////////////////////////////////
wxStaticBitmap* GroupPanel::CreateBitmap(wxWindow* parent, const SizerWrapper& sizer, const wxString& filename, int id, int alignment)
{
    wxStaticBitmap* bitmap = new wxStaticBitmap(parent, id, wxBitmap(filename, wxBITMAP_TYPE_ANY));

    if (!sizer)
        mainSizer->Add(bitmap, 0, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(bitmap, 0, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(bitmap, sizer.pos, sizer.span, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("bitmap", this);

    return bitmap;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a slider.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param nMin int
/// \param nMax int
/// \param nInitial int
/// \param style int
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return wxSlider*
///
/////////////////////////////////////////////////
wxSlider* GroupPanel::CreateSlider(wxWindow* parent, const SizerWrapper& sizer, int nMin, int nMax, int nInitial, int style, int id, int alignment, int proportion)
{
    wxSlider* slider = new wxSlider(parent, id, nInitial, nMin, nMax, wxDefaultPosition, wxDefaultSize, style);

    if (!sizer)
        mainSizer->Add(slider, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(slider, proportion, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(slider, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("slider", this);

    return slider;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a date picker control.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param dt const wxDateTime&
/// \param style int
/// \param id int
/// \param alignment int
/// \return wxDatePickerCtrl*
///
/////////////////////////////////////////////////
wxDatePickerCtrl* GroupPanel::CreateDatePicker(wxWindow* parent, const SizerWrapper& sizer, const wxDateTime& dt, int style, int id, int alignment)
{
    wxDatePickerCtrl* picker = new wxDatePickerCtrl(parent, id, dt, wxDefaultPosition, wxDefaultSize, style);

    if (!sizer)
        mainSizer->Add(picker, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(picker, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(picker, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("datetimepicker", this);

    return picker;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a time picker control.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param dt const wxDateTime&
/// \param id int
/// \param alignment int
/// \return wxTimePickerCtrl*
///
/////////////////////////////////////////////////
wxTimePickerCtrl* GroupPanel::CreateTimePicker(wxWindow* parent, const SizerWrapper& sizer, const wxDateTime& dt, int id, int alignment)
{
    wxTimePickerCtrl* picker = new wxTimePickerCtrl(parent, id, dt);

    if (!sizer)
        mainSizer->Add(picker, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(picker, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(picker, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("datetimepicker", this);

    return picker;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// layout for a date-time picker control.
///
/// \param parent wxWindow*
/// \param sizer const SizerWrapper&
/// \param dt const wxDateTime&
/// \param style int
/// \param id int
/// \param alignment int
/// \param proportion int
/// \return DateTimePicker*
///
/////////////////////////////////////////////////
DateTimePicker* GroupPanel::CreateDateTimePicker(wxWindow* parent, const SizerWrapper& sizer, const wxDateTime& dt, int style, int id, int alignment, int proportion)
{
    DateTimePicker* picker = new DateTimePicker(parent, id, dt, style);

    if (!sizer)
        mainSizer->Add(picker, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox() && dynamic_cast<wxBoxSizer*>(sizer.s) && dynamic_cast<wxBoxSizer*>(sizer.s)->GetOrientation() == wxHORIZONTAL)
        sizer.s->Add(picker, proportion, alignment | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.isBox())
        sizer.s->Add(picker, 0, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else if (sizer.canEmplace())
        sizer.gbs->Add(picker, sizer.pos, sizer.span, alignment | wxALL | wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, ELEMENT_BORDER);
    else
        layoutError("datetimepicker", this);

    return picker;
}






