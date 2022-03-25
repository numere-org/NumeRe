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

// Forward declaration of the dependencies class
class Dependencies;


/////////////////////////////////////////////////
/// \brief This class contains the pre-parsed
/// contents of a single procedure file.
/////////////////////////////////////////////////
class ProcedureElement
{
    private:
        std::vector<std::pair<int, ProcedureCommandLine>> mProcedureContents;
        std::map<std::string, int> mProcedureList;
        std::string sFileName;
        Dependencies* m_dependencies;

        void cleanCurrentLine(std::string& sProcCommandLine, const std::string& sCurrentCommand, const std::string& sFilePath);

    public:
        ProcedureElement(const std::vector<std::string>& vProcedureContents, const std::string& sFolderPath);
        ~ProcedureElement();

        std::pair<int, ProcedureCommandLine> getFirstLine();
        std::pair<int, ProcedureCommandLine> getCurrentLine(int currentLine);
        std::pair<int, ProcedureCommandLine> getNextLine(int currentline);
        int gotoProcedure(const std::string& sProcedureName);
        std::string getFileName() const
        {
            return sFileName;
        }

        bool isLastLine(int currentline);
        void setByteCode(int _nByteCode, int nCurrentLine);
        Dependencies* getDependencies();
};

#endif // PROCEDUREELEMENT_HPP

