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
#include "../datamanagement/memorymanager.hpp"
#include "define.hpp"
#include "../settings.hpp"
#include "matdatastructures.hpp"


bool performMatrixOperation(std::string& sCmd, mu::Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option);
Matrix createZeroesMatrix(size_t nLines, size_t nCols);
Matrix transposeMatrix(const Matrix& _mMatrix);
Indices getIndices(const std::string& sCmd, const Matrix& _mMatrix, mu::Parser& _parser, MemoryManager& _data, const Settings& _option);

void showMatrix(const std::vector<std::vector<mu::value_type> >&);

#endif // MATRIXOPERATIONS_HPP

