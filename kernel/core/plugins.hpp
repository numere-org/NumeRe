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
#include <cmath>
#include <ctime>
#include <random>
#include <iomanip>
#include <mgl2/mgl.h>

#include <boost/math/distributions/students_t.hpp>

#include "ui/error.hpp"
#include "datamanagement/cache.hpp"
#include "io/output.hpp"
#include "settings.hpp"
#include "utils/tools.hpp"
#include "plotting/plotdata.hpp"

#ifndef PLUGINS_HPP
#define PLUGINS_HPP
using namespace std;

/*
 * Headerdatei fuer alle Plugins!
 */

// --> Prototypen der plugin_*-Funktionen. Nur fuer den Compiler <--
void plugin_statistics(string& sCmd, MemoryManager& _data, Output& _out, Settings& _option, bool bUseCache = false, bool bUseData = false);
void plugin_histogram(string& sCmd, MemoryManager& _data, MemoryManager& _target, Output& _out, Settings& _option, PlotData& _pData, bool bUseCache = false, bool bUseData = false);
void plugin_random(string& sCmd, MemoryManager& _data, Output& _out, Settings& _option, bool bAllowOverride = false);
#endif
