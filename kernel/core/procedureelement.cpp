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

#include "procedureelement.hpp"
#include "tools.hpp"

ProcedureElement::ProcedureElement(const vector<string>& vProcedureContents)
{
    string sProcCommandLine;
    bool bBlockComment = false;
    for (size_t i = 0; i < vProcedureContents.size(); i++)
    {
        sProcCommandLine = vProcedureContents[i];
        StripSpaces(sProcCommandLine);
        if (!sProcCommandLine.length())
            continue;
        if (sProcCommandLine.substr(0,2) == "##")
            continue;
        if (sProcCommandLine.find("##") != string::npos)
            sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find("##"));
        if (sProcCommandLine.substr(0,2) == "#*" && sProcCommandLine.find("*#",2) == string::npos)
        {
            bBlockComment = true;
            continue;
        }
        if (bBlockComment && sProcCommandLine.find("*#") != string::npos)
        {
            bBlockComment = false;
            if (sProcCommandLine.find("*#") == sProcCommandLine.length()-2)
            {
                continue;
            }
            else
                sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("*#")+2);
        }
        else if (bBlockComment && sProcCommandLine.find("*#") == string::npos)
        {
            continue;
        }
        mProcedureContents[i] = sProcCommandLine;
    }
}

pair<int,string> ProcedureElement::getFirstLine()
{
    return *mProcedureContents.begin();
}

pair<int,string> ProcedureElement::getNextLine(int nCurrentLine)
{
    pair<int,string> currentLine;
    auto iter = mProcedureContents.find(nCurrentLine);
    if (iter != mProcedureContents.end())
    {
        iter++;
        if (iter != mProcedureContents.end())
            currentLine = *iter;
    }
    return currentLine;
}

bool ProcedureElement::isLastLine(int nCurrentLine)
{
    auto iter = mProcedureContents.find(nCurrentLine);
    if (iter != mProcedureContents.end())
    {
        iter++;
        if (iter != mProcedureContents.end())
            return false;
        return true;
    }
    return false;
}
