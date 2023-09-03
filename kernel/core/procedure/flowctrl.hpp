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


// Header zu Klasse "Loop"

#ifndef FLOWCTRL_HPP
#define FLOWCTRL_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

#include "../ParserLib/muParser.h"
#include "../datamanagement/memorymanager.hpp"
#include "../maths/define.hpp"
#include "../settings.hpp"
#include "../io/output.hpp"
#include "../plotting/plotdata.hpp"
#include "../script.hpp"


using namespace mu;

struct FlowCtrlCommand;


class FlowCtrl
{
    friend FlowCtrlCommand;

    private:
        bool bLoopSupressAnswer;

        Parser* _parserRef;
        MemoryManager* _dataRef;
        Output* _outRef;
        Settings* _optionRef;
        FunctionDefinitionManager* _functionRef;
        PlotData* _pDataRef;
        Script* _scriptRef;

    protected:
        enum CalculationType
        {
            CALCTYPE_NONE = 0x0,
            CALCTYPE_COMMAND = 0x1,
            CALCTYPE_NUMERICAL = 0x2,
            CALCTYPE_STRING = 0x4,
            CALCTYPE_DATAACCESS = 0x8,
            CALCTYPE_PROGRESS = 0x10,
            CALCTYPE_COMPOSE = 0x20,
            CALCTYPE_DEFINITION = 0x40,
            CALCTYPE_RETURNCOMMAND = 0x80,
            CALCTYPE_THROWCOMMAND = 0x100,
            CALCTYPE_DEBUGBREAKPOINT = 0x200,
            CALCTYPE_EXPLICIT = 0x400,
            CALCTYPE_TOCOMMAND = 0x800,
            CALCTYPE_PROCEDURECMDINTERFACE = 0x1000,
            CALCTYPE_CONTINUECMD = 0x2000,
            CALCTYPE_BREAKCMD = 0x4000,
            CALCTYPE_PROMPT = 0x8000,
            CALCTYPE_RECURSIVEEXPRESSION = 0x10000,
            CALCTYPE_SUPPRESSANSWER = 0x20000,
            CALCTYPE_ASSERT = 0x40000
        };

        enum ProcedureInterfaceRetVal
        {
            INTERFACE_EMPTY = -2,
            INTERFACE_ERROR = -1,
            INTERFACE_NONE = 1,
            INTERFACE_VALUE = 2
        };

        enum FlowCtrlStatement
        {
            FC_FOR,
            FC_IF,
            FC_WHILE,
            FC_SWITCH,
            FC_TRY,
            FC_COUNT
        };

        std::vector<FlowCtrlCommand> vCmdArray;
        std::vector<value_type> vVarArray;
        std::vector<std::string> sVarArray;
        varmap_type vVars;
        std::vector<std::vector<int>> nJumpTable;
        std::vector<int> nCalcType;
        std::string sLoopNames;

        int nFlowCtrlStatements[FC_COUNT];

        int nCurrentCommand;
        Returnvalue ReturnVal;
        bool bUseLoopParsingMode;
        bool bLockedPauseMode;
        bool bFunctionsReplaced;
        std::string sLoopPlotCompose;
        std::map<std::string,std::string> mVarMap;
        std::set<std::string> inlineClusters;
        bool bSilent;
        bool bMask;
        bool bPrintedStatus;
        bool bEvaluatingFlowControlStatements;
        int nLoopSafety;
        int nDebuggerCode;
        AssertionStats baseline;
        std::string sTestClusterName;
        int nthRecursion;

        int nReturnType;
        bool bReturnSignal;

        int for_loop(int nth_Cmd = 0, int nth_Loop = 0);
        int range_based_for_loop(int nth_Cmd = 0, int nth_Loop = 0);
        int while_loop(int nth_Cmd = 0, int nth_Loop = 0);
        int if_fork(int nth_Cmd = 0, int nth_Loop = -1);
        int switch_fork(int nth_Cmd = 0, int nth_Loop = -1);
        int try_catch(int nth_Cmd = 0, int nth_Loop = -1);

        typedef int(FlowCtrl::*FlowCtrlFunction)(int, int); ///< Definition of a generic FlowCtrl entry point

        int compile(std::string sLine, int nthCmd);
        int calc(StringView sLine, int nthCmd);
        value_type* evalHeader(int& nNum, std::string& sHeadExpression, bool bIsForHead, int nth_Cmd, const std::string& sHeadCommand);
        NumeRe::Cluster evalRangeBasedHeader(std::string& sHeadExpression, int nth_Cmd, const std::string& sHeadCommand);
        int evalForkFlowCommands(int __j, int nth_loop);

        void replaceLocalVars(MutableStringView sLine);
        void replaceLocalVars(const std::string& sOldVar, const std::string& sNewVar, size_t from = 0, size_t to = std::string::npos);

        bool checkFlowControlArgument(const std::string& sFlowControlArgument, bool isForLoop = false);
        bool checkCaseValue(const std::string& sCaseDefinition);

        std::string extractFlagsAndIndexVariables();
        void fillJumpTableAndExpandRecursives();
        void prepareSwitchExpression(int nSwitchStart);
        void checkParsingModeAndExpandDefinitions();
        void prepareLocalVarsAndReplace(std::string& sVars);
        void updateTestStats();


        virtual int procedureCmdInterface(std::string& sLine);
        virtual ProcedureInterfaceRetVal procedureInterface(std::string& sLine, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, int nth_command);
        virtual int isInline(const std::string& sProc);
        virtual int evalDebuggerBreakPoint(Parser& _parser, Settings& _option);
        virtual int getErrorInformationForDebugger();
        virtual std::vector<std::string> expandInlineProcedures(std::string& sLine);
        virtual int catchExceptionForTest(std::exception_ptr e_ptr, bool bSupressAnswer_back, int nLine);

    public:
        FlowCtrl();
        virtual ~FlowCtrl();

        int getCurrentBlockDepth() const;
        inline std::string getCurrentBlock() const
            {
                if (sLoopNames.length())
                    return sLoopNames.substr(sLoopNames.rfind(';')+1);
                else
                    return "";
            }
        inline Returnvalue getReturnValue() const
            {
                return ReturnVal;
            }
        inline bool getReturnSignal() const
            {
                return bReturnSignal;
            }
        void setCommand(std::string& __sCmd, int nCurrentLine);
        void eval();
        void reset();

        int getCurrentLineNumber() const;
        std::string getCurrentCommand() const;

        static bool isFlowCtrlStatement(const std::string& sCmd);
        static bool isAnyFlowCtrlStatement(const std::string& sCmd);

};


struct FlowCtrlCommand
{
    std::string sCommand;
    int nInputLine;
    bool bFlowCtrlStatement;
    std::string sFlowCtrlHeader;
    int nVarIndex;
    size_t nRFStepping;

    FlowCtrl::FlowCtrlFunction fcFn;


    FlowCtrlCommand(const std::string& sCmd, int nLine, bool bStatement = false, FlowCtrl::FlowCtrlFunction fn = nullptr)
        : sCommand(sCmd), nInputLine(nLine), bFlowCtrlStatement(bStatement), sFlowCtrlHeader(""), nVarIndex(-1), nRFStepping(0u), fcFn(fn) {}
};

#endif


