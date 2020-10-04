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
#include <string>
#include <fstream>
#include <map>
#include <vector>

#ifndef PROCEDURELIBRARY_HPP
#define PROCEDURELIBRARY_HPP

/////////////////////////////////////////////////
/// \brief This class manages all already read
/// and possibly pre-parsed procedure files for
/// easier and faster access.
/////////////////////////////////////////////////
class ProcedureLibrary
{
    private:
        std::map<std::string, ProcedureElement*> mLibraryEntries;

        ProcedureElement* constructProcedureElement(const std::string& sProcedureFileName);
        std::vector<std::string> getFileContents(const std::string& sProcedureFileName);

    public:
        ~ProcedureLibrary();

        ProcedureElement* getProcedureContents(const std::string& sProcedureFileName);
        void updateLibrary();
};

#endif

