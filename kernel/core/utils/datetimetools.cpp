/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#include "datetimetools.hpp"
#include "windows.h"


/////////////////////////////////////////////////
/// \brief Returns the current time as a
/// sys_time_point (i.e. a std::chrono::time_point
/// with microseconds precision).
///
/// \return sys_time_point
///
/////////////////////////////////////////////////
sys_time_point sys_time_now()
{
    return std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
}


/////////////////////////////////////////////////
/// \brief Return the date extracted from the
/// passed sys_time_point.
///
/// \param tp sys_time_point
/// \return date::year_month_day
///
/////////////////////////////////////////////////
date::year_month_day dateFromTimePoint(sys_time_point tp)
{
    return date::year_month_day(date::floor<date::days>(tp));
}


/////////////////////////////////////////////////
/// \brief Return the date extracted from the
/// passed __time64_t.
///
/// \param t __time64_t
/// \return date::year_month_day
///
/////////////////////////////////////////////////
date::year_month_day dateFromTime_t(__time64_t t)
{
    sys_time_point tp;
    tp += std::chrono::seconds(t);
    return dateFromTimePoint(tp);
}


/////////////////////////////////////////////////
/// \brief Return a time_stamp instance converted
/// from the passed sys_time_point.
///
/// \param tp sys_time_point
/// \return time_stamp
///
/////////////////////////////////////////////////
time_stamp getTimeStampFromTimePoint(sys_time_point tp)
{
    time_stamp timeStamp;

    timeStamp.m_ymd = dateFromTimePoint(tp);
    std::chrono::nanoseconds diff = tp - date::floor<date::days>(tp);

    timeStamp.m_hours = std::chrono::duration_cast<std::chrono::hours>(diff);
    diff = diff - std::chrono::duration_cast<std::chrono::hours>(diff);

    timeStamp.m_minutes = std::chrono::duration_cast<std::chrono::minutes>(diff);
    diff = diff - std::chrono::duration_cast<std::chrono::minutes>(diff);

    timeStamp.m_seconds = std::chrono::duration_cast<std::chrono::seconds>(diff);
    diff = diff - std::chrono::duration_cast<std::chrono::seconds>(diff);

    timeStamp.m_millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    diff = diff - std::chrono::duration_cast<std::chrono::milliseconds>(diff);

    timeStamp.m_microsecs = std::chrono::duration_cast<std::chrono::microseconds>(diff);

    return timeStamp;
}


/////////////////////////////////////////////////
/// \brief Return a time_stamp instance converted
/// from the passed __time64_t.
///
/// \param t __time64_t
/// \return time_stamp
///
/////////////////////////////////////////////////
time_stamp getTimeStampFromTime_t(__time64_t t)
{
    sys_time_point tp;
    tp += std::chrono::seconds(t);
    return getTimeStampFromTimePoint(tp);
}


/////////////////////////////////////////////////
/// \brief Convert a time_stamp to a
/// sys_time_point.
///
/// \param ts const time_stamp&
/// \return sys_time_point
///
/////////////////////////////////////////////////
sys_time_point getTimePointFromTimeStamp(const time_stamp& ts)
{
    auto res = date::sys_days(ts.m_ymd)
        + ts.m_hours
        + ts.m_minutes
        + ts.m_seconds
        + ts.m_millisecs
        + ts.m_microsecs;

    return res;
}


/////////////////////////////////////////////////
/// \brief Convert a time_stamp to a __time64_t.
///
/// \param ts const time_stamp&
/// \return __time64_t
///
/////////////////////////////////////////////////
__time64_t getTime_tFromTimeStamp(const time_stamp& ts)
{
    auto tp = std::chrono::time_point_cast<std::chrono::seconds>(getTimePointFromTimeStamp(ts));
    return tp.time_since_epoch().count();
}


/////////////////////////////////////////////////
/// \brief Convert a sys_time_point to a double.
/// The double is returned in units of seconds.
///
/// \param tp sys_time_point
/// \return double
///
/////////////////////////////////////////////////
double to_double(sys_time_point tp)
{
    return tp.time_since_epoch().count() * std::micro::num / (double)std::micro::den;
}


/////////////////////////////////////////////////
/// \brief Convert a double to a sys_time_point
/// assuming the double is in units of seconds.
///
/// \param d double
/// \return sys_time_point
///
/////////////////////////////////////////////////
sys_time_point to_timePoint(double d)
{
    sys_time_point tp;

    tp += std::chrono::microseconds((int64_t)std::rint(d * std::micro::den / std::micro::num));

    return tp;
}


/////////////////////////////////////////////////
/// \brief Return the current time_zone of the
/// system.
///
/// \return time_zone
///
/////////////////////////////////////////////////
time_zone getCurrentTimeZone()
{
    TIME_ZONE_INFORMATION timezone;
    GetTimeZoneInformation(&timezone);

    time_zone tz;
    tz.Bias = std::chrono::minutes(timezone.Bias);
    tz.DayLightBias = std::chrono::minutes(timezone.DaylightBias);

    return tz;
}





