/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#ifndef CELLVALUESHADER_HPP
#define CELLVALUESHADER_HPP

#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/clrpicker.h>
#include <wx/statline.h>
#include "grouppanel.hpp"
#include "../../kernel/core/ui/language.hpp"
#include "../../kernel/core/utils/stringtools.hpp"

extern double g_pixelScale;



/////////////////////////////////////////////////
/// \brief Represents the basic settings needed
/// by the CellValueShader class to actually
/// calculate the cell shading.
/////////////////////////////////////////////////
struct CellValueShaderCondition
{
    enum ConditionType
    {
        CT_LESS_THAN,
        CT_LESS_EQ_THAN,
        CT_GREATER_THAN,
        CT_GREATER_EQ_THAN,
        CT_INTERVAL_RE,
        CT_INTERVAL_RE_EXCL,
        CT_INTERVAL_IM,
        CT_INTERVAL_IM_EXCL,
        CT_EQUALS_VAL,
        CT_NOT_EQUALS_VAL,
        CT_EQUALS_STR,
        CT_NOT_EQUALS_STR,
        CT_EQUALS_ARRAY,
        CT_FIND_STR,
        CT_NOT_FIND_STR
    };

    ConditionType m_type;
    std::vector<std::complex<double>> m_vals;
    std::vector<wxString> m_strs;
};


/////////////////////////////////////////////////
/// \brief A class to handle value-based cell
/// shading using a custom colorscheme and
/// custom conditions.
/////////////////////////////////////////////////
class CellValueShader
{
    private:
        std::vector<wxColour> m_colorArray;
        CellValueShaderCondition m_condition;

        /////////////////////////////////////////////////
        /// \brief Calculate the cell background colour
        /// depending on their value and using linear
        /// interpolation.
        ///
        /// \param val double
        /// \param excl bool
        /// \return wxColour
        ///
        /////////////////////////////////////////////////
        wxColour interpolateColor(double val, bool excl) const
        {
            if (isnan(val) || isinf(val))
                return *wxWHITE;

            // Scale to number of colours
            val *= m_colorArray.size() - 1 - (excl*2); // 5-2-1 => 3

            // Get index and fraction
            int index = std::floor(val); // 3
            double part = val - index;
            index += excl; // 4

            // Safety part
            if (index+1 >= (int)m_colorArray.size() || (excl && part && index+2 >= (int)m_colorArray.size()))
                return m_colorArray.back();
            else if (index < excl)
                return m_colorArray.front();

            // Calculate the interpolation
            return wxColour((1.0-part) * m_colorArray[index].Red() + part * m_colorArray[index+1].Red(),
                            (1.0-part) * m_colorArray[index].Green() + part * m_colorArray[index+1].Green(),
                            (1.0-part) * m_colorArray[index].Blue() + part * m_colorArray[index+1].Blue());
        }

    public:
        CellValueShader() {}
        CellValueShader(const std::vector<wxColour>& colors, const CellValueShaderCondition& cond) : m_colorArray(colors), m_condition(cond) {}
        CellValueShader(const CellValueShader& shader) : m_colorArray(shader.m_colorArray), m_condition(shader.m_condition) {}

        /////////////////////////////////////////////////
        /// \brief Detect, whether this shader is
        /// actually active or an empty shader.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isActive() const
        {
            return m_colorArray.size();
        }

