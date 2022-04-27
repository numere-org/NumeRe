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
#include "datamanagement/memorymanager.hpp"
#include "settings.hpp"
#include "io/output.hpp"
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

#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

extern const std::string sVersion;

enum CommandReturnValues
{
    NUMERE_QUIT = -1,
    NO_COMMAND = 0,
    COMMAND_PROCESSED = 1,
    COMMAND_HAS_RETURNVALUE = 2
};

/*
 * Built-In-Funktionen
 * -> Diese Funktionen setzen die Basisfunktionen dieses Frameworks um
 */
CommandReturnValues commandHandler(std::string& sCmd);
std::string evaluateParameterValues(const std::string& sCmd);
bool extractFirstParameterStringValue(const std::string& sCmd, std::string& sArgument);
bool parseCmdArg(const std::string& sCmd, size_t nPos, mu::Parser& _parser, size_t& nArgument);


#endif

