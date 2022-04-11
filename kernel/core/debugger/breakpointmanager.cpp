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
#include "breakpointmanager.hpp"
#include "../utils/tools.hpp"

// Constructor
BreakpointManager::BreakpointManager()
{
    mBreakpoints.clear();
}

// Copy constructor
BreakpointManager::BreakpointManager(const BreakpointManager& _messenger) : BreakpointManager()
{
    passBreakpoints(_messenger.mBreakpoints);
}

// This member function adds a breakpoint to the passed
// file at the indicated line number
void BreakpointManager::addBreakpoint(const std::string& _sFilename, size_t nLine)
{
    // Try to find the current file in the map
    if (mBreakpoints.find(replacePathSeparator(_sFilename)) != mBreakpoints.end())
    {
        // Get the current breakpoint set of this file
        std::vector<size_t> vLine = mBreakpoints[replacePathSeparator(_sFilename)];

        // Try to find the current line
        for (size_t i = 0; i < vLine.size(); i++)
        {
            if (vLine[i] == nLine)
                return;
        }

        // If the line was not part of the breakpoint set
        // add it to this set and store it in the map
        vLine.push_back(nLine);
        mBreakpoints[replacePathSeparator(_sFilename)] = vLine;
    }
    else
    {
        // This file does not yet exist. Create it and
        // store the passed breakpoint
        std::vector<size_t> vLine;
        vLine.push_back(nLine);
        mBreakpoints[replacePathSeparator(_sFilename)] = vLine;
    }
}

// This member function removes a breakpoint from the
// passed file at the indicated line number
void BreakpointManager::removeBreakpoint(const std::string& _sFilename, size_t nLine)
{
    // Try to find the current file in the map
    if (mBreakpoints.find(replacePathSeparator(_sFilename)) == mBreakpoints.end())
    {
        // If it does not exist, do nothing
        return;
    }
    else
    {
        // Get the current breakpoint set of this file
        std::vector<size_t> vLine = mBreakpoints[replacePathSeparator(_sFilename)];

        // Try to find the current line
        for (size_t i = 0; i < vLine.size(); i++)
        {
            if (vLine[i] == nLine)
            {
                // Erase the breakpoint from the set
                vLine.erase(vLine.begin()+i);
            }
        }

        // If this was the last breakpoint, erase the whole
        // file from the map. Otherwise store the new breakpoint
        // set in the map
        if (!vLine.size())
            mBreakpoints.erase(replacePathSeparator(_sFilename));
        else
            mBreakpoints[replacePathSeparator(_sFilename)] = vLine;
    }
}

// This member function removes all breakpoints from
// the passed file
void BreakpointManager::clearBreakpoints(const std::string& _sFilename)
{
    if (mBreakpoints.find(replacePathSeparator(_sFilename)) != mBreakpoints.end())
        mBreakpoints.erase(replacePathSeparator(_sFilename));
}

// This member function accepts the breakpoints passed
// by a map of the corresponding type
void BreakpointManager::passBreakpoints(const std::map<std::string,std::vector<size_t> >& _mBreakpoints)
{
    mBreakpoints.clear();
    mBreakpoints = _mBreakpoints;
}

// This member function returns true, if the user has set
// a breakpoint in the passed file at the passed line number
bool BreakpointManager::isBreakpoint(const std::string& _sFilename, size_t nLine)
{
    // Try to find the current file in the map
    if (mBreakpoints.find(replacePathSeparator(_sFilename)) == mBreakpoints.end())
    {
        // If it does not exist, return false
        return false;
    }
    else
    {
        // Get the current breakpoint set of this file
        std::vector<size_t> vLine = mBreakpoints[replacePathSeparator(_sFilename)];

        // Try to find the current line
        for (size_t i = 0; i < vLine.size(); i++)
        {
            if (vLine[i] == nLine)
            {
                return true;
            }
        }
    }

    // Nothing found
    return false;
}

