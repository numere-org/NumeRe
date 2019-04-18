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
Cache::Cache() : FileSystem(), StringMemory(), NumeRe::ClusterManager()
{
	bSaveMutex = false;
	sCache_file = "<>/numere.cache";
	sPredefinedFuncs = ",abs(),acos(),acosh(),Ai(),arccos(),arcosh(),arcsin(),arsinh(),arctan(),artanh(),asin(),asinh(),ascii(),atan(),atanh(),avg(),bessel(),betheweizsaecker(),Bi(),binom(),cache(),char(),cmp(),cnt(),cos(),cosh(),cross(),data(),date(),dblfacul(),degree(),det(),diag(),diagonalize(),eigenvals(),eigenvects(),erf(),erfc(),exp(),faculty(),findfile(),findparam(),floor(),gamma(),gcd(),getfilelist(),getmatchingparens(),getopt(),heaviside(),hermite(),identity(),invert(),is_data(),is_nan(),is_string(),laguerre(),laguerre_a(),lcm(),legendre(),legendre_a(),ln(),log(),log10(),log2(),matfc(),matfcf(),matfl(),matflf(),max(),med(),min(),neumann(),norm(),num(),one(),pct(),phi(),prd(),radian(),rand(),range(),rect(),rint(),roof(),round(),sbessel(),sign(),sin(),sinc(),sinh(),sneumann(),solve(),split(),sqrt(),std(),strfnd(),strrfnd(),strlen(),student_t(),substr(),sum(),tan(),tanh(),theta(),time(),to_char(),to_cmd(),to_string(),to_value(),trace(),transpose(),valtostr(),Y(),zero()";
	sUserdefinedFuncs = "";
	sPredefinedCommands =  ";abort;about;audio;break;compose;cont;cont3d;continue;copy;credits;data;datagrid;define;delete;dens;dens3d;diff;draw;draw3d;edit;else;endcompose;endfor;endif;endprocedure;endwhile;eval;explicit;export;extrema;fft;find;fit;for;get;global;grad;grad3d;graph;graph3d;help;hist;hline;if;ifndef;ifndefined;info;integrate;list;load;matop;mesh;mesh3d;move;mtrxop;namespace;new;odesolve;plot;plot3d;procedure;pulse;quit;random;read;readline;regularize;remove;rename;replaceline;resample;return;save;script;set;smooth;sort;stats;stfa;str;surf;surf3d;swap;taylor;throw;undef;undefine;var;vect;vect3d;while;write;zeroes;";
	sPluginCommands = "";
	mCachesMap["cache"] = 0;
	vCacheMemory.push_back(new Memory());
}

// --> Destruktor <--
Cache::~Cache()
{
    if (cache_file.is_open())
        cache_file.close();
    for (size_t i = 0; i < vCacheMemory.size(); i++)
        delete vCacheMemory[i];
}


// --> loescht den Inhalt des Datenfile-Objekts, ohne selbiges zu zerstoeren <--
void Cache::removeCachedData()
{
	if (isValid())	// Sind ueberhaupt Daten vorhanden?
	{
        if (bSaveMutex)
            return;
        bSaveMutex = true;
		// --> Speicher, wo denn noetig freigeben <--
		for (size_t i = 0; i < vCacheMemory.size(); i++)
            delete vCacheMemory[i];
		vCacheMemory.clear();
		bSaveMutex = false;
		mCachesMap.clear();
		mCachesMap["cache"] = 0;
		vCacheMemory.push_back(new Memory());
	}
	return;
}

// --> gibt den Wert von bValidData zurueck <--
bool Cache::isValid() const
{
    if (!vCacheMemory.size())
        return false;

    for (size_t i = 0; i < vCacheMemory.size(); i++)
    {
        if (vCacheMemory[i]->getCols(false))
            return true;
    }
	return false;
}

// --> gibt den Wert von bIsSaved zurueck <--
bool Cache::getSaveStatus() const
{
    if (!vCacheMemory.size())
        return true;
    for (size_t i = 0; i < vCacheMemory.size(); i++)
    {
        if (!vCacheMemory[i]->getSaveStatus())
            return false;
    }
    return true;
}

void Cache::setSaveStatus(bool _bIsSaved)
{
    for (size_t i = 0; i < vCacheMemory.size(); i++)
    {
        vCacheMemory[i]->setSaveStatus(_bIsSaved);
    }
}

long long int Cache::getLastSaved() const
{
    long long int nLastSaved = 0;
    if (!vCacheMemory.size())
        return 0;
    nLastSaved = vCacheMemory[0]->getLastSaved();
    for (size_t i = 1; i < vCacheMemory.size(); i++)
    {
        if (vCacheMemory[i]->getLastSaved() < nLastSaved)
            nLastSaved = vCacheMemory[i]->getLastSaved();
    }
    return nLastSaved;
}

