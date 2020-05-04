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
#include "../../kernel.hpp"
#include "tools.hpp"

class Timer
{
    private:
        std::chrono::time_point<std::chrono::steady_clock> m_StartPoint;
        std::string m_ScopeName;

    public:
        Timer(const std::string& scopeName)
        {
            m_ScopeName = scopeName;
            m_StartPoint = std::chrono::steady_clock::now();
        }

        ~Timer()
        {
            Stop();
        }

        void Stop()
        {
            auto endTimePoint = std::chrono::steady_clock::now();
            std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(endTimePoint - m_StartPoint);

            NumeReKernel::print("[" + m_ScopeName + "] Measured run time: " + toString(time_span.count() * 1000, 14) + " ms.");
        }
};


#endif // TIMER_HPP