        /////////////////////////////////////////////////
        /// \brief Calculate the colour for a numerical
        /// value.
        ///
        /// \param val const std::complex<double>&
        /// \return wxColour
        ///
        /////////////////////////////////////////////////
        wxColour getColour(const std::complex<double>& val) const
        {
            switch (m_condition.m_type)
            {
                case CellValueShaderCondition::CT_EQUALS_VAL:
                {
                    if (std::find(m_condition.m_vals.begin(), m_condition.m_vals.end(), val) != m_condition.m_vals.end())
                        return m_colorArray.front();

                    break;
                }
                case CellValueShaderCondition::CT_NOT_EQUALS_VAL:
                {
                    if (std::find(m_condition.m_vals.begin(), m_condition.m_vals.end(), val) != m_condition.m_vals.end())
                        return *wxWHITE;

                    return m_colorArray.front();
                }
                case CellValueShaderCondition::CT_EQUALS_ARRAY:
                {
                    for (size_t i = 0; i < std::min(m_colorArray.size(), m_condition.m_vals.size()); i++)
                    {
                        if (m_condition.m_vals[i] == val)
                            return m_colorArray[i];
                    }

                    break;
                }
                case CellValueShaderCondition::CT_LESS_THAN:
                {
                    if (val.real() < m_condition.m_vals.front().real())
                        return m_colorArray.front();

                    break;
                }
                case CellValueShaderCondition::CT_GREATER_THAN:
                {
                    if (val.real() > m_condition.m_vals.front().real())
                        return m_colorArray.front();

                    break;
                }
                case CellValueShaderCondition::CT_LESS_EQ_THAN:
                {
                    if (val.real() <= m_condition.m_vals.front().real())
                        return m_colorArray.front();

                    break;
                }
                case CellValueShaderCondition::CT_GREATER_EQ_THAN:
                {
                    if (val.real() >= m_condition.m_vals.front().real())
                        return m_colorArray.front();

                    break;
                }
                case CellValueShaderCondition::CT_INTERVAL_RE:
                {
                    return interpolateColor((val.real() - m_condition.m_vals.front().real())
                                            / (m_condition.m_vals.back().real()-m_condition.m_vals.front().real()), false);
                }
                case CellValueShaderCondition::CT_INTERVAL_RE_EXCL:
                {
                    return interpolateColor((val.real() - m_condition.m_vals.front().real())
                                            / (m_condition.m_vals.back().real()-m_condition.m_vals.front().real()), true);
                }
                case CellValueShaderCondition::CT_INTERVAL_IM:
                {
                    return interpolateColor((val.imag() - m_condition.m_vals.front().imag())
                                            / (m_condition.m_vals.back().imag()-m_condition.m_vals.front().imag()), false);
                }
                case CellValueShaderCondition::CT_INTERVAL_IM_EXCL:
                {
                    return interpolateColor((val.imag() - m_condition.m_vals.front().imag())
                                            / (m_condition.m_vals.back().imag()-m_condition.m_vals.front().imag()), true);
                }
            }

            return *wxWHITE;
        }

        /////////////////////////////////////////////////
        /// \brief Calulate the colour for a string
        /// value.
        ///
        /// \param strVal const wxString&
        /// \return wxColour
        ///
        /////////////////////////////////////////////////
        wxColour getColour(const wxString& strVal) const
        {
            auto removeQuotes = [](const wxString& strVal) {return strVal[0] == '"' && strVal[strVal.length()-1] == '"'
                                                            ? strVal.substr(1, strVal.length()-2) : strVal;};

            if (m_condition.m_type == CellValueShaderCondition::CT_EQUALS_STR)
            {
                if (std::find(m_condition.m_strs.begin(), m_condition.m_strs.end(), removeQuotes(strVal)) != m_condition.m_strs.end())
                    return m_colorArray.front();
            }
            else if (m_condition.m_type == CellValueShaderCondition::CT_NOT_EQUALS_STR)
            {
                if (std::find(m_condition.m_strs.begin(), m_condition.m_strs.end(), removeQuotes(strVal)) != m_condition.m_strs.end())
                    return *wxWHITE;

                return m_colorArray.front();
            }
            else if (CellValueShaderCondition::CT_EQUALS_ARRAY)
            {
                for (size_t i = 0; i < std::min(m_colorArray.size(), m_condition.m_strs.size()); i++)
                {
                    if (m_condition.m_strs[i] == removeQuotes(strVal))
                        return m_colorArray[i];
                }
            }
            else if (m_condition.m_type == CellValueShaderCondition::CT_FIND_STR)
            {
                for (const auto& sStr : m_condition.m_strs)
                {
                    if (strVal.find(sStr) != std::string::npos)
                        return m_colorArray.front();
                }
            }
            else if (m_condition.m_type == CellValueShaderCondition::CT_NOT_FIND_STR)
            {
                for (const auto& sStr : m_condition.m_strs)
                {
                    if (strVal.find(sStr) != std::string::npos)
                        return *wxWHITE;
                }

                return m_colorArray.front();
            }

            return *wxWHITE;
        }
};


