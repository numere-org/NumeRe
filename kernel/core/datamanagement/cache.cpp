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
Cache::Cache() : FileSystem()
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

bool Cache::containsCacheElements(const string& sExpression)
{
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (sExpression.find(iter->first+"(") != string::npos
            && (!sExpression.find(iter->first+"(")
                || checkDelimiter(sExpression.substr(sExpression.find(iter->first+"(")-1, (iter->first).length()+2))))
        {
            return true;
        }
    }
    return false;
}

bool Cache::addCache(const string& sCache, const Settings& _option)
{
    string sCacheName = sCache.substr(0,sCache.find('('));
    string sValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_~";

    //cerr << sCache << endl;
    if ((sCacheName[0] >= '0' && sCacheName[0] <= '9') || sCacheName[0] == '~' || sCacheName == "data" || sCacheName == "string")
        //throw INVALID_CACHE_NAME;
        throw SyntaxError(SyntaxError::INVALID_CACHE_NAME, "", SyntaxError::invalid_position, sCacheName);
    if (sPredefinedFuncs.find(","+sCacheName+"()") != string::npos)
    {
        //sErrorToken = sCacheName+"()";
        //throw FUNCTION_IS_PREDEFINED;
        throw SyntaxError(SyntaxError::FUNCTION_IS_PREDEFINED, "", SyntaxError::invalid_position, sCacheName+"()");
    }
    if (sUserdefinedFuncs.length() && sUserdefinedFuncs.find(";"+sCacheName+";") != string::npos)
    {
        //sErrorToken = sCacheName;
        //throw FUNCTION_ALREADY_EXISTS;
        throw SyntaxError(SyntaxError::FUNCTION_ALREADY_EXISTS, "", SyntaxError::invalid_position, sCacheName);
    }

    for (unsigned int i = 0; i < sCacheName.length(); i++)
    {
        if (sValidChars.find(sCacheName[i]) == string::npos)
            //throw INVALID_CACHE_NAME;
            throw SyntaxError(SyntaxError::INVALID_CACHE_NAME, "", SyntaxError::invalid_position, sCacheName);
    }

    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == sCacheName)
        {
            //sErrorToken = sCacheName + "()";
            //throw CACHE_ALREADY_EXISTS;
            throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, "", SyntaxError::invalid_position, sCacheName+"()");
        }
    }

    if (sPredefinedCommands.find(";"+sCacheName+";") != string::npos)
        NumeReKernel::print(LineBreak(_lang.get("CACHE_WARNING_CMD_OVERLAP", sCacheName), _option));
    if (sPluginCommands.length() && sPluginCommands.find(";"+sCacheName+";") != string::npos)
        NumeReKernel::print(LineBreak(_lang.get("CACHE_WARNING_PLUGIN_OVERLAP"), _option));

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


bool Cache::writeString(const string& _sString, unsigned int _nthString, unsigned int nCol)
{
    //NumeReKernel::print(_sString + toString(_nthString) + toString(nCol));
    if (sStrings.empty())
    {
        if (_sString.length())
        {
            for (unsigned int i = 0; i <= nCol; i++)
            {
                sStrings.push_back(vector<string>());
            }
            if (_nthString == string::npos)
            {
                sStrings[nCol].push_back(_sString);
            }
            else
            {
                sStrings[nCol].resize(_nthString+1,"");
                sStrings[nCol][_nthString] = _sString;
            }
        }
        return true;
    }
    if (nCol >= sStrings.size())
    {
        for (unsigned int i = sStrings.size(); i <= nCol; i++)
            sStrings.push_back(vector<string>());
    }
    if (_nthString == string::npos || !sStrings[nCol].size())
    {
        if (_sString.length())
            sStrings[nCol].push_back(_sString);
        return true;
    }
    while (_nthString >= sStrings[nCol].size() && _sString.length())
        sStrings[nCol].resize(_nthString+1, "");
    if (!_sString.length() && _nthString+1 == sStrings[nCol].size())
    {
        sStrings[nCol].pop_back();
        while (sStrings[nCol].size() && !sStrings[nCol].back().length())
            sStrings[nCol].pop_back();
    }
    else// if (_sString.length())
        sStrings[nCol][_nthString] = _sString;
    return true;
}

