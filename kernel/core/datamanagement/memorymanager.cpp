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
	mCachesMap["table"] = 0;
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
		mCachesMap["table"] = 0;
		vMemory.push_back(new Memory());
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

    return vMemory[mCachesMap.at(sCache)]->sortElements(0, getLines(sCache, false)-1, 0, getCols(sCache, false)-1, sSortingExpression);
}


/////////////////////////////////////////////////
/// \brief This member function informs the
/// selected table to sort its contents according
/// the passed parameter set.
///
/// \param sCache const string&
/// \param i1 long long int
/// \param i2 long long int
/// \param j1 long long int
/// \param j2 long long int
/// \param sSortingExpression const string&
/// \return vector<int>
///
/////////////////////////////////////////////////
vector<int> MemoryManager::sortElements(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
    return vMemory[mCachesMap.at(sCache)]->sortElements(i1, i2, j1, j2, sSortingExpression);
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

    long long int nLines;
    long long int nCols;

    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == "data")
            continue;

        nLines = vMemory[iter->second]->getLines(false);
        nCols = vMemory[iter->second]->getCols(false);

        cacheFile.setDimensions(nLines, nCols);
        cacheFile.setColumnHeadings(vMemory[iter->second]->sHeadLine, nCols);
        cacheFile.setData(vMemory[iter->second]->dMemTable, nLines, nCols);
        cacheFile.setTableName(iter->first);
        cacheFile.setComment("NO COMMENT");

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

            mCachesMap[cacheFile.getTableName()] = vMemory.size();
            vMemory.push_back(new Memory());

            vMemory.back()->resizeMemory(cacheFile.getRows(), cacheFile.getCols());
            cacheFile.getData(vMemory.back()->dMemTable);
            cacheFile.getColumnHeadings(vMemory.back()->sHeadLine);

            vMemory.back()->bValidData = true;
            vMemory.back()->countAppendedZeroes();
            vMemory.back()->shrink();
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

                for (unsigned int n = 0; n < nLength-1; n++)
                {
                    sTemp[n] = cCachesMap[n];
                }

                delete[] cCachesMap;
                cCachesMap = 0;
                mCachesMap[sTemp] = nLayerIndex;
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

                for (unsigned int k = 0; k < nLength-1; k++)
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
/// \brief This member function extracts and
/// parses the \c every expression part of a MAF
/// call.
///
/// \param sDir string&
/// \param sTableName const string&
/// \return VectorIndex
///
/////////////////////////////////////////////////
VectorIndex MemoryManager::parseEvery(string& sDir, const string& sTableName)
{
    if (sDir.find("every=") != string::npos && (sDir.find("cols") != string::npos || sDir.find("lines") != string::npos))
    {
        string sEvery = getArgAtPos(sDir, sDir.find("every=")+6);
        StripSpaces(sEvery);
        Parser& _parser = NumeReKernel::getInstance()->getParser();
        _parser.DisableAccessCaching();
        sDir.erase(sDir.find("every="));

        if (sEvery.front() == '(' && sEvery.back() == ')')
            sEvery = sEvery.substr(1, sEvery.length()-2);

        if (sEvery.front() == '{' && sEvery.back() == '}')
        {
            // Definition contains a vector expression
            _parser.SetExpr(sEvery);
            int nResults;
            value_type* v = _parser.Eval(nResults);

            return VectorIndex(v, nResults, 0);
        }
        else
        {
            // Usual expression
            _parser.SetExpr(sEvery);
            int nResults;
            value_type* v = _parser.Eval(nResults);

            if (nResults == 1)
            {
                // Single result: usual every=a,a representation
                vector<long long int> idx;

                for (long long int i = intCast(v[0])-1; i < (sDir.find("cols") != string::npos ? getCols(sTableName, false) : getLines(sTableName, false)); i += intCast(v[0]))
                {
                    idx.push_back(i);
                }

                return VectorIndex(idx);
            }
            else if (nResults == 2)
            {
                // Two results: usual every=a,b representation
                vector<long long int> idx;

                for (long long int i = intCast(v[0])-1; i < (sDir.find("cols") != string::npos ? getCols(sTableName, false) : getLines(sTableName, false)); i += intCast(v[1]))
                {
                    idx.push_back(i);
                }

                return VectorIndex(idx);
            }
            else //arbitrary results: use it as if it was a vector
                return VectorIndex(v, nResults, 0);
        }
    }

    return VectorIndex(0, (sDir.find("cols") != string::npos ? getCols(sTableName, false) : getLines(sTableName, false))-1);
}


