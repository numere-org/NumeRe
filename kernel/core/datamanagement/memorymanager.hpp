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


#include <boost/nowide/fstream.hpp>
#include <string>
#include <vector>

#include "../ui/error.hpp"
#include "../settings.hpp"
#include "../structures.hpp"
#include "tableview.hpp"
#include "memory.hpp"
#include "cluster.hpp"
#include "fileadapter.hpp"


#ifndef MEMORYMANAGER_HPP
#define MEMORYMANAGER_HPP


struct TableReference
{
    size_t owned;
    size_t referenced;
};


/////////////////////////////////////////////////
/// \brief This class represents the central
/// memory managing instance. It will handle all
/// tables and clusters, which have been created
/// during the runtime.
/////////////////////////////////////////////////
class MemoryManager : public NumeRe::FileAdapter, public NumeRe::ClusterManager
{
    private:
        std::vector<Table*> m_tables;
        std::map<std::string, TableReference> m_tableReferences;
        bool bSaveMutex;
        boost::nowide::fstream cache_file;
        std::string sCache_file;
        std::string sPredefinedFuncs;
        std::string sUserdefinedFuncs;
        std::string sPredefinedCommands;
        std::string sPluginCommands;

        void reorderColumn(size_t _nLayer, const std::vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
        bool loadFromNewCacheFile();
        bool loadFromLegacyCacheFile();
        VectorIndex parseMafSelectors(std::string& sDir, const std::string& sType, const std::string& sTableName, size_t refSheet) const;
        std::vector<std::complex<double>> resolveMAF(const std::string& sTableName, std::string sDir,
                                                     std::complex<double> (MemoryManager::*MAF)(const std::string&,
                                                                                                const VectorIndex&,
                                                                                                const VectorIndex&,
                                                                                                const VectorIndex&) const) const;

        virtual bool saveLayer(std::string _sFileName, const std::string& _sTable, unsigned short nPrecision, std::string sExt = "") override
        {
            return m_tables[findTable(_sTable)]->save(ValidFileName(_sFileName, ".ndat", !sExt.length()), _sTable, nPrecision, sExt);
        }

        inline bool exists(const std::string& sTable) const
        {
            return m_tableReferences.find(sTable) != m_tableReferences.end();
        }

        size_t mapStringViewFind(StringView view) const
        {
            for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
            {
                if (view == iter->first)
                {
                    if (iter->second.owned != iter->second.referenced)
                        return iter->second.referenced;

                    return iter->second.owned;
                }
                else if (view < iter->first)
                    throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, view.to_string(), view.to_string());
            }

            return -1;
        }

        size_t findTable(const std::string& sTable) const
        {
            auto iter = m_tableReferences.find(sTable);

            if (iter == m_tableReferences.end())
                throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sTable, sTable);

            if (iter->second.owned != iter->second.referenced)
                return iter->second.referenced;

