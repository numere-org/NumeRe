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


#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cmath>
#include <vector>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

#include "../ui/error.hpp"
#include "../settings.hpp"
#include "../structures.hpp"
#include "../utils/tools.hpp"
#include "../filesystem.hpp"
#include "../version.h"
#include "../maths/resampler.h"
#include "table.hpp"
#include "memory.hpp"


#ifndef CACHE_HPP
#define CACHE_HPP

using namespace std;



/*
 * Header zur Cache-Klasse --> PARENT zur Datafile-Klasse
 */

class Cache : public FileSystem
{
	public:
		enum AppDir {LINES = Memory::LINES, COLS = Memory::COLS, GRID = Memory::GRID, ALL = Memory::ALL};
	private:
		vector<Memory*> vCacheMemory;
		bool bSaveMutex;
		fstream cache_file;
		string sCache_file;

		string sPredefinedFuncs;
		string sUserdefinedFuncs;
		string sPredefinedCommands;
		string sPluginCommands;

		vector<vector<string> > sStrings;
		map<string, string> sStringVars;

		bool checkStringvarDelimiter(const string& sToken) const;
		void replaceStringMethod(string& sLine, size_t nPos, size_t nLength, const string& sVarValue);
		void reorderColumn(long long int _nLayer, const vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);


	protected:
		bool isValid() const;							                        // gibt den Wert von bValidData zurueck
		void removeCachedData();						                        // Loescht den Inhalt von dCache, allen Arrays und setzt das Objekt auf den Urzustand zurueck
		ColumnKeys* evaluateKeyList(string& sKeyList, long long int nMax);

