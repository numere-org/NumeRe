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

#ifndef LOOP_HPP
#define LOOP_HPP

#include <string>
#include <iostream>

#include "error.hpp"
#include "ParserLib/muParser.h"
#include "datafile.hpp"
#include "define.hpp"
#include "settings.hpp"
#include "output.hpp"
#include "plotdata.hpp"
#include "script.hpp"
#include "parser_functions.hpp"
#include "built-in.hpp"
#include "tools.hpp"


using namespace std;
using namespace mu;

class Loop
{
    private:
        string sVarName;
        double* dVarAdress;
        bool bLoopSupressAnswer;

    protected:
        string** sCmd;
        value_type** vVarArray;
        string* sVarArray;
        varmap_type vVars;
        //ParserByteCode* _bytecode;
        //int* nValidByteCode;
        int** nJumpTable;
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
        int for_loop(Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, int nth_Cmd = 0, int nth_Loop = 0);
        int while_loop(Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, int nth_Cmd = 0, int nth_Loop = 0);
        int if_fork(Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, int nth_Cmd = 0, int nth_Loop = -1);
        int calc(string sLine, int nthCmd, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, string sBlock);
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
        virtual void evalDebuggerBreakPoint(Settings& _option, const map<string,string>& sStringMap);

    public:
        Loop();
        Loop(int _nDefaultLength);
        ~Loop();

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
        void reset(Parser& _parser);

};


#endif
