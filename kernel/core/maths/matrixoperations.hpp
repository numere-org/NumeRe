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

#ifndef MATRIXOPERATIONS_HPP
#define MATRIXOPERATIONS_HPP

#include <string>
#include <vector>

#include "../ParserLib/muParser.h"
#include "../datamanagement/cache.hpp"
#include "define.hpp"
#include "../settings.hpp"

using namespace std;
using namespace mu;

// Erster Index: No. of Line; zweiter Index: No. of Col (push_back verwendet dazu stets zeilen!)
typedef vector<vector<double> > Matrix;

bool performMatrixOperation(string& sCmd, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
Matrix createZeroesMatrix(unsigned int nLines, unsigned int nCols);
Matrix transposeMatrix(const Matrix& _mMatrix);
Indices getIndices(const string& sCmd, const Matrix& _mMatrix, Parser& _parser, MemoryManager& _data, const Settings& _option);

void showMatrix(const vector<vector<double> >&);

#endif // MATRIXOPERATIONS_HPP

