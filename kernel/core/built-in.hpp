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
#include <string>
#include <conio.h>
#include <windows.h>
#include <cmath>
#include <vector>

#include "ui/error.hpp"
#include "datamanagement/datafile.hpp"
#include "settings.hpp"
#include "output.hpp"
#include "plugins.hpp"
#include "ParserLib/muParser.h"
#include "utils/tools.hpp"
#include "maths/parser_functions.hpp"
#include "maths/define.hpp"
#include "plotting/plotdata.hpp"
#include "script.hpp"
#include "version.h"
#include "documentation/documentation.hpp"
#include "maths/odesolver.hpp"

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
string** BI_make_stringmatrix(Datafile& _data, Output& _out, Settings& _option, const string& sCache, long long int& nLines, long long int& nCols, int& nHeadlineCount, bool bSave = true);
void BI_splash();
/** \brief Central command handler
 *
 * \param sCmd string&
 * \param _data Datafile&
 * \param _out Output&
 * \param _option Settings&
 * \param _parser Parser&
 * \param _functions Define&
 * \param _pData PlotData&
 * \param _script Script&
 * \param bParserActive bool
 * \return int
 *
 */
int BI_CommandHandler(string& sCmd, Datafile& _data, Output& _out, Settings& _option, Parser& _parser, Define& _functions, PlotData& _pData, Script& _script, bool bParserActive = false);
void BI_Basis();
void BI_Autosave(Datafile&, Output&, Settings&);
bool BI_FileExists(const string& sFilename);
string BI_Greeting(Settings& _option);
string BI_getVarList(const string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
string BI_evalParamString(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option, Define& _functions);

#endif
