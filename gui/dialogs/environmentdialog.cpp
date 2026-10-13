/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#include <wx/statline.h>

#include "environmentdialog.hpp"
#include "../stringconv.hpp"
#include "../guilang.hpp"
#include "../globals.hpp"



#define BTN_REMOVE_ALL 10000
#define BTN_REMOVE 10001
#define BTN_ADD_UPDATE 10002

BEGIN_EVENT_TABLE(EnvironmentDialog, wxDialog)
    EVT_BUTTON(-1, EnvironmentDialog::OnButtonClick)
    EVT_TREELIST_SELECTION_CHANGED(-1, EnvironmentDialog::OnItemSelect)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Construct an EnvironmentDialog.
///
/// \param parent wxWindow*
/// \param envFilePath const wxString&
///
/////////////////////////////////////////////////
EnvironmentDialog::EnvironmentDialog(wxWindow* parent, const wxString& envFilePath)
    : wxDialog(parent, wxID_ANY, _guilang.get("GUI_ENV_HEAD"), wxDefaultPosition, wxSize(600*g_pixelScale, 400*g_pixelScale), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);

    m_envVars = panel->CreateTreeListCtrl(panel, panel->getMainSizer());
    m_envVars->AppendColumn(_guilang.get("GUI_ENV_NAME"));
    m_envVars->AppendColumn(_guilang.get("GUI_ENV_VALUE"));

    wxBoxSizer* inputSizer = panel->createGroup(wxHORIZONTAL);

    m_varName = panel->CreateTextInput(panel, inputSizer, wxEmptyString, "__var_name__", 0, wxID_ANY, wxSize(200*g_pixelScale,-1));
    m_varValue = panel->CreateTextInput(panel, inputSizer, "= ", "", 0, wxID_ANY, wxSize(300*g_pixelScale,-1), wxALIGN_CENTER_VERTICAL, 1);

    wxBoxSizer* controlSizer = panel->createGroup(wxHORIZONTAL);

    panel->CreateButton(panel, controlSizer, _guilang.get("GUI_ENV_REMOVE_ALL"), BTN_REMOVE_ALL);
    panel->CreateButton(panel, controlSizer, _guilang.get("GUI_ENV_REMOVE"), BTN_REMOVE);
    panel->CreateButton(panel, controlSizer, _guilang.get("GUI_ENV_ADD_UPDATE"), BTN_ADD_UPDATE);

    // Add a static line (i.e. the default separator)
    wxStaticLine* line = new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHORIZONTAL);
    panel->getMainSizer()->Add(line, 0, wxEXPAND | wxALL, 5);

    wxBoxSizer* buttonSizer = panel->createGroup(wxHORIZONTAL);
    panel->CreateButton(panel, buttonSizer, "Cancel", wxID_CANCEL);
    panel->CreateButton(panel, buttonSizer, "OK", wxID_OK);

    m_environment.load(wxToUtf8(envFilePath));

    if (m_environment.getVars().size())
        populate();
}


/////////////////////////////////////////////////
/// \brief Populate the treelist with the
/// available variables.
///
/// \return void
///
/////////////////////////////////////////////////
void EnvironmentDialog::populate()
{
    wxTreeListItem root = m_envVars->GetRootItem();

    for (const auto& iter : m_environment.getVars())
    {
        wxTreeListItem currentItem = m_envVars->AppendItem(root, iter.first);
        m_envVars->SetItemText(currentItem, 1, wxFromUtf8(iter.second.print()));
    }
}


/////////////////////////////////////////////////
/// \brief Event handler for clicking a button.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void EnvironmentDialog::OnButtonClick(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case BTN_REMOVE_ALL:
            //  Remove all declared variables
            m_environment.clear();
            m_envVars->DeleteAllItems();
            break;

        case BTN_REMOVE:
            // Remove a single declared variable
            {
                wxString varName;

                if (m_envVars->GetSelection().IsOk())
                    varName = m_envVars->GetItemText(m_envVars->GetSelection());
                else
                    varName = m_varName->GetValue();

                if (varName.length())
                {
                    bool removed = m_environment.removeVar(wxToUtf8(varName));

                    if (removed)
                    {
                        for (wxTreeListItem item = m_envVars->GetFirstChild(m_envVars->GetRootItem());
                             item.IsOk();
                             item = m_envVars->GetNextSibling(item))
                        {
                            if (m_envVars->GetItemText(item) == varName)
                            {
                                m_envVars->DeleteItem(item);
                                break;
                            }
                        }
                    }
                }
            }

            break;

        case BTN_ADD_UPDATE:
            // Add or update a declared variable
            {
                std::string varName = wxToUtf8(m_varName->GetValue());
                std::string varValue = wxToUtf8(m_varValue->GetValue());

                if (m_environment.addVar(varName, varValue))
                {
                    m_varName->SetValue(varName);
                    m_varValue->SetValue(wxFromUtf8(varValue));

                    m_envVars->DeleteAllItems();

                    populate();
                }
                else
                    wxMessageBox(_guilang.get("GUI_ENV_CREATION_ERROR"),
                                 _guilang.get("GUI_ENV_CREATION_ERROR_HEAD"),
                                 wxCENTRE | wxICON_ERROR | wxOK, this);
            }

            break;

        case wxID_OK:
            // Accept the changes
            m_environment.save();
            EndModal(wxID_OK);
            break;

        case wxID_CANCEL:
            // Dismiss the changes
            EndModal(wxID_CANCEL);
            break;
    }
}


/////////////////////////////////////////////////
/// \brief Event handler for selecting a variable
/// in the tree list.
///
/// \param event wxTreeListEvent&
/// \return void
///
/////////////////////////////////////////////////
void EnvironmentDialog::OnItemSelect(wxTreeListEvent& event)
{
    if (event.GetItem().IsOk())
    {
        m_varName->SetValue(m_envVars->GetItemText(event.GetItem(), 0));
        m_varValue->SetValue(m_envVars->GetItemText(event.GetItem(), 1));
    }
}


