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

#ifndef CELLFILTER_HPP
#define CELLFILTER_HPP

#include <wx/string.h>
#include <wx/dialog.h>
#include <complex>
#include <vector>
#include <utility>

/////////////////////////////////////////////////
/// \brief This structure represents a generic
/// cell value filtering condition, which
/// evaluates, whether the passed value fulfills
/// the condition. This is used for the
/// conditional formatting as well.
/////////////////////////////////////////////////
struct CellFilterCondition
{
    enum ConditionType
    {
        CT_NONE,
        CT_EQUALS_VAL,
        CT_NOT_EQUALS_VAL,
        CT_EQUALS_STR,
        CT_NOT_EQUALS_STR,
        CT_LESS_THAN,
        CT_LESS_EQ_THAN,
        CT_GREATER_THAN,
        CT_GREATER_EQ_THAN,
        CT_FIND_STR,
        CT_NOT_FIND_STR,
        CT_EMPTY,
        CT_NOT_EMPTY,
        CT_EQUALS_ARRAY,
        CT_INTERVAL_RE,
        CT_INTERVAL_RE_EXCL,
        CT_INTERVAL_IM,
        CT_INTERVAL_IM_EXCL
    };

    ConditionType m_type;
    std::vector<std::complex<double>> m_vals;
    std::vector<wxString> m_strs;

    void reset();
    std::pair<bool, size_t> eval(const std::complex<double>& val) const;
    std::pair<bool, size_t> eval(const wxString& val) const;
};

class wxChoice;
class TextField;

/////////////////////////////////////////////////
/// \brief This class implements the dialog for
/// choosing the filter properties of the
/// selected column.
/////////////////////////////////////////////////
class CellFilterDialog : public wxDialog
{
    private:
        CellFilterCondition m_condition;

        // Single values page attributes
        wxChoice* m_lt_gt_choice;
        TextField* m_lt_gt_value;

    public:
        CellFilterDialog(wxWindow* parent, CellFilterCondition cond, wxWindowID id = wxID_ANY);
        void OnButtonClick(wxCommandEvent& event);

        /////////////////////////////////////////////////
        /// \brief Get a reference to the internally
        /// available condition instance.
        ///
        /// \return const CellFilterCondition&
        ///
        /////////////////////////////////////////////////
        const CellFilterCondition& getCondition() const
        {
            return m_condition;
        }

        DECLARE_EVENT_TABLE();
};
#endif


