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

#ifndef SEARCHCTRL_HPP
#define SEARCHCTRL_HPP

#include <wx/wx.h>
#include <wx/combo.h>
#include <wx/listctrl.h>

/////////////////////////////////////////////////
/// \brief The popup control for the generic
/// search control. Implemented ontop of a list
/// view.
/////////////////////////////////////////////////
class SearchCtrlPopup : public wxListView, public wxComboPopup
{
    private:
        int m_ListId;
        bool m_enableDragDrop;
        bool m_enableColors;
        wxArrayInt m_sizes;
        wxString m_callTip;
        wxString m_callTipHighlight;

        wxArrayString split(wxString sString);

    public:
        virtual bool Create(wxWindow* parent) override
        {
            return wxListView::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_VRULES);
        }

        virtual void Init() override
        {
            m_ListId = -1;
            m_sizes.Add(200, 1);
            m_enableDragDrop = false;
            m_enableColors = false;
            m_callTip = wxEmptyString;
            m_callTipHighlight = wxEmptyString;
        }

        virtual wxWindow* GetControl() override
        {
            return this;
        }

        virtual wxString GetStringValue() const override;
        void OnKeyEvent(wxKeyEvent& event);
        void OnMouseUp(wxMouseEvent& WXUNUSED(event));
        void OnMouseMove(wxMouseEvent& event);
        void OnDragStart(wxListEvent& event);

        void Set(wxArrayString& stringArray);

        /////////////////////////////////////////////////
        /// \brief Change the column sizes. Will create
        /// as many columns as fields are in the array.
        ///
        /// \param sizes const wxArrayInt&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetColSizes(const wxArrayInt& sizes)
        {
            m_sizes = sizes;
        }

        /////////////////////////////////////////////////
        /// \brief Enable or disable the drag-drop
        /// feature of this control.
        ///
        /// \param enable bool
        /// \return void
        ///
        /////////////////////////////////////////////////
        void EnableDragDrop(bool enable)
        {
            m_enableDragDrop = enable;
        }

        /////////////////////////////////////////////////
        /// \brief Enables or disables coloring of
        /// functions in blue.
        ///
        /// \param enable bool
        /// \return void
        ///
        /////////////////////////////////////////////////
        void EnableColors(bool enable)
        {
            m_enableColors = enable;
        }

        /////////////////////////////////////////////////
        /// \brief Sets the calltips for the popup list.
        ///
        /// \param calltip const wxString&
        /// \param calltiphighlight const wxString&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetCallTips(const wxString& calltip, const wxString& calltiphighlight = wxEmptyString)
        {
            m_callTip = calltip;
            m_callTipHighlight = calltiphighlight;
        }

    private:
        wxDECLARE_EVENT_TABLE();
};


/////////////////////////////////////////////////
/// \brief Implementation of a generic search
/// control based on a combo box.
/////////////////////////////////////////////////
class SearchCtrl : public wxComboCtrl
{
    private:
        bool textChangeMutex;
        wxString textEntryValue;

    protected:
        friend class SearchCtrlPopup;

        SearchCtrlPopup* popUp;

        // Virtual functions to interact with the data
        // model
        virtual bool selectItem(const wxString& value);
        virtual wxString getDragDropText(const wxString& value);
        virtual wxArrayString getCandidates(const wxString& enteredText);

    public:
        SearchCtrl(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString, int style = wxCB_SORT) : wxComboCtrl(parent, id, value, wxDefaultPosition, wxDefaultSize, style), textChangeMutex(false)
        {
            popUp = new SearchCtrlPopup();
            SetPopupControl(popUp);
        }

        // Event handler functions
        void OnItemSelect(wxCommandEvent& event);
        void OnTextChange(wxCommandEvent& event);
        void OnMouseEnter(wxMouseEvent& event);

        DECLARE_EVENT_TABLE();
};


#endif // SEARCHCTRL_HPP

