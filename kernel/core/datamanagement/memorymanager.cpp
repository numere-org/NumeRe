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

#include <ctime>
#include <cmath>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

#include "memorymanager.hpp"
#include "../utils/tools.hpp"
#include "../version.h"
#include "../maths/resampler.h"
#include "../../kernel.hpp"
#include "tablecolumnimpl.hpp"
using namespace std;


/*
 * Realisierung der Cache-Klasse
 */


/////////////////////////////////////////////////
/// \brief Default MemoryManager constructor.
/// Creates the default table and initializes the
/// list of predefined commands.
/////////////////////////////////////////////////
MemoryManager::MemoryManager() : NumeRe::FileAdapter(), StringMemory(), NumeRe::ClusterManager()
{
	bSaveMutex = false;
	sCache_file = "<>/numere.cache";
	sPredefinedFuncs = "";
	sUserdefinedFuncs = "";
	sPredefinedCommands =  ";abort;about;audio;break;compose;cont;cont3d;continue;copy;credits;data;datagrid;define;delete;dens;dens3d;diff;draw;draw3d;edit;else;endcompose;endfor;endif;endprocedure;endwhile;eval;explicit;export;extrema;fft;find;fit;for;get;global;grad;grad3d;graph;graph3d;help;hist;hline;if;ifndef;ifndefined;imread;implot;info;integrate;list;load;matop;mesh;mesh3d;move;mtrxop;namespace;new;odesolve;plot;plot3d;procedure;pulse;quit;random;read;readline;regularize;remove;rename;replaceline;resample;return;save;script;set;smooth;sort;stats;stfa;str;surf;surf3d;swap;taylor;throw;undef;undefine;var;vect;vect3d;while;write;zeroes;";
	sPluginCommands = "";
	mCachesMap["table"] = std::make_pair(0u, 0u);
	vMemory.push_back(new Memory());

	tableColumnsCount = 0.0;
	tableLinesCount = 0.0;
}


/////////////////////////////////////////////////
/// \brief MemoryManager destructor. Clears all
/// created tables.
/////////////////////////////////////////////////
MemoryManager::~MemoryManager()
{
    if (cache_file.is_open())
        cache_file.close();

    for (size_t i = 0; i < vMemory.size(); i++)
        delete vMemory[i];
}


/////////////////////////////////////////////////
/// \brief Removes all tables in memory and re-
/// initializes the MemoryManager with the
/// default table.
///
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::removeTablesFromMemory()
{
	if (isValid())	// Sind ueberhaupt Daten vorhanden?
	{
        if (bSaveMutex)
            return;

        bSaveMutex = true;

		// --> Speicher, wo denn noetig freigeben <--
		for (size_t i = 0; i < vMemory.size(); i++)
            delete vMemory[i];

		vMemory.clear();
		bSaveMutex = false;
		mCachesMap.clear();
		mCachesMap["table"] = std::make_pair(0u, 0u);
		vMemory.push_back(new Memory());
		sDataFile.clear();
	}
}


/////////////////////////////////////////////////
/// \brief Evaluates, whether there's at least a
/// single non-empty table.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::isValid() const
{
    if (!vMemory.size())
        return false;

    for (size_t i = 0; i < vMemory.size(); i++)
    {
        if (vMemory[i]->getCols(false))
            return true;
    }

	return false;
}