string Cache::readString(unsigned int _nthString, unsigned int nCol)
{
    if (nCol >= sStrings.size())
        return "";
    if (_nthString == string::npos)
    {
        if (sStrings[nCol].size())
            return sStrings[nCol].back();
        return "";
    }
    else if (_nthString >= sStrings[nCol].size())
        return "";
    else
        return sStrings[nCol][_nthString];
    return "";
}

string Cache::maxString(unsigned int i1, unsigned int i2, unsigned int nCol)
{
    if (nCol >= sStrings.size())
        return "";
    if (i2 == string::npos || i2 > sStrings[nCol].size())
        i2 = sStrings[nCol].size();

    if (!i2 || sStrings[nCol].empty())
        return "";
    string sMax = sStrings[nCol][i1];
    for (unsigned int i = i1+1; i < i2; i++)
    {
        if (sMax < sStrings[nCol][i])
            sMax = sStrings[nCol][i];
    }
    return sMax;
}

string Cache::minString(unsigned int i1, unsigned int i2, unsigned int nCol)
{
    if (nCol >= sStrings.size())
        return "";
    if (i2 == string::npos || i2 > sStrings[nCol].size())
        i2 = sStrings[nCol].size();
    if (!i2 || sStrings[nCol].empty())
        return "";
    string sMin = sStrings[nCol][i1];
    for (unsigned int i = i1+1; i < i2; i++)
    {
        if (sMin > sStrings[nCol][i])
            sMin = sStrings[nCol][i];
    }
    return sMin;
}

string Cache::sumString(unsigned int i1, unsigned int i2, unsigned int nCol)
{
    if (nCol >= sStrings.size())
        return "";
    if (i2 == string::npos || i2 > sStrings[nCol].size())
        i2 = sStrings[nCol].size();
    if (!i2 || sStrings[nCol].empty())
        return "";
    string sSum = "";
    for (unsigned int i = i1; i < i2; i++)
    {
        sSum += sStrings[nCol][i];
    }
    return sSum;
}


bool Cache::checkStringvarDelimiter(const string& sToken) const
{
    bool isDelimitedLeft = false;
    bool isDelimitedRight = false;
    string sDelimiter = "+-*/ ()={}^&|!<>,.\\%#[]?:\";";

    // --> Versuche jeden Delimiter, der dir bekannt ist und setze bei einem Treffer den entsprechenden BOOL auf TRUE <--
    for (unsigned int i = 0; i < sDelimiter.length(); i++)
    {
        if (sDelimiter[i] == sToken[0] && sDelimiter[i] != '.')
            isDelimitedLeft = true;
        if (sDelimiter[i] == '(')
            continue;
        if (sDelimiter[i] == sToken[sToken.length()-1])
            isDelimitedRight = true;
    }

    // --> Gib die Auswertung dieses logischen Ausdrucks zurueck <--
    return (isDelimitedLeft && isDelimitedRight);
}

