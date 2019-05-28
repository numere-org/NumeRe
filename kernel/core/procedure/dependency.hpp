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

using namespace std;

// This class resembles a simple dependency containing
// a procedure name and the corresponding file name
class Dependency
{
    private:
        string sProcedureName;
        string sFileName;

    public:
        Dependency(const string& sProcName, const string& sFile) : sProcedureName(sProcName), sFileName(sFile) {}

        string& getFileName()
        {
            return sFileName;
        }

        string& getProcedureName()
        {
            return sProcedureName;
        }

        const string& getProcedureName() const
        {
            return sProcedureName;
        }
};

// This class is a child of the std::list, where
// the function unique() has been overridden (i.e. shadowed)
class DependencyList : public list<Dependency>
{
    public:
        void unique();
};

// Forward declaration of the procedure element class
class ProcedureElement;

// This class handles the dependencies of the current procedure
// file (passed as pointer to a ProcedureElement instance) and calculates
// them during construction
class Dependencies
{
    private:
        map<string, DependencyList> mDependencies;
        string sFileName;
        string sThisFileNameSpacePrefix;
        string sThisNameSpace;
        string sMainProcedure;

        void walk(ProcedureElement* procedureFile);
        int getProcedureDependencies(ProcedureElement* procedureFile, int nCurrentLine);
        string getProcedureName(string sCommandLine);
        void resolveProcedureCalls(string sCommandLine, const string& sProcedureName, const string& sCurrentNameSpace);
        string getProcedureFileName(string sProc);

    public:
        Dependencies(ProcedureElement* procedureFile);

        map<string, DependencyList>& getDependencyMap()
        {
            return mDependencies;
        }

        string getMainProcedure() const
        {
            return sMainProcedure;
        }
};


#endif // DEPENDENCY_HPP


