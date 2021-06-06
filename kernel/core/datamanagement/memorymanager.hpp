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


/////////////////////////////////////////////////
/// \brief This class represents the central
/// memory managing instance. It will handle all
/// tables and clusters, which have been created
/// during the runtime.
/////////////////////////////////////////////////
class MemoryManager : public NumeRe::FileAdapter, public StringMemory, public NumeRe::ClusterManager
{
	private:
		std::vector<Memory*> vMemory;
		std::map<std::string, std::pair<size_t, size_t>> mCachesMap;
		bool bSaveMutex;
		std::fstream cache_file;
		std::string sCache_file;
		std::string sPredefinedFuncs;
		std::string sUserdefinedFuncs;
		std::string sPredefinedCommands;
		std::string sPluginCommands;

		void reorderColumn(size_t _nLayer, const std::vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
		bool loadFromNewCacheFile();
		bool loadFromLegacyCacheFile();
		VectorIndex parseEvery(std::string& sDir, const std::string& sTableName) const;
        std::vector<double> resolveMAF(const std::string& sTableName, std::string sDir, double (MemoryManager::*MAF)(const std::string&, long long int, long long int, long long int, long long int) const) const;

        virtual bool saveLayer(std::string _sFileName, const std::string& _sTable, unsigned short nPrecision) override
		{
			return vMemory[findTable(_sTable)]->save(ValidFileName(_sFileName, ".ndat"), _sTable, nPrecision);
		}

		inline bool exists(const std::string& sTable) const
		{
		    return mCachesMap.find(sTable) != mCachesMap.end();
		}

		size_t mapStringViewFind(StringView view) const
		{
		    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
            {
                if (view == iter->first)
                {
                    if (iter->second.first != iter->second.second)
                        return iter->second.second;

                    return iter->second.first;
                }
                else if (view < iter->first)
                    throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, view.to_string(), view.to_string());
            }

            return -1;
		}

		size_t findTable(const std::string& sTable) const
		{
		    auto iter = mCachesMap.find(sTable);

		    if (iter == mCachesMap.end())
                throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sTable, sTable);

            if (iter->second.first != iter->second.second)
                return iter->second.second;

