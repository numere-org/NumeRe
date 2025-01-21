/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#ifndef DEPENDENCY_HPP
#define DEPENDENCY_HPP

#include <string>
#include <map>
#include <list>


/////////////////////////////////////////////////
/// \brief This class resembles a simple
/// dependency containing a procedure name and
/// the corresponding file name.
/////////////////////////////////////////////////
class Dependency
{
    public:
        enum DependencyType
        {
            NPRC,
            NSCR,
            NLYT,
            FILE
        };

    private:
        std::string sProcedureName;
        std::string sFileName;
        DependencyType m_type;

    public:
        Dependency(const std::string& sProcName, const std::string& sFile, DependencyType type) : sProcedureName(sProcName), sFileName(sFile), m_type(type) {}

        std::string& getFileName()
        {
            return sFileName;
        }

        const std::string& getFileName() const
        {
            return sFileName;
        }

        std::string& getProcedureName()
        {
            return sProcedureName;
        }

        const std::string& getProcedureName() const
        {
            return sProcedureName;
        }

        DependencyType getType() const
        {
            return m_type;
        }
};


/////////////////////////////////////////////////
/// \brief This class is a child of the
/// std::list, where the function unique() has
/// been overridden (i.e. shadowed).
/////////////////////////////////////////////////
class DependencyList : public std::list<Dependency>
{
    public:
        void unique();
};


// Forward declaration of the procedure element class
class ProcedureElement;

/////////////////////////////////////////////////
/// \brief This class handles the dependencies of
/// the current procedure file (passed as pointer
/// to a ProcedureElement instance) and
/// calculates them during construction.
/////////////////////////////////////////////////
class Dependencies
{
    private:
        std::map<std::string, DependencyList> mDependencies;
        std::string sFileName;
        std::string sThisFileNameSpacePrefix;
        std::string sThisNameSpace;
        std::string sMainProcedure;

        void walk(ProcedureElement* procedureFile);
        int getProcedureDependencies(ProcedureElement* procedureFile, int nCurrentLine);
        std::string getProcedureName(std::string sCommandLine) const;
        void resolveProcedureCalls(std::string sCommandLine, const std::string& sProcedureName, const std::string& sCurrentNameSpace);
        std::string getProcedureFileName(std::string sProc) const;

    public:
        Dependencies(ProcedureElement* procedureFile);

        std::map<std::string, DependencyList>& getDependencyMap()
        {
            return mDependencies;
        }

        std::string getMainProcedure() const
        {
            return sMainProcedure;
        }
};


#endif // DEPENDENCY_HPP