/////////////////////////////////////////////////
/// \brief This class implements the dialog for
/// choosing the shader properties of the
/// selected cells.
/////////////////////////////////////////////////
class CellValueShaderDialog : public wxDialog
{
    private:
        CellValueShader m_shader;
        wxNotebook* m_book;

        // Single values page attributes
        wxChoice* m_lt_gt_choice;
        TextField* m_lt_gt_value;
        wxColourPickerCtrl* m_lt_gt_colour;

        // Category values page attributes
        static const size_t m_field_count = 16;
        TextField* m_category_value[m_field_count];
        wxColourPickerCtrl* m_category_colour[m_field_count];

        // Intervall page attributes
        TextField* m_interval_start;
        TextField* m_interval_end;
        wxColourPickerCtrl* m_interval_col[3];

        // Excl-intervall page attributes
        TextField* m_interval_start_excl;
        TextField* m_interval_end_excl;
        wxColourPickerCtrl* m_interval_col_excl[5];

        // Statics to be updated everytime the
        // user clicks on "Apply"
        static wxColour LEVELCOLOUR;
        static wxColour CATEGORYCOLOUR[m_field_count];
        static wxColour LOWERCOLOUR;
        static wxColour MINCOLOUR;
        static wxColour MEDCOLOUR;
        static wxColour MAXCOLOUR;
        static wxColour HIGHERCOLOUR;
        static bool USEALLCOLOURS;

        const size_t FIELDWIDTH = 120;


        /////////////////////////////////////////////////
        /// \brief This enumeration defines the IDs for
        /// the possible selectable pages.
        /////////////////////////////////////////////////
        enum
        {
            LT_GT_EQ,
            CATEGORY,
            INTERVAL,
            INTERVAL_EXCL
        };

