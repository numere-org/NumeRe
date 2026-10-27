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
#include "../maths/resampler.h"
#include "../../kernel.hpp"
#include "tablecolumnimpl.hpp"
using namespace std;


/////////////////////////////////////////////////
/// \brief Simple structure for an one-
/// dimensional moving window.
/////////////////////////////////////////////////
struct SingleWindow
{
    size_t width;
    size_t step;
};


/////////////////////////////////////////////////
/// \brief Defines a set of two one-dimensional
/// moving windows: one for rows, one for cols.
/////////////////////////////////////////////////
struct WindowSet
{
    SingleWindow rowWin;
    SingleWindow colWin;

    WindowSet(const std::string& sWinDef)
    {
        rowWin.width = 0;
        rowWin.step = 1;
        colWin.width = 0;
        colWin.step = 1;

        if (sWinDef.find("window=(") != std::string::npos)
        {
            mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

            // Isolate the parenthesis
            StringView winDef(sWinDef, sWinDef.find("window=(") + 7);
            winDef.remove_from(getMatchingParenthesis(winDef));

            // Assign and evaluate
            _parser.SetExpr(winDef.subview(1));
            int nRes = 0;
            const mu::StackItem* a = _parser.Eval(nRes);

            if (nRes == 2)
            {
                rowWin.width = a[0].get()[0].getNum().asUI64();
                colWin.width = a[0].get()[1].getNum().asUI64();
                rowWin.step = std::max((uint64_t)1, a[1].get()[0].getNum().asUI64());
                colWin.step = std::max((uint64_t)1, a[1].get()[1].getNum().asUI64());
            }
        }
    }
};


/*
 * Realisierung der Cache-Klasse
 */


/////////////////////////////////////////////////
/// \brief Default MemoryManager constructor.
/// Creates the default table and initializes the
/// list of predefined commands.
/////////////////////////////////////////////////
MemoryManager::MemoryManager() : NumeRe::FileAdapter(), NumeRe::ClusterManager()
{
	bSaveMutex = false;
	sCache_file = "<>/numere.cache";
	sPredefinedFuncs = "";
	sUserdefinedFuncs = "";
	sPredefinedCommands =  ";abort;about;audio;break;compose;cont;cont3d;continue;copy;credits;data;datagrid;define;delete;dens;dens3d;diff;draw;draw3d;edit;else;endcompose;endfor;endif;endprocedure;endwhile;eval;explicit;export;extrema;fft;find;fit;for;get;global;grad;grad3d;graph;graph3d;help;hist;hline;if;ifndef;ifndefined;imread;implot;info;integrate;list;load;matop;mesh;mesh3d;move;mtrxop;namespace;new;odesolve;plot;plot3d;procedure;pulse;quit;random;read;readline;regularize;remove;rename;replaceline;resample;return;save;script;set;smooth;sort;stats;stfa;str;surf;surf3d;swap;taylor;throw;undef;undefine;var;vect;vect3d;while;write;zeroes;";
	sPluginCommands = "";
	m_tableReferences["table"] = TableReference{0, 0};
	m_tableReferences["string"] = TableReference{1ull, 1ull};
	m_tables.push_back(new Table());
	m_tables.push_back(new Table());

	tableColumnsCount = mu::Value(0.0);
	tableLinesCount = mu::Value(0.0);
}


/////////////////////////////////////////////////
/// \brief MemoryManager destructor. Clears all
/// created tables.
/////////////////////////////////////////////////
MemoryManager::~MemoryManager()
{
    if (cache_file.is_open())
        cache_file.close();

    for (size_t i = 0; i < m_tables.size(); i++)
        delete m_tables[i];
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
		for (size_t i = 0; i < m_tables.size(); i++)
            delete m_tables[i];

		m_tables.clear();
		bSaveMutex = false;
		m_tableReferences.clear();
		m_tableReferences["table"] = TableReference{0, 0};
		m_tableReferences["string"] = TableReference{1ull, 1ull};
		m_tables.push_back(new Table());
		m_tables.push_back(new Table());
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
    if (!m_tables.size())
        return false;

    for (size_t i = 0; i < m_tables.size(); i++)
    {
        if (m_tables[i]->getCols())
            return true;
    }

	return false;
}