void Cache::replaceStringMethod(string& sLine, size_t nPos, size_t nLength, const string& sVarValue)
{
    if (sLine[nPos+nLength] != '.')
    {
        sLine.replace(nPos, nLength, "\"" + sVarValue + "\"");
        return;
    }
    string sDelimiter = "+-*/ ={^&|!,\\%#?:\";";
    string sMethod = "";
    string sArgument = "";
    size_t nFinalPos = 0;
    for (size_t i = nPos+nLength+1; i < sLine.length(); i++)
    {
        if (sLine[i] == '(')
        {
            sMethod = sLine.substr(nPos+nLength+1, i-(nPos+nLength+1));
            sArgument = sLine.substr(i, getMatchingParenthesis(sLine.substr(i))+1);
            nFinalPos = i += getMatchingParenthesis(sLine.substr(i))+1;
            break;
        }
        else if (sDelimiter.find(sLine[i]) != string::npos)
        {
            sMethod = sLine.substr(nPos+nLength+1, i-(nPos+nLength+1));
            nFinalPos = i;
            break;
        }
    }
    if (!sArgument.length())
        sArgument = "()";
    if (sMethod == "len")
    {
        sLine.replace(nPos, nFinalPos-nPos, "strlen(\"" + sVarValue + "\")");
    }
    else if (sMethod == "at")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", 1)";
        else
            sArgument.insert(1, "\"" + sVarValue + "\", ");
        sLine.replace(nPos, nFinalPos-nPos, "char" + sArgument);
    }
    else if (sMethod == "sub")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", 1)";
        else
            sArgument.insert(1, "\"" + sVarValue + "\", ");
        sLine.replace(nPos, nFinalPos-nPos, "substr" + sArgument);
    }
    else if (sMethod == "fnd")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", \"" + sVarValue + "\")";
        else if (sArgument.find(',') == string::npos)
            sArgument.insert(sArgument.length()-1, ", \"" + sVarValue + "\"");
        else
        {
            string sTemp = "(";
            sArgument.erase(0,1);
            sTemp += getNextArgument(sArgument, true);
            sTemp += ", \"" + sVarValue + "\"";
            if (sArgument[sArgument.find_first_not_of(' ')] == ')')
                sArgument = sTemp + ")";
            else
                sArgument = sTemp + ", " + sArgument;
        }
        sLine.replace(nPos, nFinalPos-nPos, "strfnd" + sArgument);
    }
    else if (sMethod == "rfnd")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", \"" + sVarValue + "\")";
        else if (sArgument.find(',') == string::npos)
            sArgument.insert(sArgument.length()-1, ", \"" + sVarValue + "\"");
        else
        {
            string sTemp = "(";
            sArgument.erase(0,1);
            sTemp += getNextArgument(sArgument, true);
            sTemp += ", \"" + sVarValue + "\"";
            if (sArgument[sArgument.find_first_not_of(' ')] == ')')
                sArgument = sTemp + ")";
            else
                sArgument = sTemp + ", " + sArgument;
        }
        sLine.replace(nPos, nFinalPos-nPos, "strrfnd" + sArgument);
    }
    else if (sMethod == "mtch")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", \"" + sVarValue + "\")";
        else if (sArgument.find(',') == string::npos)
            sArgument.insert(sArgument.length()-1, ", \"" + sVarValue + "\"");
        else
        {
            string sTemp = "(";
            sArgument.erase(0,1);
            sTemp += getNextArgument(sArgument, true);
            sTemp += ", \"" + sVarValue + "\"";
            if (sArgument[sArgument.find_first_not_of(' ')] == ')')
                sArgument = sTemp + ")";
            else
                sArgument = sTemp + ", " + sArgument;
        }
        sLine.replace(nPos, nFinalPos-nPos, "strmatch" + sArgument);
    }
    else if (sMethod == "rmtch")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", \"" + sVarValue + "\")";
        else if (sArgument.find(',') == string::npos)
            sArgument.insert(sArgument.length()-1, ", \"" + sVarValue + "\"");
        else
        {
            string sTemp = "(";
            sArgument.erase(0,1);
            sTemp += getNextArgument(sArgument, true);
            sTemp += ", \"" + sVarValue + "\"";
            if (sArgument[sArgument.find_first_not_of(' ')] == ')')
                sArgument = sTemp + ")";
            else
                sArgument = sTemp + ", " + sArgument;
        }
        sLine.replace(nPos, nFinalPos-nPos, "strrmatch" + sArgument);
    }
    else if (sMethod == "nmtch")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", \"" + sVarValue + "\")";
        else if (sArgument.find(',') == string::npos)
            sArgument.insert(sArgument.length()-1, ", \"" + sVarValue + "\"");
        else
        {
            string sTemp = "(";
            sArgument.erase(0,1);
            sTemp += getNextArgument(sArgument, true);
            sTemp += ", \"" + sVarValue + "\"";
            if (sArgument[sArgument.find_first_not_of(' ')] == ')')
                sArgument = sTemp + ")";
            else
                sArgument = sTemp + ", " + sArgument;
        }
        sLine.replace(nPos, nFinalPos-nPos, "str_not_match" + sArgument);
    }
    else if (sMethod == "nrmtch")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", \"" + sVarValue + "\")";
        else if (sArgument.find(',') == string::npos)
            sArgument.insert(sArgument.length()-1, ", \"" + sVarValue + "\"");
        else
        {
            string sTemp = "(";
            sArgument.erase(0,1);
            sTemp += getNextArgument(sArgument, true);
            sTemp += ", \"" + sVarValue + "\"";
            if (sArgument[sArgument.find_first_not_of(' ')] == ')')
                sArgument = sTemp + ")";
            else
                sArgument = sTemp + ", " + sArgument;
        }
        sLine.replace(nPos, nFinalPos-nPos, "str_not_rmatch" + sArgument);
    }
    else if (sMethod == "splt")
    {
        if (sArgument == "()")
            sArgument = "(\"" + sVarValue + "\", \",\")";
        else
            sArgument.insert(1, "\"" + sVarValue + "\", ");
        sLine.replace(nPos, nFinalPos-nPos, "split" + sArgument);
    }
}

