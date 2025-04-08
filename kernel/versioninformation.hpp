/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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

#ifndef VERSIONINFORMATION_HPP
#define VERSIONINFORMATION_HPP

#include <string>
#include "core/utils/datetimetools.hpp"

sys_time_point getBuildDate();
std::string printBuildDate();
std::string getBuildYear();
std::string getVersion();
std::string getVersionName();
std::string getSubVersion();
std::string getFullVersion();
std::string getFullVersionWithArchitecture();
std::string getFileVersion();
double getFloatingPointVersion();

#endif // VERSIONINFORMATION_HPP

