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

#include <string>
#include "../structures.hpp"

#ifndef SORTER_HPP
#define SORTER_HPP

/////////////////////////////////////////////////
/// \brief Abstract parent class to implement
/// the sorting functionality (using Quicksort)
/// on a more generic level.
/////////////////////////////////////////////////
class Sorter
{
    private:
        // Quicksort implementation
        bool qSortImplementation(int* nIndex, int nElements, int nColumn, long long int nLeft, long long int nRight, int nSign);

    protected:
        // Comparision functions (have to be implemented in any derived class)
        virtual int compare(int i, int j, int col) = 0; // -1 if  i < j, 0 for i == j and +1 for j > 0
        virtual bool isValue(int line, int col) = 0;

    public:
        virtual ~Sorter() {};

        // Quicksort interface function
        bool qSort(int* nIndex, int nElements, int nColumn, long long int nLeft, long long int nRight, int nSign);

        // Function for hierarchical sorting
        bool sortSubList(int* nIndex, int nElements, ColumnKeys* KeyList, long long int i1, long long int i2, long long int j1, int nSign, long long int nColumns);

        // Function for preparing the ColumnKeys object based upon the passed string
        ColumnKeys* evaluateKeyList(std::string& sKeyList, long long int nColumnCount);

};


#endif // SORTER_HPP

