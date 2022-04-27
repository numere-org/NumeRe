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


#ifndef FILEOPS_HPP
#define FILEOPS_HPP

#include <string>
#include <vector>
#include "../commandlineparser.hpp"
#include "../datamanagement/memorymanager.hpp"
#include "../settings.hpp"
#include "../ParserLib/muParser.h"

using namespace mu;

bool removeFile(CommandLineParser& cmdParser);
bool moveOrCopyFiles(CommandLineParser& cmdParser);
bool generateTemplate(const std::string& sFile, const std::string& sTempl, const std::vector<std::string>& vTokens, Settings& _option);


#endif // FILEOPS_HPP
