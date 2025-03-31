/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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


#ifndef DATAOPS_HPP
#define DATAOPS_HPP

#include <string>
#include "memorymanager.hpp"
#include "../settings.hpp"
#include "../commandlineparser.hpp"
#include "../ParserLib/muParser.h"
#include "../io/output.hpp"
#include "../maths/define.hpp"

void load_data(MemoryManager& _data, Settings& _option, mu::Parser& _parser, std::string sFileName = "", std::string sFileFormat = "");
void append_data(CommandLineParser& cmdParser);
void clear_cache(MemoryManager& _data, Settings& _option, bool bIgnore = false);
bool deleteCacheEntry(CommandLineParser& cmdParser);
bool CopyData(CommandLineParser& cmdParser);
bool moveData(CommandLineParser& cmdParser);
bool sortData(CommandLineParser& cmdParser);
bool writeToFile(CommandLineParser& cmdParser);
bool readFromFile(CommandLineParser& cmdParser);
bool readImage(CommandLineParser& cmdParser);


#endif // DATAOPS_HPP