        /////////////////////////////////////////////////
        /// \brief This private member function creates
        /// the single value comparison page.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void createLtGtPage()
        {
            GroupPanel* _panel = new GroupPanel(m_book);

            // Create the condition box
            wxStaticBoxSizer* hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_CONDITION"), wxVERTICAL, _panel,  _panel->getMainSizer(), 0);
            wxBoxSizer* condSizer = _panel->createGroup(wxHORIZONTAL, hsizer, 0);
            _panel->AddStaticText(hsizer->GetStaticBox(), condSizer, _guilang.get("GUI_DLG_CVS_VALUES"));

            // Define the possible comparisons
            wxArrayString lt_gt;
            lt_gt.Add("<");
            lt_gt.Add("<=");
            lt_gt.Add(">");
            lt_gt.Add(">=");
            lt_gt.Add("==");
            lt_gt.Add("!=");
            lt_gt.Add(_guilang.get("GUI_DLG_CVS_LT_GT_EQ_CONTAINS"));
            lt_gt.Add(_guilang.get("GUI_DLG_CVS_LT_GT_EQ_NOT_CONTAINS"));

            // Create the dropdown containing the possible comparisons
            m_lt_gt_choice = _panel->CreateChoices(hsizer->GetStaticBox(), condSizer, lt_gt);
            m_lt_gt_choice->SetSelection(0);

            m_lt_gt_value = _panel->CreateTextInput(hsizer->GetStaticBox(), condSizer, _guilang.get("GUI_DLG_CVS_LT_GT_EQ_VALUE"), wxEmptyString, 0, wxID_ANY, wxSize(320, -1));

            // We accept multiple values here: show a description
            _panel->AddStaticText(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_LT_GT_EQ_VALUE_EXP"));

            hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_COLORS"), wxHORIZONTAL, _panel, _panel->getMainSizer(), 0);
            m_lt_gt_colour = new wxColourPickerCtrl(hsizer->GetStaticBox(), wxID_ANY);
            m_lt_gt_colour->SetColour(LEVELCOLOUR);
            hsizer->Add(m_lt_gt_colour, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

            m_book->AddPage(_panel, _guilang.get("GUI_DLG_CVS_LT_GT_EQ"));
        }

        /////////////////////////////////////////////////
        /// \brief This private member function creates
        /// the category value comparison page.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void createCategoryPage()
        {
            GroupPanel* _panel = new GroupPanel(m_book);

            wxStaticBoxSizer* vsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_COLORS"), wxVERTICAL, _panel, _panel->getMainSizer(), 0);

            // Create the array of colour-value pairs
            for (size_t i = 0; i < m_field_count; i++)
            {
                wxBoxSizer* hsizer = _panel->createGroup(wxHORIZONTAL, vsizer, 0);
                m_category_colour[i] = new wxColourPickerCtrl(vsizer->GetStaticBox(), wxID_ANY);
                m_category_colour[i]->SetColour(CATEGORYCOLOUR[i]);
                hsizer->Add(m_category_colour[i], 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

                m_category_value[i] = _panel->CreateTextInput(vsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_CATEGORY_VALUE"), wxEmptyString, 0, wxID_ANY, wxSize(-1, -1), wxALIGN_CENTER_VERTICAL, 1);
            }

            // Enable scrolling
            _panel->SetScrollbars(0, 20, 0, 200);

            m_book->AddPage(_panel, _guilang.get("GUI_DLG_CVS_CATEGORY_EQ"));
        }

        /////////////////////////////////////////////////
        /// \brief This private member function creates
        /// the interval page.
        ///
        /// \param minVal double
        /// \param maxVal double
        /// \return void
        ///
        /////////////////////////////////////////////////
        void createIntervalPage(double minVal, double maxVal)
        {
            GroupPanel* _panel = new GroupPanel(m_book);

            wxStaticBoxSizer* hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_CONDITION"), wxHORIZONTAL, _panel,  _panel->getMainSizer(), 0);
            // Create the condition box
            m_interval_start = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_START"), toString(minVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));
            _panel->AddStaticText(hsizer->GetStaticBox(), hsizer, "<=   " + _guilang.get("GUI_DLG_CVS_VALUES") + "   <=");
            m_interval_end = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_END"), toString(maxVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));

            hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_COLORS"), wxVERTICAL, _panel, _panel->getMainSizer(), 0);
            wxBoxSizer* colorsizer = _panel->createGroup(wxHORIZONTAL, hsizer, 0);

            // Create three colour pickers
            for (size_t i = 0; i < 3; i++)
                m_interval_col[i] = new wxColourPickerCtrl(hsizer->GetStaticBox(), wxID_ANY);

            // Set the statics as colours and change the enable state
            // of the middle picker
            m_interval_col[0]->SetColour(MINCOLOUR);
            m_interval_col[1]->SetColour(MEDCOLOUR);
            m_interval_col[1]->Enable(USEALLCOLOURS);
            m_interval_col[2]->SetColour(MAXCOLOUR);

            // Add the pickers to the sizer
            for (size_t i = 0; i < 3; i++)
                colorsizer->Add(m_interval_col[i], 0, wxALIGN_CENTER | wxALL, 5);

            wxCheckBox* check = _panel->CreateCheckBox(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_COLOR_COUNT", "3"));
            check->SetValue(USEALLCOLOURS);

            m_book->AddPage(_panel, _guilang.get("GUI_DLG_CVS_INTERVAL"));
        }

        /////////////////////////////////////////////////
        /// \brief This private member function creates
        /// the excluded interval page.
        ///
        /// \param minVal double
        /// \param maxVal double
        /// \return void
        ///
        /////////////////////////////////////////////////
        void createIntervalExclPage(double minVal, double maxVal)
        {
            GroupPanel* _panel = new GroupPanel(m_book);

            // Create the condition box
            wxStaticBoxSizer* hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_CONDITION"), wxHORIZONTAL, _panel,  _panel->getMainSizer(), 0);
            m_interval_start_excl = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_EXCL_START"), toString(minVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));
            _panel->AddStaticText(hsizer->GetStaticBox(), hsizer, "<=   " + _guilang.get("GUI_DLG_CVS_VALUES") + "   <=");
            m_interval_end_excl = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_EXCL_END"), toString(maxVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));

            hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_COLORS"), wxVERTICAL, _panel, _panel->getMainSizer(), 0);
            wxBoxSizer* colorsizer = _panel->createGroup(wxHORIZONTAL, hsizer, 0);

            // Create 5 colour pickers
            for (size_t i = 0; i < 5; i++)
                m_interval_col_excl[i] = new wxColourPickerCtrl(hsizer->GetStaticBox(), wxID_ANY);

            // Set the statics as colours and change the enable state
            // of the middle picker
            m_interval_col_excl[0]->SetColour(LOWERCOLOUR);
            m_interval_col_excl[1]->SetColour(MINCOLOUR);
            m_interval_col_excl[2]->SetColour(MEDCOLOUR);
            m_interval_col_excl[2]->Enable(USEALLCOLOURS);
            m_interval_col_excl[3]->SetColour(MAXCOLOUR);
            m_interval_col_excl[4]->SetColour(HIGHERCOLOUR);

            // Add the first picker followed by a separating line
            colorsizer->Add(m_interval_col_excl[0], 0, wxALIGN_CENTER | wxALL, 5);
            wxStaticLine* line = new wxStaticLine(hsizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVERTICAL);
            colorsizer->Add(line, 0, wxEXPAND | wxALL, 5);

            // Add the middle three pickers
            for (size_t i = 1; i < 4; i++)
                colorsizer->Add(m_interval_col_excl[i], 0, wxALIGN_CENTER | wxALL, 5);

            // Add anoother separating line and the last picker
            line = new wxStaticLine(hsizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVERTICAL);
            colorsizer->Add(line, 0, wxEXPAND | wxALL, 5);
            colorsizer->Add(m_interval_col_excl[4], 0, wxALIGN_CENTER | wxALL, 5);

            wxCheckBox* check = _panel->CreateCheckBox(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_COLOR_COUNT", "5"));
            check->SetValue(USEALLCOLOURS);

            m_book->AddPage(_panel, _guilang.get("GUI_DLG_CVS_INTERVAL_EXCL"));
        }

        /////////////////////////////////////////////////
        /// \brief Event handler for the colour count
        /// checkboxes.
        ///
        /// \param event wxCommandEvent&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void OnCheckBox(wxCommandEvent& event)
        {
            if (m_book->GetSelection() == INTERVAL)
                m_interval_col[1]->Enable(event.IsChecked());
            else if (m_book->GetSelection() == INTERVAL_EXCL)
                m_interval_col_excl[2]->Enable(event.IsChecked());
        }


    public:
        /////////////////////////////////////////////////
        /// \brief Create the dialog.
        ///
        /// \param parent wxWindow*
        /// \param minVal double
        /// \param maxVal double
        /// \param id wxWindowID
        ///
        /////////////////////////////////////////////////
        CellValueShaderDialog(wxWindow* parent, double minVal, double maxVal, wxWindowID id = wxID_ANY) : wxDialog(parent, id, _guilang.get("GUI_DLG_CVS_HEAD"))
        {
            wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
            SetSizer(vsizer);
            m_book = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(640*g_pixelScale, 320*g_pixelScale));

            // Create the various pages containing different shader conditions
            createLtGtPage();
            createCategoryPage();
            createIntervalPage(minVal, maxVal);
            createIntervalExclPage(minVal, maxVal);

            vsizer->Add(m_book, 0, wxALL, 5);

            // Select the single condition page
            m_book->SetSelection(LT_GT_EQ);

            // Create the apply and cancel buttons
            wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
            vsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 0);
            wxButton* okButton = new wxButton(this, wxID_OK, _guilang.get("GUI_OPTIONS_OK"), wxDefaultPosition, wxDefaultSize, 0);
            wxButton* cancelButton = new wxButton(this, wxID_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"), wxDefaultPosition, wxDefaultSize, 0);
            buttonSizer->Add(okButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
            buttonSizer->Add(cancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
            vsizer->SetSizeHints(this);
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
        void OnButtonClick(wxCommandEvent& event)
        {
            if (event.GetId() == wxID_OK)
            {
                // Prepare the needed internals for the
                // shader
                CellValueShaderCondition cond;
                std::vector<wxColour> colors;

                int sel = m_book->GetSelection();

                switch (sel)
                {
                    case LT_GT_EQ:
                    {
                        // Extract the value and convert multiple values into
                        // a vector
                        std::string val = m_lt_gt_value->GetValue().ToStdString();

                        if (!val.length())
                            break;

                        std::vector<std::string> vecVal;

                        if (val.front() == '{' && val.back() == '}')
                            vecVal = toStrVector(val);
                        else
                            vecVal.push_back(val);

                        if (!vecVal.size())
                            break;

                        // Get colour and update the statics
                        colors.push_back(m_lt_gt_colour->GetColour());
                        LEVELCOLOUR = m_lt_gt_colour->GetColour();

                        // Get the string from the dropdown
                        wxString condType = m_lt_gt_choice->GetString(m_lt_gt_choice->GetSelection());

                        // Determine the correct condition depending on the
                        // dropdown and the type of the comparison value
                        if (condType == "<")
                            cond.m_type = CellValueShaderCondition::CT_LESS_THAN;
                        else if (condType == ">")
                            cond.m_type = CellValueShaderCondition::CT_GREATER_THAN;
                        else if (condType == "<=")
                            cond.m_type = CellValueShaderCondition::CT_LESS_EQ_THAN;
                        else if (condType == ">=")
                            cond.m_type = CellValueShaderCondition::CT_GREATER_EQ_THAN;
                        else if (condType == "==" && (isConvertible(vecVal.front(), CONVTYPE_VALUE)
                                                      || isConvertible(vecVal.front(), CONVTYPE_DATE_TIME)))
                            cond.m_type = CellValueShaderCondition::CT_EQUALS_VAL;
                        else if (condType == "==")
                            cond.m_type = CellValueShaderCondition::CT_EQUALS_STR;
                        else if (condType == "!=" && (isConvertible(vecVal.front(), CONVTYPE_VALUE)
                                                      || isConvertible(vecVal.front(), CONVTYPE_DATE_TIME)))
                            cond.m_type = CellValueShaderCondition::CT_NOT_EQUALS_VAL;
                        else if (condType == "!=")
                            cond.m_type = CellValueShaderCondition::CT_NOT_EQUALS_STR;
                        else if (condType == _guilang.get("GUI_DLG_CVS_LT_GT_EQ_CONTAINS"))
                            cond.m_type = CellValueShaderCondition::CT_FIND_STR;
                        else if (condType == _guilang.get("GUI_DLG_CVS_LT_GT_EQ_NOT_CONTAINS"))
                            cond.m_type = CellValueShaderCondition::CT_NOT_FIND_STR;

                        // Insert the comparison values into the internal vectors
                        for (const auto& s : vecVal)
                        {
                            if (isConvertible(s, CONVTYPE_DATE_TIME))
                                cond.m_vals.push_back(to_double(StrToTime(s)));
                            else if (isConvertible(s, CONVTYPE_VALUE))
                                cond.m_vals.push_back(StrToCmplx(s));
                            else
                                cond.m_strs.push_back(s);
                        }

                        break;
                    }
                    case CATEGORY:
                    {
                        cond.m_type = CellValueShaderCondition::CT_EQUALS_ARRAY;

                        // Extract the values and colours
                        for (size_t i = 0; i < m_field_count; i++)
                        {
                            std::string val = m_category_value[i]->GetValue().ToStdString();

                            if (!val.length())
                                continue;

                            // Get colour and update the statics
                            colors.push_back(m_category_colour[i]->GetColour());
                            CATEGORYCOLOUR[i] = m_category_colour[i]->GetColour();

                            if (isConvertible(val, CONVTYPE_DATE_TIME))
                                cond.m_vals.push_back(to_double(StrToTime(val)));
                            else if (isConvertible(val, CONVTYPE_VALUE))
                                cond.m_vals.push_back(StrToCmplx(val));
                            else
                                cond.m_vals.push_back(NAN);

                            cond.m_strs.push_back(val);
                        }

                        break;
                    }
                    case INTERVAL:
                    {
                        // Get the colours
                        for (size_t i = 0; i < 3; i++)
                        {
                            if (m_interval_col[i]->IsEnabled())
                                colors.push_back(m_interval_col[i]->GetColour());
                        }

                        // Update statics
                        MINCOLOUR = m_interval_col[0]->GetColour();

                        if (m_interval_col[1]->IsEnabled())
                            MEDCOLOUR = m_interval_col[1]->GetColour();

                        MAXCOLOUR = m_interval_col[2]->GetColour();
                        USEALLCOLOURS = m_interval_col[1]->IsEnabled();

                        // Get the values
                        std::string val_start = m_interval_start->GetValue().ToStdString();
                        std::string val_end = m_interval_end->GetValue().ToStdString();

                        // Set the correct condition
                        cond.m_type = CellValueShaderCondition::CT_INTERVAL_RE;

                        // Convert the values into internal types
                        if (isConvertible(val_start, CONVTYPE_DATE_TIME))
                            cond.m_vals.push_back(to_double(StrToTime(val_start)));
                        else if (isConvertible(val_start, CONVTYPE_VALUE))
                            cond.m_vals.push_back(StrToCmplx(val_start));
                        else
                            cond.m_vals.push_back(NAN);

                        if (isConvertible(val_end, CONVTYPE_DATE_TIME))
                            cond.m_vals.push_back(to_double(StrToTime(val_end)));
                        else if (isConvertible(val_end, CONVTYPE_VALUE))
                            cond.m_vals.push_back(StrToCmplx(val_end));
                        else
                            cond.m_vals.push_back(NAN);

                        break;
                    }
                    case INTERVAL_EXCL:
                    {
                        // Get the colours
                        for (size_t i = 0; i < 5; i++)
                        {
                            if (m_interval_col_excl[i]->IsEnabled())
                                colors.push_back(m_interval_col_excl[i]->GetColour());
                        }

                        // Update statics
                        LOWERCOLOUR = m_interval_col_excl[0]->GetColour();
                        MINCOLOUR = m_interval_col_excl[1]->GetColour();

                        if (m_interval_col_excl[2]->IsEnabled())
                            MEDCOLOUR = m_interval_col_excl[2]->GetColour();

                        MAXCOLOUR = m_interval_col_excl[3]->GetColour();
                        HIGHERCOLOUR = m_interval_col_excl[4]->GetColour();
                        USEALLCOLOURS = m_interval_col_excl[2]->IsEnabled();

                        // Get the values
                        std::string val_start = m_interval_start_excl->GetValue().ToStdString();
                        std::string val_end = m_interval_end_excl->GetValue().ToStdString();

                        // Set the correct condition
                        cond.m_type = CellValueShaderCondition::CT_INTERVAL_RE_EXCL;

                        // Convert the values into internal types
                        if (isConvertible(val_start, CONVTYPE_DATE_TIME))
                            cond.m_vals.push_back(to_double(StrToTime(val_start)));
                        else if (isConvertible(val_start, CONVTYPE_VALUE))
                            cond.m_vals.push_back(StrToCmplx(val_start));
                        else
                            cond.m_vals.push_back(NAN);

                        if (isConvertible(val_end, CONVTYPE_DATE_TIME))
                            cond.m_vals.push_back(to_double(StrToTime(val_end)));
                        else if (isConvertible(val_end, CONVTYPE_VALUE))
                            cond.m_vals.push_back(StrToCmplx(val_end));
                        else
                            cond.m_vals.push_back(NAN);

                        break;
                    }
                }

                m_shader = CellValueShader(colors, cond);
            }

            // Return the ID of the clicked button
            EndModal(event.GetId());
        }

        /////////////////////////////////////////////////
        /// \brief Get a reference to the internally
        /// available shader instance.
        ///
        /// \return const CellValueShader&
        ///
        /////////////////////////////////////////////////
        const CellValueShader& getShader() const
        {
            return m_shader;
        }

        DECLARE_EVENT_TABLE();
};


BEGIN_EVENT_TABLE(CellValueShaderDialog, wxDialog)
    EVT_BUTTON(-1, CellValueShaderDialog::OnButtonClick)
    EVT_CHECKBOX(-1, CellValueShaderDialog::OnCheckBox)
END_EVENT_TABLE()


wxColour CellValueShaderDialog::LEVELCOLOUR  = wxColour(128,128,255);
wxColour CellValueShaderDialog::LOWERCOLOUR  = wxColour(64,64,64);
wxColour CellValueShaderDialog::MINCOLOUR    = wxColour(128,0,0);
wxColour CellValueShaderDialog::MEDCOLOUR    = wxColour(255,128,0);
wxColour CellValueShaderDialog::MAXCOLOUR    = wxColour(255,255,128);
wxColour CellValueShaderDialog::HIGHERCOLOUR = wxColour(255,255,255);
bool CellValueShaderDialog::USEALLCOLOURS = false;

wxColour fromHSL(unsigned hue, double saturation, double lightness)
{
    double C = (1.0 - std::abs(2*lightness-1)) * saturation;
    double X = C * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
    double m = lightness - C/2.0;

    if (hue < 60)
        return wxColour((C+m)*255, (X+m)*255, m*255);

    if (hue < 120)
        return wxColour((X+m)*255, (C+m)*255, m*255);

    if (hue < 180)
        return wxColour(m*255, (C+m)*255, (X+m)*255);

    if (hue < 240)
        return wxColour(m*255, (X+m)*255, (C+m)*255);

    if (hue < 300)
        return wxColour((X+m)*255, m*255, (C+m)*255);

    return wxColour((C+m)*255, m*255, (X+m)*255);
}


//static wxColour goodBase = fromHSL(132, 0.5625, 0.8583);
//static wxColour neutralBase = fromHSL(48, 1, 0.8042);
//static wxColour badBase = fromHSL(352, 1, 0.8917);
//static wxColour infoBase = fromHSL(196, 0.725, 0.8583);

wxColour CellValueShaderDialog::CATEGORYCOLOUR[16] = {
    fromHSL(132, 0.5625, 0.8583), fromHSL(48, 1, 0.8042), fromHSL(352, 1, 0.8917), fromHSL(196, 0.725, 0.8583),
    fromHSL(132, 0.5625, 0.8583*0.8), fromHSL(48, 1, 0.8042*0.8), fromHSL(352, 1, 0.8917*0.8), fromHSL(196, 0.725, 0.8583*0.8),
    fromHSL(132, 0.5625, 0.8583*0.6), fromHSL(48, 1, 0.8042*0.6), fromHSL(352, 1, 0.8917*0.6), fromHSL(196, 0.725, 0.8583*0.6),
    fromHSL(132, 0.5625, 0.8583*0.4), fromHSL(48, 1, 0.8042*0.4), fromHSL(352, 1, 0.8917*0.4), fromHSL(196, 0.725, 0.8583*0.4)};

#endif // CELLVALUESHADER_HPP




