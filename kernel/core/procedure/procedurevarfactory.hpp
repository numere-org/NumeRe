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
        mu::Parser* _parserRef;
        MemoryManager* _dataRef;
        Settings* _optionRef;
        FunctionDefinitionManager* _functionRef;
        PlotData* _pDataRef;
        Script* _scriptRef;

        Procedure* _currentProcedure;

        std::string sProcName;
        size_t nth_procedure;
        bool inliningMode;

        enum VarType
        {
            NUMTYPE,
            CLUSTERTYPE,
            TABLETYPE
        };

        std::map<std::string,VarType> mLocalArgs;

        void init();

        std::string replaceProcedureName(std::string sProcedureName) const;
        std::string createMangledArgName(const std::string& sDefinedName) const;
        std::string createMangledVarName(const std::string& sDefinedName) const;

        std::string resolveArguments(std::string sProcedureCommandLine, size_t nMapSize = std::string::npos);
        std::string resolveLocalVars(std::string sProcedureCommandLine, size_t nMapSize = std::string::npos);
        std::string resolveLocalTables(std::string sProcedureCommandLine, size_t nMapSize = std::string::npos);
        size_t countVarListElements(const std::string& sVarList);
        void checkArgument(const std::string& sArgument, const std::string& sArgumentList, size_t nCurrentIndex);
        void checkArgumentValue(const std::string& sArgument, const std::string& sArgumentList, size_t nCurrentIndex);
        bool checkSymbolName(const std::string& sSymbolName) const;
        void createLocalInlineVars(std::string sVarList, mu::DataType defType);
        void evaluateProcedureArguments(std::string& currentArg, std::string& currentValue, const std::string& sArgumentList);

    public:
        std::map<std::string, std::string> mArguments;
        std::map<std::string, std::pair<std::string, mu::Variable*>> mLocalVars;
        std::map<std::string, std::string> mLocalTables;

        std::string sInlineVarDef;
        std::vector<std::string> vInlineArgDef;

        ProcedureVarFactory();
        ProcedureVarFactory(Procedure* _procedure, const std::string& sProc, size_t currentProc, bool _inliningMode = false);
        ~ProcedureVarFactory();

        void reset();
        bool delayDeletionOfReturnedTable(const std::string& sTableName);
        bool isReference(const std::string& sArgName) const;
        std::map<std::string,std::string> createProcedureArguments(std::string sArgumentList, std::string sArgumentValues);
        void createLocalVars(std::string sVarList, mu::DataType defType = mu::TYPE_NUMERICAL);
        void createLocalStrings(std::string sStringList)
        {
            return createLocalVars(sStringList, mu::TYPE_STRING);
        }
        void createLocalClasses(std::string sClassList)
        {
            return createLocalVars(sClassList, mu::TYPE_OBJECT);
        }
        void createLocalTables(std::string sTableList);
        void createLocalClusters(std::string sClusterList)
        {
            return createLocalVars(sClusterList, mu::TYPE_CLUSTER);
        }
        std::string createTestStatsCluster();

        std::string resolveVariables(const std::string& sProcedureCommandLine)
            {
                return resolveLocalTables(resolveArguments(resolveLocalVars(sProcedureCommandLine)));
            }
};


#endif // PROCEDUREVARFACTORY_HPP

