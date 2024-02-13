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

#ifndef PARSER_FUNCTIONS_HPP
#define PARSER_FUNCTIONS_HPP

#include <string>
#include <vector>

#include "../settings.hpp"
#include "../datamanagement/memorymanager.hpp"
#include "../ParserLib/muParser.h"
#include "define.hpp"
#include "../datamanagement/dataaccess.hpp"

// Tools & Stuff
std::string evaluateTargetOptionInCommand(std::string& sCmd, const std::string& sDefaultTarget, Indices& _idx, mu::Parser& _parser, MemoryManager& _data, const Settings& _option);
size_t findVariableInExpression(const std::string& sExpr, const std::string& sVarName, size_t nPosStart = 0);
void convertVectorToExpression(std::string&, const Settings&);
std::string addMissingVectorComponent(const std::string&, const std::string&, const std::string&, bool);
mu::value_type* getPointerToVariable(const std::string& sVarName, mu::Parser& _parser);
std::string promptForUserInput(const std::string& __sCommand);
int integralFactorial(int nNumber);
bool evaluateIndices(const std::string& sCache, Indices& _idx, MemoryManager& _data);
std::vector<double> readAndParseIntervals(std::string& sExpr, mu::Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option, bool bEraseInterval = false);
size_t getPositionOfFirstDelimiter(StringView sLine);

#endif


