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
#include "../io/filesystem.hpp"
#include "../version.h"
#include "../maths/resampler.h"
#include "table.hpp"
#include "memory.hpp"
#include "stringmemory.hpp"
#include "cluster.hpp"


#ifndef CACHE_HPP
#define CACHE_HPP

using namespace std;



/*
 * Header zur Cache-Klasse --> PARENT zur Datafile-Klasse
 */

class MemoryManager : public FileSystem, public StringMemory, public NumeRe::ClusterManager
{
	public:
		enum AppDir {LINES = Memory::LINES, COLS = Memory::COLS, GRID = Memory::GRID, ALL = Memory::ALL};
	private:
		vector<Memory*> vMemory;
		bool bSaveMutex;
		fstream cache_file;
		string sCache_file;

		string sPredefinedFuncs;
		string sUserdefinedFuncs;
		string sPredefinedCommands;
		string sPluginCommands;

		void reorderColumn(long long int _nLayer, const vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
		bool loadFromNewCacheFile();
		bool loadFromLegacyCacheFile();


	protected:
		bool isValid() const;							                        // gibt den Wert von bValidData zurueck
		void removeDataInMemory();						                        // Loescht den Inhalt von dCache, allen Arrays und setzt das Objekt auf den Urzustand zurueck

		inline bool resizeTables(long long int _nLines, long long int _nCols, const string& _sCache)
		{
			return vMemory[mCachesMap.at(_sCache)]->resizeMemory(_nLines, _nCols);
		}
		inline double readFromTable(long long int _nLine, long long int _nCol, const string& _sCache) const
		{
			return vMemory[mCachesMap.at(_sCache)]->readMem(_nLine, _nCol);
		}
		inline vector<double> readFromTable(const VectorIndex& _vLine, const VectorIndex& _vCol, const string& _sCache) const
		{
			return vMemory[mCachesMap.at(_sCache)]->readMem(_vLine, _vCol);
		}
		inline void copyTableElementsToVector(vector<double>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol, const string& _sCache) const
		{
			vMemory[mCachesMap.at(_sCache)]->copyElementsInto(vTarget, _vLine, _vCol);
		}
		inline string getTableHeadlineElement(long long int _i, const string& _sCache) const
		{
			return vMemory[mCachesMap.at(_sCache)]->getHeadLineElement(_i);
		}
		inline vector<string> getTableHeadlineElement(const VectorIndex& _vCol, const string& _sCache) const
		{
			return vMemory[mCachesMap.at(_sCache)]->getHeadLineElement(_vCol);
		}
		inline bool setTableHeadlineElement(long long int _i, const string& _sCache, string _sHead)
		{
			return vMemory[mCachesMap.at(_sCache)]->setHeadLineElement(_i, _sHead);
		}
		inline long long int getAppendedZeroes(long long int _i, const string& _sCache) const
		{
			return vMemory[mCachesMap.at(_sCache)]->getAppendedZeroes(_i);
		}
		inline bool isValidElement(long long int _nLine, long long int _nCol, const string& _sCache) const
		{
			return vMemory[mCachesMap.at(_sCache)]->isValidElement(_nLine, _nCol);
		}
		inline bool saveLayer(string _sFileName, const string& _sCache, unsigned short nPrecision)
		{
			return vMemory[mCachesMap.at(_sCache)]->save(ValidFileName(_sFileName, ".ndat"), _sCache, nPrecision);
		}
		inline int getHeadlineCount(const string& _sCache) const
		{
		    return vMemory[mCachesMap.at(_sCache)]->getHeadlineCount();
		}

	public:
		MemoryManager();										// Standard-Konstruktor
		~MemoryManager();										// Destruktor (wendet delete[] auf die Matrix und alle Arrays an, sofern es noetig ist)


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


		inline bool writeToTable(long long int _nLine, long long int _nCol, const string& _sCache, double _dData)
		{
			return vMemory[mCachesMap.at(_sCache)]->writeData(_nLine, _nCol, _dData);
		}
		inline bool writeToTable(Indices& _idx, const string& _sCache, double* _dData, unsigned int _nNum)
		{
			return vMemory[mCachesMap.at(_sCache)]->writeData(_idx, _dData, _nNum);
		}

		inline long long int getTableLines(const string& _sCache, bool _bFull = false) const
		{
			return vMemory[mCachesMap.at(_sCache)]->getLines(_bFull);
		}
		inline long long int getTableCols(const string& _sCache, bool _bFull) const
		{
			return vMemory[mCachesMap.at(_sCache)]->getCols(_bFull);
		}
		inline int getSize(long long int _nLayer) const
		{
			if (vMemory.size() && _nLayer < vMemory.size())
				return vMemory[_nLayer]->getSize();
			else
				return 0;
		}
		inline string matchTableAsParameter(const string& sExpression, char cFollowing = ' ')
		{
			for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
			{
				if (findParameter(sExpression, iter->first, cFollowing))
					return iter->first;
			}
			return "";
		}

		bool getSaveStatus() const;                     // gibt bIsSaved zurueck
		void setSaveStatus(bool _bIsSaved);             // setzt bIsSaved
		long long int getLastSaved() const;             // gibt nLastSaved zurueck
		bool isTable(const string& sCache);
		bool containsTables(const string& sExpression);
		bool addTable(const string& sCache, const Settings& _option);
		bool deleteTable(const string& sCache);
		inline void deleteEntry(long long int _nLine, long long int _nCol, const string& _sCache)
		{
			vMemory[mCachesMap.at(_sCache)]->deleteEntry(_nLine, _nCol);
		}
		void deleteBulk(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0)
		{
			vMemory[mCachesMap.at(_sCache)]->deleteBulk(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		void deleteBulk(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			vMemory[mCachesMap.at(_sCache)]->deleteBulk(_vLine, _vCol);
		}
		inline unsigned int getNumberOfTables() const
		{
			return mCachesMap.size();
		}
		inline const map<string, long long int>& getTableMap() const
		{
			return mCachesMap;
		}
		inline string getTableNames() const
		{
			string sReturn = ";";
			for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
			{
				sReturn += iter->first + ";";
			}
			return sReturn;
		}
		inline void renameTable(const string& sCache, const string& sNewName, bool bForceRenaming = false)
		{
			if (isTable(sNewName))
			{
				throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, "", SyntaxError::invalid_position, sNewName + "()");
			}
			if (!isTable(sCache))
			{
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache);
			}
			if (sCache == "cache" && !bForceRenaming)
			{
				throw SyntaxError(SyntaxError::CACHE_CANNOT_BE_RENAMED, "", SyntaxError::invalid_position, "cache");
			}
			mCachesMap[sNewName] = mCachesMap[sCache];
			mCachesMap.erase(sCache);
			setSaveStatus(false);
			return;
		}
		inline void swapTables(const string& sCache1, const string& sCache2)
		{
			if (!isTable(sCache1))
			{
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache1);
			}
			if (!isTable(sCache2))
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
		bool saveToCacheFile();
		bool loadFromCacheFile();

		inline NumeRe::Table extractTable(const string& _sTable)
		{
			return vMemory[mCachesMap.at(_sTable)]->extractTable(_sTable);
		}
		inline NumeRe::Table extractTable(long long int _nLayer, const string& _sTable = "")
		{
			return vMemory[_nLayer]->extractTable(_sTable);
		}
		inline void importTable(NumeRe::Table _table, const string& _sTable)
		{
			return vMemory[mCachesMap.at(_sTable)]->importTable(_table);
		}
		inline void importTable(NumeRe::Table _table, long long int _nLayer)
		{
			return vMemory[_nLayer]->importTable(_table);
		}

		// MAFIMPLEMENTATIONS
		inline double std(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->std(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double std(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->std(_vLine, _vCol);
		}
		inline double avg(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->avg(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double avg(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->avg(_vLine, _vCol);
		}
		inline double max(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->max(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double max(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->max(_vLine, _vCol);
		}
		inline double min(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->min(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double min(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->min(_vLine, _vCol);
		}
		inline double prd(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->prd(_vLine, _vCol);
		}
		inline double prd(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->prd(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double sum(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->sum(_vLine, _vCol);
		}
		inline double sum(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->sum(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double num(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->num(_vLine, _vCol);
		}
		inline double num(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->num(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double and_func(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->and_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double and_func(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->and_func(_vLine, _vCol);
		}
		inline double or_func(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->or_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double or_func(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->or_func(_vLine, _vCol);
		}
		inline double xor_func(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->xor_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double xor_func(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->xor_func(_vLine, _vCol);
		}
		inline double cnt(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->cnt(_vLine, _vCol);
		}
		inline double cnt(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->cnt(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double norm(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->norm(_vLine, _vCol);
		}
		inline double norm(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->norm(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double cmp(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, double dRef = 0.0, int nType = 0)
		{
			return vMemory[mCachesMap.at(_sCache)]->cmp(_vLine, _vCol, dRef, nType);
		}
		inline double cmp(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dRef = 0.0, int nType = 0)
		{
			return vMemory[mCachesMap.at(_sCache)]->cmp(VectorIndex(i1, i2), VectorIndex(j1, j2), dRef, nType);
		}
		inline double med(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			return vMemory[mCachesMap.at(_sCache)]->med(_vLine, _vCol);
		}
		inline double med(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1)
		{
			return vMemory[mCachesMap.at(_sCache)]->med(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}
		inline double pct(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, double dPct = 0.5)
		{
			return vMemory[mCachesMap.at(_sCache)]->pct(_vLine, _vCol, dPct);
		}
		inline double pct(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dPct = 0.5)
		{
			return vMemory[mCachesMap.at(_sCache)]->pct(VectorIndex(i1, i2), VectorIndex(j1, j2), dPct);
		}

		inline bool smooth(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, const NumeRe::FilterSettings& _settings, AppDir Direction = ALL)
		{
			return vMemory[mCachesMap.at(_sCache)]->smooth(_vLine, _vCol, _settings, (Memory::AppDir)Direction);
		}
		inline bool retoque(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, AppDir Direction = ALL)
		{
			return vMemory[mCachesMap.at(_sCache)]->retoque(_vLine, _vCol, (Memory::AppDir)Direction);
		}
		inline bool resample(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, unsigned int nSamples = 0, AppDir Direction = ALL)
		{
			return vMemory[mCachesMap.at(_sCache)]->resample(_vLine, _vCol, nSamples, (Memory::AppDir)Direction);
		}
};

#endif
