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

#include "procedurelibrary.hpp"
#include "../ui/error.hpp"
#include "../utils/tools.hpp"

// destructor avoiding memory leaks; releases the memory allocated for each ProcedureElement
ProcedureLibrary::~ProcedureLibrary()
{
    for (auto iter = mLibraryEntries.begin(); iter != mLibraryEntries.end(); ++iter)
        delete (iter->second);
}

// constructs a new ProcedureElement, if the file exists. Otherwise returns a nullptr
ProcedureElement* ProcedureLibrary::constructProcedureElement(const string& sProcedureFileName)
{
    if (fileExists(sProcedureFileName))
    {
        ProcedureElement* element = new ProcedureElement(getFileContents(sProcedureFileName), sProcedureFileName);
        return element;
    }
    return nullptr;
}

// Reads the content of the passed file and returns it as a std::vector
vector<string> ProcedureLibrary::getFileContents(const string& sProcedureFileName)
{
    ifstream proc_in;
    vector<string> vProcContents;
    string currentline;

    proc_in.open(sProcedureFileName.c_str());
    if (proc_in.fail())
        throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sProcedureFileName, SyntaxError::invalid_position, sProcedureFileName);

    while (!proc_in.eof())
    {
        getline(proc_in, currentline);
        vProcContents.push_back(currentline);
    }
    return vProcContents;
}

// Returns the ProcedureElement pointer to the desired procedure. It also creates the element, if it doesn't already exist
ProcedureElement* ProcedureLibrary::getProcedureContents(const string& sProcedureFileName)
{
    if (mLibraryEntries.find(sProcedureFileName) == mLibraryEntries.end())
    {
        ProcedureElement* element = constructProcedureElement(sProcedureFileName);
        if (element)
            mLibraryEntries[sProcedureFileName] = element;
        else
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, sProcedureFileName, SyntaxError::invalid_position, sProcedureFileName);
    }
    return mLibraryEntries[sProcedureFileName];
}

// Perform an update, e.g. if a procedure was deleted.
void ProcedureLibrary::updateLibrary()
{
    for (auto iter = mLibraryEntries.begin(); iter != mLibraryEntries.end(); )
    {
        delete (iter->second);
        ProcedureElement* element = constructProcedureElement(iter->first);
        if (element)
        {
            iter->second = element;
            iter++;
        }
        else
        {
            iter = mLibraryEntries.erase(iter);
        }
    }
}

