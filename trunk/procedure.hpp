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

#include "error.hpp"
#include "ParserLib/muParser.h"
#include "datafile.hpp"
#include "settings.hpp"
#include "tools.hpp"
#include "built-in.hpp"
#include "parser_functions.hpp"
#include "plotdata.hpp"
#include "loop.hpp"
#include "plugin.hpp"
#include "define.hpp"
#include "filesystem.hpp"
#include "output.hpp"
#include "script.hpp"

using namespace std;
using namespace mu;

extern bool bSupressAnswer;

class Procedure : /*public FileSystem,*/ public Loop, public Plugin
{
    private:
        fstream fProcedure;
        string sProcNames;
        string sCurrentProcedureName;
        unsigned int nCurrentLine;
        string sProcKeep;
        string sProcPlotCompose;
        string sNameSpace;
        string sCallingNameSpace;
        string sThisNameSpace;
        string sLastWrittenProcedureFile;
        bool bProcSupressAnswer;
        bool bWritingTofile;
        int nFlags;
        int nthBlock;
        //int nReturnType;
        //Loop _block;

        Returnvalue ProcCalc(string sLine, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script);
        bool setProcName(const string& sProc, bool bInstallFileName = false);
        int procedureInterface(string& sLine, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_procedure = 0, int nth_command = 0);
        int procedureCmdInterface(string& sLine);
        void deleteVars(Parser& _parser, Datafile& _data, string** sLocalVars, unsigned int nLocalVarMapSize, double* dLocalVars, string** sLocalStrings, unsigned int nLocalStrMapSize, string** sVarMap, unsigned int nVarMapSize);
    public:
        Procedure();
        Procedure(const Procedure& _procedure);
        ~Procedure();

        Returnvalue execute(string sProc, string sVarList, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, unsigned int nth_procedure = 0);
        bool writeProcedure(string sProcedureLine);
        bool isInline(const string& sProc);
        inline string getCurrentProcedureName() const
            {return sCurrentProcedureName;}
        inline unsigned int GetCurrentLine() const
            {return nCurrentLine;}
        inline int  getReturnType() const
            {return nReturnType;}
        inline bool is_writing() const
            {return bWritingTofile;}
        void replaceReturnVal(string& sLine, Parser& _parser, const Returnvalue& _return, unsigned int nPos, unsigned int nPos2, const string& sReplaceName);
};

#endif // PROCEDURE_HPP
