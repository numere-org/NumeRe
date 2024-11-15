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

#ifndef DATETIMETOOLS_HPP
#define DATETIMETOOLS_HPP

#include <ctime>
#include <chrono>
#include "../../../externals/date/include/date/date.h"

/////////////////////////////////////////////////
/// \brief This structure defines all fields
/// necessary to create a time stamp or a
/// formatted date.
/////////////////////////////////////////////////
struct time_stamp
{
    date::year_month_day m_ymd;
    std::chrono::hours m_hours;
    std::chrono::minutes m_minutes;
    std::chrono::seconds m_seconds;
    std::chrono::milliseconds m_millisecs;
    std::chrono::microseconds m_microsecs;

    time_stamp();
};


/////////////////////////////////////////////////
/// \brief This structure defines the information
/// for a time zone.
/////////////////////////////////////////////////
struct time_zone
{
    std::chrono::minutes Bias;
    std::chrono::minutes DayLightBias;
};


/////////////////////////////////////////////////
/// \brief This is a typedef for a custom
/// system_clock time_point with microseconds
/// precision.
/////////////////////////////////////////////////
using sys_time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;


sys_time_point sys_time_now();

date::year_month_day dateFromTimePoint(sys_time_point tp);
date::year_month_day dateFromTime_t(__time64_t t);

time_stamp getTimeStampFromTimePoint(sys_time_point tp);
time_stamp getTimeStampFromTime_t(__time64_t t);

sys_time_point getTimePointFromTime_t(__time64_t t);
sys_time_point getTimePointFromYMD(int year, int month, int day);
sys_time_point getTimePointFromHMS(int hours, int minutes, int seconds = 0, int milliseconds = 0, int microseconds = 0);

sys_time_point getTimePointFromTimeStamp(const time_stamp& ts);
__time64_t getTime_tFromTimeStamp(const time_stamp& ts);

double to_double(sys_time_point tp);
sys_time_point to_timePoint(double d);

size_t getWeekDay(sys_time_point tp);
size_t getWeekNum(sys_time_point tp);

time_zone getCurrentTimeZone();

#endif // DATETIMETOOLS_HPP

