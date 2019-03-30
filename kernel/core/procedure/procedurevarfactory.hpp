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

#include "../utils/tools.hpp"
#include "../ui/error.hpp"
#include "../datamanagement/datafile.hpp"
#include "../maths/functionimplementation.hpp"
#include "../maths/define.hpp"
#include "../settings.hpp"
#include "../io/output.hpp"
#include "../plotting/plotdata.hpp"
#include "../script.hpp"

#ifndef PROCEDUREVARFACTORY_HPP
#define PROCEDUREVARFACTORY_HPP

using namespace std;
using namespace mu;

// forward declaration of the procedure class
class Procedure;

class ProcedureVarFactory
{
    private:
        Parser* _parserRef;
        Datafile* _dataRef;
        Settings* _optionRef;
        Define* _functionRef;
        Output* _outRef;
        PlotData* _pDataRef;
        Script* _scriptRef;

        Procedure* _currentProcedure;

        string sProcName;
        unsigned int nth_procedure;

        void init();

        string replaceProcedureName(string sProcedureName);

        string resolveArguments(string sProcedureCommandLine);
        string resolveLocalVars(string sProcedureCommandLine);
        string resolveLocalStrings(string sProcedureCommandLine);
        string resolveLocalTables(string sProcedureCommandLine);

        unsigned int countVarListElements(const string& sVarList);

        void checkKeywordsInArgument(const string& sArgument, const string& sArgumentList, unsigned int nCurrentIndex);

    public:
        string** sArgumentMap;
        string** sLocalVars;
        string** sLocalStrings;
        string** sLocalTables;

        double* dLocalVars;

        unsigned int nArgumentMapSize;
        unsigned int nLocalVarMapSize;
        unsigned int nLocalStrMapSize;
        unsigned int nLocalTableSize;

        ProcedureVarFactory();
        ProcedureVarFactory(Procedure* _procedure, const string& sProc, unsigned int currentProc);
        ~ProcedureVarFactory();

        void reset();
        map<string,string> createProcedureArguments(string sArgumentList, string sArgumentValues);
        void createLocalVars(string sVarList);
        void createLocalStrings(string sStringList);
        void createLocalTables(string sTableList);

        string resolveVariables(string sProcedureCommandLine)
            {
                return resolveLocalTables(resolveArguments(resolveLocalStrings(resolveLocalVars(sProcedureCommandLine))));
            }
};


#endif // PROCEDUREVARFACTORY_HPP

