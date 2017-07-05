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

#include "tools.hpp"
#include "error.hpp"
#include "datafile.hpp"
#include "parser.hpp"
#include "define.hpp"
#include "settings.hpp"
#include "output.hpp"
#include "plotdata.hpp"
#include "script.hpp"

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

        string resolveArguments(string sProcedureCommandLine);
        string resolveLocalVars(string sProcedureCommandLine);
        string resolveLocalStrings(string sProcedureCommandLine);
        string resolveLocalTables(string sProcedureCommandLine);

        unsigned int countVarListElements(const string& sVarList);

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
        ProcedureVarFactory(Procedure* _procedure, Parser* _parser, Datafile* _data, Settings* _option, Define* _functions, Output* _out, PlotData* _pData, Script* _script, const string& sProc, unsigned int currentProc);
        ~ProcedureVarFactory();

        map<string,string> createProcedureArguments(string sArgumentList, string sArgumentValues);
        void createLocalVars(string sVarList);
        void createLocalStrings(string sStringList);
        void createLocalTables(string sTableList);

        string resolveVariables(string sProcedureCommandLine)
            {
                return resolveLocalTables(resolveLocalStrings(resolveLocalVars(resolveArguments(sProcedureCommandLine))));
            }
};


#endif // PROCEDUREVARFACTORY_HPP

