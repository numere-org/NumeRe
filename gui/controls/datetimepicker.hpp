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

#include <wx/wx.h>
#include <wx/calctrl.h>
#include <wx/timectrl.h>
#include <wx/combo.h>
#include <wx/settings.h>

#ifndef DATETIMEPICKER_HPP
#define DATETIMEPICKER_HPP

enum
{
    DT_PICKER_DATE = 0x1,
    DT_PICKER_TIME = 0x2
};

class DateTimePicker;


/////////////////////////////////////////////////
/// \brief Defines the popup for the
/// DateTimePicker control.
/////////////////////////////////////////////////
class DateTimePickerPopup : public wxPanel, public wxComboPopup
{
    public:
        DateTimePickerPopup(int style);
        bool Create(wxWindow* parent);

        wxWindow* GetControl()
        {
            return this;
        }
        wxString GetStringValue() const;
        wxSize GetAdjustedSize(int minWidth, int prefHeight, int maxHeight);

        void OnPopup();

        static wxString GetDisplayDateTimeString(const wxDateTime& dateTime, int style = DT_PICKER_DATE | DT_PICKER_TIME);
    private:
        wxCalendarCtrl*   m_calendar;
        wxTimePickerCtrl* m_timePicker;
        int m_style;

        DateTimePicker* GetCustomDateTimePicker();

        wxDateTime GetDateTime() const;
        void SetDateTime(const wxDateTime& dateTime);

        void OnOKButtonClicked(wxCommandEvent&);
};




/////////////////////////////////////////////////
/// \brief Defines a DateTimePickerCtrl
/////////////////////////////////////////////////
class DateTimePicker : public wxComboCtrl
{
    public:
        DateTimePicker(wxWindow* parent, int id = wxID_ANY, const wxDateTime& dateTime = wxDefaultDateTime, int style = DT_PICKER_DATE | DT_PICKER_TIME);

        wxDateTime GetDateTime() const;
        void SetDateTime(const wxDateTime& dateTime);

    private:
        int m_style;
        DateTimePickerPopup* m_popup;
};

#endif // DATETIMEPICKER_HPP


