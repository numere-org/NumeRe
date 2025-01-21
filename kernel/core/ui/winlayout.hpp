/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef WINLAYOUT_HPP
#define WINLAYOUT_HPP

#include <string>
#include <set>

#include "../commandlineparser.hpp"

void windowCommand(CommandLineParser& cmdParser);
std::set<std::string> getEventProcedures(const std::string& sLayoutFile);
void dialogCommand(CommandLineParser& cmdParser);


#endif // WINLAYOUT_HPP



