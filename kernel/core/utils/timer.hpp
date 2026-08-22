/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>
#include <string>
#include "../ParserLib/muHelpers.hpp"
#include "tools.hpp"

/////////////////////////////////////////////////
/// \brief This class is an internal and external
/// timer class for measuring durations.
/////////////////////////////////////////////////
class Timer
{
    private:
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> m_StartPoint;
        std::string m_ScopeName;
        bool m_stopped;
        bool m_internal;

    public:
        /////////////////////////////////////////////////
        /// \brief Constructor
        ///
        /// \param scopeName const std::string&
        /// \param internal bool
        ///
        /////////////////////////////////////////////////
        Timer(const std::string& scopeName = "", bool internal = true)
        {
            m_ScopeName = scopeName;
            m_internal = internal;
            Start();
        }

        /////////////////////////////////////////////////
        /// \brief Destructor. Prints the measured time
        /// during destruction, if the timer was not
        /// stopped before.
        /////////////////////////////////////////////////
        ~Timer()
        {
            if (!m_stopped && m_internal)
                print(Stop());
        }

        /////////////////////////////////////////////////
        /// \brief (Re-)start the timer.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void Start()
        {
            m_StartPoint = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
            m_stopped = false;
        }

        /////////////////////////////////////////////////
        /// \brief Stop the timer and obtain the measured
        /// duration.
        ///
        /// \return std::chrono::nanoseconds
        ///
        /////////////////////////////////////////////////
        std::chrono::nanoseconds Stop()
        {
            std::chrono::nanoseconds time_span = getElapsed();
            m_stopped = true;
            return time_span;
        }

        /////////////////////////////////////////////////
        /// \brief Get the name of of the timer.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string getScopeName() const
        {
            return m_ScopeName;
        }

        /////////////////////////////////////////////////
        /// \brief Get the elapsed duration since the
        /// last start of the timer without stopping it.
        ///
        /// \return std::chrono::nanoseconds
        ///
        /////////////////////////////////////////////////
        std::chrono::nanoseconds getElapsed() const
        {
            if (m_stopped)
                return std::chrono::nanoseconds(0);

            auto endTimePoint = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
            return endTimePoint - m_StartPoint;
        }

        /////////////////////////////////////////////////
        /// \brief Determine, whether this timer is
        /// running.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool running() const
        {
            return !m_stopped;
        }

        /////////////////////////////////////////////////
        /// \brief Internal helper for printing a
        /// duration together with the timer's name.
        ///
        /// \param time_span std::chrono::nanoseconds
        /// \return void
        ///
        /////////////////////////////////////////////////
        void print(std::chrono::nanoseconds time_span) const
        {
            mu::print("[" + m_ScopeName + "] Measured run time: "
                      + toString(time_span.count() / 1000.0 - (time_span.count() >= 100 ? 0.1 : 0.0), 14) + " µs.");
        }
};


#endif // TIMER_HPP