            return iter->second.first;
		}

	public:
		MemoryManager();
		~MemoryManager();

		enum AppDir {LINES = Memory::LINES, COLS = Memory::COLS, GRID = Memory::GRID, ALL = Memory::ALL};

		// Variables for the parser
		double tableLinesCount;
		double tableColumnsCount;
        bool updateDimensionVariables(StringView sTableName);


        // OTHER METHODS
		inline void setPredefinedFuncs(const std::string& sFuncs)
		{
			sPredefinedFuncs = sFuncs;
		}

		inline void setUserdefinedFuncs(const std::string& sUserFuncs)
		{
			sUserdefinedFuncs = sUserFuncs;
		}

		inline void setPluginCommands(const std::string& sPluginCmds)
		{
			sPluginCommands = sPluginCmds;
        }

		inline void setPredefinedCommands(const std::string& sCommands)
		{
		    sPredefinedCommands = sCommands;
		}



		// VALIDATION METHODS
        bool isValid() const;
		bool isTable(const std::string& sTable) const;

        bool isEmpty(const std::string& sTable) const
        {
            if (exists(sTable))
                return !vMemory[findTable(sTable)]->getCols();

            return true;
        }

		bool isValidElement(long long int _nLine, long long int _nCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->isValidElement(_nLine, _nCol);

            return false;
		}



		// RECOGNITION METHODS
		bool containsTables(const std::string& sExpression);
		bool containsTablesOrClusters(const std::string& sCmdLine);

		inline std::string matchTableAsParameter(const std::string& sExpression, char cFollowing = ' ')
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
		bool addTable(const std::string& sCache, const Settings& _option);
		bool deleteTable(const std::string& sCache);
        void removeData(bool bAutoSave = false);
        void removeTablesFromMemory();

        bool resizeTable(long long int _nLines, long long int _nCols, const std::string& _sTable)
		{
			return vMemory[findTable(_sTable)]->resizeMemory(_nLines, _nCols);
		}

        void deleteEntry(long long int _nLine, long long int _nCol, const std::string& _sCache)
		{
			vMemory[findTable(_sCache)]->deleteEntry(_nLine, _nCol);
		}

		void deleteBulk(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0)
		{
			vMemory[findTable(_sCache)]->deleteBulk(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		void deleteBulk(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			vMemory[findTable(_sCache)]->deleteBulk(_vLine, _vCol);
		}


		// TABLE INFORMATION AND SAVING METHODS
		bool getSaveStatus() const;
		void setSaveStatus(bool _bIsSaved);
		long long int getLastSaved() const;
		void setCacheFileName(std::string _sFileName);
		bool saveToCacheFile();
		bool loadFromCacheFile();

        inline unsigned int getNumberOfTables() const
		{
			return mCachesMap.size();
		}

		inline const std::map<std::string, std::pair<size_t, size_t>>& getTableMap() const
		{
			return mCachesMap;
		}

		inline std::string getTableNames() const
		{
			std::string sReturn = ";";

			for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
			{
				sReturn += iter->first + ";";
			}

			return sReturn;
		}


        // GLOBAL TABLE ACCESS METHODS
		Memory* getTable(const std::string& sTable);
		virtual void melt(Memory* _mem, const std::string& sTable) override;

		inline void renameTable(const std::string& sCache, const std::string& sNewName, bool bForceRenaming = false)
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

		inline void swapTables(const std::string& sCache1, const std::string& sCache2)
		{
			if (!isTable(sCache1))
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache1);

			if (!isTable(sCache2))
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache2);

			std::pair<size_t,size_t> temp = mCachesMap[sCache1];
			mCachesMap[sCache1] = mCachesMap[sCache2];
			mCachesMap[sCache2] = temp;
		}

		void addReference(const std::string& sTable, const std::string& sReference)
		{
		    if (!isTable(sTable))
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sTable);

            if (!sReference.length())
            {
                mCachesMap[sTable].second = mCachesMap[sTable].first;
                return;
            }
            else if (!isTable(sReference))
				throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sReference);

			auto iter = mCachesMap.find(sReference.substr(0, sReference.find('(')));
			mCachesMap[sTable].second = iter->second.first;
		}


		// TABLE EXTRACTOR AND IMPORTER METHODS
        inline NumeRe::Table extractTable(const std::string& _sTable, const VectorIndex& lines = VectorIndex(0, VectorIndex::OPEN_END), const VectorIndex& cols = VectorIndex(0, VectorIndex::OPEN_END))
		{
			return vMemory[findTable(_sTable)]->extractTable(_sTable, lines, cols);
		}

		inline NumeRe::Table extractTable(long long int _nLayer, const std::string& _sTable = "", const VectorIndex& lines = VectorIndex(0, VectorIndex::OPEN_END), const VectorIndex& cols = VectorIndex(0, VectorIndex::OPEN_END))
		{
			return vMemory[_nLayer]->extractTable(_sTable, lines, cols);
		}

		inline void importTable(NumeRe::Table _table, const std::string& _sTable, const VectorIndex& lines = VectorIndex(0, VectorIndex::OPEN_END), const VectorIndex& cols = VectorIndex(0, VectorIndex::OPEN_END))
		{
			return vMemory[findTable(_sTable)]->importTable(_table, lines, cols);
		}

		inline void importTable(NumeRe::Table _table, long long int _nLayer, const VectorIndex& lines = VectorIndex(0, VectorIndex::OPEN_END), const VectorIndex& cols = VectorIndex(0, VectorIndex::OPEN_END))
		{
			return vMemory[_nLayer]->importTable(_table, lines, cols);
		}


		// TABLE INPLACE MODIFICATION METHODS
		std::vector<int> sortElements(const std::string& sLine);
		std::vector<int> sortElements(const std::string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0, const std::string& sSortingExpression = "");

		inline bool smooth(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, const NumeRe::FilterSettings& _settings, AppDir Direction = ALL)
		{
			return vMemory[findTable(_sCache)]->smooth(_vLine, _vCol, _settings, (Memory::AppDir)Direction);
		}

		inline bool retouch(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, AppDir Direction = ALL)
		{
			return vMemory[findTable(_sCache)]->retouch(_vLine, _vCol, (Memory::AppDir)Direction);
		}

		inline bool resample(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, unsigned int nSamples = 0, AppDir Direction = ALL)
		{
			return vMemory[findTable(_sCache)]->resample(_vLine, _vCol, nSamples, (Memory::AppDir)Direction);
		}





		// DIMENSION ACCESS METHODS
		inline long long int getLines(StringView sTable, bool _bFull = false) const
		{
		    size_t idx = mapStringViewFind(sTable);

		    if (idx != (size_t)-1)
                return vMemory[idx]->getLines(_bFull);

            return 0;
		}

		inline long long int getLines(const std::string& sTable, bool _bFull = false) const
		{
		    auto iter = mCachesMap.find(sTable);

		    if (iter != mCachesMap.end())
                return vMemory[findTable(sTable)]->getLines(_bFull);

            return 0;
		}

		inline long long int getCols(StringView sTable, bool _bFull = false) const
		{
		    size_t idx = mapStringViewFind(sTable);

		    if (idx != (size_t)-1)
                return vMemory[idx]->getCols(_bFull);

            return 0;
		}

		inline long long int getCols(const std::string& sTable, bool _bFull = false) const
		{
		    auto iter = mCachesMap.find(sTable);

		    if (iter != mCachesMap.end())
                return vMemory[findTable(sTable)]->getCols(_bFull);

            return 0;
		}

		inline int getSize(long long int _nLayer) const
		{
			if (vMemory.size() && _nLayer < vMemory.size())
				return vMemory[_nLayer]->getSize();

			return 0;
		}



        // READ ACCESS METHODS
        double getElement(long long int _nLine, long long int _nCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->readMem(_nLine, _nCol);

            return NAN;
		}

		std::vector<double> getElement(const VectorIndex& _vLine, const VectorIndex& _vCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->readMem(_vLine, _vCol);

            return std::vector<double>();
		}

		void copyElementsInto(std::vector<double>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol, const std::string& _sTable) const
		{
			vMemory[findTable(_sTable)]->copyElementsInto(vTarget, _vLine, _vCol);
		}

		int getHeadlineCount(const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->getHeadlineCount();

            return 0;
		}

		std::string getHeadLineElement(long long int _i, const std::string& _sTable) const
		{
			if (exists(_sTable))
                return vMemory[findTable(_sTable)]->getHeadLineElement(_i);

            return "";
		}

		std::vector<std::string> getHeadLineElement(const VectorIndex& _vCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->getHeadLineElement(_vCol);

            return std::vector<std::string>();
		}

		std::string getTopHeadLineElement(long long int _i, const std::string& _sTable) const
        {
            return getHeadLineElement(_i, _sTable).substr(0, getHeadLineElement(_i, _sTable).find("\\n"));
        }

		long long int getAppendedZeroes(long long int _i, const std::string& _sTable) const
		{
			return vMemory[findTable(_sTable)]->getAppendedZeroes(_i);
		}



        // WRITE ACCESS METHODS
		inline void writeToTable(long long int _nLine, long long int _nCol, const std::string& _sCache, double _dData)
		{
			vMemory[findTable(_sCache)]->writeData(_nLine, _nCol, _dData);
		}

		inline void writeToTable(Indices& _idx, const std::string& _sCache, double* _dData, unsigned int _nNum)
		{
			vMemory[findTable(_sCache)]->writeData(_idx, _dData, _nNum);
		}

		bool setHeadLineElement(long long int _i, const std::string& _sTable, std::string _sHead)
		{
			return vMemory[findTable(_sTable)]->setHeadLineElement(_i, _sHead);
		}



		// MAF METHODS
		// IMPLEMENTATIONS FOR THE TABLE METHODS
		std::vector<double> std(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::std);
        }

		std::vector<double> avg(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::avg);
        }

		std::vector<double> max(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::max);
        }

		std::vector<double> min(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::min);
        }

		std::vector<double> prd(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::prd);
        }

        std::vector<double> sum(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::sum);
        }

		std::vector<double> num(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::num);
        }

		std::vector<double> and_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::and_func);
        }

		std::vector<double> or_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::or_func);
        }

		std::vector<double> xor_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::xor_func);
        }

		std::vector<double> cnt(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::cnt);
        }

		std::vector<double> norm(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::norm);
        }

		std::vector<double> med(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::med);
        }

		std::vector<double> cmp(const std::string& sTable, std::string sDir, double dRef = 0.0, int nType = 0) const
        {
            std::vector<double> vResults;

            long long int nGridOffset = sDir.find("grid") != std::string::npos ? 2 : 0;

            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getCols(sTable, false))
                        continue;

                    vResults.push_back(cmp(sTable, 0, getLines(sTable, false)-1, _idx[i]+nGridOffset, -1, dRef, nType));
                }
            }
            else if (sDir.find("lines") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getLines(sTable, false))
                        continue;

                    vResults.push_back(cmp(sTable, _idx[i]+nGridOffset, -1, 0, getCols(sTable, false)-1, dRef, nType));
                }
            }
            else
                vResults.push_back(cmp(sTable, 0, getLines(sTable, false)-1, nGridOffset, getCols(sTable, false)-1, dRef, nType));

            if (!vResults.size())
                vResults.push_back(NAN);

            return vResults;
        }

		std::vector<double> pct(const std::string& sTable, std::string sDir, double dPct = 0.5) const
        {
            std::vector<double> vResults;

            long long int nGridOffset = sDir.find("grid") != std::string::npos ? 2 : 0;

            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getCols(sTable, false))
                        continue;

                    vResults.push_back(pct(sTable, 0, getLines(sTable, false)-1, _idx[i]+nGridOffset, -1, dPct));
                }
            }
            else if (sDir.find("lines") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getLines(sTable, false))
                        continue;

                    vResults.push_back(pct(sTable, _idx[i]+nGridOffset, -1, 0, getCols(sTable, false)-1, dPct));
                }
            }
            else
                vResults.push_back(pct(sTable, 0, getLines(sTable, false)-1, nGridOffset, getCols(sTable, false)-1, dPct));

            if (!vResults.size())
                vResults.push_back(NAN);

            return vResults;
        }

		std::vector<double> size(const std::string& sTable, std::string sDir) const
        {
            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
                return vMemory[findTable(sTable)]->size(_idx, COLS | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else if (sDir.find("lines") != std::string::npos)
                return vMemory[findTable(sTable)]->size(_idx, LINES | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else
                return vMemory[findTable(sTable)]->size(VectorIndex(), sDir.find("grid") != std::string::npos ? GRID : ALL);
        }

		std::vector<double> minpos(const std::string& sTable, std::string sDir) const
        {
            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
                return vMemory[findTable(sTable)]->minpos(_idx, COLS | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else if (sDir.find("lines") != std::string::npos)
                return vMemory[findTable(sTable)]->minpos(_idx, LINES | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else
                return vMemory[findTable(sTable)]->minpos(VectorIndex(0, VectorIndex::OPEN_END), sDir.find("grid") != std::string::npos ? GRID : ALL);
        }

		std::vector<double> maxpos(const std::string& sTable, std::string sDir) const
        {
            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
                return vMemory[findTable(sTable)]->maxpos(_idx, COLS | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else if (sDir.find("lines") != string::npos)
                return vMemory[findTable(sTable)]->maxpos(_idx, LINES | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else
                return vMemory[findTable(sTable)]->maxpos(VectorIndex(0, VectorIndex::OPEN_END), sDir.find("grid") != std::string::npos ? GRID : ALL);
        }


        // IMPLEMENTATIONS FOR THE MAFS
		inline double std(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->std(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double std(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->std(_vLine, _vCol);
		}

		inline double avg(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->avg(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double avg(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->avg(_vLine, _vCol);
		}

		inline double max(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->max(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double max(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->max(_vLine, _vCol);
		}

		inline double min(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->min(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double min(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->min(_vLine, _vCol);
		}

		inline double prd(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->prd(_vLine, _vCol);
		}

		inline double prd(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->prd(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double sum(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->sum(_vLine, _vCol);
		}

		inline double sum(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->sum(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double num(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->num(_vLine, _vCol);
		}

		inline double num(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->num(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double and_func(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->and_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double and_func(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->and_func(_vLine, _vCol);
		}

		inline double or_func(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->or_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double or_func(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->or_func(_vLine, _vCol);
		}

		inline double xor_func(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->xor_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double xor_func(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->xor_func(_vLine, _vCol);
		}

		inline double cnt(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->cnt(_vLine, _vCol);
		}

		inline double cnt(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->cnt(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double norm(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->norm(_vLine, _vCol);
		}

		inline double norm(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->norm(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double cmp(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, double dRef = 0.0, int nType = 0) const
		{
			return vMemory[findTable(_sCache)]->cmp(_vLine, _vCol, dRef, nType);
		}

		inline double cmp(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dRef = 0.0, int nType = 0) const
		{
			return vMemory[findTable(_sCache)]->cmp(VectorIndex(i1, i2), VectorIndex(j1, j2), dRef, nType);
		}

		inline double med(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->med(_vLine, _vCol);
		}

		inline double med(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->med(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline double pct(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, double dPct = 0.5) const
		{
			return vMemory[findTable(_sCache)]->pct(_vLine, _vCol, dPct);
		}

		inline double pct(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dPct = 0.5) const
		{
			return vMemory[findTable(_sCache)]->pct(VectorIndex(i1, i2), VectorIndex(j1, j2), dPct);
		}

};

#endif
