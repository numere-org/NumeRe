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

#ifndef DEBUGMESSENGER_HPP
#define DEBUGMESSENGER_HPP

#include <map>
#include <string>
#include <vector>

using namespace std;

class Debugmessenger
{
    private:
        map<string,vector<size_t> > mBreakpoints;

    public:
        Debugmessenger();
        Debugmessenger(const Debugmessenger& _messenger);

        void addBreakpoint(const string& _sFilename, size_t nLine);
        void removeBreakpoint(const string& _sFilename, size_t nLine);
        void clearBreakpoints(const string& _sFilename);
        void passBreakpoints(const map<string,vector<size_t> >& _mBreakpoints);
        bool hasBreakpoints(const string& _sFilename);
        bool isBreakpoint(const string& _sFilename, size_t nLine);
};

#endif // DEBUGMESSENGER_HPP

