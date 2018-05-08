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
#include "../utils/tools.hpp"

ProcedureElement::ProcedureElement(const vector<string>& vProcedureContents)
{
    string sProcCommandLine;
    bool bBlockComment = false;
    for (size_t i = 0; i < vProcedureContents.size(); i++)
    {
        // get the current line
        sProcCommandLine = vProcedureContents[i];
        StripSpaces(sProcCommandLine);

        // skip easy cases
        if (!sProcCommandLine.length())
            continue;
        if (sProcCommandLine.substr(0,2) == "##")
            continue;

        // Already inside of a block comment?
        if (bBlockComment)
        {
            if (sProcCommandLine.find("*#") != string::npos)
            {
                sProcCommandLine.erase(0, sProcCommandLine.find("*#")+2);
                bBlockComment = false;
            }
            else
                continue;
        }

        // examine the string: consider also quotation marks
        int nQuotes = 0;
        for (size_t j = 0; j < sProcCommandLine.length(); j++)
        {
            if (sProcCommandLine[j] == '"'
                && (!j || (j && sProcCommandLine[j-1] != '\\')))
                nQuotes++;
            if (!(nQuotes % 2) && sProcCommandLine.substr(j,2) == "##")
            {
                // that's a standard line comment
                sProcCommandLine.erase(j);
                break;
            }
            if (!(nQuotes % 2) && sProcCommandLine.substr(j,2) == "#*")
            {
                // this is a block comment
                if (sProcCommandLine.find("*#", j+2) != string::npos)
                {
                    sProcCommandLine.erase(j, sProcCommandLine.find("*#", j+2)-j+2);
                }
                else
                {
                    sProcCommandLine.erase(j);
                    bBlockComment = true;
                    break;
                }
            }
        }

        // remove whitespaces
        StripSpaces(sProcCommandLine);
        // skip empty lines
        if (!sProcCommandLine.length())
            continue;
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
