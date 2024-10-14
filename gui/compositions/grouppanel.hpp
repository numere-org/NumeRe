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

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>
#include <wx/treelist.h>
#include <wx/collpane.h>
#include <wx/datectrl.h>
#include <wx/timectrl.h>

#ifndef GROUPPANEL_HPP
#define GROUPPANEL_HPP

#include "../controls/treelistctrl.h"
#include "../controls/datetimepicker.hpp"



/////////////////////////////////////////////////
/// \brief This class is a extension to the
/// standard wxSpinCtrl to combine it with a
/// read- and changeable label.
/////////////////////////////////////////////////
class SpinBut : public wxSpinCtrl
{
    public:
        wxStaticText* m_label;

        SpinBut(wxWindow* parent, wxWindowID id, const wxSize& size, int nMin, int nMax, int nInitial) : wxSpinCtrl(parent, id, wxEmptyString, wxDefaultPosition, size, wxSP_ARROW_KEYS, nMin, nMax, nInitial), m_label(nullptr)
        {}

        /////////////////////////////////////////////////
        /// \brief Set a new label
        ///
        /// \param label const wxString&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetLabel(const wxString& label)
        {
            if (m_label)
                m_label->SetLabel(label);
        }

        /////////////////////////////////////////////////
        /// \brief Get the current label
        ///
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString GetLabel()
        {
            if (m_label)
                return m_label->GetLabel();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Show or hide the label and the
        /// control.
        ///
        /// \param show bool
        /// \return virtual bool
        ///
        /////////////////////////////////////////////////
        virtual bool Show(bool show)
        {
            if (m_label)
                m_label->Show(show);

            return wxSpinCtrl::Show(show);
        }

        /////////////////////////////////////////////////
        /// \brief Enable or disable the label and the
        /// control.
        ///
        /// \param enable bool
        /// \return virtual bool
        ///
        /////////////////////////////////////////////////
        virtual bool Enable(bool enable)
        {
            if (m_label)
                m_label->Enable(enable);

            return wxSpinCtrl::Enable(enable);
        }
};



/////////////////////////////////////////////////
/// \brief This class is a extension to the
/// standard wxTextCtrl to combine it with a
/// read- and changeable label.
/////////////////////////////////////////////////
class TextField : public wxTextCtrl
{
    public:
        wxStaticText* m_label;

        TextField(wxWindow* parent, wxWindowID id, const wxString& sDefault, const wxSize& size, int nStyle) : wxTextCtrl(parent, id, sDefault, wxDefaultPosition, size, nStyle), m_label(nullptr), m_highlightColour(*wxBLUE)
        {}

        /////////////////////////////////////////////////
        /// \brief Set a new label
        ///
        /// \param label const wxString&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetLabel(const wxString& label)
        {
            if (m_label)
                m_label->SetLabel(label);
        }

        /////////////////////////////////////////////////
        /// \brief Get the current label
        ///
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString GetLabel()
        {
            if (m_label)
                return m_label->GetLabel();

            return "";
        }

        /////////////////////////////////////////////////
        /// \brief Show or hide the label and the
        /// control.
        ///
        /// \param show bool
        /// \return virtual bool
        ///
        /////////////////////////////////////////////////
        virtual bool Show(bool show)
        {
            if (m_label)
                m_label->Show(show);

            return wxTextCtrl::Show(show);
        }

        /////////////////////////////////////////////////
        /// \brief Enable or disable the label and the
        /// control.
        ///
        /// \param enable bool
        /// \return virtual bool
        ///
        /////////////////////////////////////////////////
        virtual bool Enable(bool enable)
        {
            if (m_label)
                m_label->Enable(enable);

            return wxTextCtrl::Enable(enable);
        }

        void SetMarkupText(const wxString& text);
        void OnUrlClick(wxTextUrlEvent& event);

        /////////////////////////////////////////////////
        /// \brief Set the highlight colour for markup
        /// link styles.
        ///
        /// \param col const wxColour&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetHighlightColour(const wxColour& col)
        {
            m_highlightColour = col;
        }


        DECLARE_EVENT_TABLE();

    private:
        wxColour m_highlightColour;
};



/////////////////////////////////////////////////
/// \brief This class simplifies the creation of
/// simple windows and creates a common layout
/// among all windows.
/////////////////////////////////////////////////
class GroupPanel : public wxScrolledWindow
{
    private:
        wxBoxSizer* verticalSizer;
        wxBoxSizer* horizontalSizer;
        wxBoxSizer* mainSizer;

    public:
        GroupPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                   long style = wxTAB_TRAVERSAL, bool useVerticalSizer = true);

        wxBoxSizer* getVerticalSizer();
        wxBoxSizer* getHorizontalSizer();
        wxBoxSizer* getMainSizer();

        // A spacer
        void AddSpacer(int nSize = 10, wxSizer* sizer = nullptr);

        // Some static text
        wxStaticText* AddStaticText(wxWindow* parent, wxSizer* sizer, const wxString& text, int id = wxID_STATIC,
                                    int alignment = wxALIGN_CENTER_VERTICAL);