/////////////////////////////////////////////////
/// \brief Returns, whether there's at least a
/// single table in memory, which has not been
/// saved yet.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::getSaveStatus() const
{
    if (!vMemory.size())
        return true;

    for (size_t i = 0; i < vMemory.size(); i++)
    {
        if (!vMemory[i]->getSaveStatus())
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Changes the save status of all tables
/// in memory.
///
/// \param _bIsSaved bool
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::setSaveStatus(bool _bIsSaved)
{
    for (size_t i = 0; i < vMemory.size(); i++)
    {
        vMemory[i]->setSaveStatus(_bIsSaved);
    }
}


/////////////////////////////////////////////////
/// \brief Returns the earliest time-point, when
/// a table was saved. This value is used to
/// determine the elapsed time for the autosave
/// interval.
///
/// \return long long int
///
/////////////////////////////////////////////////
long long int MemoryManager::getLastSaved() const
{
    long long int nLastSaved = 0;

    if (!vMemory.size())
        return 0;

    nLastSaved = vMemory[0]->getLastSaved();

    for (size_t i = 1; i < vMemory.size(); i++)
    {
        if (vMemory[i]->getLastSaved() < nLastSaved)
            nLastSaved = vMemory[i]->getLastSaved();
    }

    return nLastSaved;
}


/////////////////////////////////////////////////
/// \brief This member function wraps the sorting
/// functionality and evaluates the passed
/// parameter string before delegating to the
/// actual implementation.
///
/// \param sLine const string&
/// \return vector<int>
///
/////////////////////////////////////////////////
vector<int> MemoryManager::sortElements(const string& sLine)
{
    if (!isValid())
        return vector<int>();

    string sCache;
    string sSortingExpression = "-set";

    if (findCommand(sLine).sString != "sort")
    {
        sCache = findCommand(sLine).sString;
    }

    if (findParameter(sLine, "sort", '='))
    {
        if (getArgAtPos(sLine, findParameter(sLine, "sort", '=')+4) == "desc")
            sSortingExpression += " desc";
    }
    else
    {
        for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
        {
            if (findParameter(sLine, iter->first, '='))
            {
                if (getArgAtPos(sLine, findParameter(sLine, iter->first, '=')+5) == "desc")
                    sSortingExpression += " desc";

                sCache = iter->first;
                break;
            }
            else if (findParameter(sLine, iter->first))
            {
                sCache = iter->first;
                break;
            }
        }
    }

    if (findParameter(sLine, "cols", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, findParameter(sLine, "cols", '=')+4);
    else if (findParameter(sLine, "c", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, findParameter(sLine, "c", '=')+1);

    if (findParameter(sLine, "index"))
        sSortingExpression += " index";

    return vMemory[findTable(sCache)]->sortElements(0, getLines(sCache, false)-1, 0, getCols(sCache, false)-1, sSortingExpression);
}


/////////////////////////////////////////////////
/// \brief This member function informs the
/// selected table to sort its contents according
/// the passed parameter set.
///
/// \param sCache const string&
/// \param i1 int
/// \param i2 int
/// \param j1 int
/// \param j2 int
/// \param sSortingExpression const string&
/// \return vector<int>
///
/////////////////////////////////////////////////
vector<int> MemoryManager::sortElements(const string& sCache, int i1, int i2, int j1, int j2, const string& sSortingExpression)
{
    return vMemory[findTable(sCache)]->sortElements(i1, i2, j1, j2, sSortingExpression);
}


/////////////////////////////////////////////////
/// \brief This member function updates the name
/// of the cache file.
///
/// \param _sFileName string
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::setCacheFileName(string _sFileName)
{
    if (_sFileName.length())
    {
        sCache_file = ValidFileName(_sFileName, ".cache");
    }
}


/////////////////////////////////////////////////
/// \brief This member function saves the
/// contents of this class to the cache file so
/// that they may be restored after a restart.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::saveToCacheFile()
{
    if (bSaveMutex)
        return false;

    bSaveMutex = true;

    sCache_file = ValidFileName(sCache_file, ".cache");

    NumeRe::CacheFile cacheFile(sCache_file);

    cacheFile.setNumberOfTables(mCachesMap.size() - isTable("data"));
    cacheFile.writeCacheHeader();

    int nLines;
    int nCols;

    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == "data")
            continue;

        nLines = vMemory[iter->second.first]->getLines(false);
        nCols = vMemory[iter->second.first]->getCols(false);

        cacheFile.setDimensions(nLines, nCols);
        cacheFile.setData(&vMemory[iter->second.first]->memArray, nLines, nCols);
        cacheFile.setTableName(iter->first);
        cacheFile.setComment(vMemory[iter->second.first]->m_meta.comment);

        cacheFile.write();
    }

    setSaveStatus(true);

    bSaveMutex = false;
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function wraps the loading
/// of the tables from the cache file. It will
/// automatically detect the type of the cache
/// file.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::loadFromCacheFile()
{
    if (bSaveMutex)
        return false;

    bSaveMutex = true;
    sCache_file = ValidFileName(sCache_file, ".cache");

    if (!loadFromNewCacheFile())
        return loadFromLegacyCacheFile();

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function tries to load the
/// contents of the cache file in the new cache
/// file format. If it does not succeed, false is
/// returned.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::loadFromNewCacheFile()
{
    NumeRe::CacheFile cacheFile(sCache_file);

    try
    {
        cacheFile.readCacheHeader();
        size_t nCaches = cacheFile.getNumberOfTables();

        for (size_t i = 0; i < vMemory.size(); i++)
            delete vMemory[i];

        vMemory.clear();
        mCachesMap.clear();

        for (size_t i = 0; i < nCaches; i++)
        {
            cacheFile.read();

            mCachesMap[cacheFile.getTableName()] = std::make_pair(vMemory.size(), vMemory.size());
            vMemory.push_back(new Memory());

            vMemory.back()->resizeMemory(cacheFile.getRows(), cacheFile.getCols());
            cacheFile.getData(&vMemory.back()->memArray);

            vMemory.back()->shrink();

            if (cacheFile.getComment() != "NO COMMENT")
                vMemory.back()->m_meta.comment = cacheFile.getComment();
        }

        bSaveMutex = false;
        return true;

    }
    catch (...)
    {
        cacheFile.close();
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function loads the
/// contents of the cache file assuming the
/// legacy format.
///
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::loadFromLegacyCacheFile()
{
    char*** cHeadLine = 0;
    char* cCachesMap = 0;
    double* dCache = 0;
    bool* bValidData = 0;
    long long int nCols = 0;
    long long int nLines = 0;
    long long int nLayers = 0;

    long int nMajor = 0;
    long int nMinor = 0;
    long int nBuild = 0;
    size_t nLength = 0;
    size_t cachemapssize = 0;
    long long int nLayerIndex = 0;

    if (!cache_file.is_open())
        cache_file.close();

    cache_file.open(sCache_file.c_str(), ios_base::in | ios_base::binary);

    if (!isValid() && cache_file.good())
    {
        time_t tTime = 0;
        cache_file.read((char*)&nMajor, sizeof(long int));

        if (cache_file.fail() || cache_file.eof())
        {
            cache_file.close();
            bSaveMutex = false;
            return false;
        }

        cache_file.read((char*)&nMinor, sizeof(long int));
        cache_file.read((char*)&nBuild, sizeof(long int));
        cache_file.read((char*)&tTime, sizeof(time_t));
        cache_file.read((char*)&nLines, sizeof(long long int));
        cache_file.read((char*)&nCols, sizeof(long long int));

        if (nMajor*100+nMinor*10+nBuild >= 107 && nLines < 0 && nCols < 0)
        {
            nLines *= -1;
            nCols *= -1;
            cache_file.read((char*)&nLayers, sizeof(long long int));
            nLayers *= -1;
            cache_file.read((char*)&cachemapssize, sizeof(size_t));

            for (size_t i = 0; i < vMemory.size(); i++)
                delete vMemory[i];

            vMemory.clear();
            mCachesMap.clear();

            for (size_t i = 0; i < cachemapssize; i++)
            {
                nLength = 0;
                nLayerIndex = 0;
                cache_file.read((char*)&nLength, sizeof(size_t));
                cCachesMap = new char[nLength];
                cache_file.read(cCachesMap, sizeof(char)*nLength);
                cache_file.read((char*)&nLayerIndex, sizeof(long long int));
                string sTemp;
                sTemp.resize(nLength-1);

                for (size_t n = 0; n < nLength-1; n++)
                {
                    sTemp[n] = cCachesMap[n];
                }

                delete[] cCachesMap;
                cCachesMap = 0;
                mCachesMap[sTemp] = std::pair<size_t,size_t>(nLayerIndex, nLayerIndex);
                vMemory.push_back(new Memory());
            }
        }
        else
            nLayers = 1;

        cHeadLine = new char**[nLayers];
        dCache = new double[nLayers];
        bValidData = new bool[nLayers];

        for (long long int i = 0; i < nLayers; i++)
        {
            cHeadLine[i] = new char*[nCols];

            for (long long int j = 0; j < nCols; j++)
            {
                nLength = 0;
                cache_file.read((char*)&nLength, sizeof(size_t));
                cHeadLine[i][j] = new char[nLength];
                cache_file.read(cHeadLine[i][j], sizeof(char)*nLength);
                string sHead;

                for (size_t k = 0; k < nLength-1; k++)
                {
                    sHead += cHeadLine[i][j][k];
                }

                if (i < cachemapssize)
                    vMemory[i]->setHeadLineElement(j, sHead);
            }
        }

        cache_file.seekg(sizeof(long long int)*nLayers*nCols, ios_base::cur);

        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                cache_file.read((char*)dCache, sizeof(double)*nLayers);

                for (long long int k = 0; k < cachemapssize; k++)
                    vMemory[k]->writeData(i, j, dCache[k]);
            }
        }

        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                cache_file.read((char*)bValidData, sizeof(bool)*nLayers);

                for (long long int k = 0; k < cachemapssize; k++)
                {
                    if (!bValidData[k])
                        vMemory[k]->writeData(i, j, NAN);
                }
            }
        }

        for (size_t i = 0; i < vMemory.size(); i++)
        {
            vMemory[i]->shrink();
        }

        cache_file.close();
        setSaveStatus(true);
    }
    else
    {
        bSaveMutex = false;

        if (cHeadLine)
        {
            for (long long int i = 0; i < nLayers; i++)
            {
                for (long long int j = 0; j < nCols; j++)
                    delete[] cHeadLine[i][j];

                delete[] cHeadLine[i];
            }

            delete[] cHeadLine;
        }

        if (dCache)
            delete[] dCache;

        return false;
    }

    bSaveMutex = false;

    if (cHeadLine)
    {
        for (long long int i = 0; i < nLayers; i++)
        {
            for (long long int j = 0; j < nCols; j++)
                delete[] cHeadLine[i][j];

            delete[] cHeadLine[i];
        }

        delete[] cHeadLine;
    }

    if (dCache)
        delete[] dCache;

    return true;
}


/////////////////////////////////////////////////
/// \brief Returns the maximal number of elements
/// in the selected column range.
///
/// \param cols const VectorIndex&
/// \param _sTable const std::string&
/// \return int
///
/////////////////////////////////////////////////
int MemoryManager::getColElements(const VectorIndex& cols, const std::string& _sTable) const
{
    if (!exists(_sTable))
        return 0;

    Memory* _mem = vMemory[findTable(_sTable)];

    int nElems = 0;

    if (cols.isOpenEnd())
        cols.setOpenEndIndex(_mem->getCols()-1);

    for (size_t i = 0; i < cols.size(); i++)
    {
        if (_mem->getElemsInColumn(cols[i]) > nElems)
            nElems = _mem->getElemsInColumn(cols[i]);
    }

    return nElems;
}


/////////////////////////////////////////////////
/// \brief This member function converts the
/// selected column to the needed type, if this
/// column shall be overwritten by another than
/// the current type.
///
/// \param col int
/// \param _sCache const std::string&
/// \param type TableColumn::ColumnType
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::overwriteColumn(int col, const std::string& _sCache, TableColumn::ColumnType type)
{
    if (getCols(_sCache) <= col)
        return;

    convert_for_overwrite(vMemory[findTable(_sCache)]->memArray[col], col, type);
}


/////////////////////////////////////////////////
/// \brief This member function extracts and
/// parses the \c every or \c cell expression
/// part of a MAF call.
///
/// \param sDir std::string&
/// \param sType const std::string&
/// \param sTableName const std::string&
/// \return VectorIndex
///
/////////////////////////////////////////////////
VectorIndex MemoryManager::parseEveryCell(std::string& sDir, const std::string& sType, const std::string& sTableName) const
{
    int iterationDimension;

    if (sType == "every")
        iterationDimension = sDir.find("cols") != std::string::npos ? getCols(sTableName) : getLines(sTableName);
    else
        iterationDimension = sDir.find("cols") != std::string::npos ? getLines(sTableName) : getCols(sTableName);

    if (sDir.find(sType + "=(") != std::string::npos
        && (sDir.find("cols") != std::string::npos || sDir.find("lines") != std::string::npos))
    {
        std::string sEveryCell = sDir.substr(sDir.find(sType + "=(") + sType.length()+1);
        sEveryCell.erase(getMatchingParenthesis(sEveryCell)+1);
        StripSpaces(sEveryCell);
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        _parser.DisableAccessCaching();
        updateDimensionVariables(sTableName);

        if (sEveryCell.front() == '(' && sEveryCell.back() == ')')
            sEveryCell = sEveryCell.substr(1, sEveryCell.length()-2);

        if (sEveryCell.front() == '{' && sEveryCell.back() == '}')
        {
            // Definition contains a vector expression
            _parser.SetExpr(sEveryCell);
            int nResults;
            mu::Array* v = _parser.Eval(nResults);

            return VectorIndex(v[0]);
        }
        else
        {
            // Usual expression
            _parser.SetExpr(sEveryCell);
            int nResults;
            mu::Array* v = _parser.Eval(nResults);

            if (nResults == 1)
            {
                // Single result: usual every=a,a representation
                std::vector<int> idx;

                for (int i = v[0].getAsScalarInt()-1; i < iterationDimension; i += v[0].getAsScalarInt())
                {
                    idx.push_back(i);
                }

                return VectorIndex(idx);
            }
            else if (nResults == 2)
            {
                // Two results: usual every=a,b representation
                std::vector<int> idx;

                for (int i = v[0].getAsScalarInt()-1; i < iterationDimension; i += v[1].getAsScalarInt())
                {
                    idx.push_back(i);
                }

                return VectorIndex(idx);
            }
            else //arbitrary results: use it as if it was a vector
                return VectorIndex(v[0]);
        }
    }

    return VectorIndex(0, iterationDimension-1);
}


/////////////////////////////////////////////////
/// \brief This member function is the abstract
/// implementation of a MAF function call. Most
/// of the MAFs use this abstraction (except
/// \c cmp and \c pct).
///
/// \param sTableName std::string& const
/// \param sDir std::string
/// \param MAF (std::complex<double>*)
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> MemoryManager::resolveMAF(const std::string& sTableName, std::string sDir, std::complex<double> (MemoryManager::*MAF)(const std::string&, const VectorIndex&, const VectorIndex&) const) const
{
    std::vector<std::complex<double>> vResults;
    int nlines = getLines(sTableName, false);
    int ncols = getCols(sTableName, false);

    // Find the "grid" parameter and use it as an offset
    int nGridOffset = sDir.find("grid") != std::string::npos ? 2 : 0;

    // If a grid is required, get the grid dimensions
    // of this table
    if (nGridOffset)
    {
        std::vector<std::complex<double>> vSize = vMemory[findTable(sTableName)]->size(VectorIndex(), VectorIndex(), GRID);
        nlines = vSize.front().real();
        ncols = vSize.back().real()+nGridOffset; // compensate the offset
    }

    // Get the vector index corresponding to a possible
    // every definition
    VectorIndex _everyIdx = parseEveryCell(sDir, "every", sTableName);
    VectorIndex _cellsIdx = parseEveryCell(sDir, "cells", sTableName);

    // Resolve the actual call to the MAF
    if (sDir.find("cols") != std::string::npos)
    {
        for (size_t i = 0; i < _everyIdx.size(); i++)
        {
            if (_everyIdx[i]+nGridOffset < 0 || _everyIdx[i]+nGridOffset >= ncols)
                continue;

            vResults.push_back((this->*MAF)(sTableName, _cellsIdx, VectorIndex(_everyIdx[i]+nGridOffset)));
        }
    }
    else if (sDir.find("lines") != std::string::npos)
    {
        _cellsIdx.apply_offset(nGridOffset);

        for (size_t i = 0; i < _everyIdx.size(); i++)
        {
            if (_everyIdx[i] < 0 || _everyIdx[i] >= nlines)
                continue;

            vResults.push_back((this->*MAF)(sTableName, VectorIndex(_everyIdx[i]), _cellsIdx));
        }
    }
    else
        vResults.push_back((this->*MAF)(sTableName, VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(nGridOffset, VectorIndex::OPEN_END)));

    if (!vResults.size())
        vResults.push_back(NAN);

    return vResults;
}


/////////////////////////////////////////////////
/// \brief Removes the "data()" table, if it is
/// available.
///
/// \param bAutoSave bool
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::removeData(bool bAutoSave)
{
    if (vMemory.size() && mCachesMap.find("data") != mCachesMap.end())
    {
        deleteTable("data");
        sDataFile = "";
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns a pointer
/// to an existing Memory instance representing
/// the selected table or a nullpointer, if the
/// table does not exist.
///
/// \param sTable const string&
/// \return Memory*
///
/////////////////////////////////////////////////
Memory* MemoryManager::getTable(const string& sTable)
{
    if (mCachesMap.find(sTable.substr(0, sTable.find('('))) == mCachesMap.end())
        return nullptr;

    return vMemory[findTable(sTable.substr(0, sTable.find('(')))];
}


/////////////////////////////////////////////////
/// \brief Copy one table to another one (and
/// create the missing table automatically, if
/// needed).
///
/// \param source const std::string&
/// \param target const std::string&
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::copyTable(const std::string& source, const std::string& target)
{
    Memory* sourceTable = vMemory[findTable(source.substr(0, source.find('(')))];

    // Create the target table, if it does not exist
    if (!exists(target.substr(0, target.find('('))))
        addTable(target, NumeReKernel::getInstance()->getSettings());

    Memory* targetTable = vMemory[findTable(target.substr(0, target.find('(')))];

    // Use the assignment operator overload
    *targetTable = *sourceTable;
}


/////////////////////////////////////////////////
/// \brief Copy some contents of one table to
/// another one (and create the missing table
/// automatically, if needed). Overload for only
/// copying a part of a table.
///
/// \param source const std::string&
/// \param sourceIdx const Indices&
/// \param target const std::string&
/// \param targetIdx const Indices&
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::copyTable(const std::string& source, const Indices& sourceIdx, const std::string& target, const Indices& targetIdx)
{
    if (sourceIdx.row.isFullRange(getLines(source)-1) && sourceIdx.col.isFullRange(getCols(source)-1)
        && targetIdx.row.isFullRange(getLines(target)-1) && targetIdx.col.isFullRange(getCols(target)-1))
    {
        copyTable(source, target);
        return;
    }

    // Get the extract as a table
    NumeRe::Table extract = extractTable(source, sourceIdx.row, sourceIdx.col);

    // Create the target table, if it does not exist
    if (!exists(target.substr(0, target.find('('))))
        addTable(target, NumeReKernel::getInstance()->getSettings());

    // Insert the copied table at the new location
    insertCopiedTable(extract, target, targetIdx.row, targetIdx.col, false);
}


/////////////////////////////////////////////////
/// \brief This member function either combines
/// the contents of the passed Memory instance
/// with an existing one with the passed table
/// name, or simply appends it to the list of
/// known tables.
///
/// \param _mem Memory*
/// \param sTable const string&
/// \param overrideTarget bool
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::melt(Memory* _mem, const string& sTable, bool overrideTarget)
{
    // Ensure that the table exists
    if (!_mem || !_mem->memArray.size())
        return;

    // Is a corresponding table known?
    if (mCachesMap.find(sTable) == mCachesMap.end())
    {
        // Append the new table
        mCachesMap[sTable] = std::make_pair(vMemory.size(), vMemory.size());
        vMemory.push_back(_mem);
    }
    else if (overrideTarget)
    {
        // Shall the original table be overwritten?
        delete vMemory[mCachesMap[sTable].second];
        vMemory[mCachesMap[sTable].second] = _mem;
    }
    else
    {
        // Combine both tables
        Memory* _existingMem = vMemory[mCachesMap[sTable].second];

        size_t nCols = _existingMem->memArray.size();

        // Resize the existing table to fit the contents
        // of both tables
        _existingMem->resizeMemory(1, nCols + _mem->memArray.size());

        // Move the contents
        for (size_t j = 0; j < _mem->memArray.size(); j++)
        {
            if (_mem->memArray[j])
                _existingMem->memArray[j+nCols].reset(_mem->memArray[j].release());
        }

        _existingMem->setSaveStatus(false);
        _existingMem->nCalcLines = -1;
        _existingMem->setMetaData(_existingMem->getMetaData().melt(_mem->getMetaData()));

        // Delete the passed instance (it is not
        // needed any more).
        delete _mem;
    }
}


/////////////////////////////////////////////////
/// \brief This member function updates the
/// dimension variables for the selected table to
/// be used in expressions.
///
/// \param sTableName StringView
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::updateDimensionVariables(StringView sTableName) const
{
    // Determine the type of table
    if (sTableName != "string")
    {
        // Update the dimensions for the selected
        // numerical table
        tableLinesCount = getLines(sTableName, false);
        tableColumnsCount = getCols(sTableName, false);
    }
    else
    {
        // Update the dimensions for the selected
        // string table
        tableLinesCount = getStringElements();
        tableColumnsCount = getStringCols();
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function evaluates,
/// whether the passed command line contains
/// tables or clusters.
///
/// \param sCmdLine const string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::containsTablesOrClusters(const string& sCmdLine)
{
    if (sCmdLine.find_first_of("({") == std::string::npos)
        return false;

    // Check the first character directly
    size_t nQuotes = sCmdLine.front() == '"';

    // Search through the expression -> We do not need to examine the first character
    for (size_t i = 1; i < sCmdLine.length(); i++)
    {
        // Consider quotation marks
        if (sCmdLine[i] == '"' && sCmdLine[i-1] != '\\')
            nQuotes++;

        if (nQuotes % 2)
            continue;

        // Is this a candidate for a table
        if (sCmdLine[i] == '('
            && (isalnum(sCmdLine[i-1]) || sCmdLine[i-1] == '_' || sCmdLine[i-1] == '~'))
        {
            size_t nStartPos = i-1;

            // Try to find the starting position
            while (nStartPos > 0 && (isalnum(sCmdLine[nStartPos-1]) || sCmdLine[nStartPos-1] == '_' || sCmdLine[nStartPos-1] == '~'))
            {
                nStartPos--;
            }

            // Try to find the candidate in the internal map
            if (mCachesMap.find(sCmdLine.substr(nStartPos, i - nStartPos)) != mCachesMap.end())
                return true;
        }

        // Is this a candidate for a table
        if (sCmdLine[i] == '{'
            && (isalnum(sCmdLine[i-1]) || sCmdLine[i-1] == '_' || sCmdLine[i-1] == '~' || sCmdLine[i-1] == '[' || sCmdLine[i-1] == ']'))
        {
            size_t nStartPos = i-1;

            // Try to find the starting position
            while (nStartPos > 0 && (isalnum(sCmdLine[nStartPos-1])
                                     || sCmdLine[nStartPos-1] == '_'
                                     || sCmdLine[nStartPos-1] == '~'
                                     || sCmdLine[nStartPos-1] == '['
                                     || sCmdLine[nStartPos-1] == ']'))
            {
                nStartPos--;
            }

            // Try to find the candidate in the internal map
            if (mClusterMap.find(sCmdLine.substr(nStartPos, i - nStartPos)) != mClusterMap.end())
                return true;
        }
    }

    return false;

    //return containsTables(sCmdLine) || containsClusters(sCmdLine);
}


/////////////////////////////////////////////////
/// \brief This member function returns, whether
/// the passed table name corresponds to a known
/// table.
///
/// \param sTable const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::isTable(const std::string& sTable) const
{
    if (mCachesMap.find(sTable.substr(0, sTable.find('('))) != mCachesMap.end())
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function returns, whether
/// the passed table name corresponds to a known
/// table.
///
/// \param sTable StringView
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::isTable(StringView sTable) const
{
    sTable = sTable.subview(0, sTable.find('('));

    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (sTable == iter->first)
            return true;
        else if (sTable < iter->first)
            return false;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function detects, whether
/// a table is used in the current expression.
///
/// \param sExpression const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::containsTables(const std::string& sExpression)
{
    if (sExpression.find('(') == std::string::npos)
        return false;

    // Check the first character directly
    size_t nQuotes = sExpression.front() == '"';

    // Search through the expression -> We do not need to examine the first character
    for (size_t i = 1; i < sExpression.length(); i++)
    {
        // Consider quotation marks
        if (sExpression[i] == '"' && sExpression[i-1] != '\\')
            nQuotes++;

        // Is this a candidate for a table
        if (!(nQuotes % 2)
            && sExpression[i] == '('
            && (isalnum(sExpression[i-1]) || sExpression[i-1] == '_' || sExpression[i-1] == '~'))
        {
            size_t nStartPos = i-1;

            // Try to find the starting position
            while (nStartPos > 0 && (isalnum(sExpression[nStartPos-1]) || sExpression[nStartPos-1] == '_' || sExpression[nStartPos-1] == '~'))
            {
                nStartPos--;
            }

            // Try to find the candidate in the internal map
            if (mCachesMap.find(sExpression.substr(nStartPos, i - nStartPos)) != mCachesMap.end())
                return true;
        }

        /*if (!(nQuotes % 2))
        {
            // If the current character might probably be an
            // identifier for a table, search for the next
            // nonmatching character and try to find the obtained
            // string in the internal map
            if (isalpha(sExpression[i]) || sExpression[i] == '_' || sExpression[i] == '~')
            {
                size_t nStartPos = i;

                do
                {
                    i++;
                }
                while (isalnum(sExpression[i]) || sExpression[i] == '_' || sExpression[i] == '~');

                if (sExpression[i] == '(')
                {
                    if (mCachesMap.find(sExpression.substr(nStartPos, i - nStartPos)) != mCachesMap.end())
                        return true;
                }
            }
        }*/
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function creates a new
/// table. It is checked, whether its name is
/// valid and not already used elsewhere.
///
/// \param sCache const string&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::addTable(const string& sCache, const Settings& _option)
{
    string sCacheName = sCache.substr(0,sCache.find('('));
    static const string sVALIDCHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_~";

    // Ensure that the name of the new table contains only
    // valid characters
    if ((sCacheName[0] >= '0' && sCacheName[0] <= '9') || sCacheName[0] == '~' || sCacheName == "data" || sCacheName == "string" || sCacheName.find_first_not_of(sVALIDCHARACTERS) != string::npos)
        throw SyntaxError(SyntaxError::INVALID_CACHE_NAME, "", SyntaxError::invalid_position, sCacheName);

    // Ensure that the new table does not match a
    // predefined function
    if (sPredefinedFuncs.find(","+sCacheName+"()") != string::npos)
        throw SyntaxError(SyntaxError::FUNCTION_IS_PREDEFINED, "", SyntaxError::invalid_position, sCacheName+"()");

    // Ensure that the new table does not match a
    // user-defined function
    if (sUserdefinedFuncs.length() && sUserdefinedFuncs.find(";"+sCacheName+";") != string::npos)
        throw SyntaxError(SyntaxError::FUNCTION_ALREADY_EXISTS, "", SyntaxError::invalid_position, sCacheName);

    // Ensure that the new table does not already
    // exist
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == sCacheName)
            throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, "", SyntaxError::invalid_position, sCacheName+"()");
    }

    // Warn, if the new table equals an existing command
    if (sPredefinedCommands.find(";"+sCacheName+";") != string::npos)
        NumeReKernel::print(LineBreak(_lang.get("CACHE_WARNING_CMD_OVERLAP", sCacheName), _option));

    // Warn, if the new table equals a plugin command
    if (sPluginCommands.length() && sPluginCommands.find(";"+sCacheName+";") != string::npos)
        NumeReKernel::print(LineBreak(_lang.get("CACHE_WARNING_PLUGIN_OVERLAP"), _option));

    // Actually create the new table
    mCachesMap[sCacheName] = std::make_pair(vMemory.size(), vMemory.size());
    vMemory.push_back(new Memory());

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function removes the
/// selected table.
///
/// \param sCache const string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::deleteTable(const string& sCache)
{
    if (sCache == "table")
        return false;

    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == sCache)
        {
            if (vMemory.size() > iter->second.first)
            {
                delete vMemory[iter->second.first];
                vMemory.erase(vMemory.begin() + iter->second.first);
            }
            else
                return false;

            for (auto iter2 = mCachesMap.begin(); iter2 != mCachesMap.end(); ++iter2)
            {
                if (iter2->second.first > iter->second.first)
                    iter2->second.first--;

                if (iter2->second.second > iter->second.first)
                    iter2->second.second--;
                else if (iter2->second.second == iter->second.first)
                    iter2->second.second = iter2->second.first;

                mCachesMap[iter2->first] = iter2->second;
            }

            mCachesMap.erase(iter);

            if (getSaveStatus() && MemoryManager::isValid())
                setSaveStatus(false);
            else if (!MemoryManager::isValid())
            {
                if (fileExists(getProgramPath()+"/numere.cache"))
                {
                    string sCachefile = getProgramPath() + "/numere.cache";
                    remove(sCachefile.c_str());
                }
            }

            return true;
        }
    }

    return false;
}


