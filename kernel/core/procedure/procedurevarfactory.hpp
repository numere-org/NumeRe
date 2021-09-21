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
#include "../datamanagement/memorymanager.hpp"
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

/////////////////////////////////////////////////
/// \brief This class is the variable factory
/// used by procedure instances to create their
/// local variables and resolve calls to them.
/////////////////////////////////////////////////
class ProcedureVarFactory
{
    private:
        Parser* _parserRef;
        MemoryManager* _dataRef;
        Settings* _optionRef;
        FunctionDefinitionManager* _functionRef;
        Output* _outRef;
        PlotData* _pDataRef;
        Script* _scriptRef;

        Procedure* _currentProcedure;

        string sProcName;
        unsigned int nth_procedure;
        bool inliningMode;

        enum VarType
        {
            NUMTYPE,
            STRINGTYPE,
            CLUSTERTYPE,
            TABLETYPE
        };

        std::map<std::string,VarType> mLocalArgs;

        void init();

        string replaceProcedureName(string sProcedureName) const;
        std::string createMangledArgName(const std::string& sDefinedName) const;
        std::string createMangledVarName(const std::string& sDefinedName) const;

        string resolveArguments(string sProcedureCommandLine, size_t nMapSize = string::npos);
        string resolveLocalVars(string sProcedureCommandLine, size_t nMapSize = string::npos);
        string resolveLocalStrings(string sProcedureCommandLine, size_t nMapSize = string::npos);
        string resolveLocalTables(string sProcedureCommandLine, size_t nMapSize = string::npos);
        string resolveLocalClusters(string sProcedureCommandLine, size_t nMapSize = string::npos);
        unsigned int countVarListElements(const string& sVarList);
        void checkArgument(const string& sArgument, const string& sArgumentList, unsigned int nCurrentIndex);
        void checkArgumentValue(const string& sArgument, const string& sArgumentList, unsigned int nCurrentIndex);
        bool checkSymbolName(const std::string& sSymbolName) const;
        void createLocalInlineVars(string sVarList);
        void createLocalInlineStrings(string sVarList);
        void evaluateProcedureArguments(std::string& currentArg, std::string& currentValue, const std::string& sArgumentList);

    public:
        std::map<std::string, std::string> mArguments;
        std::map<std::string, std::pair<std::string, mu::value_type*>> mLocalVars;
        std::map<std::string, std::pair<std::string, std::string>> mLocalStrings;
        std::map<std::string, std::string> mLocalTables;
        std::map<std::string, std::string> mLocalClusters;

        string sInlineVarDef;
        string sInlineStringDef;

        ProcedureVarFactory();
        ProcedureVarFactory(Procedure* _procedure, const string& sProc, unsigned int currentProc, bool _inliningMode = false);
        ~ProcedureVarFactory();

        void reset();
        bool isReference(const std::string& sArgName) const;
        map<string,string> createProcedureArguments(string sArgumentList, string sArgumentValues);
        void createLocalVars(string sVarList);
        void createLocalStrings(string sStringList);
        void createLocalTables(string sTableList);
        void createLocalClusters(string sClusterList);
        std::string createTestStatsCluster();

        string resolveVariables(const string& sProcedureCommandLine)
            {
                return resolveLocalTables(resolveLocalClusters(resolveArguments(resolveLocalStrings(resolveLocalVars(sProcedureCommandLine)))));
            }
};


#endif // PROCEDUREVARFACTORY_HPP

