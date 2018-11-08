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

#include <string>
#include <map>
#include <vector>
#include "procedurecommandline.hpp"

#ifndef PROCEDUREELEMENT_HPP
#define PROCEDUREELEMENT_HPP

using namespace std;

// This class contains the pre-parsed contents of a single
// procedure file
class ProcedureElement
{
    private:
        map<int, ProcedureCommandLine> mProcedureContents;
        map<string, int> mProcedureList;

        void cleanCurrentLine(string& sProcCommandLine, const string& sCurrentCommand, const string& sFolderPath);

    public:
        ProcedureElement(const vector<string>& vProcedureContents, const string& sFolderPath);

        pair<int, ProcedureCommandLine> getFirstLine();
        pair<int, ProcedureCommandLine> getCurrentLine(int currentLine);
        pair<int, ProcedureCommandLine> getNextLine(int currentline);
        int gotoProcedure(const string& sProcedureName);

        bool isLastLine(int currentline);
        void setByteCode(int _nByteCode, int nCurrentLine);
};

#endif // PROCEDUREELEMENT_HPP

