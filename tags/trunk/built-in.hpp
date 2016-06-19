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

#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <sstream>
//#include <cstring>
#include <string>
#include <conio.h>
#include <windows.h>
#include <cmath>
#include <vector>

#include "error.hpp"
#include "datafile.hpp"
#include "settings.hpp"
#include "output.hpp"
#include "plugins.hpp"
#include "ParserLib/muParser.h"
#include "tools.hpp"
//#include "menues.hpp"
#include "parser_functions.hpp"
#include "define.hpp"
#include "plotdata.hpp"
#include "script.hpp"
#include "version.h"
#include "documentation.hpp"
#include "odesolver.hpp"

#ifndef BUILT_IN_HPP
#define BUILT_IN_HPP

using namespace std;
using namespace mu;


extern const string sVersion;
extern int nLINE_LENGTH;
extern const string PI_TT;
extern const string PI_HIST;
extern const string PI_MED;
extern const string PI_RAND;
extern const string sParserVersion;

/*
 * Built-In-Funktionen
 * -> Diese Funktionen setzen die Basisfunktionen dieses Frameworks um
 */
void BI_load_data(Datafile& _data, Settings& _option, Parser& _parser, string sFileName = "");
void BI_show_data(Datafile& _data, Output& _out, Settings& _option, const string& sCache, bool bData = false, bool bCache = false, bool bSave = false, bool bDefaultName = true);
void BI_remove_data(Datafile& _data, Settings& _option, bool bIgnore = false);
//void BI_edit_header(Datafile&, Settings&);
void BI_append_data(const string& sCmd, Datafile& _data, Settings& _option, Parser& _parser);
void BI_clear_cache(Datafile& _data, Settings& _option, bool bIgnore = false);
void BI_show_credits(Parser& _parser, Settings& _option);
void BI_splash();
//void BI_hline(int nLength = -1);
int BI_CheckKeyword(string& sCmd, Datafile& _data, Output& _out, Settings& _option, Parser& _parser, Define& _functions, PlotData& _pData, Script& _script, bool bParserActive = false);
void BI_Basis();
//int BI_menue_based(Datafile&, Output&, Settings&, Parser&, Define&, PlotData&, Script&);
void BI_Autosave(Datafile&, Output&, Settings&);
bool BI_FileExists(const string& sFilename);
void BI_ListOptions(Settings& _option);
bool BI_parseStringArgs(const string& sCmd, string& sArgument, Parser& _parser, Datafile& _data, Settings& _option);
string BI_Greeting(Settings& _option);
string BI_evalParamString(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option, Define& _functions);
bool BI_deleteCacheEntry(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool BI_ListFiles(const string& sCmd, const Settings& _option);
bool BI_ListDirectory(const string& sDir, const string& sParams, const Settings& _option);
bool BI_CopyData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool BI_moveData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool BI_removeFile(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool BI_moveFile(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool BI_copyFile(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool BI_newObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
bool BI_editObject(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
bool BI_writeToFile(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
bool BI_readFromFile(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
string BI_getVarList(const string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
bool BI_generateTemplate(const string& sFile, const string& sTempl, const vector<string>& vTokens, Settings& _option);

#endif
