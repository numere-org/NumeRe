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


#include "cache.hpp"
#include "../../kernel.hpp"
using namespace std;


/*
 * Realisierung der Cache-Klasse
 */

// --> Standard-Konstruktor <--
MemoryManager::MemoryManager() : FileSystem(), StringMemory(), NumeRe::ClusterManager()
{
	bSaveMutex = false;
	sCache_file = "<>/numere.cache";
	sPredefinedFuncs = ",abs(),acos(),acosh(),Ai(),arccos(),arcosh(),arcsin(),arsinh(),arctan(),artanh(),asin(),asinh(),ascii(),atan(),atanh(),avg(),bessel(),betheweizsaecker(),Bi(),binom(),cache(),char(),cmp(),cnt(),cos(),cosh(),cross(),data(),date(),dblfacul(),degree(),det(),diag(),diagonalize(),eigenvals(),eigenvects(),erf(),erfc(),exp(),faculty(),findfile(),findparam(),floor(),gamma(),gcd(),getfilelist(),getmatchingparens(),getopt(),heaviside(),hermite(),identity(),invert(),is_data(),is_nan(),is_string(),laguerre(),laguerre_a(),lcm(),legendre(),legendre_a(),ln(),log(),log10(),log2(),matfc(),matfcf(),matfl(),matflf(),max(),med(),min(),neumann(),norm(),num(),one(),pct(),phi(),prd(),radian(),rand(),range(),rect(),rint(),roof(),round(),sbessel(),sign(),sin(),sinc(),sinh(),sneumann(),solve(),split(),sqrt(),std(),strfnd(),strrfnd(),strlen(),student_t(),substr(),sum(),tan(),tanh(),theta(),time(),to_char(),to_cmd(),to_string(),to_value(),trace(),transpose(),valtostr(),Y(),zero()";
	sUserdefinedFuncs = "";
	sPredefinedCommands =  ";abort;about;audio;break;compose;cont;cont3d;continue;copy;credits;data;datagrid;define;delete;dens;dens3d;diff;draw;draw3d;edit;else;endcompose;endfor;endif;endprocedure;endwhile;eval;explicit;export;extrema;fft;find;fit;for;get;global;grad;grad3d;graph;graph3d;help;hist;hline;if;ifndef;ifndefined;info;integrate;list;load;matop;mesh;mesh3d;move;mtrxop;namespace;new;odesolve;plot;plot3d;procedure;pulse;quit;random;read;readline;regularize;remove;rename;replaceline;resample;return;save;script;set;smooth;sort;stats;stfa;str;surf;surf3d;swap;taylor;throw;undef;undefine;var;vect;vect3d;while;write;zeroes;";
	sPluginCommands = "";
	mCachesMap["cache"] = 0;
	vMemory.push_back(new Memory());
}

// --> Destruktor <--
MemoryManager::~MemoryManager()
{
    if (cache_file.is_open())
        cache_file.close();
    for (size_t i = 0; i < vMemory.size(); i++)
        delete vMemory[i];
}


// --> loescht den Inhalt des Datenfile-Objekts, ohne selbiges zu zerstoeren <--
void MemoryManager::removeDataInMemory()
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
		mCachesMap["cache"] = 0;
		vMemory.push_back(new Memory());
	}
	return;
}

// --> gibt den Wert von bValidData zurueck <--
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

// --> gibt den Wert von bIsSaved zurueck <--
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

void MemoryManager::setSaveStatus(bool _bIsSaved)
{
    for (size_t i = 0; i < vMemory.size(); i++)
    {
        vMemory[i]->setSaveStatus(_bIsSaved);
    }
}

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

vector<int> MemoryManager::sortElements(const string& sLine) // cache -sort[[=desc]] cols=1[2:3]4[5:9]10:
{
    if (!isValid())
        return vector<int>();

    string sCache;
    string sSortingExpression = "-set";

    if (findCommand(sLine).sString != "sort")
    {
        sCache = findCommand(sLine).sString;
    }
    if (matchParams(sLine, "sort", '='))
    {
        if (getArgAtPos(sLine, matchParams(sLine, "sort", '=')+4) == "desc")
            sSortingExpression += " desc";
    }
    else
    {
        for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
        {
            if (matchParams(sLine, iter->first, '='))
            {
                if (getArgAtPos(sLine, matchParams(sLine, iter->first, '=')+5) == "desc")
                    sSortingExpression += " desc";
                sCache = iter->first;
                break;
            }
            else if (matchParams(sLine, iter->first))
            {
                sCache = iter->first;
                break;
            }
        }
    }

    if (matchParams(sLine, "cols", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, matchParams(sLine, "cols", '=')+4);
    else if (matchParams(sLine, "c", '='))
        sSortingExpression += " cols=" + getArgAtPos(sLine, matchParams(sLine, "c", '=')+1);
    if (matchParams(sLine, "index"))
        sSortingExpression += " index";

    return vMemory[mCachesMap.at(sCache)]->sortElements(0, getTableLines(sCache, false)-1, 0, getTableCols(sCache, false)-1, sSortingExpression);
}

vector<int> MemoryManager::sortElements(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
    return vMemory[mCachesMap.at(sCache)]->sortElements(i1, i2, j1, j2, sSortingExpression);
}

void MemoryManager::setCacheFileName(string _sFileName)
{
    if (_sFileName.length())
    {
        sCache_file = FileSystem::ValidFileName(_sFileName, ".cache");
    }
    return;
}

bool MemoryManager::saveToCacheFile()
{
    if (bSaveMutex)
        return false;

    bSaveMutex = true;

    sCache_file = FileSystem::ValidFileName(sCache_file, ".cache");

    NumeRe::CacheFile cacheFile(sCache_file);

    cacheFile.setNumberOfTables(mCachesMap.size());
    cacheFile.writeCacheHeader();

    long long int nLines;
    long long int nCols;

    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
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

bool MemoryManager::loadFromCacheFile()
{
    if (bSaveMutex)
        return false;

    bSaveMutex = true;
    sCache_file = FileSystem::ValidFileName(sCache_file, ".cache");

    if (!loadFromNewCacheFile())
        return loadFromLegacyCacheFile();

    return true;
}

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

bool MemoryManager::isTable(const string& sCache)
{
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == sCache.substr(0,sCache.find('(')))
            return true;
    }
    return false;
}

// This member function detects, whether a table is used
// in the current expression
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

// This member function creates a new table
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
    {
        throw SyntaxError(SyntaxError::FUNCTION_IS_PREDEFINED, "", SyntaxError::invalid_position, sCacheName+"()");
    }

    // Ensure that the new table does not match a
    // user-defined function
    if (sUserdefinedFuncs.length() && sUserdefinedFuncs.find(";"+sCacheName+";") != string::npos)
    {
        throw SyntaxError(SyntaxError::FUNCTION_ALREADY_EXISTS, "", SyntaxError::invalid_position, sCacheName);
    }

    // Ensure that the new table does not already
    // exist
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == sCacheName)
        {
            throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, "", SyntaxError::invalid_position, sCacheName+"()");
        }
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

bool MemoryManager::deleteTable(const string& sCache)
{
    if (sCache == "cache")
        return false;
    //cerr << sCache << endl;
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
            //cerr << 4 << endl;
            if (getSaveStatus() && MemoryManager::isValid())
            {
                setSaveStatus(false);
            }
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


