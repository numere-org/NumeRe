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


#ifndef DOCUMENTATION_HPP
#define DOCUMENTATION_HPP

#include <string>
#include "../settings.hpp"

extern const std::string sVersion;

void doc_Help(const std::string&, Settings&);
std::string doc_HelpAsHTML(const std::string&, bool, Settings& _option);
void doc_SearchFct(const std::string& sToLookFor, Settings& _option);


#endif

