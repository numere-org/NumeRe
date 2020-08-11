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


#include <fstream>
#include <string>
#include <vector>

#include "../ui/error.hpp"
#include "../settings.hpp"
#include "../structures.hpp"
#include "table.hpp"
#include "memory.hpp"
#include "stringmemory.hpp"
#include "cluster.hpp"
#include "fileadapter.hpp"


#ifndef MEMORYMANAGER_HPP
#define MEMORYMANAGER_HPP

using namespace std;


/////////////////////////////////////////////////
/// \brief This class represents the central
/// memory managing instance. It will handle all
/// tables and clusters, which have been created
/// during the runtime.
/////////////////////////////////////////////////
class MemoryManager : public NumeRe::FileAdapter, public StringMemory, public NumeRe::ClusterManager
{
	private:
		vector<Memory*> vMemory;
		map<string, long long int> mCachesMap;
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
		VectorIndex parseEvery(string& sDir, const string& sTableName);
        vector<double> resolveMAF(const string& sTableName, string sDir, double (MemoryManager::*MAF)(const string&, long long int, long long int, long long int, long long int));

        virtual bool saveLayer(string _sFileName, const string& _sTable, unsigned short nPrecision) override
		{
			return vMemory[mCachesMap.at(_sTable)]->save(ValidFileName(_sFileName, ".ndat"), _sTable, nPrecision);
		}

		inline bool exists(const string& sTable) const
		{
		    return mCachesMap.find(sTable) != mCachesMap.end();
		}

	public:
		MemoryManager();
		~MemoryManager();

		enum AppDir {LINES = Memory::LINES, COLS = Memory::COLS, GRID = Memory::GRID, ALL = Memory::ALL};

		// Variables for the parser
		double tableLinesCount;
		double tableColumnsCount;
        bool updateDimensionVariables(const string& sTableName);


        // OTHER METHODS
		inline void setPredefinedFuncs(const string& sFuncs)
		{
			sPredefinedFuncs = sFuncs;
		}

		inline void setUserdefinedFuncs(const string& sUserFuncs)
		{
			sUserdefinedFuncs = sUserFuncs;
		}

		inline void setPluginCommands(const string& sPluginCmds)
		{
			sPluginCommands = sPluginCmds;
        }

		inline void setPredefinedCommands(const string& sCommands)
		{
		    sPredefinedCommands = sCommands;
		}



		// VALIDATION METHODS
        bool isValid() const;
		bool isTable(const string& sCache) const;

        bool isEmpty(const string& sTable) const
        {
            if (exists(sTable))
                return !vMemory[mCachesMap.at(sTable)]->getCols();

            return true;
        }

		bool isValidElement(long long int _nLine, long long int _nCol, const string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[mCachesMap.at(_sTable)]->isValidElement(_nLine, _nCol);

            return false;
		}



		// RECOGNITION METHODS
		bool containsTables(const string& sExpression);
		bool containsTablesOrClusters(const string& sCmdLine);

		inline string matchTableAsParameter(const string& sExpression, char cFollowing = ' ')
		{
			for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
			{
				if (findParameter(sExpression, iter->first, cFollowing))
					return iter->first;
			}

			return "";
		}



        // GLOBAL TABLE METHODS
        // CREATION AND DELETION
		bool addTable(const string& sCache, const Settings& _option);
		bool deleteTable(const string& sCache);
        void removeData(bool bAutoSave = false);
        void removeTablesFromMemory();

        bool resizeTable(long long int _nLines, long long int _nCols, const string& _sTable)
		{
			return vMemory[mCachesMap.at(_sTable)]->resizeMemory(_nLines, _nCols);
		}

        void deleteEntry(long long int _nLine, long long int _nCol, const string& _sCache)
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


