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
#include "datafile.hpp"
#include "../settings.hpp"
#include "../ParserLib/muParser.h"
#include "../io/output.hpp"
#include "../maths/define.hpp"

using namespace std;
using namespace mu;

string** make_stringmatrix(Datafile& _data, Output& _out, Settings& _option, const string& sCache, long long int& nLines, long long int& nCols, int& nHeadlineCount, size_t nPrecision, bool bSave = true);
void load_data(Datafile& _data, Settings& _option, Parser& _parser, string sFileName = "");
void show_data(Datafile& _data, Output& _out, Settings& _option, const string& sCache, size_t nPrecision, bool bData = false, bool bCache = false, bool bSave = false, bool bDefaultName = true);
void remove_data(Datafile& _data, Settings& _option, bool bIgnore = false);
void append_data(const string& sCmd, Datafile& _data, Settings& _option);
void clear_cache(Datafile& _data, Settings& _option, bool bIgnore = false);
bool deleteCacheEntry(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool CopyData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool moveData(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option);
bool sortData(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
bool writeToFile(string& sCmd, Datafile& _data, Settings& _option);
bool readFromFile(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);
bool readImage(string& sCmd, Parser& _parser, Datafile& _data, Settings& _option);


#endif // DATAOPS_HPP
