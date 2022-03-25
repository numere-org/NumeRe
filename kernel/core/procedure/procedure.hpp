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

using namespace std;
using namespace mu;

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
        friend class ProcedureVarFactory; // For FlowCtrl Enums

        fstream fProcedure;
        string sProcNames;
        string sCurrentProcedureName;
        int nCurrentLine;
        string sNameSpace;
        string sCallingNameSpace;
        string sThisNameSpace;
        string sLastWrittenProcedureFile;
        string sProcCommandLine;
        bool bProcSupressAnswer;
        bool bWritingTofile;
        int nFlags;
        int nthBlock;
        ProcedureVarFactory* _varFactory;

        FunctionDefinitionManager _localDef;

        void init();

        Returnvalue ProcCalc(string sLine, string sCurrentCommand, int& nByteCode, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script);
        bool setProcName(const string& sProc, bool bInstallFileName = false);
        void resetProcedure(Parser& _parser, bool bSupressAnswer);
        void extractCurrentNamespace(const string& sProc);
        bool handleVariableDefinitions(string& sProcCommandLine, const string& sCommand);
        void readFromInclude(ifstream& fInclude, int nIncludeType, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_procedure);
        int handleIncludeSyntax(string& sProcCommandLine, ifstream& fInclude, bool bReadingFromInclude);
        void extractProcedureInformation(const string& sCmdLine, size_t nPos, string& sProcName, string& sArgList, string& sFileName);

        virtual int procedureCmdInterface(string& sLine) override;
        virtual vector<string> expandInlineProcedures(string& sLine) override;
        int isInlineable(const string& sProc, const string& sFileName, int* nInlineFlag = nullptr);
        int applyInliningRuleset(const string& sCommandLine, const string& sArgumentList);
        size_t countProceduresInLine(const string& sCommandLine);
        vector<string> getInlined(const string& sProc, const string& sArgumentList, const string& sFileName, size_t nProcedures);

    public:
        Procedure();
        Procedure(const Procedure& _procedure);
        ~Procedure();

        Returnvalue execute(string sProc, string sVarList, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, unsigned int nth_procedure = 0);
        static std::string mangleName(std::string sProcedureName);
        virtual FlowCtrl::ProcedureInterfaceRetVal procedureInterface(string& sLine, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, int nth_command = 0) override;
        bool writeProcedure(string sProcedureLine);
        virtual int isInline(const string& sProc) override;
        virtual int evalDebuggerBreakPoint(Parser& _parser, Settings& _option) override;
        virtual int getErrorInformationForDebugger() override;
        virtual int catchExceptionForTest(exception_ptr e_ptr, bool bSupressAnswer_back, int nLine) override;

        inline void setPredefinedFuncs(const string& sPredefined)
            {
                _localDef.setPredefinedFuncs(sPredefined);
            }
        inline string getCurrentProcedureName() const
            {return sCurrentProcedureName;}
        unsigned int GetCurrentLine() const;
        inline int getReturnType() const
            {return nReturnType;}
        inline bool is_writing() const
            {return bWritingTofile;}
        inline int getProcedureFlags() const
            {return nFlags;}
        inline int& getDebuggerCode()
            {
                return nDebuggerCode;
            }
        size_t replaceReturnVal(string& sLine, Parser& _parser, const Returnvalue& _return, unsigned int nPos, unsigned int nPos2, const string& sReplaceName);
};

#endif // PROCEDURE_HPP


