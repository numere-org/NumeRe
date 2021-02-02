/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#ifndef COMMAND_IMPLEMENTATIONS_HPP
#define COMMAND_IMPLEMENTATIONS_HPP

#include "../datamanagement/datafile.hpp"
#include "../datamanagement/memorymanager.hpp"
#include "../ParserLib/muParser.h"
#include "../settings.hpp"
#include "define.hpp"

#include <string>
#include <vector>

using namespace std;
using namespace mu;

vector<double> integrate(const string&, MemoryManager&, Parser&, const Settings&, FunctionDefinitionManager&);
vector<double> integrate2d(const string&, MemoryManager&, Parser&, const Settings&, FunctionDefinitionManager&);
vector<double> differentiate(const string& sCmd, Parser& _parser, MemoryManager& _data, const Settings& _option, FunctionDefinitionManager& _functions);
bool findExtrema(string& sCmd, MemoryManager& _data, Parser& _parser, const Settings& _option, FunctionDefinitionManager& _functions);
bool findZeroes(string& sCmd, MemoryManager& _data, Parser& _parser, const Settings& _option, FunctionDefinitionManager& _functions);
void taylor(string& sCmd, Parser& _parser, const Settings& _option, FunctionDefinitionManager& _functions);
bool fitDataSet(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
bool fastFourierTransform(string& sCmd, Parser& _parser, MemoryManager& _data, const Settings& _option);
bool fastWaveletTransform(string& sCmd, Parser& _parser, MemoryManager& _data, const Settings& _option);
bool evalPoints(string& sCmd, MemoryManager& _data, Parser& _parser, const Settings& _option, FunctionDefinitionManager& _functions);
bool createDatagrid(string& sCmd, string& sTargetCache, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
bool writeAudioFile(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
bool regularizeDataSet(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
bool analyzePulse(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
bool shortTimeFourierAnalysis(string& sCmd, string& sTargetCache, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
void boneDetection(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
bool calculateSplines(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
void rotateTable(std::string& sCmd);

#endif // COMMAND_IMPLEMENTATIONS_HPP