vector<int> Cache::sortElements(const string& sLine) // cache -sort[[=desc]] cols=1[2:3]4[5:9]10:
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

    return vCacheMemory[mCachesMap.at(sCache)]->sortElements(0, getCacheLines(sCache, false)-1, 0, getCacheCols(sCache, false)-1, sSortingExpression);
}

vector<int> Cache::sortElements(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2, const string& sSortingExpression)
{
    return vCacheMemory[mCachesMap.at(sCache)]->sortElements(i1, i2, j1, j2, sSortingExpression);
}

void Cache::setCacheFileName(string _sFileName)
{
    if (_sFileName.length())
    {
        sCache_file = FileSystem::ValidFileName(_sFileName, ".cache");
    }
    return;
}

bool Cache::saveCache()
{
    if (bSaveMutex)
        return false;
    bSaveMutex = true;
    long long int nSavingLayers = mCachesMap.size();
    long long int nSavingCols = 8;
    long long int nSavingLines = 128;
    long long int nColMax = 0;
    long long int nLineMax = 0;
    bool* bValidElement = 0;
    double* dCache = 0;
    if (!vCacheMemory.size())
    {
        bSaveMutex = false;
        return false;
    }
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (vCacheMemory[iter->second]->getCols(false) > nColMax)
            nColMax = vCacheMemory[iter->second]->getCols(false);
        if (vCacheMemory[iter->second]->getLines(false) > nLineMax)
            nLineMax = vCacheMemory[iter->second]->getLines(false);
    }

    while (nSavingCols < nColMax)
        nSavingCols *= 2;
    while (nSavingLines < nLineMax)
        nSavingLines *= 2;
    sCache_file = FileSystem::ValidFileName(sCache_file, ".cache");

    char*** cHeadLine = new char**[nSavingLayers];
    bValidElement = new bool[nSavingLayers];
    dCache = new double[nSavingLayers];

    char** cCachesList = new char*[mCachesMap.size()];
    long long int** nAppZeroesTemp = new long long int*[nSavingLayers];
    for (long long int i = 0; i < nSavingLayers; i++)
    {
        cHeadLine[i] = new char*[nSavingCols];
        nAppZeroesTemp[i] = new long long int[nSavingCols];
        for (long long int j = 0; j < nSavingCols; j++)
        {
            nAppZeroesTemp[i][j] = vCacheMemory[i]->getAppendedZeroes(j) - (vCacheMemory[i]->getLines(true) - nSavingLines);
            cHeadLine[i][j] = new char[vCacheMemory[i]->getHeadLineElement(j).length()+1];
            for (unsigned int k = 0; k < vCacheMemory[i]->getHeadLineElement(j).length(); k++)
            {
                cHeadLine[i][j][k] = vCacheMemory[i]->getHeadLineElement(j)[k];
            }
            cHeadLine[i][j][vCacheMemory[i]->getHeadLineElement(j).length()] = '\0';
        }
    }
    //cerr << 1.1 << endl;
    int n = 0;
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        cCachesList[n] = new char[(iter->first).length()+1];
        for (unsigned int k = 0; k < (iter->first).length(); k++)
            cCachesList[n][k] = (iter->first)[k];
        cCachesList[n][(iter->first).length()] = '\0';
        n++;
    }
    //cerr << 1.2 << endl;
    long int nMajor = AutoVersion::MAJOR;
    long int nMinor = AutoVersion::MINOR;
    long int nBuild = AutoVersion::BUILD;
    if (cache_file.is_open())
        cache_file.close();
    cache_file.open(sCache_file.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
    //cerr << 1.3 << endl;
    if (vCacheMemory.size() && cache_file.good())
    {
        setSaveStatus(true);
        long long int nDimTemp = -nSavingLines;
        time_t tTime = time(0);
        cache_file.write((char*)&nMajor, sizeof(long));
        cache_file.write((char*)&nMinor, sizeof(long));
        cache_file.write((char*)&nBuild, sizeof(long));
        cache_file.write((char*)&tTime, sizeof(time_t));
        cache_file.write((char*)&nDimTemp, sizeof(long long int));
        nDimTemp = -nSavingCols;
        cache_file.write((char*)&nDimTemp, sizeof(long long int));
        nDimTemp = -nSavingLayers;
        cache_file.write((char*)&nDimTemp, sizeof(long long int));
        size_t cachemapsize = mCachesMap.size();
        cache_file.write((char*)&cachemapsize, sizeof(size_t));
        n = 0;
        //cerr << 2 << endl;
        for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
        {
            nDimTemp = iter->second;
            size_t nLength = (iter->first).length()+1;
            cache_file.write((char*)&nLength, sizeof(size_t));
            cache_file.write(cCachesList[n], sizeof(char)*(nLength));
            cache_file.write((char*)&nDimTemp, sizeof(long long int));
            n++;
        }
        //cerr << 3 << endl;
        for (long long int i = 0; i < nSavingLayers; i++)
        {
            for (long long int j = 0; j < nSavingCols; j++)
            {
                size_t nlength = vCacheMemory[i]->getHeadLineElement(j).length()+1;
                //cerr << nlength << endl;
                cache_file.write((char*)&nlength, sizeof(size_t));
                cache_file.write(cHeadLine[i][j], sizeof(char)*(vCacheMemory[i]->getHeadLineElement(j).length()+1));
            }
        }
        //cerr << 4 << endl;
        for (long long int i = 0; i < nSavingLayers; i++)
            cache_file.write((char*)nAppZeroesTemp[i], sizeof(long long int)*nSavingCols);
        for (long long int i = 0; i < nSavingLines; i++)
        {
            for (long long int j = 0; j < nSavingCols; j++)
            {
                for (long long int k = 0; k < nSavingLayers; k++)
                    dCache[k] = vCacheMemory[k]->readMem(i, j);
                cache_file.write((char*)dCache, sizeof(double)*nSavingLayers);
            }
        }
        //cerr << 5 << endl;
        for (long long int i = 0; i < nSavingLines; i++)
        {
            for (long long int j = 0; j < nSavingCols; j++)
            {
                for (long long int k = 0; k < nSavingLayers; k++)
                    bValidElement[k] = !isnan(vCacheMemory[k]->readMem(i, j));
                cache_file.write((char*)bValidElement, sizeof(bool)*nSavingLayers);
            }
        }
        //cerr << 6 << endl;
        cache_file.close();
    }
    else
    {
        for (long long int i = 0; i < nSavingLayers; i++)
        {
            for (long long int j = 0; j < nSavingCols; j++)
            {
                delete[] cHeadLine[i][j];
            }
            delete[] cHeadLine[i];
            delete[] nAppZeroesTemp[i];
        }
        if (bValidElement)
            delete[] bValidElement;
        if (dCache)
            delete[] dCache;
        delete[] cHeadLine;
        delete[] nAppZeroesTemp;
        for (unsigned int i = 0; i < mCachesMap.size(); i++)
            delete[] cCachesList[i];
        delete[] cCachesList;
        bSaveMutex = false;
        return false;
    }
    for (long long int i = 0; i < nSavingLayers; i++)
    {
        for (long long int j = 0; j < nSavingCols; j++)
            delete[] cHeadLine[i][j];
        delete[] cHeadLine[i];
        delete[] nAppZeroesTemp[i];
    }
    if (bValidElement)
        delete[] bValidElement;
    if (dCache)
        delete[] dCache;
    delete[] cHeadLine;
    delete[] nAppZeroesTemp;
    for (unsigned int i = 0; i < mCachesMap.size(); i++)
        delete[] cCachesList[i];
    delete[] cCachesList;
    bSaveMutex = false;
    return true;
}

