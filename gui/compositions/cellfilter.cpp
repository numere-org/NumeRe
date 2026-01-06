/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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

#include "cellfilter.hpp"

#include "grouppanel.hpp"
#include "../guilang.hpp"
#include "../../kernel/core/utils/stringtools.hpp"

extern double g_pixelScale;

BEGIN_EVENT_TABLE(CellFilterDialog, wxDialog)
    EVT_BUTTON(-1, CellFilterDialog::OnButtonClick)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Reset a filter to its default state.
///
/// \return void
///
/////////////////////////////////////////////////
void CellFilterCondition::reset()
{
    m_type = CT_NONE;
    m_strs.clear();
    m_vals.clear();
}


/////////////////////////////////////////////////
/// \brief Evaluate the condition for a numerical
/// value. The returned pair is defined as
/// (fulfilled, index), where index references
/// the index of the comparison value (if any).
///
/// \param val const std::complex<double>&
/// \return std::pair<bool, size_t>
///
/////////////////////////////////////////////////
std::pair<bool, size_t> CellFilterCondition::eval(const std::complex<double>& val) const
{
    if (!m_vals.size() && m_type != CT_EMPTY && m_type != CT_NOT_EMPTY)
        return std::pair<bool, size_t>(false, 0);

    switch (m_type)
    {
        case CT_EQUALS_VAL:
        {
            if (std::find(m_vals.begin(), m_vals.end(), val) != m_vals.end())
                return std::pair<bool, size_t>(true, 0);

            break;
        }
        case CT_NOT_EQUALS_VAL:
        {
            if (std::find(m_vals.begin(), m_vals.end(), val) != m_vals.end())
                return std::pair<bool, size_t>(false, 0);

            return std::pair<bool, size_t>(true, 0);
        }
        case CT_EQUALS_ARRAY:
        {
            for (size_t i = 0; i < m_vals.size(); i++)
            {
                if (m_vals[i] == val)
                    return std::pair<bool, size_t>(true, i);
            }

            break;
        }
        case CT_LESS_THAN:
        {
            if (val.real() < m_vals.front().real())
                return std::pair<bool, size_t>(true, 0);

            break;
        }
        case CT_GREATER_THAN:
        {
            if (val.real() > m_vals.front().real())
                return std::pair<bool, size_t>(true, 0);

            break;
        }
        case CT_LESS_EQ_THAN:
        {
            if (val.real() <= m_vals.front().real())
                return std::pair<bool, size_t>(true, 0);

            break;
        }
        case CT_GREATER_EQ_THAN:
        {
            if (val.real() >= m_vals.front().real())
                return std::pair<bool, size_t>(true, 0);

            break;
        }
        case CT_EMPTY:
        {
            if (val != val)
                return std::pair<bool, size_t>(true, 0);

            break;
        }
        case CT_NOT_EMPTY:
        {
            if (val == val)
                return std::pair<bool, size_t>(true, 0);

            break;
        }
    }

    return std::pair<bool, size_t>(false, 0);
}


/////////////////////////////////////////////////
/// \brief Evaluate the condition for a string
/// value. The returned pair is defined as
/// (fulfilled, index), where index references
/// the index of the comparison value (if any).
///
/// \param val const wxString&
/// \return std::pair<bool, size_t>
///
/////////////////////////////////////////////////
std::pair<bool, size_t> CellFilterCondition::eval(const wxString& val) const
{
    if (!val.length() && m_type == CT_EMPTY)
        return std::pair<bool, size_t>(true, 0);

    if (!val.length()
        || (!m_strs.size() && m_type != CT_NOT_EMPTY))
        return std::pair<bool, size_t>(false, 0);

    auto removeQuotes = [](const wxString& strVal) {return strVal[0] == '"' && strVal[strVal.length()-1] == '"'
                                                    ? strVal.substr(1, strVal.length()-2) : strVal;};

    if (m_type == CT_EQUALS_STR)
    {
        if (std::find(m_strs.begin(), m_strs.end(), removeQuotes(val)) != m_strs.end())
            return std::pair<bool, size_t>(true, 0);
    }
    else if (m_type == CT_NOT_EQUALS_STR)
    {
        if (std::find(m_strs.begin(), m_strs.end(), removeQuotes(val)) != m_strs.end())
            return std::pair<bool, size_t>(false, 0);

        return std::pair<bool, size_t>(true, 0);
    }
    else if (m_type == CT_EQUALS_ARRAY)
    {
        for (size_t i = 0; i < m_strs.size(); i++)
        {
            if (m_strs[i] == removeQuotes(val))
                return std::pair<bool, size_t>(true, i);
        }
    }
    else if (m_type == CT_FIND_STR)
    {
        for (const auto& sStr : m_strs)
        {
            if (val.find(sStr) != std::string::npos)
                return std::pair<bool, size_t>(true, 0);
        }
    }
    else if (m_type == CT_NOT_FIND_STR)
    {
        for (const auto& sStr : m_strs)
        {
            if (val.find(sStr) != std::string::npos)
                return std::pair<bool, size_t>(false, 0);
        }

        return std::pair<bool, size_t>(true, 0);
    }
    else if (m_type == CT_EMPTY)
    {
        if (!removeQuotes(val).size())
            return std::pair<bool, size_t>(true, 0);
    }
    else if (m_type == CT_NOT_EMPTY)
    {
        if (removeQuotes(val).size())
            return std::pair<bool, size_t>(true, 0);
    }

    return std::pair<bool, size_t>(false, 0);
}








