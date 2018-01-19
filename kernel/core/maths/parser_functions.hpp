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


//#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <sstream>
#include <iostream>
#include <ios>
#include <iomanip>

#include <vector>
#include <boost/tokenizer.hpp>
#include <gsl/gsl_sort.h>
#include <gsl/gsl_statistics.h>
#include <Eigen/Dense>

#include "../ui/error.hpp"
#include "../settings.hpp"
#include "../datamanagement/datafile.hpp"
#include "../utils/tools.hpp"
#include "../ParserLib/muParser.h"
#include "define.hpp"
#include "../plotting/plotdata.hpp"
#include "../plugin.hpp"
#include "../plotting/graph_helper.hpp"
#include "fitcontroller.hpp"

using namespace std;
using namespace mu;


#ifndef PARSER_FUNCTIONS_HPP
#define PARSER_FUNCTIONS_HPP
extern const string sParserVersion;
extern int nLINE_LENGTH;

/*
 * Globale Variablen fuer die erweitererten Parser-Funktionen
 */





// Erster Index: No. of Line; zweiter Index: No. of Col (push_back verwendet dazu stets zeilen!)
typedef vector<vector<double> > Matrix;

void parser_ListVar(mu::ParserBase&, const Settings&, const Datafile&);
void parser_ListConst(const mu::ParserBase&, const Settings&);
void parser_ListExprVar(mu::ParserBase&, const Settings&, const Datafile&);
void parser_ListFunc(const Settings& _option, const string& sType = "all");
void parser_ListDefine(const Define& _functions, const Settings& _option);
void parser_ListLogical(const Settings& _option);
void parser_ListCmd(const Settings& _option);
void parser_ListUnits(const Settings& _option);
void parser_ListPlugins(Parser& _parser, Datafile& _data, const Settings& _option);

// Commands
vector<double> parser_Integrate(const string&, Datafile&, Parser&, const Settings&, Define&);
vector<double> parser_Integrate_2(const string&, Datafile&, Parser&, const Settings&, Define&);
vector<double> parser_Diff(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option, Define& _functions);
bool parser_findExtrema(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions);
bool parser_findZeroes(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions);
double parser_LocalizeExtremum(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
double parser_LocalizeZero(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
void parser_Taylor(string& sCmd, Parser& _parser, const Settings& _option, Define& _functions);
bool parser_fit(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool parser_fft(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool parser_wavelet(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool parser_evalPoints(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions);
bool parser_datagrid(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool parser_writeAudio(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool parser_regularize(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool parser_pulseAnalysis(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool parser_stfa(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool parser_spline(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);

void parser_splash(Parser&);

// Tools & Stuff
bool parser_CheckVarOccurence(Parser&, const string_type&);
string parser_GetDataElement(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option, bool bReplaceNANs = true);
void parser_VectorToExpr(string&, const Settings&);
string parser_AddVectorComponent(const string&, const string&, const string&, bool);
bool parser_ExprNotEmpty(const string&);
bool parser_CheckMultArgFunc(const string&, const string&);
void parser_ReplaceEntities(string&, const string&, Datafile&, Parser&, const Settings&, bool);
int parser_SplitArgs(string& sToSplit, string& sSecArg, const char& cSep, const Settings& _option, bool bIgnoreSurroundingParenthesis = false);
int parser_LineBreak(const Settings&);
void parser_CheckIndices(int&, int&);
void parser_CheckIndices(long long int&, long long int&);
double* parser_GetVarAdress(const string& sVarName, Parser& _parser);
string parser_Prompt(const string& __sCommand);
int int_faculty(int nNumber);
Indices parser_getIndices(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
Indices parser_getIndices(const string& sCmd, const Matrix& _mMatrix, Parser& _parser, Datafile& _data, const Settings& _option);
bool parser_parseCmdArg(const string& sCmd, const string& sParam, Parser& _parser, int& nArgument);
bool parser_evalIndices(const string& sCache, Indices& _idx, Datafile& _data);
vector<double> parser_IntervalReader(string& sExpr, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option, bool bEraseInterval = false);
void printUnits(const string& sUnit, const string& sDesc, const string& sDim, const string& sValues, unsigned int nWindowsize);

// String-Parser
int parser_StringParser(string&, string&, Datafile&, Parser&, const Settings&, bool bSilent = false);
unsigned int parser_getDelimiterPos(const string&);
int parser_countStrings(const string& sCmd);
int parser_numStrings(const string& sCmd);
string parser_getMaxString(const string& sCmd);
string parser_getMinString(const string& sCmd);
string parser_getSumString(const string& sCmd);

// Plotting
void parser_Plot(string& sCmd, Datafile& _data, Parser& _parser, Settings& _option, Define& _functions, PlotData& _pData);

// Matrix-Operations
bool parser_matrixOperations(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
Matrix parser_ZeroesMatrix(unsigned int nLines, unsigned int nCols);
Matrix parser_transposeMatrix(const Matrix& _mMatrix);
#endif
