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
        CT_GREATER_THAN,
        CT_INTERVAL_RE,
        CT_INTERVAL_RE_EXCL,
        CT_INTERVAL_IM,
        CT_INTERVAL_IM_EXCL,
        CT_CONTAINS_VAL,
        CT_CONTAINS_STR,
        CT_FIND_STR,
        CT_NOT_FIND_STR
    };

    ConditionType m_type;
    std::vector<mu::value_type> m_vals;
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
            if (index+1 >= m_colorArray.size() || (excl && part && index+2 >= m_colorArray.size()))
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

        bool isActive() const
        {
            return m_colorArray.size();
        }

        wxColour getColour(const mu::value_type& val) const
        {
            switch (m_condition.m_type)
            {
                case CellValueShaderCondition::CT_CONTAINS_VAL:
                {
                    if (std::find(m_condition.m_vals.begin(), m_condition.m_vals.end(), val) != m_condition.m_vals.end())
                        return m_colorArray.front();

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

        wxColour getColour(const wxString& strVal) const
        {
            if (m_condition.m_type == CellValueShaderCondition::CT_CONTAINS_STR)
            {
                if (std::find(m_condition.m_strs.begin(), m_condition.m_strs.end(),
                              (strVal[0] == '"' && strVal[strVal.length()-1] == '"' ? strVal.substr(1, strVal.length()-2) : strVal)) != m_condition.m_strs.end())
                    return m_colorArray.front();
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


class CellValueShaderDialog : public wxDialog
{
    private:
        CellValueShader m_shader;
        wxNotebook* m_book;

        wxChoice* m_lt_gt_choice;
        TextField* m_lt_gt_value;
        wxColourPickerCtrl* m_lt_gt_colour;

        TextField* m_interval_start;
        TextField* m_interval_end;
        wxColourPickerCtrl* m_interval_col[3];

        TextField* m_interval_start_excl;
        TextField* m_interval_end_excl;
        wxColourPickerCtrl* m_interval_col_excl[5];

        static wxColour LEVELCOLOUR;
        static wxColour LOWERCOLOUR;
        static wxColour MINCOLOUR;
        static wxColour MEDCOLOUR;
        static wxColour MAXCOLOUR;
        static wxColour HIGHERCOLOUR;

        const size_t FIELDWIDTH = 120;

        enum
        {
            LT_GT_EQ,
            INTERVAL,
            INTERVAL_EXCL
        };

        void createLtGtPage()
        {
            GroupPanel* _panel = new GroupPanel(m_book);

            wxStaticBoxSizer* hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_CONDITION"), wxHORIZONTAL, _panel,  _panel->getMainSizer(), 0);
            _panel->AddStaticText(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_VALUES"));
            wxArrayString lt_gt;
            lt_gt.Add("<");
            lt_gt.Add(">");
            lt_gt.Add("==");
            lt_gt.Add("CONTAINS");
            lt_gt.Add("NOT CONTAINS");
            m_lt_gt_choice = _panel->CreateChoices(hsizer->GetStaticBox(), hsizer, lt_gt);
            m_lt_gt_choice->SetSelection(0);
            m_lt_gt_value = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_LT_GT_EQ_VALUE"), wxEmptyString, 0, wxID_ANY, wxSize(240, -1));

            m_lt_gt_colour = new wxColourPickerCtrl(_panel, wxID_ANY);
            m_lt_gt_colour->SetColour(LEVELCOLOUR);
            _panel->getMainSizer()->Add(m_lt_gt_colour, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

            m_book->AddPage(_panel, _guilang.get("GUI_DLG_CVS_LT_GT_EQ"));
        }

        void createIntervalPage(double minVal, double maxVal)
        {
            GroupPanel* _panel = new GroupPanel(m_book);

            wxStaticBoxSizer* hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_CONDITION"), wxHORIZONTAL, _panel,  _panel->getMainSizer(), 0);
            m_interval_start = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_START"), toString(minVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));
            _panel->AddStaticText(hsizer->GetStaticBox(), hsizer, "<=   " + _guilang.get("GUI_DLG_CVS_VALUES") + "   <=");
            m_interval_end = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_END"), toString(maxVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));

            wxBoxSizer* colorSizer = _panel->createGroup(wxHORIZONTAL, _panel->getMainSizer(), 0);

            for (size_t i = 0; i < 3; i++)
                m_interval_col[i] = new wxColourPickerCtrl(_panel, wxID_ANY);

            m_interval_col[0]->SetColour(MINCOLOUR);
            m_interval_col[1]->SetColour(MEDCOLOUR);
            m_interval_col[2]->SetColour(MAXCOLOUR);

            for (size_t i = 0; i < 3; i++)
                colorSizer->Add(m_interval_col[i], 0, wxALIGN_CENTER | wxALL, 5);

            m_book->AddPage(_panel, _guilang.get("GUI_DLG_CVS_INTERVAL"));
        }

        void createIntervalExclPage(double minVal, double maxVal)
        {
            GroupPanel* _panel = new GroupPanel(m_book);

            wxStaticBoxSizer* hsizer = _panel->createGroup(_guilang.get("GUI_DLG_CVS_CONDITION"), wxHORIZONTAL, _panel,  _panel->getMainSizer(), 0);
            m_interval_start_excl = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_EXCL_START"), toString(minVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));
            _panel->AddStaticText(hsizer->GetStaticBox(), hsizer, "<=   " + _guilang.get("GUI_DLG_CVS_VALUES") + "   <=");
            m_interval_end_excl = _panel->CreateTextInput(hsizer->GetStaticBox(), hsizer, _guilang.get("GUI_DLG_CVS_INTERVAL_EXCL_END"), toString(maxVal, 5), 0, wxID_ANY, wxSize(FIELDWIDTH, -1));

            wxBoxSizer* colorSizer = _panel->createGroup(wxHORIZONTAL, _panel->getMainSizer(), 0);

            for (size_t i = 0; i < 5; i++)
                m_interval_col_excl[i] = new wxColourPickerCtrl(_panel, wxID_ANY);

            m_interval_col_excl[0]->SetColour(LOWERCOLOUR);
            m_interval_col_excl[1]->SetColour(MINCOLOUR);
            m_interval_col_excl[2]->SetColour(MEDCOLOUR);
            m_interval_col_excl[3]->SetColour(MAXCOLOUR);
            m_interval_col_excl[4]->SetColour(HIGHERCOLOUR);

            for (size_t i = 0; i < 5; i++)
                colorSizer->Add(m_interval_col_excl[i], 0, wxALIGN_CENTER | wxALL, 5);

            m_book->AddPage(_panel, _guilang.get("GUI_DLG_CVS_INTERVAL_EXCL"));
        }

    public:
        CellValueShaderDialog(wxWindow* parent, double minVal, double maxVal, wxWindowID id = wxID_ANY) : wxDialog(parent, id, _guilang.get("GUI_DLG_CVS_HEAD"))
        {
            wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
            SetSizer(vsizer);
            m_book = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(700*g_pixelScale, 320*g_pixelScale));

            createLtGtPage();
            createIntervalPage(minVal, maxVal);
            createIntervalExclPage(minVal, maxVal);

            vsizer->Add(m_book, 0, wxALL, 5);
            m_book->SetSelection(LT_GT_EQ);

            wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
            vsizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 0);
            wxButton* okButton = new wxButton(this, wxID_OK, _guilang.get("GUI_OPTIONS_OK"), wxDefaultPosition, wxDefaultSize, 0);
            wxButton* cancelButton = new wxButton(this, wxID_CANCEL, _guilang.get("GUI_OPTIONS_CANCEL"), wxDefaultPosition, wxDefaultSize, 0);
            buttonSizer->Add(okButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
            buttonSizer->Add(cancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
            vsizer->SetSizeHints(this);
        }

        void OnButtonClick(wxCommandEvent& event)
        {
            if (event.GetId() == wxID_OK)
            {
                CellValueShaderCondition cond;
                std::vector<wxColour> colors;

                int sel = m_book->GetSelection();

                switch (sel)
                {
                    case LT_GT_EQ:
                    {
                        colors.push_back(m_lt_gt_colour->GetColour());
                        LEVELCOLOUR = m_lt_gt_colour->GetColour();
                        wxString condType = m_lt_gt_choice->GetString(m_lt_gt_choice->GetSelection());
                        std::string val = m_lt_gt_value->GetValue().ToStdString();

                        if (condType == "<")
                            cond.m_type = CellValueShaderCondition::CT_LESS_THAN;
                        else if (condType == ">")
                            cond.m_type = CellValueShaderCondition::CT_GREATER_THAN;
                        else if (condType == "==" && (isConvertible(val, CONVTYPE_VALUE) || isConvertible(val, CONVTYPE_DATE_TIME)))
                            cond.m_type = CellValueShaderCondition::CT_CONTAINS_VAL;
                        else if (condType == "==")
                            cond.m_type = CellValueShaderCondition::CT_CONTAINS_STR;
                        else if (condType == "CONTAINS")
                            cond.m_type = CellValueShaderCondition::CT_FIND_STR;
                        else if (condType == "NOT CONTAINS")
                            cond.m_type = CellValueShaderCondition::CT_NOT_FIND_STR;

                        if (isConvertible(val, CONVTYPE_DATE_TIME))
                            cond.m_vals.push_back(to_double(StrToTime(val)));
                        else if (isConvertible(val, CONVTYPE_VALUE))
                            cond.m_vals.push_back(StrToCmplx(val));
                        else
                            cond.m_strs.push_back(val);

                        break;
                    }
                    case INTERVAL:
                    {
                        for (size_t i = 0; i < 3; i++)
                            colors.push_back(m_interval_col[i]->GetColour());

                        MINCOLOUR = m_interval_col[0]->GetColour();
                        MEDCOLOUR = m_interval_col[1]->GetColour();
                        MAXCOLOUR = m_interval_col[2]->GetColour();

                        std::string val_start = m_interval_start->GetValue().ToStdString();
                        std::string val_end = m_interval_end->GetValue().ToStdString();

                        cond.m_type = CellValueShaderCondition::CT_INTERVAL_RE;

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
                        for (size_t i = 0; i < 5; i++)
                            colors.push_back(m_interval_col_excl[i]->GetColour());

                        LOWERCOLOUR = m_interval_col_excl[0]->GetColour();
                        MINCOLOUR = m_interval_col_excl[1]->GetColour();
                        MEDCOLOUR = m_interval_col_excl[2]->GetColour();
                        MAXCOLOUR = m_interval_col_excl[3]->GetColour();
                        HIGHERCOLOUR = m_interval_col_excl[4]->GetColour();

                        std::string val_start = m_interval_start_excl->GetValue().ToStdString();
                        std::string val_end = m_interval_end_excl->GetValue().ToStdString();

                        cond.m_type = CellValueShaderCondition::CT_INTERVAL_RE_EXCL;

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

            EndModal(event.GetId());
        }

        const CellValueShader& getShader() const
        {
            return m_shader;
        }

        DECLARE_EVENT_TABLE();
};


BEGIN_EVENT_TABLE(CellValueShaderDialog, wxDialog)
    EVT_BUTTON(-1, CellValueShaderDialog::OnButtonClick)
END_EVENT_TABLE()


wxColour CellValueShaderDialog::LEVELCOLOUR  = wxColour(128,128,255);
wxColour CellValueShaderDialog::LOWERCOLOUR  = wxColour(128,128,255);
wxColour CellValueShaderDialog::MINCOLOUR    = wxColour(128,255,255);
wxColour CellValueShaderDialog::MEDCOLOUR    = wxColour(128,255,128);
wxColour CellValueShaderDialog::MAXCOLOUR    = wxColour(255,255,128);
wxColour CellValueShaderDialog::HIGHERCOLOUR = wxColour(255,255,255);

#endif // CELLVALUESHADER_HPP
