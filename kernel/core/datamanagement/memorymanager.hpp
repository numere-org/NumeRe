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
        std::vector<mu::value_type> resolveMAF(const std::string& sTableName, std::string sDir, mu::value_type (MemoryManager::*MAF)(const std::string&, long long int, long long int, long long int, long long int) const) const;

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
		mu::value_type tableLinesCount;
		mu::value_type tableColumnsCount;
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

        bool resizeTable(int _nCols, const std::string& _sTable)
		{
			return vMemory[findTable(_sTable)]->resizeMemory(1, _nCols);
		}

        void deleteEntry(int _nLine, int _nCol, const std::string& _sCache)
		{
			vMemory[findTable(_sCache)]->deleteEntry(_nLine, _nCol);
		}

		void deleteBulk(const std::string& _sCache, int i1, int i2, int j1 = 0, int j2 = 0)
		{
			vMemory[findTable(_sCache)]->deleteBulk(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		void deleteBulk(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol)
		{
			vMemory[findTable(_sCache)]->deleteBulk(_vLine, _vCol);
		}

		void shrink(const std::string& _sCache)
		{
		    vMemory[findTable(_sCache)]->shrink();
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

		inline NumeRe::Table extractTable(int _nLayer, const std::string& _sTable = "", const VectorIndex& lines = VectorIndex(0, VectorIndex::OPEN_END), const VectorIndex& cols = VectorIndex(0, VectorIndex::OPEN_END))
		{
			return vMemory[_nLayer]->extractTable(_sTable, lines, cols);
		}

		inline void importTable(NumeRe::Table _table, const std::string& _sTable, const VectorIndex& lines = VectorIndex(0, VectorIndex::OPEN_END), const VectorIndex& cols = VectorIndex(0, VectorIndex::OPEN_END))
		{
			return vMemory[findTable(_sTable)]->importTable(_table, lines, cols);
		}

		inline void importTable(NumeRe::Table _table, int _nLayer, const VectorIndex& lines = VectorIndex(0, VectorIndex::OPEN_END), const VectorIndex& cols = VectorIndex(0, VectorIndex::OPEN_END))
		{
			return vMemory[_nLayer]->importTable(_table, lines, cols);
		}


		// TABLE INPLACE MODIFICATION METHODS
		std::vector<int> sortElements(const std::string& sLine);
		std::vector<int> sortElements(const std::string& sCache, int i1, int i2, int j1 = 0, int j2 = 0, const std::string& sSortingExpression = "");

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
		inline int getLines(StringView sTable, bool _bFull = false) const
		{
		    size_t idx = mapStringViewFind(sTable);

		    if (idx != (size_t)-1)
                return vMemory[idx]->getLines(_bFull);

            return 0;
		}

		inline int getLines(const std::string& sTable, bool _bFull = false) const
		{
		    auto iter = mCachesMap.find(sTable);

		    if (iter != mCachesMap.end())
                return vMemory[findTable(sTable)]->getLines(_bFull);

            return 0;
		}

		inline int getCols(StringView sTable, bool _bFull = false) const
		{
		    size_t idx = mapStringViewFind(sTable);

		    if (idx != (size_t)-1)
                return vMemory[idx]->getCols(_bFull);

            return 0;
		}

		inline int getCols(const std::string& sTable, bool _bFull = false) const
		{
		    auto iter = mCachesMap.find(sTable);

		    if (iter != mCachesMap.end())
                return vMemory[findTable(sTable)]->getCols(_bFull);

            return 0;
		}

		inline int getSize(int _nLayer) const
		{
			if (vMemory.size() && _nLayer < (int)vMemory.size())
				return vMemory[_nLayer]->getSize();

			return 0;
		}



        // READ ACCESS METHODS
        mu::value_type getElement(int _nLine, int _nCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->readMem(_nLine, _nCol);

            return NAN;
		}

		std::vector<mu::value_type> getElement(const VectorIndex& _vLine, const VectorIndex& _vCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->readMem(_vLine, _vCol);

            return std::vector<mu::value_type>();
		}

		ValueVector getElementMixed(const VectorIndex& _vLine, const VectorIndex& _vCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->readMixedMem(_vLine, _vCol);

            return ValueVector();
		}

		TableColumn::ColumnType getType(const VectorIndex& _vCol, const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->getType(_vCol);

            return TableColumn::TYPE_NONE;
		}

		void copyElementsInto(std::vector<mu::value_type>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol, const std::string& _sTable) const
		{
			vMemory[findTable(_sTable)]->copyElementsInto(vTarget, _vLine, _vCol);
		}

		int getHeadlineCount(const std::string& _sTable) const
		{
		    if (exists(_sTable))
                return vMemory[findTable(_sTable)]->getHeadlineCount();

            return 0;
		}

		std::string getHeadLineElement(int _i, const std::string& _sTable) const
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

		std::string getTopHeadLineElement(int _i, const std::string& _sTable) const
        {
            return getHeadLineElement(_i, _sTable).substr(0, getHeadLineElement(_i, _sTable).find("\\n"));
        }

		int getAppendedZeroes(int _i, const std::string& _sTable) const
		{
			return vMemory[findTable(_sTable)]->getAppendedZeroes(_i);
		}

		int getColElements(const VectorIndex& cols, const std::string& _sTable) const;



        // WRITE ACCESS METHODS
		inline void writeToTable(int _nLine, int _nCol, const std::string& _sCache, mu::value_type _dData)
		{
			vMemory[findTable(_sCache)]->writeData(_nLine, _nCol, _dData);
		}

		inline void writeToTable(int _nLine, int _nCol, const std::string& _sCache, const std::string& _sValue)
		{
			vMemory[findTable(_sCache)]->writeData(_nLine, _nCol, _sValue);
		}

		inline void writeToTable(Indices& _idx, const std::string& _sCache, mu::value_type* _dData, unsigned int _nNum)
		{
			vMemory[findTable(_sCache)]->writeData(_idx, _dData, _nNum);
		}

		inline void writeToTable(Indices& _idx, const std::string& _sCache, const ValueVector& _values)
		{
			vMemory[findTable(_sCache)]->writeData(_idx, _values);
		}

		bool setHeadLineElement(int _i, const std::string& _sTable, std::string _sHead)
		{
			return vMemory[findTable(_sTable)]->setHeadLineElement(_i, _sHead);
		}

		void overwriteColumn(int col, const std::string& _sCache, TableColumn::ColumnType type);


		// MAF METHODS
		// IMPLEMENTATIONS FOR THE TABLE METHODS
		std::vector<mu::value_type> std(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::std);
        }

		std::vector<mu::value_type> avg(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::avg);
        }

		std::vector<mu::value_type> max(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::max);
        }

		std::vector<mu::value_type> min(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::min);
        }

		std::vector<mu::value_type> prd(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::prd);
        }

        std::vector<mu::value_type> sum(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::sum);
        }

		std::vector<mu::value_type> num(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::num);
        }

		std::vector<mu::value_type> and_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::and_func);
        }

		std::vector<mu::value_type> or_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::or_func);
        }

		std::vector<mu::value_type> xor_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::xor_func);
        }

		std::vector<mu::value_type> cnt(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::cnt);
        }

		std::vector<mu::value_type> norm(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::norm);
        }

		std::vector<mu::value_type> med(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::med);
        }

		std::vector<mu::value_type> cmp(const std::string& sTable, std::string sDir, mu::value_type dRef = 0.0, int nType = 0) const
        {
            std::vector<mu::value_type> vResults;

            long long int nGridOffset = sDir.find("grid") != std::string::npos ? 2 : 0;

            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getCols(sTable, false))
                        continue;

                    vResults.push_back(cmp(sTable, 0, getLines(sTable, false)-1, _idx[i]+nGridOffset, -1, dRef.real(), nType));
                }
            }
            else if (sDir.find("lines") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getLines(sTable, false))
                        continue;

                    vResults.push_back(cmp(sTable, _idx[i]+nGridOffset, -1, 0, getCols(sTable, false)-1, dRef.real(), nType));
                }
            }
            else
                vResults.push_back(cmp(sTable, 0, getLines(sTable, false)-1, nGridOffset, getCols(sTable, false)-1, dRef.real(), nType));

            if (!vResults.size())
                vResults.push_back(NAN);

            return vResults;
        }

		std::vector<mu::value_type> pct(const std::string& sTable, std::string sDir, mu::value_type dPct = 0.5) const
        {
            std::vector<mu::value_type> vResults;

            long long int nGridOffset = sDir.find("grid") != std::string::npos ? 2 : 0;

            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getCols(sTable, false))
                        continue;

                    vResults.push_back(pct(sTable, 0, getLines(sTable, false)-1, _idx[i]+nGridOffset, -1, dPct.real()));
                }
            }
            else if (sDir.find("lines") != std::string::npos)
            {
                for (size_t i = 0; i < _idx.size(); i++)
                {
                    if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset >= getLines(sTable, false))
                        continue;

                    vResults.push_back(pct(sTable, _idx[i]+nGridOffset, -1, 0, getCols(sTable, false)-1, dPct.real()));
                }
            }
            else
                vResults.push_back(pct(sTable, 0, getLines(sTable, false)-1, nGridOffset, getCols(sTable, false)-1, dPct.real()));

            if (!vResults.size())
                vResults.push_back(NAN);

            return vResults;
        }

		std::vector<mu::value_type> size(const std::string& sTable, std::string sDir) const
        {
            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
                return vMemory[findTable(sTable)]->size(_idx, COLS | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else if (sDir.find("lines") != std::string::npos)
                return vMemory[findTable(sTable)]->size(_idx, LINES | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else
                return vMemory[findTable(sTable)]->size(VectorIndex(), sDir.find("grid") != std::string::npos ? GRID : ALL);
        }

		std::vector<mu::value_type> minpos(const std::string& sTable, std::string sDir) const
        {
            VectorIndex _idx = parseEvery(sDir, sTable);

            if (sDir.find("cols") != std::string::npos)
                return vMemory[findTable(sTable)]->minpos(_idx, COLS | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else if (sDir.find("lines") != std::string::npos)
                return vMemory[findTable(sTable)]->minpos(_idx, LINES | (sDir.find("grid") != std::string::npos ? GRID : 0));
            else
                return vMemory[findTable(sTable)]->minpos(VectorIndex(0, VectorIndex::OPEN_END), sDir.find("grid") != std::string::npos ? GRID : ALL);
        }

		std::vector<mu::value_type> maxpos(const std::string& sTable, std::string sDir) const
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
		inline mu::value_type std(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->std(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type std(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->std(_vLine, _vCol);
		}

		inline mu::value_type avg(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->avg(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type avg(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->avg(_vLine, _vCol);
		}

		inline mu::value_type max(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->max(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type max(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->max(_vLine, _vCol);
		}

		inline mu::value_type min(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->min(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type min(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->min(_vLine, _vCol);
		}

		inline mu::value_type prd(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->prd(_vLine, _vCol);
		}

		inline mu::value_type prd(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->prd(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type sum(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->sum(_vLine, _vCol);
		}

		inline mu::value_type sum(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->sum(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type num(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->num(_vLine, _vCol);
		}

		inline mu::value_type num(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->num(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type and_func(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->and_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type and_func(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->and_func(_vLine, _vCol);
		}

		inline mu::value_type or_func(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->or_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type or_func(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->or_func(_vLine, _vCol);
		}

		inline mu::value_type xor_func(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->xor_func(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type xor_func(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->xor_func(_vLine, _vCol);
		}

		inline mu::value_type cnt(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->cnt(_vLine, _vCol);
		}

		inline mu::value_type cnt(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->cnt(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type norm(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->norm(_vLine, _vCol);
		}

		inline mu::value_type norm(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->norm(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type cmp(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, mu::value_type dRef = 0.0, int nType = 0) const
		{
			return vMemory[findTable(_sCache)]->cmp(_vLine, _vCol, dRef, nType);
		}

		inline mu::value_type cmp(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, mu::value_type dRef = 0.0, int nType = 0) const
		{
			return vMemory[findTable(_sCache)]->cmp(VectorIndex(i1, i2), VectorIndex(j1, j2), dRef, nType);
		}

		inline mu::value_type med(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol) const
		{
			return vMemory[findTable(_sCache)]->med(_vLine, _vCol);
		}

		inline mu::value_type med(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1) const
		{
			return vMemory[findTable(_sCache)]->med(VectorIndex(i1, i2), VectorIndex(j1, j2));
		}

		inline mu::value_type pct(const std::string& _sCache, const VectorIndex& _vLine, const VectorIndex& _vCol, mu::value_type dPct = 0.5) const
		{
			return vMemory[findTable(_sCache)]->pct(_vLine, _vCol, dPct);
		}

		inline mu::value_type pct(const std::string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, mu::value_type dPct = 0.5) const
		{
			return vMemory[findTable(_sCache)]->pct(VectorIndex(i1, i2), VectorIndex(j1, j2), dPct);
		}

};

#endif


