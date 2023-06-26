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
#include <map>
#include <vector>

#include "sorter.hpp"

#ifndef STRINGMEMORY_HPP
#define STRINGMEMORY_HPP

// Internal string memory implementation. Separated to be able to
// use Sorter as base class for strings and for the DataFile object
class StringInternalMemory : public Sorter
{
    private:
        bool bSortCaseInsensitive;

        void reorderColumn(const std::vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
		virtual int compare(int i, int j, int col) override;
		virtual bool isValue(int line, int col) override;

    public:
        StringInternalMemory() : bSortCaseInsensitive(false) {}

        std::vector<std::vector<std::string> > sStrings;

		std::vector<int> sortElements(long long int i1, long long int i2, long long int j1, long long int j2, const std::string& sSortingExpression);
};


// The string memory class, which will be included into the Cache
// by inheritance
class StringMemory
{
    private:
        StringInternalMemory _stringIntMem;

    public:
		bool writeString(const std::string& _sString, size_t _nthString = std::string::npos, size_t nCol = 0);
		std::string readString(size_t _nthString = std::string::npos, size_t nCol = 0);

		std::string maxString(size_t i1 = 0, size_t i2 = std::string::npos, size_t nCol = 0);
		std::string maxString(VectorIndex _vLine, VectorIndex _vCol);
		std::string minString(size_t i1 = 0, size_t i2 = std::string::npos, size_t nCol = 0);
		std::string minString(VectorIndex _vLine, VectorIndex _vCol);
		std::string sumString(size_t i1 = 0, size_t i2 = std::string::npos, size_t nCol = 0);
		std::string sumString(VectorIndex _vLine, VectorIndex _vCol);

		// Returns the number of stored elements in either a
		// specific column or the maximal line count of the
		// whole object
		inline size_t getStringElements(size_t nCol = std::string::npos) const
		{
			if (nCol == std::string::npos)
			{
			    // No column selected. Return the maximal line count
				size_t nCnt = 0;

				for (size_t i = 0; i < _stringIntMem.sStrings.size(); i++)
				{
					if (nCnt < _stringIntMem.sStrings[i].size())
						nCnt = _stringIntMem.sStrings[i].size();
				}

				return nCnt;
			}
			else if (nCol >= _stringIntMem.sStrings.size())
				return 0u;
			else
				return _stringIntMem.sStrings[nCol].size();
			return 0u;
		}

		// Return number of columns
		inline size_t getStringCols() const
		{
			return _stringIntMem.sStrings.size();
		}

		// Removes all elements of a selected column
		inline bool removeStringElements(size_t nCol = 0)
		{
			if (nCol < _stringIntMem.sStrings.size())
			{
				if (_stringIntMem.sStrings[nCol].size())
					_stringIntMem.sStrings[nCol].clear();
				return true;
			}
			return false;
		}

		// Cleares the content of the whole string object
		inline bool clearStringElements()
		{
			if (_stringIntMem.sStrings.size())
			{
				_stringIntMem.sStrings.clear();
				return true;
			}
			return false;
		}

		// Returns the size of a selected column or the whole
		// object in bytes
		inline int getStringSize(size_t nCol = std::string::npos) const
		{
			if (nCol == std::string::npos)
			{
			    // No column selected. Return the total size
				size_t nSize = 0u;

				// Recursively call this function for a specific column
				for (size_t i = 0; i < _stringIntMem.sStrings.size(); i++)
				{
					nSize += getStringSize(i);
				}

				return nSize;
			}
			else if (nCol < _stringIntMem.sStrings.size())
			{
			    // Selected a valid column
				if (_stringIntMem.sStrings[nCol].size())
				{
					size_t nSize = 0;

					// Count the number of caracters and multiply it with the
					// bytesize of a character
					for (size_t i = 0; i < _stringIntMem.sStrings[nCol].size(); i++)
						nSize += _stringIntMem.sStrings[nCol][i].size() * sizeof(char);

					return nSize;
				}
				else
					return 0u;
			}
			else
				return 0u;
		}

		// Wrapper for the internal string sorting function
		inline std::vector<int> sortStringElements(long long int i1, long long int i2, long long int j1, long long int j2, const std::string& sSortingExpression)
		{
		    return _stringIntMem.sortElements(i1, i2, j1, j2, sSortingExpression);
		}
};

#endif // STRINGMEMORY_HPP