/////////////////////////////////////////////////
/// \brief This member function is the abstract
/// implementation of a MAF function call. Most
/// of the MAFs use this abstraction (except
/// \c cmp and \c pct).
///
/// \param sTableName string& const
/// \param sDir string
/// \param MAF (double*)
/// \return vector<double>
///
/////////////////////////////////////////////////
vector<double> MemoryManager::resolveMAF(const string& sTableName, string sDir, double (MemoryManager::*MAF)(const string&, long long int, long long int, long long int, long long int))
{
    vector<double> vResults;

    // Find the "grid" parameter and use it as an offset
    long long int nGridOffset = sDir.find("grid") != string::npos ? 2 : 0;

    // Get the vector index corresponding to a possible
    // every definition
    VectorIndex _idx = parseEvery(sDir, sTableName);

    // Resolve the actual call to the MAF
    if (sDir.find("cols") != string::npos)
    {
        for (size_t i = 0; i < _idx.size(); i++)
        {
            if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset > getCols(sTableName, false))
                continue;

            vResults.push_back((this->*MAF)(sTableName, 0, getLines(sTableName, false), _idx[i]+nGridOffset, -1));
        }
    }
    else if (sDir.find("lines") != string::npos)
    {
        for (size_t i = 0; i < _idx.size(); i++)
        {
            if (_idx[i]+nGridOffset < 0 || _idx[i]+nGridOffset > getLines(sTableName, false))
                continue;

            vResults.push_back((this->*MAF)(sTableName, _idx[i]+nGridOffset, -1, 0, getCols(sTableName, false)));
        }
    }
    else
        vResults.push_back((this->*MAF)(sTableName, 0, getLines(sTableName, false), nGridOffset, getCols(sTableName, false)));

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

    return vMemory[mCachesMap.at(sTable.substr(0, sTable.find('(')))];
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
/// \return void
///
/////////////////////////////////////////////////
void MemoryManager::melt(Memory* _mem, const string& sTable)
{
    // Ensure that the table exists
    if (!_mem)
        return;

    // Is a corresponding table known?
    if (mCachesMap.find(sTable) == mCachesMap.end())
    {
        // Append the new table
        long long int nIndex = vMemory.size();
        mCachesMap[sTable] = nIndex;
        vMemory.push_back(_mem);
    }
    else
    {
        // Combine both tables
        Memory* _existingMem = vMemory[mCachesMap[sTable]];

        long long int nCols = _existingMem->getCols(false);

        // Resize the existing table to fit the contents
        // of both tables
        _existingMem->resizeMemory(std::max(_existingMem->getLines(false), _mem->getLines(false)), _existingMem->getCols(false) + _mem->getCols(false));

        // Copy the contents
        for (long long int i = 0; i < _mem->getLines(false); i++)
        {
            for (long long int j = 0; j < _mem->getCols(false); j++)
            {
                if (!i)
                    _existingMem->setHeadLineElement(j + nCols, _mem->getHeadLineElement(j));

                _existingMem->writeData(i, j + nCols, _mem->readMem(i, j));
            }
        }

        sDataFile = "Merged Data";
        _existingMem->setSaveStatus(false);

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
/// \param sTableName const string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::updateDimensionVariables(const string& sTableName)
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
    return containsTables(" " + sCmdLine + " ") || containsClusters(" " + sCmdLine + " ");
}


/////////////////////////////////////////////////
/// \brief This member function returns, whether
/// the passed table name corresponds to a known
/// table.
///
/// \param sTable const string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::isTable(const string& sTable) const
{
    if (mCachesMap.find(sTable.substr(0, sTable.find('('))) != mCachesMap.end())
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function detects, whether
/// a table is used in the current expression.
///
/// \param sExpression const string&
/// \return bool
///
/////////////////////////////////////////////////
bool MemoryManager::containsTables(const string& sExpression)
{
    size_t nQuotes = 0;

    if (sExpression.find('(') == string::npos)
        return false;

    // Search through the expression
    for (size_t i = 0; i < sExpression.length(); i++)
    {
        // Consider quotation marks
        if (sExpression[i] == '"' && (!i || sExpression[i-1] != '\\'))
            nQuotes++;

        if (!(nQuotes % 2))
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
    long long int nIndex = vMemory.size();
    mCachesMap[sCacheName] = nIndex;
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
            if (vMemory.size() > iter->second)
            {
                delete vMemory[iter->second];
                vMemory.erase(vMemory.begin() + iter->second);
            }
            else
                return false;

            for (auto iter2 = mCachesMap.begin(); iter2 != mCachesMap.end(); ++iter2)
            {
                if (iter2->second > iter->second)
                    mCachesMap[iter2->first] = iter2->second-1;
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


