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

#include "../datamanagement/memorymanager.hpp"
#include "../ParserLib/muParser.h"
#include "../settings.hpp"
#include "../commandlineparser.hpp"
#include "define.hpp"

#include <string>
#include <vector>

bool integrate(CommandLineParser& cmdParser);
bool integrate2d(CommandLineParser& cmdParser);
bool differentiate(CommandLineParser& cmdParser);
bool findExtrema(CommandLineParser& cmdParser);
bool findZeroes(CommandLineParser& cmdParser);
void taylor(CommandLineParser& cmdParser);
bool fitDataSet(CommandLineParser& cmdParser);
bool fastFourierTransform(CommandLineParser& cmdParser);
bool fastWaveletTransform(CommandLineParser& cmdParser);
bool evalPoints(CommandLineParser& cmdParser);
bool createDatagrid(CommandLineParser& cmdParser);
bool writeAudioFile(CommandLineParser& cmdParser);
bool readAudioFile(CommandLineParser& cmdParser);
bool seekInAudioFile(CommandLineParser& cmdParser);
bool regularizeDataSet(CommandLineParser& cmdParser);
bool analyzePulse(CommandLineParser& cmdParser);
bool shortTimeFourierAnalysis(CommandLineParser& cmdParser);
void boneDetection(CommandLineParser& cmdParser);
bool calculateSplines(CommandLineParser& cmdParser);
void rotateTable(CommandLineParser& cmdParser);
void particleSwarmOptimizer(CommandLineParser& cmdParser);
void urlExecute(CommandLineParser& cmdParser);
void mailClient(CommandLineParser& cmdParser);

#endif // COMMAND_IMPLEMENTATIONS_HPP


