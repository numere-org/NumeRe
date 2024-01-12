/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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


// Class: Procedure

#ifndef PROCEDURE_HPP
#define PROCEDURE_HPP

#include <iostream>
#include <string>
#include <cmath>
#include <iomanip>
#include <fstream>

#include "../ui/error.hpp"
#include "../ParserLib/muParser.h"
#include "../datamanagement/memorymanager.hpp"
#include "../settings.hpp"
#include "../utils/tools.hpp"
#include "../built-in.hpp"
#include "../maths/parser_functions.hpp"
#include "../plotting/plotdata.hpp"
#include "flowctrl.hpp"
#include "plugin.hpp"
#include "../maths/define.hpp"
#include "../io/filesystem.hpp"
#include "../io/output.hpp"
#include "../script.hpp"

// forward declaration of the var factory
class ProcedureVarFactory;


/////////////////////////////////////////////////
/// \brief This class implements the logic to
/// evaluate complex procedures, which may be
/// called recursively.
/////////////////////////////////////////////////
class Procedure : public FlowCtrl, public PackageManager
{
    private:
        friend class NumeReDebugger;
        friend class FlowCtrl;
        friend class ProcedureVarFactory; // For FlowCtrl Enums

        std::fstream fProcedure;
        std::string sProcNames;
        std::string sCurrentProcedureName;
        int nCurrentLine;
        std::string sNameSpace;
        std::string sCallingNameSpace;
        std::string sThisNameSpace;
        std::string sLastWrittenProcedureFile;
        std::string sProcCommandLine;
        bool bProcSupressAnswer;
        bool bWritingTofile;
        int nFlags;
        int nthBlock;
        ProcedureVarFactory* _varFactory;

        FunctionDefinitionManager _localDef;

        void init();

        Returnvalue ProcCalc(std::string sLine, std::string sCurrentCommand, int& nByteCode, mu::Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script);
        bool setProcName(StringView sProc, bool bInstallFileName = false);
        void resetProcedure(mu::Parser& _parser, bool bSupressAnswer);
        void extractCurrentNamespace(StringView sProc);
        bool handleVariableDefinitions(std::string& sProcCommandLine, const std::string& sCommand);
        int handleIncludeSyntax(std::string& sProcCommandLine, std::ifstream& fInclude, bool bReadingFromInclude);
        void extractProcedureInformation(const std::string& sCmdLine, size_t nPos, std::string& sProcName, std::string& sArgList, std::string& sFileName);

        virtual int procedureCmdInterface(StringView sLine) override;
        virtual std::vector<std::string> expandInlineProcedures(std::string& sLine) override;
        int isInlineable(const std::string& sProc, const std::string& sFileName, int* nInlineFlag = nullptr);
        int applyInliningRuleset(const std::string& sCommandLine, const std::string& sArgumentList);
        size_t countProceduresInLine(const std::string& sCommandLine);
        std::vector<std::string> getInlined(const std::string& sProc, const std::string& sArgumentList, const std::string& sFileName, size_t nProcedures);

    public:
        Procedure();
        Procedure(const Procedure& _procedure);
        ~Procedure();

        Returnvalue execute(StringView sProc, std::string sVarList, mu::Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, size_t nth_procedure = 0);
        static std::string mangleName(std::string sProcedureName);
        static std::string nameSpaceToPath(std::string sEncodedNameSpace, const std::string& thisPath, const std::string& thisFilePath = "");
        static void cleanRelativeNameSpaces(std::string& nameSpace);
        virtual FlowCtrl::ProcedureInterfaceRetVal procedureInterface(std::string& sLine, mu::Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, int nth_command = 0) override;
        bool writeProcedure(std::string sProcedureLine);
        virtual int isInline(const std::string& sProc) override;
        virtual int evalDebuggerBreakPoint(mu::Parser& _parser, Settings& _option) override;
        virtual int getErrorInformationForDebugger() override;
        virtual int catchExceptionForTest(std::exception_ptr e_ptr, bool bSupressAnswer_back, int nLine, bool cleanUp) override;
        size_t GetCurrentLine() const;
        size_t replaceReturnVal(std::string& sLine, mu::Parser& _parser, const Returnvalue& _return, size_t nPos, size_t nPos2, const std::string& sReplaceName);

        void setPredefinedFuncs(const std::string& sPredefined)
        {
            _localDef.setPredefinedFuncs(sPredefined);
        }

        std::string getCurrentProcedureName() const
        {
            return sCurrentProcedureName;
        }

        int getReturnType() const
        {
            return nReturnType;
        }

        bool is_writing() const
        {
            return bWritingTofile;
        }

        int getProcedureFlags() const
        {
            return nFlags;
        }

        int& getDebuggerCode()
        {
            return nDebuggerCode;
        }

        std::string getThisNameSpace() const
        {
            return sThisNameSpace;
        }
};

#endif // PROCEDURE_HPP


