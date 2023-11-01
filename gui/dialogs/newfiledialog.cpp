/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2023  Erik Haenel et al.

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

#include "newfiledialog.hpp"
#include "../compositions/grouppanel.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../globals.hpp"

#define IMAGESIZEPIX 64

BEGIN_EVENT_TABLE(NewFileDialog, wxDialog)
    EVT_LIST_ITEM_SELECTED(-1, NewFileDialog::OnSelectItem)
    EVT_LIST_ITEM_ACTIVATED(-1, NewFileDialog::OnActivateItem)
    EVT_BUTTON(ID_NEWFILE_OK, NewFileDialog::OnOK)
    EVT_BUTTON(ID_NEWFILE_ABORT, NewFileDialog::OnAbort)
    EVT_CLOSE(NewFileDialog::OnClose)
END_EVENT_TABLE()

extern Language _guilang;

/////////////////////////////////////////////////
/// \brief Constructor for the new file dialog.
///
/// \param parent wxWindow*
/// \param iconPath const wxString&
///
/////////////////////////////////////////////////
NewFileDialog::NewFileDialog(wxWindow* parent, const wxString& iconPath) : wxDialog(parent, wxID_ANY, "NumeRe: " + _guilang.get("GUI_TB_NEW_SELECT"), wxDefaultPosition, wxSize(640*g_pixelScale, 500*g_pixelScale), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    m_selectedFileType = FILE_NOTYPE;
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

    // Create a grouped panel
    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC, false);

    // Create the main UI elements
    m_fileTypes = panel->CreateListView(panel, panel->getMainSizer(), wxLC_ICON | wxLC_SINGLE_SEL);
    m_description = panel->CreateTextInput(panel, panel->getMainSizer(), wxEmptyString, wxEmptyString,
                                           wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH, wxID_ANY, wxSize(420*g_pixelScale, -1));

    // Set the default text of the description
    wxString descText = _guilang.get("GUI_NEWDLG_DEFAULT");
    m_description->SetDefaultStyle(wxTextAttr(wxColour(128,128,128), wxNullColour, m_description->GetFont().Bold()));
    m_description->AppendText(descText.substr(0, descText.find('\n')));
    m_description->SetDefaultStyle(wxTextAttr(wxColour(128,128,128), wxNullColour, m_description->GetFont()));
    m_description->AppendText(descText.substr(descText.find('\n')));
    m_description->SetHighlightColour(wxColour(128,128,128));

    // Create an image list containing the file types
    wxImageList* images = new wxImageList(IMAGESIZEPIX, IMAGESIZEPIX);

    wxIcon nscr(iconPath + "/nscr.png", wxBITMAP_TYPE_PNG, IMAGESIZEPIX, IMAGESIZEPIX);
    wxIcon nprc(iconPath + "/nprc.png", wxBITMAP_TYPE_PNG, IMAGESIZEPIX, IMAGESIZEPIX);
    wxIcon nlyt(iconPath + "/nlyt.png", wxBITMAP_TYPE_PNG, IMAGESIZEPIX, IMAGESIZEPIX);
    wxIcon napp(iconPath + "/napp.png", wxBITMAP_TYPE_PNG, IMAGESIZEPIX, IMAGESIZEPIX);
    wxIcon generic(iconPath + "/generic.png", wxBITMAP_TYPE_PNG, IMAGESIZEPIX, IMAGESIZEPIX);

    images->Add(nscr);
    images->Add(nprc);
    images->Add(nlyt);
    images->Add(napp);
    images->Add(generic);

    m_fileTypes->AssignImageList(images, wxIMAGE_LIST_NORMAL);

    // Fill the file type descriptions
    m_fileTypeDesc.Add(_guilang.get("GUI_NEWDLG_NSCR"));
    m_fileTypeDesc.Add(_guilang.get("GUI_NEWDLG_NPRC"));
    m_fileTypeDesc.Add(_guilang.get("GUI_NEWDLG_LAYOUT"));
    m_fileTypeDesc.Add(_guilang.get("GUI_NEWDLG_NAPP"));
    m_fileTypeDesc.Add(_guilang.get("GUI_NEWDLG_EMPTYFILE"));

    // Insert the file types into the list view
    for (size_t i = 0; i < m_fileTypeDesc.size(); i++)
    {
        m_fileTypes->InsertItem(i, m_fileTypeDesc[i].substr(0, m_fileTypeDesc[i].find('\n')), i);
    }

    // Create buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_okButton = panel->CreateButton(this, buttonSizer, "OK", ID_NEWFILE_OK);
    panel->CreateButton(this, buttonSizer, "Cancel", ID_NEWFILE_ABORT);

    m_okButton->Enable(false);

    vsizer->Add(panel, 1, wxEXPAND, 0);
    vsizer->Add(buttonSizer, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_HORIZONTAL, 0);

    SetSizer(vsizer);
    Layout();
    CentreOnParent();
}


/////////////////////////////////////////////////
/// \brief Common handler used, when the user
/// selects or activates one of the displayed
/// list elements.
///
/// \param itemId int
/// \return void
///
/////////////////////////////////////////////////
void NewFileDialog::selectItem(int itemId)
{
    m_description->Clear();
    m_description->SetMarkupText(m_fileTypeDesc[itemId].substr(m_fileTypeDesc[itemId].find('\n')+1));

    if (itemId == 0)
        m_selectedFileType = FILE_NSCR;
    else if (itemId == 1)
        m_selectedFileType = FILE_NPRC;
    else if (itemId == 2)
        m_selectedFileType = FILE_NLYT;
    else if (itemId == 3)
        m_selectedFileType = FILE_NAPP;
    else if (itemId == 4)
        m_selectedFileType = FILE_NONSOURCE;
}


/////////////////////////////////////////////////
/// \brief Event handler used, when the user
/// clicks on an item in the displayed list.
///
/// \param event wxListEvent&
/// \return void
///
/////////////////////////////////////////////////
void NewFileDialog::OnSelectItem(wxListEvent& event)
{
    selectItem(event.GetIndex());
    m_okButton->Enable();
}


/////////////////////////////////////////////////
/// \brief Event handler used, when the user
/// double-clicks on an item in the displayed
/// list.
///
/// \param event wxListEvent&
/// \return void
///
/////////////////////////////////////////////////
void NewFileDialog::OnActivateItem(wxListEvent& event)
{
    selectItem(event.GetIndex());
    EndModal(wxID_OK);
}


/////////////////////////////////////////////////
/// \brief Event handler for the cancel button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NewFileDialog::OnAbort(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}


/////////////////////////////////////////////////
/// \brief Event handler for the closing
/// cross-mark of the window.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void NewFileDialog::OnClose(wxCloseEvent& event)
{
    EndModal(wxID_CANCEL);
}


/////////////////////////////////////////////////
/// \brief Event handler for the OK button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void NewFileDialog::OnOK(wxCommandEvent& event)
{
    if (m_selectedFileType == FILE_NOTYPE)
        EndModal(wxID_CANCEL);
    else
        EndModal(wxID_OK);
}


/////////////////////////////////////////////////
/// \brief Interface function to obtain the
/// selected file type.
///
/// \return FileFilterType
///
/////////////////////////////////////////////////
FileFilterType NewFileDialog::GetSelection() const
{
    return m_selectedFileType;
}