        // Group adders
        wxStaticBoxSizer* createGroup(const wxString& sGroupName, int orient = wxVERTICAL, wxWindow* parent = nullptr,
                                      wxSizer* sizer = nullptr, int expand = 0);
        wxBoxSizer* createGroup(int orient = wxVERTICAL, wxSizer* sizer = nullptr, int expand = 0);
        wxCollapsiblePane* createCollapsibleGroup(const wxString& label, wxWindow* parent = nullptr, wxSizer* sizer = nullptr);

        // Text fields et al.
        wxTextCtrl* CreatePathInput(wxWindow* parent, wxSizer* sizer, const wxString& description, int buttonID, int id = wxID_ANY);
        TextField* CreateTextInput(wxWindow* parent, wxSizer* sizer, const wxString& description, const wxString& sDefault = wxEmptyString,
                                   int nStyle = 0, int id = wxID_ANY, const wxSize& size = wxSize(310,-1),
                                   int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 0);

        // A lamp
        TextField* CreateLamp(wxWindow* parent, wxSizer* sizer, const wxString& description, const wxString& sDefault = wxEmptyString,
                              int nStyle = 0, int id = wxID_ANY, const wxSize& size = wxSize(20,10), int alignment = wxALIGN_CENTER_VERTICAL,
                              int proportion = 0);

        // A check box
        wxCheckBox* CreateCheckBox(wxWindow* parent, wxSizer* sizer, const wxString& description, int id = wxID_ANY,
                                   int alignment = wxALIGN_CENTER_VERTICAL);

        // A spin button
        SpinBut* CreateSpinControl(wxWindow* parent, wxSizer* sizer, const wxString& description, int nMin, int nMax, int nInitial,
                                   int id = wxID_ANY, int alignment = wxALIGN_CENTER_VERTICAL);

        // Lists and tree lists
        wxListView* CreateListView(wxWindow* parent, wxSizer* sizer, int nStyle = wxLC_REPORT, wxSize size = wxDefaultSize,
                                   int id = wxID_ANY, int proportion = 1);
        wxTreeListCtrl* CreateTreeListCtrl(wxWindow* parent, wxSizer* sizer, int nStyle = wxTL_SINGLE, wxSize size = wxDefaultSize,
                                           int id = wxID_ANY, int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 1);
        wxcode::wxTreeListCtrl* CreateWxcTreeListCtrl(wxWindow* parent, wxSizer* sizer,
                                                      int nStyle = wxTR_TWIST_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT | wxTR_EXTENDED,
                                                      wxSize size = wxDefaultSize, int id = wxID_ANY,
                                                      int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 1);

        // A regular button
        wxButton* CreateButton(wxWindow* parent, wxSizer* sizer, const wxString& description, int id = wxID_ANY,
                               int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 1);

        // A Radio box
        wxRadioBox* CreateRadioBox(wxWindow* parent, wxSizer* sizer, const wxString& description, const wxArrayString& choices,
                                   int style = wxHORIZONTAL, int id = wxID_ANY, int alignment = wxALIGN_CENTER_VERTICAL);

        // Drop down et al.
        wxChoice* CreateChoices(wxWindow* parent, wxSizer* sizer, const wxArrayString& choices, int id = wxID_ANY,
                                int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 1);
        wxComboBox* CreateComboBox(wxWindow* parent, wxSizer* sizer, const wxArrayString& choices, int id = wxID_ANY,
                                   int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 1);

        // A gauge (waitbar)
        wxGauge* CreateGauge(wxWindow* parent, wxSizer* sizer, int style, int id = wxID_ANY, int alignment = wxALIGN_CENTER_VERTICAL,
                             int proportion = 0);

        // Some static bitmap
        wxStaticBitmap* CreateBitmap(wxWindow* parent, wxSizer* sizer, const wxString& filename, int id = wxID_ANY,
                                     int alignment = wxALIGN_CENTER_VERTICAL);

        // A slider
        wxSlider* CreateSlider(wxWindow* parent, wxSizer* sizer, int nMin, int nMax, int nInitial, int style, int id = wxID_ANY,
                               int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 0);

        // Date time pickers
        wxDatePickerCtrl* CreateDatePicker(wxWindow* parent, wxSizer* sizer, const wxDateTime& dt = wxDefaultDateTime,
                                           int style = wxDP_SHOWCENTURY | wxDP_DROPDOWN, int id = wxID_ANY,
                                           int alignment = wxALIGN_CENTER_VERTICAL);
        wxTimePickerCtrl* CreateTimePicker(wxWindow* parent, wxSizer* sizer, const wxDateTime& dt = wxDefaultDateTime, int id = wxID_ANY,
                                           int alignment = wxALIGN_CENTER_VERTICAL);
        DateTimePicker* CreateDateTimePicker(wxWindow* parent, wxSizer* sizer, const wxDateTime& dt = wxDefaultDateTime,
                                             int style = DT_PICKER_DATE | DT_PICKER_TIME, int id = wxID_ANY,
                                             int alignment = wxALIGN_CENTER_VERTICAL, int proportion = 1);
};


#endif // GROUPPANEL_HPP