bool Cache::containsStringVars(const string& _sLine) const
{
    if (!sStringVars.size())
        return false;
    string sLine = " " + _sLine + " ";
    for (auto iter = sStringVars.begin(); iter != sStringVars.end(); ++iter)
    {
        if (sLine == iter->first)
            return true;
        if (sLine.find(iter->first) != string::npos
            && sLine[sLine.find(iter->first)+(iter->first).length()] != '('
            && checkStringvarDelimiter(sLine.substr(sLine.find(iter->first)-1, (iter->first).length()+2))
            )
            return true;
    }
    return false;
}

void Cache::getStringValues(string& sLine, unsigned int nPos)
{
    if (!sStringVars.size())
        return;
    unsigned int __nPos = nPos;
    sLine += " ";
    for (auto iter = sStringVars.begin(); iter != sStringVars.end(); ++iter)
    {
        __nPos = nPos;
        while (sLine.find(iter->first, __nPos) != string::npos)
        {
            __nPos = sLine.find(iter->first, __nPos)+1;
            if (sLine[__nPos+(iter->first).length()-1] == '(')
                continue;
            if (__nPos == 1)
            {
                if (checkStringvarDelimiter(" " + sLine.substr(0, (iter->first).length()+1)) && !isInQuotes(sLine, 0, true))
                {
                    if (sLine[(iter->first).length()] == '.')
                    {
                        replaceStringMethod(sLine, 0, (iter->first).length(), iter->second);
                    }
                    else
                    {
                        sLine.replace(0,(iter->first).length(), "\"" + iter->second + "\"");
                    }
                }
                continue;
            }
            if (checkStringvarDelimiter(sLine.substr(__nPos-2, (iter->first).length()+2)) && !isInQuotes(sLine, __nPos-1, true))
            {
                if (sLine[__nPos+(iter->first).length()-1] == '.')
                {
                    replaceStringMethod(sLine, __nPos-1, (iter->first).length(), iter->second);
                }
                else
                    sLine.replace(__nPos-1,(iter->first).length(), "\"" + iter->second + "\"");
            }
        }
    }
    return;
}

