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


#ifndef PLUGINS_HPP
#define PLUGINS_HPP

#include <string>
#include <cmath>
#include <mgl2/mgl.h>

#include "ui/error.hpp"
#include "datamanagement/memorymanager.hpp"
#include "io/output.hpp"
#include "settings.hpp"
#include "utils/tools.hpp"
#include "plotting/plotdata.hpp"

/*
 * Headerdatei fuer alle Plugins!
 */

void plugin_statistics(std::string& sCmd, MemoryManager& _data);
void plugin_histogram(std::string& sCmd, MemoryManager& _data, MemoryManager& _target, Output& _out, Settings& _option, PlotData& _pData, bool bUseCache = false, bool bUseData = false);
void plugin_random(std::string& sCmd);
#endif

