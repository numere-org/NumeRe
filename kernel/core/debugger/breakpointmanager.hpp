/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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

#ifndef BREAKPOINTMANAGER_HPP
#define BREAKPOINTMANAGER_HPP

#include <map>
#include <string>
#include <vector>

/////////////////////////////////////////////////
/// \brief Represents the properties of a
/// breakpoint.
/////////////////////////////////////////////////
struct Breakpoint
{
    std::string m_condition;
    bool m_enabled;
    bool m_isConditional;

    Breakpoint(const std::string& condition = "true", bool enable = true) : m_condition(condition), m_enabled(enable)
    {
        m_isConditional = m_condition.length() && m_condition != "true" && m_condition != "false";
    }

    Breakpoint(bool enable) : m_condition(enable ? "true" : "false"), m_enabled(enable), m_isConditional(false)
    { }

    bool isActive(bool needsLocks);
};


/////////////////////////////////////////////////
/// \brief Manages all breakpoints set within the
/// editor.
/////////////////////////////////////////////////
class BreakpointManager
{
    private:
        std::map<std::string, std::map<size_t, Breakpoint>> mBreakpoints;

    public:
        BreakpointManager();
        BreakpointManager(const BreakpointManager& _messenger);

        void addBreakpoint(const std::string& _sFilename, size_t nLine, const Breakpoint& bp);
        void removeBreakpoint(const std::string& _sFilename, size_t nLine);
        void clearBreakpoints(const std::string& _sFilename);
        void passBreakpoints(const std::map<std::string, std::map<size_t, Breakpoint>>& _mBreakpoints);
        bool isBreakpoint(const std::string& _sFilename, size_t nLine) const;
        Breakpoint getBreakpoint(const std::string& _sFilename, size_t nLine) const;
};

#endif // BREAKPOINTMANAGER_HPP