/////////////////////////////////////////////////
/// \brief Create the dialog.
///
/// \param parent wxWindow*
/// \param id wxWindowID
///
/////////////////////////////////////////////////
CellFilterDialog::CellFilterDialog(wxWindow* parent, CellFilterCondition cond, wxWindowID id) : wxDialog(parent, id, _guilang.get("GUI_DLG_FILTER_HEAD"), wxDefaultPosition, wxSize(640*g_pixelScale, 180*g_pixelScale))
{
    m_condition = cond;

    // Create the page containing different shader conditions
    GroupPanel* _panel = new GroupPanel(this);

    // Create the condition box
    wxStaticBoxSizer* hsizer = _panel->createGroup(_guilang.get("GUI_DLG_FILTER_CONDITION"), wxVERTICAL, _panel, _panel->getMainSizer(), 0);
    wxBoxSizer* condSizer = _panel->createGroup(wxHORIZONTAL, hsizer, 0);
    _panel->AddStaticText(hsizer->GetStaticBox(), condSizer, _guilang.get("GUI_DLG_CVS_VALUES"));

    // Define the possible comparisons
    wxArrayString lt_gt;
    lt_gt.Add("==");
    lt_gt.Add("!=");
    lt_gt.Add("<");
    lt_gt.Add("<=");
    lt_gt.Add(">");
    lt_gt.Add(">=");
    lt_gt.Add(_guilang.get("GUI_DLG_CVS_LT_GT_EQ_CONTAINS"));
    lt_gt.Add(_guilang.get("GUI_DLG_CVS_LT_GT_EQ_NOT_CONTAINS"));
    lt_gt.Add(_guilang.get("GUI_DLG_CVS_LT_GT_EMPTY"));
    lt_gt.Add(_guilang.get("GUI_DLG_CVS_LT_GT_NOT_EMPTY"));

    // Create the dropdown containing the possible comparisons
    m_lt_gt_choice = _panel->CreateChoices(hsizer->GetStaticBox(), condSizer, lt_gt);
    m_lt_gt_value = _panel->CreateTextInput(hsizer->GetStaticBox(), condSizer, _guilang.get("GUI_DLG_CVS_LT_GT_EQ_VALUE"), wxEmptyString, 0, wxID_ANY, wxSize(320, -1));

    if (cond.m_type == CellFilterCondition::CT_NONE)
        m_lt_gt_choice->SetSelection(0);
    else
    {
        // Initialize with the previous values from the filter - if any
        if (cond.m_type == CellFilterCondition::CT_EQUALS_VAL || cond.m_type == CellFilterCondition::CT_EQUALS_STR)
            m_lt_gt_choice->SetSelection(0);
        else if (cond.m_type == CellFilterCondition::CT_NOT_EQUALS_VAL || cond.m_type == CellFilterCondition::CT_NOT_EQUALS_STR)
            m_lt_gt_choice->SetSelection(1);
        else if (cond.m_type <= CellFilterCondition::CT_NOT_EMPTY)
            m_lt_gt_choice->SetSelection(cond.m_type - 3);

        if (cond.m_vals.size())
        {
            std::string sVal = cond.m_vals.size() > 1 ? toString(cond.m_vals) : toString(cond.m_vals.front());
            m_lt_gt_value->SetValue(sVal);
        }
        else if (cond.m_strs.size())
        {
            std::string sVal;

            for (size_t i = 0; i < cond.m_strs.size(); i++)
            {
                if (sVal.length())
                    sVal += ", ";

                sVal += cond.m_strs[i];
            }

            m_lt_gt_value->SetValue(cond.m_strs.size() > 1 ? "{" + sVal + "}" : sVal);
        }
    }

    // We accept multiple values here: show a description
    _panel->AddStaticText(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_LT_GT_EQ_VALUE_EXP"));

    // Create the apply and cancel buttons
    wxBoxSizer* buttonSizer = _panel->createGroup(wxHORIZONTAL);
    _panel->CreateButton(_panel, buttonSizer, _guilang.get("GUI_OPTIONS_OK"), wxID_OK);
    _panel->CreateButton(_panel, buttonSizer, _guilang.get("GUI_OPTIONS_CANCEL"), wxID_CANCEL);
}


/////////////////////////////////////////////////
/// \brief Event handler for the apply and cancel
/// buttons. Will create a valid shader if the
/// user clicked on apply.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void CellFilterDialog::OnButtonClick(wxCommandEvent& event)
{
    if (event.GetId() == wxID_OK)
    {
        // Reset the values first
        m_condition.m_strs.clear();
        m_condition.m_vals.clear();

        // Extract the value and convert multiple values into
        // a vector
        std::string val = m_lt_gt_value->GetValue().ToStdString();
        std::vector<std::string> vecVal;

        if (val.front() == '{' && val.back() == '}')
            vecVal = toStrVector(val);
        else
            vecVal.push_back(val);

        // Get the string from the dropdown
        wxString condType = m_lt_gt_choice->GetString(m_lt_gt_choice->GetSelection());

        // Determine the correct condition depending on the
        // dropdown and the type of the comparison value
        if (condType == "<")
            m_condition.m_type = CellFilterCondition::CT_LESS_THAN;
        else if (condType == ">")
            m_condition.m_type = CellFilterCondition::CT_GREATER_THAN;
        else if (condType == "<=")
            m_condition.m_type = CellFilterCondition::CT_LESS_EQ_THAN;
        else if (condType == ">=")
            m_condition.m_type = CellFilterCondition::CT_GREATER_EQ_THAN;
        else if (condType == "==" && (isConvertible(vecVal.front(), CONVTYPE_VALUE)
                                      || isConvertible(vecVal.front(), CONVTYPE_DATE_TIME)))
            m_condition.m_type = CellFilterCondition::CT_EQUALS_VAL;
        else if (condType == "==")
            m_condition.m_type = CellFilterCondition::CT_EQUALS_STR;
        else if (condType == "!=" && (isConvertible(vecVal.front(), CONVTYPE_VALUE)
                                      || isConvertible(vecVal.front(), CONVTYPE_DATE_TIME)))
            m_condition.m_type = CellFilterCondition::CT_NOT_EQUALS_VAL;
        else if (condType == "!=")
            m_condition.m_type = CellFilterCondition::CT_NOT_EQUALS_STR;
        else if (condType == _guilang.get("GUI_DLG_CVS_LT_GT_EQ_CONTAINS"))
            m_condition.m_type = CellFilterCondition::CT_FIND_STR;
        else if (condType == _guilang.get("GUI_DLG_CVS_LT_GT_EQ_NOT_CONTAINS"))
            m_condition.m_type = CellFilterCondition::CT_NOT_FIND_STR;
        else if (condType == _guilang.get("GUI_DLG_CVS_LT_GT_EMPTY"))
            m_condition.m_type = CellFilterCondition::CT_EMPTY;
        else if (condType == _guilang.get("GUI_DLG_CVS_LT_GT_NOT_EMPTY"))
            m_condition.m_type = CellFilterCondition::CT_NOT_EMPTY;

        // Insert the comparison values into the internal vectors
        for (const auto& s : vecVal)
        {
            if (isConvertible(s, CONVTYPE_DATE_TIME))
                m_condition.m_vals.push_back(to_double(StrToTime(s)));
            else if (isConvertible(s, CONVTYPE_VALUE))
                m_condition.m_vals.push_back(StrToCmplx(s));
            else
                m_condition.m_strs.push_back(s);
        }
    }

    // Return the ID of the clicked button
    EndModal(event.GetId());
}