		// TABLE INFORMATION AND SAVING METHODS
		bool getSaveStatus() const;
		void setSaveStatus(bool _bIsSaved);
		long long int getLastSaved() const;
		void setCacheFileName(string _sFileName);
		bool saveToCacheFile();
		bool loadFromCacheFile();

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


        // GLOBAL TABLE ACCESS METHODS
		Memory* getTable(const string& sTable);
		virtual void melt(Memory* _mem, const string& sTable) override;

		inline void renameTable(const string& sCache, const string& sNewName, bool bForceRenaming = false)
		{
			if (isTable(sNewName))
				throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, "", SyntaxError::invalid_position, sNewName + "()");

			if (!isTable(sCache))
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache);

			if (sCache == "table" && !bForceRenaming)
				throw SyntaxError(SyntaxError::CACHE_CANNOT_BE_RENAMED, "", SyntaxError::invalid_position, "table");

			mCachesMap[sNewName] = mCachesMap[sCache];
			mCachesMap.erase(sCache);
			setSaveStatus(false);
		}

		inline void swapTables(const string& sCache1, const string& sCache2)
		{
			if (!isTable(sCache1))
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache1);

			if (!isTable(sCache2))
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache2);

			long long int nTemp = mCachesMap[sCache1];
			mCachesMap[sCache1] = mCachesMap[sCache2];
			mCachesMap[sCache2] = nTemp;
		}


		// TABLE EXTRACTOR AND IMPORTER METHODS
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


		// TABLE INPLACE MODIFICATION METHODS
		vector<int> sortElements(const string& sLine);
		vector<int> sortElements(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0, const string& sSortingExpression = "");

		inline bool smooth(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, const NumeRe::FilterSettings& _settings, AppDir Direction = ALL)
		{
			return vMemory[mCachesMap.at(_sCache)]->smooth(_vLine, _vCol, _settings, (Memory::AppDir)Direction);
		}

		inline bool retouch(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, AppDir Direction = ALL)
		{
			return vMemory[mCachesMap.at(_sCache)]->retouch(_vLine, _vCol, (Memory::AppDir)Direction);
		}

		inline bool resample(const string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, unsigned int nSamples = 0, AppDir Direction = ALL)
		{
			return vMemory[mCachesMap.at(_sCache)]->resample(_vLine, _vCol, nSamples, (Memory::AppDir)Direction);
		}





		// DIMENSION ACCESS METHODS
		inline long long int getLines(const string& sTable, bool _bFull = false) const
		{
		    if (exists(sTable))
                return vMemory[mCachesMap.at(sTable)]->getLines(_bFull);

            return 0;
		}

		inline long long int getCols(const string& sTable, bool _bFull = false) const
		{
		    if (exists(sTable))
                return vMemory[mCachesMap.at(sTable)]->getCols(_bFull);

            return 0;
		}

		inline int getSize(long long int _nLayer) const
		{
			if (vMemory.size() && _nLayer < vMemory.size())
				return vMemory[_nLayer]->getSize();

			return 0;
		}



        // READ ACCESS METHODS
        double getElement(long long int _nLine, long long int _nCol, const string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[mCachesMap.at(_sTable)]->readMem(_nLine, _nCol);

            return NAN;
		}

		vector<double> getElement(const VectorIndex& _vLine, const VectorIndex& _vCol, const string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[mCachesMap.at(_sTable)]->readMem(_vLine, _vCol);

            return vector<double>();
		}

		void copyElementsInto(vector<double>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol, const string& _sTable) const
		{
			vMemory[mCachesMap.at(_sTable)]->copyElementsInto(vTarget, _vLine, _vCol);
		}

		int getHeadlineCount(const string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[mCachesMap.at(_sTable)]->getHeadlineCount();

            return 0;
		}

		string getHeadLineElement(long long int _i, const string& _sTable) const
		{
			if (exists(_sTable))
                return vMemory[mCachesMap.at(_sTable)]->getHeadLineElement(_i);

            return "";
		}

		vector<string> getHeadLineElement(const VectorIndex& _vCol, const string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[mCachesMap.at(_sTable)]->getHeadLineElement(_vCol);

            return vector<string>();
		}

		string getTopHeadLineElement(long long int _i, const string& _sTable) const
        {
            return getHeadLineElement(_i, _sTable).substr(0, getHeadLineElement(_i, _sTable).find("\\n"));
        }

		long long int getAppendedZeroes(long long int _i, const string& _sTable) const
		{
			return vMemory[mCachesMap.at(_sTable)]->getAppendedZeroes(_i);
		}



        // WRITE ACCESS METHODS
		inline bool writeToTable(long long int _nLine, long long int _nCol, const string& _sCache, double _dData)
		{
			return vMemory[mCachesMap.at(_sCache)]->writeData(_nLine, _nCol, _dData);
		}

		inline bool writeToTable(Indices& _idx, const string& _sCache, double* _dData, unsigned int _nNum)
		{
			return vMemory[mCachesMap.at(_sCache)]->writeData(_idx, _dData, _nNum);
		}

		bool setHeadLineElement(long long int _i, const string& _sTable, string _sHead)
		{
			return vMemory[mCachesMap.at(_sTable)]->setHeadLineElement(_i, _sHead);
		}



		// MAF METHODS
		// IMPLEMENTATIONS FOR THE TABLE METHODS
		vector<double> std(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::std);
        }

		vector<double> avg(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::avg);
        }

		vector<double> max(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::max);
        }

		vector<double> min(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::min);
        }

		vector<double> prd(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::prd);
        }

        vector<double> sum(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::sum);
        }

		vector<double> num(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::num);
        }

		vector<double> and_func(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::and_func);
        }

		vector<double> or_func(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::or_func);
        }

		vector<double> xor_func(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::xor_func);
        }

		vector<double> cnt(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::cnt);
        }

		vector<double> norm(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::norm);
        }

		vector<double> med(const string& sTable, string sDir)
        {
            return resolveMAF(sTable, sDir, MemoryManager::med);
        }

		vector<double> cmp(const string& sTable, string sDir, double dRef = 0.0, int nType = 0)
        {
            vector<double> vResults;

            long long int nGridOffset = sDir.find("grid") != string::npos ? 2 : 0;

            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset > getCols(sTable, false))
                        continue;

                    vResults.push_back(cmp(sTable, 0, getLines(sTable, false), _idx[i]+nGridOffset, -1, dRef, nType));
                }
            }
            else if (sDir.find("lines") != string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset > getLines(sTable, false))
                        continue;

                    vResults.push_back(cmp(sTable, _idx[i]+nGridOffset, -1, 0, getCols(sTable, false), dRef, nType));
                }
            }
            else
                vResults.push_back(cmp(sTable, 0, getLines(sTable, false), nGridOffset, getCols(sTable, false), dRef, nType));

            if (!vResults.size())
                vResults.push_back(NAN);

            return vResults;
        }

		vector<double> pct(const string& sTable, string sDir, double dPct = 0.5)
        {
            vector<double> vResults;

            long long int nGridOffset = sDir.find("grid") != string::npos ? 2 : 0;

            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset > getCols(sTable, false))
                        continue;

                    vResults.push_back(pct(sTable, 0, getLines(sTable, false), _idx[i]+nGridOffset, -1, dPct));
                }
            }
            else if (sDir.find("lines") != string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset > getLines(sTable, false))
                        continue;

                    vResults.push_back(pct(sTable, _idx[i]+nGridOffset, -1, 0, getCols(sTable, false), dPct));
                }
            }
            else
                vResults.push_back(pct(sTable, 0, getLines(sTable, false), nGridOffset, getCols(sTable, false), dPct));

            if (!vResults.size())
                vResults.push_back(NAN);

            return vResults;
        }


        // IMPLEMENTATIONS FOR THE MAFS
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

};

#endif
