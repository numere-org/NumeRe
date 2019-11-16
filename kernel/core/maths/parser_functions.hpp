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


#include <string>
#include <vector>

#include "../settings.hpp"
#include "../datamanagement/datafile.hpp"
#include "../ParserLib/muParser.h"
#include "define.hpp"
#include "../datamanagement/dataaccess.hpp"

using namespace std;
using namespace mu;

#ifndef PARSER_FUNCTIONS_HPP
#define PARSER_FUNCTIONS_HPP

// Tools & Stuff
string evaluateTargetOptionInCommand(string& sCmd, const string& sDefaultTarget, Indices& _idx, Parser& _parser, Datafile& _data, const Settings& _option);
bool isVariableInAssignedExpression(Parser&, const string_type&);
size_t findVariableInExpression(const string& sExpr, const string& sVarName);
void convertVectorToExpression(string&, const Settings&);
string addMissingVectorComponent(const string&, const string&, const string&, bool);
double* getPointerToVariable(const string& sVarName, Parser& _parser);
string promptForUserInput(const string& __sCommand);
int integralFactorial(int nNumber);
bool evaluateIndices(const string& sCache, Indices& _idx, Datafile& _data);
vector<double> readAndParseIntervals(string& sExpr, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option, bool bEraseInterval = false);
unsigned int getPositionOfFirstDelimiter(const string&);

#endif