bool Cache::loadCache()
{
    if (bSaveMutex)
        return false;
    bSaveMutex = true;
    sCache_file = FileSystem::ValidFileName(sCache_file, ".cache");
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
            for (size_t i = 0; i < vCacheMemory.size(); i++)
                delete vCacheMemory[i];
            vCacheMemory.clear();
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
                vCacheMemory.push_back(new Memory());
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
                //cerr << nLength << endl;
                cHeadLine[i][j] = new char[nLength];
                cache_file.read(cHeadLine[i][j], sizeof(char)*nLength);
                string sHead;
                for (unsigned int k = 0; k < nLength-1; k++)
                {
                    sHead += cHeadLine[i][j][k];
                }
                if (i < cachemapssize)
                    vCacheMemory[i]->setHeadLineElement(j, sHead);
            }
        }
        cache_file.seekg(sizeof(long long int)*nLayers*nCols, ios_base::cur);

        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                cache_file.read((char*)dCache, sizeof(double)*nLayers);
                for (long long int k = 0; k < cachemapssize; k++)
                    vCacheMemory[k]->writeData(i, j, dCache[k]);
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
                        vCacheMemory[k]->writeData(i, j, NAN);
                }
            }
        }

        for (size_t i = 0; i < vCacheMemory.size(); i++)
        {
            vCacheMemory[i]->shrink();
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

bool Cache::isCacheElement(const string& sCache)
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
bool Cache::containsCacheElements(const string& sExpression)
{
    size_t nQuotes = 0;

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
bool Cache::addCache(const string& sCache, const Settings& _option)
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
    long long int nIndex = vCacheMemory.size();
    mCachesMap[sCacheName] = nIndex;
    vCacheMemory.push_back(new Memory());

    return true;
}

bool Cache::deleteCache(const string& sCache)
{
    if (sCache == "cache")
        return false;
    //cerr << sCache << endl;
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == sCache)
        {
            if (vCacheMemory.size() > iter->second)
            {
                delete vCacheMemory[iter->second];
                vCacheMemory.erase(vCacheMemory.begin() + iter->second);
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
            if (getSaveStatus() && Cache::isValid())
            {
                setSaveStatus(false);
            }
            else if (!Cache::isValid())
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


