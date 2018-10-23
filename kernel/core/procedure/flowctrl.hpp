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
#include "../datamanagement/datafile.hpp"
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

class FlowCtrl
{
    private:
        string sVarName;
        double* dVarAdress;
        bool bLoopSupressAnswer;

        Parser* _parserRef;
        Datafile* _dataRef;
        Output* _outRef;
        Settings* _optionRef;
        Define* _functionRef;
        PlotData* _pDataRef;
        Script* _scriptRef;

    protected:
        enum CalculationType
        {
            CALCTYPE_NONE = 0,
            CALCTYPE_COMMAND = 1,
            CALCTYPE_NUMERICAL = 2,
            CALCTYPE_STRING = 4,
            CALCTYPE_DATAACCESS = 8,
            CALCTYPE_PROGRESS = 16,
            CALCTYPE_COMPOSE = 32,
            CALCTYPE_DEFINITION = 64,
            CALCTYPE_RETURNCOMMAND = 128,
            CALCTYPE_THROWCOMMAND = 256,
            CALCTYPE_DEBUGBREAKPOINT = 512,
            CALCTYPE_EXPLICIT = 1024,
            CALCTYPE_TOCOMMAND = 2048,
            CALCTYPE_PROCEDURECMDINTERFACE = 4096,
            CALCTYPE_CONTINUECMD = 8192,
            CALCTYPE_BREAKCMD = 16384,
            CALCTYPE_PROMPT = 32768,
            CALCTYPE_RECURSIVEEXPRESSION = 65536,
            CALCTYPE_SUPPRESSANSWER = 131072
        };
        string** sCmd;
        value_type** vVarArray;
        string* sVarArray;
        varmap_type vVars;
        int** nJumpTable;
        int* nCalcType;
        unsigned int nJumpTableLength;
        string sLoopNames;
        int nCmd;
        int nLoop;
        int nIf;
        int nWhile;
        int nDefaultLength;
        int nVarArray;
        int nReturnType;
        Returnvalue ReturnVal;
        bool bUseLoopParsingMode;
        bool bLockedPauseMode;
        bool bFunctionsReplaced;
        string sLoopPlotCompose;
        map<string,string> mVarMap;

        void generateCommandArray();
        int for_loop(int nth_Cmd = 0, int nth_Loop = 0);
        int while_loop(int nth_Cmd = 0, int nth_Loop = 0);
        int if_fork(int nth_Cmd = 0, int nth_Loop = -1);
        int calc(string sLine, int nthCmd, string sBlock);
        value_type* evalHeader(int& nNum, string& sHeadExpression, bool bIsForHead, int nth_Cmd);
        int evalLoopFlowCommands(int __j, int nth_loop);
        int evalForkFlowCommands(int __j, int nth_loop);
        void replaceLocalVars(string& sLine);
        bool bSilent;
        bool bMask;
        bool bPrintedStatus;
        bool bBreakSignal;
        bool bContinueSignal;
        bool bReturnSignal;
        int nLoopSavety;

        virtual int procedureCmdInterface(string& sLine);
        virtual int procedureInterface(string& sLine, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_loop, int nth_command);
        virtual bool isInline(const string& sProc);
        virtual void evalDebuggerBreakPoint(Parser& _parser, Settings& _option, const map<string,string>& sStringMap);

    public:
        FlowCtrl();
        FlowCtrl(int _nDefaultLength);
        ~FlowCtrl();

        int nthRecursion;
        inline int getLoop() const
            {return nLoop + nIf + nWhile;};
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
        void setCommand(string& __sCmd, Parser& _parser, Datafile& _data, Define& _functions, Settings& _option, Output& _out, PlotData& _pData, Script& _script);
        void eval(Parser& _parser, Datafile& _data, Define& _functions, Settings& _option, Output& _out, PlotData& _pData, Script& _script);
        void reset();

};


#endif