void Cache::setStringValue(const string& sVar, const string& sValue)
{
    string sValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_1234567890~";

    if (sVar[0] >= '0' && sVar[0] <= '9')
        //throw STRINGVARS_MUSTNT_BEGIN_WITH_A_NUMBER;
        throw SyntaxError(SyntaxError::STRINGVARS_MUSTNT_BEGIN_WITH_A_NUMBER, "", SyntaxError::invalid_position, sVar);
    for (unsigned int i = 0; i < sVar.length(); i++)
    {
        if (sValidChars.find(sVar[i]) == string::npos)
        {
            //sErrorToken = sVar[i];
            throw SyntaxError(SyntaxError::STRINGVARS_MUSTNT_CONTAIN, "", SyntaxError::invalid_position, sVar.substr(i,1));
        }
    }

    if (sValue[0] == '"' && sValue[sValue.length()-1] == '"')
        sStringVars[sVar] = sValue.substr(1,sValue.length()-2);
    else
        sStringVars[sVar] = sValue;
    if (sStringVars[sVar].find('"') != string::npos)
    {
        unsigned int nPos = 0;
        while (sStringVars[sVar].find('"', nPos) != string::npos)
        {
            nPos = sStringVars[sVar].find('"', nPos);
            if (sStringVars[sVar][nPos-1] == '\\')
            {
                nPos++;
                continue;
            }
            else
            {
                sStringVars[sVar].insert(nPos,1,'\\');
                nPos += 2;
                continue;
            }
        }
    }
    return;
}

void Cache::removeStringVar(const string& sVar)
{
    if (!sStringVars.size())
        return;
    for (auto iter = sStringVars.begin(); iter != sStringVars.end(); ++iter)
    {
        if (iter->first == sVar)
        {
            sStringVars.erase(iter);
            return;
        }
    }
}

// cols=1[2:3]4[5:9]10:
ColumnKeys* Cache::evaluateKeyList(string& sKeyList, long long int nMax)
{
    ColumnKeys* keys = new ColumnKeys();
    if (sKeyList.find(':') == string::npos && sKeyList.find('[') == string::npos)
    {
        keys->nKey[0] = StrToInt(sKeyList)-1;
        sKeyList.clear();
    }
    else
    {
        unsigned int nLastIndex = 0;
        for (unsigned int n = 0; n < sKeyList.length(); n++)
        {
            if (sKeyList[n] == ':')
            {
                if (n != nLastIndex)
                    keys->nKey[0] = StrToInt(sKeyList.substr(nLastIndex, n-nLastIndex))-1;

                if (n+1 == sKeyList.length())
                    keys->nKey[1] = nMax;

                for (size_t i = n+1; i < sKeyList.length(); i++)
                {
                    if (sKeyList[i] == '[' || sKeyList[i] == ':' || sKeyList[i] == ',')
                    {
                        keys->nKey[1] = StrToInt(sKeyList.substr(n+1, i-n-1));
                        sKeyList.erase(0,i+1);
                        break;
                    }
                    else if (i+1 == sKeyList.length())
                    {
                        if (i == n+1)
                            keys->nKey[1] = nMax;
                        else
                            keys->nKey[1] = StrToInt(sKeyList.substr(n+1));
                        sKeyList.clear();
                        break;
                    }
                }

                break;
            }
            else if (sKeyList[n] == '[' && sKeyList.find(']', n) != string::npos)
            {
                keys->nKey[0] = StrToInt(sKeyList.substr(nLastIndex, n-nLastIndex))-1;
                string sColArray;

                size_t i = getMatchingParenthesis(sKeyList.substr(n));
                if (i != string::npos)
                {
                    sColArray = sKeyList.substr(n+1, i-1);
                    sKeyList.erase(0, i+n+1);
                }

                keys->subkeys = evaluateKeyList(sColArray, nMax);

                break;
            }
            else if (sKeyList[n] == '[')
            {
                delete keys;
                return nullptr;
            }
        }
    }
    return keys;
}




