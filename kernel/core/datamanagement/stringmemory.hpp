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

using namespace std;

// Internal string memory implementation. Separated to be able to
// use Sorter as base class for strings and for the DataFile object
class StringInternalMemory : public Sorter
{
    private:
        bool bSortCaseInsensitive;

        void reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
		virtual int compare(int i, int j, int col);
		virtual bool isValue(int line, int col);

    public:
        StringInternalMemory() : bSortCaseInsensitive(false) {}

        vector<vector<string> > sStrings;
		map<string, string> sStringVars;

		vector<int> sortElements(long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression);
};


// The string memory class, which will be included into the Cache
// by inheritance
class StringMemory
{
    private:
        StringInternalMemory _stringIntMem;

		bool checkStringvarDelimiter(const string& sToken) const;
		void replaceStringMethod(string& sLine, size_t nPos, size_t nLength, const string& sVarValue);
		void replaceSearchMethods(string& sLine, size_t nFinalPos, size_t nLength, const string& sVarValue, const string& sMethod, string& sArgument);
		void replaceAccessMethods(string& sLine, size_t nFinalPos, size_t nLength, const string& sVarValue, const string& sMethod, string& sArgument);

    public:
		bool writeString(const string& _sString, unsigned int _nthString = string::npos, unsigned int nCol = 0);
		string readString(unsigned int _nthString = string::npos, unsigned int nCol = 0);
		string maxString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
		string minString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
		string sumString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);

		// Returns the number of stored elements in either a
		// specific column or the maximal line count of the
		// whole object
		inline unsigned int getStringElements(unsigned int nCol = string::npos) const
		{
			if (nCol == string::npos)
			{
			    // No column selected. Return the maximal line count
				unsigned int nCnt = 0;

				for (unsigned int i = 0; i < _stringIntMem.sStrings.size(); i++)
				{
					if (nCnt < _stringIntMem.sStrings[i].size())
						nCnt = _stringIntMem.sStrings[i].size();
				}

				return nCnt;
			}
			else if (nCol >= _stringIntMem.sStrings.size())
				return 0;
			else
				return _stringIntMem.sStrings[nCol].size();
			return 0;
		}

		// Return number of columns
		inline unsigned int getStringCols() const
		{
			return _stringIntMem.sStrings.size();
		}

		// Removes all elements of a selected column
		inline bool removeStringElements(unsigned int nCol = 0)
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
		inline int getStringSize(unsigned int nCol = string::npos) const
		{
			if (nCol == string::npos)
			{
			    // No column selected. Return the total size
				unsigned int nSize = 0;

				// Recursively call this function for a specific column
				for (unsigned int i = 0; i < _stringIntMem.sStrings.size(); i++)
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
					int nSize = 0;

					// Count the number of caracters and multiply it with the
					// bytesize of a character
					for (unsigned int i = 0; i < _stringIntMem.sStrings[nCol].size(); i++)
						nSize += _stringIntMem.sStrings[nCol][i].size() * sizeof(char);

					return nSize;
				}
				else
					return 0;
			}
			else
				return 0;
		}

		// Wrapper for the internal string sorting function
		inline vector<int> sortStringElements(long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
		{
		    return _stringIntMem.sortElements(i1, i2, j1, j2, sSortingExpression);
		}

		// STRINGVARFUNCS
		bool containsStringVars(const string& sLine) const;
		void getStringValues(string& sLine, unsigned int nPos = 0);
		void setStringValue(const string& sVar, const string& sValue);
		void removeStringVar(const string& sVar);

		// Returns a reference to the internal string variable map
		inline const map<string, string>& getStringVars() const
		{
			return _stringIntMem.sStringVars;
		}


};

#endif // STRINGMEMORY_HPP