		inline bool resizeCache(long long int _nLines, long long int _nCols, const string& _sCache)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->resizeMemory(_nLines, _nCols);
		}
		inline double readFromCache(long long int _nLine, long long int _nCol, const string& _sCache) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->readMem(_nLine, _nCol);
		}
		inline vector<double> readFromCache(const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& _sCache) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->readMem(_vLine, _vCol);
		}
		inline void copyCachedElementsInto(vector<double>* vTarget, const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& _sCache) const
		{
			vCacheMemory[mCachesMap.at(_sCache)]->copyElementsInto(vTarget, _vLine, _vCol);
		}
		inline string getCacheHeadLineElement(long long int _i, const string& _sCache) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->getHeadLineElement(_i);
		}
		inline vector<string> getCacheHeadLineElement(vector<long long int> _vCol, const string& _sCache) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->getHeadLineElement(_vCol);
		}
		inline bool setCacheHeadLineElement(long long int _i, const string& _sCache, string _sHead)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->setHeadLineElement(_i, _sHead);
		}
		inline long long int getAppendedZeroes(long long int _i, const string& _sCache) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->getAppendedZeroes(_i);
		}
		inline bool isValidElement(long long int _nLine, long long int _nCol, const string& _sCache) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->isValidElement(_nLine, _nCol);
		}
		inline bool saveLayer(string _sFileName, const string& _sCache)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->save(ValidFileName(_sFileName, ".ndat"));
		}
		inline int getHeadlineCount(const string& _sCache) const
		{
		    return vCacheMemory[mCachesMap.at(_sCache)]->getHeadlineCount();
		}

	public:
		Cache();										// Standard-Konstruktor
		~Cache();										// Destruktor (wendet delete[] auf die Matrix und alle Arrays an, sofern es noetig ist)


		map<string, long long int> mCachesMap;

		inline void setPredefinedFuncs(const string& sFuncs)
		{
			sPredefinedFuncs = sFuncs;
			return;
		}
		inline void setUserdefinedFuncs(const string& sUserFuncs)
		{
			sUserdefinedFuncs = sUserFuncs;
			return;
		}
		inline void setPluginCommands(const string& sPluginCmds)
		{
			sPluginCommands = sPluginCmds;
			return;
		}


		inline bool writeToCache(long long int _nLine, long long int _nCol, const string& _sCache, double _dData)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->writeData(_nLine, _nCol, _dData);
		}
		inline bool writeToCache(Indices& _idx, const string& _sCache, double* _dData, unsigned int _nNum)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->writeData(_idx, _dData, _nNum);
		}

		inline long long int getCacheLines(const string& _sCache, bool _bFull = false) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->getLines(_bFull);
		}
		inline long long int getCacheCols(const string& _sCache, bool _bFull) const
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->getCols(_bFull);
		}
		inline int getSize(long long int _nLayer) const
		{
			if (vCacheMemory.size() && _nLayer < vCacheMemory.size())
				return vCacheMemory[_nLayer]->getSize();
			else
				return 0;
		}
		inline string matchCache(const string& sExpression, char cFollowing = ' ')
		{
			for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
			{
				if (matchParams(sExpression, iter->first, cFollowing))
					return iter->first;
			}
			return "";
		}

		bool getSaveStatus() const;                     // gibt bIsSaved zurueck
		void setSaveStatus(bool _bIsSaved);             // setzt bIsSaved
		long long int getLastSaved() const;             // gibt nLastSaved zurueck
		bool isCacheElement(const string& sCache);
		bool containsCacheElements(const string& sExpression);
		bool addCache(const string& sCache, const Settings& _option);
		bool deleteCache(const string& sCache);
		inline void deleteEntry(long long int _nLine, long long int _nCol, const string& _sCache)
		{
			vCacheMemory[mCachesMap.at(_sCache)]->deleteEntry(_nLine, _nCol);
		}
		void deleteBulk(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0)
		{
			vCacheMemory[mCachesMap.at(_sCache)]->deleteBulk(i1, i2, j1, j2);
		}
		void deleteBulk(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			vCacheMemory[mCachesMap.at(_sCache)]->deleteBulk(_vLine, _vCol);
		}
		inline unsigned int getCacheCount() const
		{
			return mCachesMap.size();
		}
		inline map<string, long long int> getCacheList() const
		{
			return mCachesMap;
		}
		inline string getCacheNames() const
		{
			string sReturn = ";";
			for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
			{
				sReturn += iter->first + ";";
			}
			return sReturn;
		}
		inline void renameCache(const string& sCache, const string& sNewName, bool bForceRenaming = false)
		{
			if (isCacheElement(sNewName))
			{
				throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, "", SyntaxError::invalid_position, sNewName + "()");
			}
			if (!isCacheElement(sCache))
			{
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache);
			}
			if (sCache == "cache" && !bForceRenaming)
			{
				throw SyntaxError(SyntaxError::CACHE_CANNOT_BE_RENAMED, "", SyntaxError::invalid_position, "cache");
			}
			mCachesMap[sNewName] = mCachesMap[sCache];
			mCachesMap.erase(sCache);
			return;
		}
		inline void swapCaches(const string& sCache1, const string& sCache2)
		{
			if (!isCacheElement(sCache1))
			{
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache1);
			}
			if (!isCacheElement(sCache2))
			{
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache2);
			}
			long long int nTemp = mCachesMap[sCache1];
			mCachesMap[sCache1] = mCachesMap[sCache2];
			mCachesMap[sCache2] = nTemp;
			return;
		}

		vector<int> sortElements(const string& sLine);               // wandelt das Kommando in einen Ausdruck um, startet qSort und fuehrt die Sortierung aus
		vector<int> sortElements(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0, const string& sSortingExpression = "");
		void setCacheFileName(string _sFileName);
		bool saveCache();
		bool loadCache();

		inline Table extractTable(const string& _sTable)
		{
			return vCacheMemory[mCachesMap.at(_sTable)]->extractTable(_sTable);
		}
		inline Table extractTable(long long int _nLayer, const string& _sTable = "")
		{
			return vCacheMemory[_nLayer]->extractTable(_sTable);
		}

		// STRINGFUNCS
		bool writeString(const string& _sString, unsigned int _nthString = string::npos, unsigned int nCol = 0);
		string readString(unsigned int _nthString = string::npos, unsigned int nCol = 0);
		string maxString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
		string minString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
		string sumString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
		inline unsigned int getStringElements(unsigned int nCol = string::npos) const
		{
			if (nCol == string::npos)
			{
				unsigned int nCnt = 0;
				for (unsigned int i = 0; i < sStrings.size(); i++)
				{
					if (nCnt < sStrings[i].size())
						nCnt = sStrings[i].size();
				}
				return nCnt;
			}
			else if (nCol >= sStrings.size())
				return 0;
			else
				return sStrings[nCol].size();
			return 0;
		}
		inline unsigned int getStringCols() const
		{
			return sStrings.size();
		}
		inline bool removeStringElements(unsigned int nCol = 0)
		{
			if (nCol < sStrings.size())
			{
				if (sStrings[nCol].size())
					sStrings[nCol].clear();
				return true;
			}
			return false;
		}
		inline bool clearStringElements()
		{
			if (sStrings.size())
			{
				sStrings.clear();
				return true;
			}
			return false;
		}
		inline int getStringSize(unsigned int nCol = string::npos) const
		{
			if (nCol == string::npos)
			{
				unsigned int nSize = 0;
				for (unsigned int i = 0; i < sStrings.size(); i++)
				{
					nSize += getStringSize(i);
				}
				return nSize;
			}
			else if (nCol < sStrings.size())
			{
				if (sStrings[nCol].size())
				{
					int nSize = 0;
					for (unsigned int i = 0; i < sStrings[nCol].size(); i++)
						nSize += sStrings[nCol][i].size() * sizeof(char);
					return nSize;
				}
				else
					return 0;
			}
			else
				return 0;
		}
		// STRINGVARFUNCS
		bool containsStringVars(const string& sLine) const;
		void getStringValues(string& sLine, unsigned int nPos = 0);
		void setStringValue(const string& sVar, const string& sValue);
		void removeStringVar(const string& sVar);
		inline const map<string, string>& getStringVars() const
		{
			return sStringVars;
		}

		// MAFIMPLEMENTATIONS
		inline double std(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->std(i1, i2, j1, j2);
		}
		inline double std(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->std(_vLine, _vCol);
		}
		inline double avg(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->avg(i1, i2, j1, j2);
		}
		inline double avg(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->avg(_vLine, _vCol);
		}
		inline double max(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->max(i1, i2, j1, j2);
		}
		inline double max(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->max(_vLine, _vCol);
		}
		inline double min(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->min(i1, i2, j1, j2);
		}
		inline double min(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->min(_vLine, _vCol);
		}
		inline double prd(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->prd(_vLine, _vCol);
		}
		inline double prd(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->prd(i1, i2, j1, j2);
		}
		inline double sum(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->sum(_vLine, _vCol);
		}
		inline double sum(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->sum(i1, i2, j1, j2);
		}
		inline double num(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->num(_vLine, _vCol);
		}
		inline double num(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->num(i1, i2, j1, j2);
		}
		inline double and_func(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->and_func(i1, i2, j1, j2);
		}
		inline double and_func(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->and_func(_vLine, _vCol);
		}
		inline double or_func(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->or_func(i1, i2, j1, j2);
		}
		inline double or_func(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->or_func(_vLine, _vCol);
		}
		inline double xor_func(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->xor_func(i1, i2, j1, j2);
		}
		inline double xor_func(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->xor_func(_vLine, _vCol);
		}
		inline double cnt(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->cnt(_vLine, _vCol);
		}
		inline double cnt(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->cnt(i1, i2, j1, j2);
		}
		inline double norm(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->norm(_vLine, _vCol);
		}
		inline double norm(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->norm(i1, i2, j1, j2);
		}
		inline double cmp(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef = 0.0, int nType = 0)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->cmp(_vLine, _vCol, dRef, nType);
		}
		inline double cmp(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dRef = 0.0, int nType = 0)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->cmp(i1, i2, j1, j2, dRef, nType);
		}
		inline double med(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->med(_vLine, _vCol);
		}
		inline double med(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->med(i1, i2, j1, j2);
		}
		inline double pct(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct = 0.5)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->pct(_vLine, _vCol, dPct);
		}
		inline double pct(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dPct = 0.5)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->pct(i1, i2, j1, j2, dPct);
		}

		inline bool smooth(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nOrder = 1, AppDir Direction = ALL)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->smooth(i1, i2, j1, j2, nOrder, (Memory::AppDir)Direction);
		}
		inline bool retoque(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, AppDir Direction = ALL)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->retoque(i1, i2, j1, j2, (Memory::AppDir)Direction);
		}
		inline bool resample(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nSamples = 0, AppDir Direction = ALL)
		{
			return vCacheMemory[mCachesMap.at(_sCache)]->resample(i1, i2, j1, j2, nSamples, (Memory::AppDir)Direction);
		}
};

#endif
