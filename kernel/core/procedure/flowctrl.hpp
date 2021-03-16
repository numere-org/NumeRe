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
#include <iostream>

#include "../ui/error.hpp"
#include "../ParserLib/muParser.h"
#include "../datamanagement/memorymanager.hpp"
#include "../maths/define.hpp"
#include "../settings.hpp"
#include "../io/output.hpp"
#include "../plotting/plotdata.hpp"
#include "../script.hpp"
#include "../maths/parser_functions.hpp"
#include "../built-in.hpp"
#include "../utils/tools.hpp"


using namespace std;
using namespace mu;

struct FlowCtrlCommand
{
    string sCommand;
    int nInputLine;
    bool bFlowCtrlStatement;

    FlowCtrlCommand(const string& sCmd, int nLine, bool bStatement = false) : sCommand(sCmd), nInputLine(nLine), bFlowCtrlStatement(bStatement) {}
};

class FlowCtrl
{
    private:
        string sVarName;
        double* dVarAdress;
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
        vector<FlowCtrlCommand> vCmdArray;
        value_type** vVarArray;
        string* sVarArray;
        varmap_type vVars;
        int** nJumpTable;
        int* nCalcType;
        unsigned int nJumpTableLength;
        string sLoopNames;
        int nLoop;
        int nIf;
        int nWhile;
        int nSwitch;
        int nDefaultLength;
        int nVarArray;
        int nReturnType;
        int nCurrentCommand;
        Returnvalue ReturnVal;
        bool bUseLoopParsingMode;
        bool bLockedPauseMode;
        bool bFunctionsReplaced;
        string sLoopPlotCompose;
        map<string,string> mVarMap;
        bool bSilent;
        bool bMask;
        bool bPrintedStatus;
        bool bBreakSignal;
        bool bContinueSignal;
        bool bReturnSignal;
        bool bEvaluatingFlowControlStatements;
        int nLoopSavety;
        int nDebuggerCode;

        int for_loop(int nth_Cmd = 0, int nth_Loop = 0);
        int while_loop(int nth_Cmd = 0, int nth_Loop = 0);
        int if_fork(int nth_Cmd = 0, int nth_Loop = -1);
        int switch_fork(int nth_Cmd = 0, int nth_Loop = -1);
        int calc(string sLine, int nthCmd);
        value_type* evalHeader(int& nNum, string& sHeadExpression, bool bIsForHead, int nth_Cmd);
        int evalLoopFlowCommands(int __j, int nth_loop);
        int evalForkFlowCommands(int __j, int nth_loop);
        void replaceLocalVars(string& sLine);
        bool checkFlowControlArgument(const string& sFlowControlArgument, bool isForLoop = false);
        bool checkCaseValue(const string& sCaseDefinition);
        string extractFlagsAndIndexVariables();
        void fillJumpTableAndExpandRecursives();
        void prepareSwitchExpression(int nSwitchStart);
        void checkParsingModeAndExpandDefinitions();
        void prepareLocalVarsAndReplace(string& sVars);


        virtual int procedureCmdInterface(string& sLine);
        virtual int procedureInterface(string& sLine, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_loop, int nth_command);
        virtual int isInline(const string& sProc);
        virtual int evalDebuggerBreakPoint(Parser& _parser, Settings& _option);
        virtual int getErrorInformationForDebugger();
        virtual vector<string> expandInlineProcedures(string& sLine);
        virtual int catchExceptionForTest(exception_ptr e_ptr, bool bSupressAnswer_back, int nLine);

    public:
        FlowCtrl();
        FlowCtrl(int _nDefaultLength);
        virtual ~FlowCtrl();

        int nthRecursion;
        inline int getLoop() const
            {return nLoop + nIf + nWhile + nSwitch;};
        inline string getCurrentBlock() const
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
        void setCommand(string& __sCmd, int nCurrentLine);
        void eval();
        void reset();

        int getCurrentLineNumber() const;
        string getCurrentCommand() const;

};


#endif
