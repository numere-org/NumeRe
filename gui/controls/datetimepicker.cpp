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

#include "datetimepicker.hpp"
#include "../../kernel/core/utils/tools.hpp"

// DateTimePickerPopup definition

DateTimePickerPopup::DateTimePickerPopup(int style) : wxPanel(), wxComboPopup(), m_style(style) {}


bool DateTimePickerPopup::Create(wxWindow* parent)
{
    if (!wxPanel::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE | wxTAB_TRAVERSAL))
        return false;

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);

    m_calendar = new wxCalendarCtrl(this, wxID_ANY, wxDefaultDateTime,
                                    wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxCAL_MONDAY_FIRST | wxCAL_SHOW_WEEK_NUMBERS);
    m_calendar->Show(m_style & DT_PICKER_DATE);
    mainSizer->Add(m_calendar, wxSizerFlags(1).Expand().Border(wxALL, 2));

    m_timePicker = new wxTimePickerCtrl(this, wxID_ANY);
    m_timePicker->Show(m_style & DT_PICKER_TIME);
    subSizer->Add(m_timePicker, wxSizerFlags(1).Expand().Border(wxALL, 2).ReserveSpaceEvenIfHidden());

    wxButton* OKButton = new wxButton(this, wxID_OK);
    OKButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DateTimePickerPopup::OnOKButtonClicked, this);
    subSizer->Add(OKButton, wxSizerFlags(1).Expand().Border(wxALL, 2));

    mainSizer->Add(subSizer, wxSizerFlags().Expand());
    SetSizerAndFit(mainSizer);

    return true;
}


wxString DateTimePickerPopup::GetStringValue() const
{
    return GetDisplayDateTimeString(GetDateTime(), m_style);
}


wxSize DateTimePickerPopup::GetAdjustedSize(int WXUNUSED(minWidth), int WXUNUSED(prefHeight), int WXUNUSED(maxHeight))
{
    //@fixme: if possible, take into account the method parameters
    return GetSize();
}


void DateTimePickerPopup::OnPopup()
{
    DateTimePicker* picker = GetCustomDateTimePicker();

    if (picker)
        SetDateTime(picker->GetDateTime());
}


wxDateTime DateTimePickerPopup::GetDateTime() const
{
    wxDateTime dateOnly, timeOnly;

    dateOnly = m_calendar->GetDate();
    wxCHECK(dateOnly.IsValid(), wxInvalidDateTime);

    timeOnly = m_timePicker->GetValue();
    wxCHECK(timeOnly.IsValid(), wxInvalidDateTime);

    return wxDateTime(dateOnly.GetDay(), dateOnly.GetMonth(), dateOnly.GetYear(),
                      timeOnly.GetHour(), timeOnly.GetMinute(), timeOnly.GetSecond());
}


void DateTimePickerPopup::SetDateTime(const wxDateTime& dateTime)
{
    if (!dateTime.IsValid())
        return;

    m_calendar->SetDate(dateTime);
    m_timePicker->SetValue(dateTime);
}


wxString DateTimePickerPopup::GetDisplayDateTimeString(const wxDateTime& dateTime, int style)
{
    if (!dateTime.IsValid())
        return "---";

    if ((style & DT_PICKER_DATE) && (style & DT_PICKER_TIME))
        return dateTime.FormatISOCombined(',');
    else if (style & DT_PICKER_DATE)
        return dateTime.FormatISODate();

    return dateTime.FormatISOTime();
}


DateTimePicker* DateTimePickerPopup::GetCustomDateTimePicker()
{
    wxCHECK(IsCreated(), NULL);

    return dynamic_cast<DateTimePicker*>(GetComboCtrl());
}


void DateTimePickerPopup::OnOKButtonClicked(wxCommandEvent&)
{
    Dismiss();
}





DateTimePicker::DateTimePicker(wxWindow* parent, int id, const wxDateTime& dateTime, int style)
    : wxComboCtrl(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxCB_SIMPLE | wxTE_PROCESS_ENTER), m_style(style)
{
    m_popup = new DateTimePickerPopup(m_style);
    SetPopupControl(m_popup);

    SetDateTime(dateTime);
}


wxDateTime DateTimePicker::GetDateTime() const
{
    time_stamp time = getTimeStampFromTimePoint(StrToTime(GetValue().ToStdString()));

    wxDateTime dt;

    dt.Set((unsigned)time.m_ymd.day(), (wxDateTime::Month)((unsigned)time.m_ymd.month()-1), (int)time.m_ymd.year(),
           time.m_hours.count(), time.m_minutes.count(), time.m_seconds.count(), time.m_millisecs.count());

    return dt;
}


void DateTimePicker::SetDateTime(const wxDateTime& dateTime)
{
    SetValue(DateTimePickerPopup::GetDisplayDateTimeString(dateTime, m_style));
}