            return iter->second.owned;
        }

    public:
        MemoryManager();
        ~MemoryManager();

        enum AppDir {ROWS = Table::ROWS, COLS = Table::COLS, GRID = Table::GRID, ALL = Table::ALL};

        // Variables for the parser
        mutable mu::Variable tableRowsCount;
        mutable mu::Variable tableColumnsCount;
        mutable mu::Variable tableSheetsCount;
        bool updateDimensionVariables(StringView sTableName) const;


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
        bool isTable(const std::string& sTable, bool only = false) const;
        bool isTable(StringView sTable, bool only = false) const;

        bool isEmpty(const std::string& sTable) const
        {
            if (exists(sTable))
                return !m_tables[findTable(sTable)]->getCols();

            return true;
        }

        bool isValidElement(size_t nRow, size_t nCol, size_t nSheet, const std::string& _sTable) const
        {
            if (exists(_sTable))
                return m_tables[findTable(_sTable)]->isValidElement(nRow, nCol, nSheet);

            return false;
        }



        // RECOGNITION METHODS
        bool containsTables(const std::string& sExpression);
        bool containsTablesOrClusters(const std::string& sCmdLine);

        inline std::string matchTableAsParameter(const std::string& sExpression, char cFollowing = ' ')
        {
            for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
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


        // TABLE INFORMATION AND SAVING METHODS
        VectorIndex arrayToIndex(const mu::Array& arr, const std::string& sTable, size_t sheet) const;
        VectorIndex arrayToSheetIndex(const mu::Array& arr, const std::string& sTable) const;
        bool getSaveStatus() const;
        void setSaveStatus(bool _bIsSaved);
        long long int getLastSaved() const;
        bool saveToCacheFile();
        bool loadFromCacheFile();

        inline size_t getNumberOfTables() const
        {
            return m_tableReferences.size();
        }

        inline const std::map<std::string, TableReference>& getTableMap() const
        {
            return m_tableReferences;
        }

        inline std::string getTableNames() const
        {
            std::string sReturn = ";";

            for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
            {
                sReturn += iter->first + ";";
            }

            return sReturn;
        }


        // GLOBAL TABLE ACCESS METHODS
        Table& getTable(const std::string& sTable);
        Table& getTable(StringView sTable);
        const Table& getTable(const std::string& sTable) const;
        const Table& getTable(StringView sTable) const;
        virtual void melt(Table* _mem, const std::string& sTable, bool overrideTarget = false) override;

        inline void renameTable(const std::string& sCache, const std::string& sNewName, bool bForceRenaming = false)
        {
            if (isTable(sNewName))
                throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, "", SyntaxError::invalid_position, sNewName + "()");

            if (!isTable(sCache))
                throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sCache);

            if ((sCache == "table" || sCache == "string") && !bForceRenaming)
                throw SyntaxError(SyntaxError::CACHE_CANNOT_BE_RENAMED, "", SyntaxError::invalid_position, sCache);

            m_tableReferences[sNewName] = m_tableReferences[sCache];
            m_tableReferences.erase(sCache);
            setSaveStatus(false);
        }

        inline void swapTables(std::string sTable1, std::string sTable2)
        {
            if (!isTable(sTable1))
                throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sTable1);

            if (!isTable(sTable2))
                throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sTable2);

            size_t tab1 = m_tableReferences[sTable1].referenced;
            size_t tab2 = m_tableReferences[sTable2].referenced;

            for (auto& iter : m_tableReferences)
            {
                if (iter.second.owned == tab1)
                    iter.second.owned = tab2;
                else if (iter.second.owned == tab2)
                    iter.second.owned = tab1;

                if (iter.second.referenced == tab1)
                    iter.second.referenced = tab2;
                else if (iter.second.referenced == tab2)
                    iter.second.referenced = tab1;
            }
        }

        void addReference(const std::string& sTable, const std::string& sReference)
        {
            if (!isTable(sTable))
                throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sTable);

            if (!sReference.length())
            {
                m_tableReferences[sTable].referenced = m_tableReferences[sTable].owned;
                return;
            }
            else if (!isTable(sReference))
                throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, "", SyntaxError::invalid_position, sReference);

            auto iter = m_tableReferences.find(sReference.substr(0, sReference.find('(')));
            m_tableReferences[sTable].referenced = iter->second.owned;
        }

        void copyTable(const std::string& source, const std::string& target);
        void copyTable(const std::string& source, const Indices& sourceIdx, const std::string& target, const Indices& targetIdx);


        // TABLE INPLACE MODIFICATION METHODS
        std::vector<int> sortElements(const std::string& sLine);
        std::vector<int> sortElements(const std::string& sCache,
                                      const VectorIndex& _vRow,
                                      const VectorIndex& _vCol,
                                      const VectorIndex& _vSheet,
                                      const std::string& sSortingExpression = "");


        bool isValueLike(const VectorIndex& _vCol, const VectorIndex& _vSheet, const std::string& _sTable) const
        {
            if (exists(_sTable))
                return m_tables[findTable(_sTable)]->isValueLike(_vCol, _vSheet);

            return false;
        }

        std::string getHeadLineElement(size_t nCol, size_t sheet, const std::string& _sTable) const
        {
            if (exists(_sTable))
                return m_tables[findTable(_sTable)]->getHeadLineElement(nCol, sheet);

            return "";
        }

        std::vector<std::string> getHeadLineElement(const VectorIndex& _vCol, const VectorIndex& _vSheet, const std::string& _sTable) const
        {
            if (exists(_sTable))
                return m_tables[findTable(_sTable)]->getHeadLineElement(_vCol, _vSheet);

            return std::vector<std::string>();
        }

        std::string getTopHeadLineElement(size_t nCol, size_t sheet, const std::string& _sTable) const
        {
            std::string sHeadLineElement = getHeadLineElement(nCol, sheet, _sTable);
            return sHeadLineElement.substr(0, sHeadLineElement.find('\n'));
        }

        int getCols(const VectorIndex& sheets, const std::string& _sTable) const;
        int getRows(const VectorIndex& cols, const VectorIndex& sheets, const std::string& _sTable) const;


        // MAF METHODS
        // IMPLEMENTATIONS FOR THE TABLE METHODS
        std::vector<std::complex<double>> std(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::std);
        }

        std::vector<std::complex<double>> avg(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::avg);
        }

        std::vector<std::complex<double>> max(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::max);
        }

        std::vector<std::complex<double>> min(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::min);
        }

        std::vector<std::complex<double>> prd(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::prd);
        }

        std::vector<std::complex<double>> sum(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::sum);
        }

        std::vector<std::complex<double>> num(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::num);
        }

        std::vector<std::complex<double>> and_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::and_func);
        }

        std::vector<std::complex<double>> or_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::or_func);
        }

        std::vector<std::complex<double>> xor_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::xor_func);
        }

        std::vector<std::complex<double>> cnt(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::cnt);
        }

        std::vector<std::complex<double>> norm(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::norm);
        }

        std::vector<std::complex<double>> med(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::med);
        }

        std::vector<std::complex<double>> exc(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::exc);
        }

        std::vector<std::complex<double>> skew(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::skew);
        }

        std::vector<std::complex<double>> stderr_func(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::stderr_func);
        }

        std::vector<std::complex<double>> rms(const std::string& sTable, std::string sDir) const
        {
            return resolveMAF(sTable, sDir, MemoryManager::rms);
        }

        std::vector<std::complex<double>> cmp(const std::string& sTable, std::string sDir, std::complex<double> dRef = 0.0, int nType = 0) const;
        std::vector<std::complex<double>> pct(const std::string& sTable, std::string sDir, std::complex<double> dPct = 0.5) const;
        std::vector<std::complex<double>> size(const std::string& sTable, std::string sDir) const;
        std::vector<std::complex<double>> minpos(const std::string& sTable, std::string sDir) const;
        std::vector<std::complex<double>> maxpos(const std::string& sTable, std::string sDir) const;



        // IMPLEMENTATIONS FOR THE MAFS
        inline std::complex<double> std(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->std(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> avg(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->avg(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> max(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->max(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> min(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->min(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> prd(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->prd(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> sum(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->sum(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> num(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->num(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> and_func(const std::string& _sCache,
                                             const VectorIndex& _vRow,
                                             const VectorIndex& _vCol,
                                             const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->and_func(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> or_func(const std::string& _sCache,
                                            const VectorIndex& _vRow,
                                            const VectorIndex& _vCol,
                                            const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->or_func(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> xor_func(const std::string& _sCache,
                                             const VectorIndex& _vRow,
                                             const VectorIndex& _vCol,
                                             const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->xor_func(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> cnt(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->cnt(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> norm(const std::string& _sCache,
                                         const VectorIndex& _vRow,
                                         const VectorIndex& _vCol,
                                         const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->norm(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> cmp(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        size_t sheet,
                                        std::complex<double> dRef = 0.0, int nType = 0) const
        {
            return m_tables[findTable(_sCache)]->cmp(_vRow, _vCol, sheet, dRef, nType);
        }

        inline std::complex<double> med(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->med(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> pct(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet,
                                        std::complex<double> dPct = 0.5) const
        {
            return m_tables[findTable(_sCache)]->pct(_vRow, _vCol, _vSheet, dPct);
        }

        inline std::complex<double> exc(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->exc(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> skew(const std::string& _sCache,
                                         const VectorIndex& _vRow,
                                         const VectorIndex& _vCol,
                                         const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->skew(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> stderr_func(const std::string& _sCache,
                                                const VectorIndex& _vRow,
                                                const VectorIndex& _vCol,
                                                const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->stderr_func(_vRow, _vCol, _vSheet);
        }

        inline std::complex<double> rms(const std::string& _sCache,
                                        const VectorIndex& _vRow,
                                        const VectorIndex& _vCol,
                                        const VectorIndex& _vSheet) const
        {
            return m_tables[findTable(_sCache)]->rms(_vRow, _vCol, _vSheet);
        }

};

#endif