/////////////////////////////////////////////////
/// \brief Converts an arbitrary mu::Array into a
/// VectorIndex instance.
///
/// \param arr const mu::Array&
/// \param sTable const std::string&
/// \param sheet size_t
/// \return VectorIndex
///
/////////////////////////////////////////////////
VectorIndex MemoryManager::arrayToIndex(const mu::Array& arr, const std::string& sTable, size_t sheet) const
{
    if (arr.getCommonType() == mu::TYPE_STRING)
        return VectorIndex(m_tables[findTable(sTable)]->findCols(arr.as_str_vector(), sheet, false, false));
    else if (arr.getCommonType() == mu::TYPE_NUMERICAL)
        return VectorIndex(arr);

    VectorIndex idx;

    for (size_t i = 0; i < arr.size(); i++)
    {
        if (arr.get(i).getType() == mu::TYPE_NUMERICAL)
            idx.push_back(arr.get(i).getNum().asI64()-1);
        else if (arr.get(i).getType() == mu::TYPE_STRING)
        {
            std::vector<size_t> cols = m_tables[findTable(sTable)]->findCols({arr.get(i).getStr()}, sheet, false, false);

            for (size_t c : cols)
            {
                idx.push_back(c-1);
            }
        }
    }

    if (!idx.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX, "INTERNAL INDEXING ERROR", SyntaxError::invalid_position, idx.to_string());

    return idx;
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
    if (!m_tables.size())
        return true;

    for (size_t i = 0; i < m_tables.size(); i++)
    {
        if (!m_tables[i]->getSaveStatus())
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
    for (size_t i = 0; i < m_tables.size(); i++)
    {
        m_tables[i]->setSaveStatus(_bIsSaved);
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

    if (!m_tables.size())
        return 0;

    nLastSaved = m_tables[0]->getLastSaved();

    for (size_t i = 1; i < m_tables.size(); i++)
    {
        if (m_tables[i]->getLastSaved() < nLastSaved)
            nLastSaved = m_tables[i]->getLastSaved();
    }

    return nLastSaved;
}


/////////////////////////////////////////////////
/// \brief This member function wraps the sorting
/// functionality and evaluates the passed
/// parameter string before delegating to the
/// actual implementation.
///
/// \param sLine const std::string&
/// \return std::vector<int>
///
/////////////////////////////////////////////////
std::vector<int> MemoryManager::sortElements(const std::string& sLine)
{
    if (!isValid())
        return std::vector<int>();

    std::string sCache;
    std::string sSortingExpression = "-set";

    if (findCommand(sLine).sString != "sort")
        sCache = findCommand(sLine).sString;

    if (findParameter(sLine, "sort", '='))
    {
        if (getArgAtPos(sLine, findParameter(sLine, "sort", '=')+4) == "desc")
            sSortingExpression += " desc";
    }
    else
    {
        for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
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

    Table& table = getTable(sCache);

    return table.sortElements(VectorIndex(0, table.getRows()-1),
                              VectorIndex(0, table.getCols()-1),
                              VectorIndex(0, table.getSheets()-1),
                              sSortingExpression);
}


/////////////////////////////////////////////////
/// \brief This member function informs the
/// selected table to sort its contents according
/// the passed parameter set.
///
/// \param sCache const std::string&
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \param sSortingExpression const std::string&
/// \return std::vector<int>
///
/////////////////////////////////////////////////
std::vector<int> MemoryManager::sortElements(const std::string& sCache, const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet, const std::string& sSortingExpression)
{
    return m_tables[findTable(sCache)]->sortElements(_vRow, _vCol, _vSheet, sSortingExpression);
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
    std::string sTempFile = sCache_file + ".temp";
    bool success = true;

    // Write the data to a temporary file first
    if (sCache_file.length())
    {
        NumeRe::CacheFile cacheFile(sTempFile);

        cacheFile.setNumberOfTables(m_tableReferences.size() - isTable("data"));
        cacheFile.writeCacheHeader();

        int64_t nRows;
        int64_t nCols;
        int64_t nLayers;

        for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
        {
            if (iter->first == "data")
                continue;

            nRows = m_tables[iter->second.owned]->getRows();
            nCols = m_tables[iter->second.owned]->getCols();
            nLayers = m_tables[iter->second.owned]->getSheets();

            cacheFile.setDimensions(nRows, nCols, nLayers);
            cacheFile.setData(&m_tables[iter->second.owned]->m_data, nRows, nCols, nLayers);
            cacheFile.setTableName(iter->first);
            cacheFile.setComment(m_tables[iter->second.owned]->m_meta.comment);

            success = success && cacheFile.write();
        }
    }

    // If the file has been written successfully, move it to the
    // actual file name
    if (success)
        moveFile(sTempFile, sCache_file);

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

        for (size_t i = 0; i < m_tables.size(); i++)
            delete m_tables[i];

        m_tables.clear();
        m_tableReferences.clear();

        for (size_t i = 0; i < nCaches; i++)
        {
            cacheFile.read();

            m_tableReferences[cacheFile.getTableName()] = TableReference{m_tables.size(), m_tables.size()};
            m_tables.push_back(new Table());

            m_tables.back()->reserve(cacheFile.getCols(), cacheFile.getLayers());
            cacheFile.getData(&m_tables.back()->m_data);

            if (cacheFile.getComment() != "NO COMMENT")
                m_tables.back()->m_meta.comment = cacheFile.getComment();
        }

        if (m_tableReferences.find("table") == m_tableReferences.end())
        {
            m_tableReferences["table"] = TableReference{m_tables.size(), m_tables.size()};
            m_tables.push_back(new Table());
        }

        if (m_tableReferences.find("string") == m_tableReferences.end())
        {
            m_tableReferences["string"] = TableReference{m_tables.size(), m_tables.size()};
            m_tables.push_back(new Table());
        }

        bSaveMutex = false;
        return true;

    }
    catch (SyntaxError& e)
    {
        cacheFile.close();
        g_logger.error("Could not load tables from the cache file. Catched error code: " + toString((size_t)e.errorcode));
    }
    catch (...)
    {
        cacheFile.close();
        g_logger.error("Could not load tables from the cache file.");
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

            for (size_t i = 0; i < m_tables.size(); i++)
                delete m_tables[i];

            m_tables.clear();
            m_tableReferences.clear();

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
                m_tableReferences[sTemp] = TableReference{nLayerIndex, nLayerIndex};
                m_tables.push_back(new Table());
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

                if (i < (int)cachemapssize)
                    m_tables[i]->setHeadLineElement(j, 0, sHead);
            }
        }

        cache_file.seekg(sizeof(long long int)*nLayers*nCols, ios_base::cur);

        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                cache_file.read((char*)dCache, sizeof(double)*nLayers);

                for (long long int k = 0; k < (long long int)cachemapssize; k++)
                    m_tables[k]->write(i, j, 0, dCache[k]);
            }
        }

        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                cache_file.read((char*)bValidData, sizeof(bool)*nLayers);

                for (long long int k = 0; k < (long long int)cachemapssize; k++)
                {
                    if (!bValidData[k])
                        m_tables[k]->write(i, j, 0, NAN);
                }
            }
        }

        for (size_t i = 0; i < m_tables.size(); i++)
        {
            m_tables[i]->shrink();
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
/// \param sheets const VectorIndex&
/// \param _sTable const std::string&
/// \return int
///
/////////////////////////////////////////////////
int MemoryManager::getColElements(const VectorIndex& cols, const VectorIndex& sheets, const std::string& _sTable) const
{
    if (!exists(_sTable))
        return 0;

    Table* _mem = m_tables[findTable(_sTable)];

    int nElems = 0;

    if (cols.isOpenEnd())
        cols.setOpenEndIndex(_mem->getCols()-1);

    if (sheets.isOpenEnd())
        sheets.setOpenEndIndex(_mem->getSheets()-1);

    for (size_t k = 0; k < sheets.size(); k++)
    {
        int s = sheets[k];

        if (s < 0 || s >= _mem->getSheets())
            continue;

        for (size_t i = 0; i < cols.size(); i++)
        {
            nElems = std::max(nElems, _mem->getElemsInColumn(cols[i], s));
        }
    }

    return nElems;
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

#warning TODO (numere#1#10/25/26): Fix the sheets in this function
    size_t sheet = 0;

    const Table& table = getTable(sTableName);

    if (sType == "every")
        iterationDimension = sDir.find("cols") != std::string::npos ? table.getCols() : table.getRows();
    else
        iterationDimension = sDir.find("cols") != std::string::npos ? table.getRows() : table.getCols();

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
            const mu::StackItem* v = _parser.Eval(nResults);
            return arrayToIndex(v[0].get(), sTableName, sheet);
        }
        else
        {
            // Usual expression
            _parser.SetExpr(sEveryCell);
            int nResults;
            const mu::StackItem* v = _parser.Eval(nResults);

            if (nResults == 1 && v[0].get().size() == 1)
            {
                // Single result: usual every=a,a representation
                std::vector<int> idx;

                for (int i = v[0].get().getAsScalarInt()-1; i < iterationDimension; i += v[0].get().getAsScalarInt())
                {
                    idx.push_back(i);
                }

                return VectorIndex(idx);
            }
            else if (nResults == 2)
            {
                // Two results: usual every=a,b representation
                std::vector<int> idx;

                for (int i = v[0].get().getAsScalarInt()-1; i < iterationDimension; i += v[1].get().getAsScalarInt())
                {
                    idx.push_back(i);
                }

                return VectorIndex(idx);
            }
            else //arbitrary results: use it as if it was a vector
                return arrayToIndex(v[0].get(), sTableName, sheet);
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
std::vector<std::complex<double>> MemoryManager::resolveMAF(const std::string& sTableName,
                                                            std::string sDir,
                                                            std::complex<double> (MemoryManager::*MAF)(const std::string&,
                                                                                                       const VectorIndex&,
                                                                                                       const VectorIndex&,
                                                                                                       const VectorIndex&) const) const
{
    std::vector<std::complex<double>> vResults;
    const Table& table = getTable(sTableName);
    int nlines = table.getRows();
    int ncols = table.getCols();
    int nsheets = table.getSheets();

    // Find the "grid" parameter and use it as an offset
    int nGridOffset = sDir.find("grid") != std::string::npos ? 2 : 0;

    // Get the vector index corresponding to a possible
    // every or cell definition
    VectorIndex _everyIdx = parseEveryCell(sDir, "every", sTableName);
    VectorIndex _cellsIdx = parseEveryCell(sDir, "cells", sTableName);
    VectorIndex _sheetIdx(0, nsheets-1);

    // If a grid is required, get the grid dimensions
    // of this table
    if (nGridOffset)
    {
        std::vector<std::complex<double>> vSize = table.size(VectorIndex(), VectorIndex(), _sheetIdx.front(), GRID);
        nlines = vSize.front().real();
        ncols = vSize.back().real()+nGridOffset; // compensate the offset
    }

    // Get the moving window definition
    WindowSet win(sDir);

    // Validate the window set
    win.colWin.width = std::min(win.colWin.width, (size_t)ncols);
    win.rowWin.width = std::min(win.rowWin.width, (size_t)nlines);

    // Resolve the actual call to the MAF
    if (sDir.find("cols") != std::string::npos)
    {
        for (size_t i = 0; i < _everyIdx.size(); i++)
        {
            if (_everyIdx[i]+nGridOffset < 0 || _everyIdx[i]+nGridOffset >= ncols)
                continue;

            // Handle a possible moving window
            if (win.rowWin.width)
            {
                size_t nCurStep = 0;

                // As long as the window fits the indices
                while (nCurStep+win.rowWin.width <= _cellsIdx.size())
                {
                    vResults.push_back((this->*MAF)(sTableName,
                                                    _cellsIdx.subidx(nCurStep, win.rowWin.width),
                                                    VectorIndex(_everyIdx[i]+nGridOffset),
                                                    _sheetIdx));
                    nCurStep += win.rowWin.step;
                }
            }
            else
                vResults.push_back((this->*MAF)(sTableName,
                                                _cellsIdx,
                                                VectorIndex(_everyIdx[i]+nGridOffset),
                                                _sheetIdx));
        }
    }
    else if (sDir.find("lines") != std::string::npos)
    {
        _cellsIdx.apply_offset(nGridOffset);

        for (size_t i = 0; i < _everyIdx.size(); i++)
        {
            if (_everyIdx[i] < 0 || _everyIdx[i] >= nlines)
                continue;

            // Handle a possible moving window
            if (win.colWin.width)
            {
                size_t nCurStep = 0;

                // As long as the window fits the indices
                while (nCurStep+win.colWin.width <= _cellsIdx.size())
                {
                    vResults.push_back((this->*MAF)(sTableName,
                                                    VectorIndex(_everyIdx[i]),
                                                    _cellsIdx.subidx(nCurStep, win.colWin.width),
                                                    _sheetIdx));
                    nCurStep += win.colWin.step;
                }
            }
            else
                vResults.push_back((this->*MAF)(sTableName,
                                                VectorIndex(_everyIdx[i]),
                                                _cellsIdx,
                                                _sheetIdx));
        }
    }
    else
        vResults.push_back((this->*MAF)(sTableName,
                                        VectorIndex(0, VectorIndex::OPEN_END),
                                        VectorIndex(nGridOffset, VectorIndex::OPEN_END),
                                        _sheetIdx));

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
    if (m_tables.size() && m_tableReferences.find("data") != m_tableReferences.end())
    {
        deleteTable("data");
        sDataFile = "";
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns a
/// reference to an existing Memory instance
/// representing the selected table. Throws, if
/// the table does not exist.
///
/// \param sTable const string&
/// \return Memory&
///
/////////////////////////////////////////////////
Table& MemoryManager::getTable(const string& sTable)
{
    return *m_tables[findTable(sTable.substr(0, sTable.find('(')))];
}


/////////////////////////////////////////////////
/// \brief This member function returns a
/// reference to an existing Memory instance
/// representing the selected table. Throws, if
/// the table does not exist.
///
/// \param sTable StringView
/// \return Memory&
///
/////////////////////////////////////////////////
Table& MemoryManager::getTable(StringView sTable)
{
    return *m_tables[mapStringViewFind(sTable.subview(0, sTable.find('(')))];
}


/////////////////////////////////////////////////
/// \brief This member function returns a const
/// reference to an existing Memory instance
/// representing the selected table. Throws, if
/// the table does not exist.
///
/// \param sTable const string&
/// \return const Memory&
///
/////////////////////////////////////////////////
const Table& MemoryManager::getTable(const string& sTable) const
{
    return *m_tables[findTable(sTable.substr(0, sTable.find('(')))];
}


/////////////////////////////////////////////////
/// \brief This member function returns a const
/// reference to an existing Memory instance
/// representing the selected table. Throws, if
/// the table does not exist.
///
/// \param sTable StringView
/// \return const Memory&
///
/////////////////////////////////////////////////
const Table& MemoryManager::getTable(StringView sTable) const
{
    return *m_tables[mapStringViewFind(sTable.subview(0, sTable.find('(')))];
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
    Table* sourceTable = m_tables[findTable(source.substr(0, source.find('(')))];

    // Create the target table, if it does not exist
    if (!exists(target.substr(0, target.find('('))))
        addTable(target, NumeReKernel::getInstance()->getSettings());

    Table* targetTable = m_tables[findTable(target.substr(0, target.find('(')))];

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
    Table& sourceTable = getTable(source);
    Table& targetTable = getTable(target);

    if (sourceIdx.row.isFullRange(sourceTable.getRows()-1) && sourceIdx.col.isFullRange(sourceTable.getCols()-1)
        && targetIdx.row.isFullRange(targetTable.getRows()-1) && targetIdx.col.isFullRange(targetTable.getCols()-1))
    {
        copyTable(source, target);
        return;
    }

    // Get the extract as a table
    NumeRe::TableView extract = sourceTable.extractView(source, sourceIdx.row, sourceIdx.col, sourceIdx.layer);

    // Create the target table, if it does not exist
    if (!exists(target.substr(0, target.find('('))))
        addTable(target, NumeReKernel::getInstance()->getSettings());

    // Insert the copied table at the new location
    targetTable.insertCopiedView(extract, targetIdx.row, targetIdx.col, targetIdx.layer, false);
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
void MemoryManager::melt(Table* _mem, const string& sTable, bool overrideTarget)
{
    // Ensure that the table exists
    if (!_mem || _mem->m_data.empty())
        return;

    // Is a corresponding table known?
    if (m_tableReferences.find(sTable) == m_tableReferences.end())
    {
        // Append the new table
        m_tableReferences[sTable] = TableReference{m_tables.size(), m_tables.size()};
        m_tables.push_back(_mem);
    }
    else if (overrideTarget)
    {
        // Shall the original table be overwritten?
        delete m_tables[m_tableReferences[sTable].referenced];
        m_tables[m_tableReferences[sTable].referenced] = _mem;
    }
    else
    {
        // Combine both tables
        Table* _existingMem = m_tables[m_tableReferences[sTable].referenced];

        size_t sheets = _existingMem->m_data.sheets();

        // Resize the existing table to fit the contents
        // of both tables
        _existingMem->m_data.reserve(sheets + _mem->m_data.sheets());

        // Move the contents
        for (size_t k = 0; k < _mem->m_data.sheets(); k++)
        {
            if (_mem->m_data.m_sheets[k])
                _existingMem->m_data.m_sheets[k+sheets].reset(_mem->m_data.m_sheets[k].release());
        }

        _existingMem->setSaveStatus(false);
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
    // Update the dimensions for the selected
    // table
    tableLinesCount = mu::Value(getTable(sTableName).getRows());
    tableColumnsCount = mu::Value(getTable(sTableName).getCols());

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
    size_t nVar2StrParserEnd = 0;

    // Search through the expression -> We do not need to examine the first character
    for (size_t i = 1; i < sCmdLine.length(); i++)
    {
        // Consider quotation marks
        if (isQuotationMark(sCmdLine, i))
            nQuotes++;

        if (nQuotes % 2)
            continue;

        // Jump over the var2str parser operator
        if (sCmdLine[i-1] == '#')
        {
            while (sCmdLine[i] == '~')
            {
                i++;
                nVar2StrParserEnd = i;
            }
        }

        // Is this a candidate for a table
        if (sCmdLine[i] == '('
            && (isalnum(sCmdLine[i-1])
                || sCmdLine[i-1] == '_'
                || sCmdLine[i-1] == '~'))
        {
            size_t nStartPos = i-1;

            // Try to find the starting position
            while (nStartPos > nVar2StrParserEnd && (isalnum(sCmdLine[nStartPos-1])
                                                     || sCmdLine[nStartPos-1] == '_'
                                                     || sCmdLine[nStartPos-1] == '~'))
            {
                nStartPos--;
            }

            // Try to find the candidate in the internal map
            if (m_tableReferences.find(sCmdLine.substr(nStartPos, i - nStartPos)) != m_tableReferences.end())
                return true;
        }

        // Is this a candidate for a table
        if (sCmdLine[i] == '{'
            && (isalnum(sCmdLine[i-1])
                || sCmdLine[i-1] == '_'
                || sCmdLine[i-1] == '~'
                || sCmdLine[i-1] == '['
                || sCmdLine[i-1] == ']'))
        {
            size_t nStartPos = i-1;

            // Try to find the starting position
            while (nStartPos > nVar2StrParserEnd && (isalnum(sCmdLine[nStartPos-1])
                                                     || sCmdLine[nStartPos-1] == '_'
                                                     || sCmdLine[nStartPos-1] == '~'
                                                     || sCmdLine[nStartPos-1] == '['
                                                     || sCmdLine[nStartPos-1] == ']'))
            {
                nStartPos--;
            }

            // Try to find the candidate in the internal map
            if (isCluster(sCmdLine.substr(nStartPos, i - nStartPos)))
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
/// \param only bool
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::isTable(const std::string& sTable, bool only) const
{
    if (only && (sTable.find('(') == std::string::npos
                 || getMatchingParenthesis(sTable) != sTable.length() - 1))
        return false;

    if (m_tableReferences.find(sTable.substr(0, sTable.find('('))) != m_tableReferences.end())
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function returns, whether
/// the passed table name corresponds to a known
/// table.
///
/// \param sTable StringView
/// \param only bool
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::isTable(StringView sTable, bool only) const
{
    if (only && (sTable.find('(') == std::string::npos
                 || getMatchingParenthesis(sTable) != sTable.length() - 1))
        return false;

    sTable = sTable.subview(0, sTable.find('('));
    sTable.strip();

    for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
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
    size_t nVar2StrParserEnd = 0;

    // Search through the expression -> We do not need to examine the first character
    for (size_t i = 1; i < sExpression.length(); i++)
    {
        // Consider quotation marks
        if (isQuotationMark(sExpression, i))
            nQuotes++;

        if (nQuotes % 2)
            continue;

        // Jump over the var2str parser operator
        if (sExpression[i-1] == '#')
        {
            while (sExpression[i] == '~')
            {
                i++;
                nVar2StrParserEnd = i;
            }
        }

        // Is this a candidate for a table
        if (sExpression[i] == '('
            && (isalnum(sExpression[i-1])
                || sExpression[i-1] == '_'
                || sExpression[i-1] == '~'))
        {
            size_t nStartPos = i-1;

            // Try to find the starting position
            while (nStartPos > nVar2StrParserEnd
                   && (isalnum(sExpression[nStartPos-1])
                       || sExpression[nStartPos-1] == '_'
                       || sExpression[nStartPos-1] == '~'))
            {
                nStartPos--;
            }

            // Try to find the candidate in the internal map
            if (m_tableReferences.find(sExpression.substr(nStartPos, i - nStartPos)) != m_tableReferences.end())
                return true;
        }
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
    for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
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
    m_tableReferences[sCacheName] = TableReference{m_tables.size(), m_tables.size()};
    m_tables.push_back(new Table());

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function removes the
/// selected table.
///
/// \param sTable const string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::deleteTable(const string& sTable)
{
    if (sTable == "table" || sTable == "string")
        return false;

    for (auto iter = m_tableReferences.begin(); iter != m_tableReferences.end(); ++iter)
    {
        if (iter->first == sTable)
        {
            if (m_tables.size() > iter->second.owned)
            {
                delete m_tables[iter->second.owned];
                m_tables.erase(m_tables.begin() + iter->second.owned);
            }
            else
                return false;

            for (auto iter2 = m_tableReferences.begin(); iter2 != m_tableReferences.end(); ++iter2)
            {
                if (iter2->second.owned > iter->second.owned)
                    iter2->second.owned--;

                if (iter2->second.referenced > iter->second.owned)
                    iter2->second.referenced--;
                else if (iter2->second.referenced == iter->second.owned)
                    iter2->second.referenced = iter2->second.owned;

                m_tableReferences[iter2->first] = iter2->second;
            }

            m_tableReferences.erase(iter);

            if (getSaveStatus() && MemoryManager::isValid())
                setSaveStatus(false);
            else if (!MemoryManager::isValid())
            {
                if (fileExists(getProgramPath()+"/numere.cache"))
                {
                    string sCachefile = getProgramPath() + "/numere.cache";
                    boost::nowide::remove(sCachefile.c_str());
                }
            }

            return true;
        }
    }

    return false;
}


