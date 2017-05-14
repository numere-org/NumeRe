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
#include "../kernel.hpp"
using namespace std;

/*
 * Realisierung der Cache-Klasse
 */

// --> Standard-Konstruktor <--
Cache::Cache() : FileSystem()
{
	nLines = 128;
	nCols = 8;
	nLayers = 4;
	dCache = 0;
	sHeadLine = 0;
	nAppendedZeroes = 0;
	//bValidElement = 0;
	bValidData = false;
	bSaveMutex = false;
	bIsSaved = true;
	nLastSaved = time(0);
	sCache_file = "<>/numere.cache";
	sPredefinedFuncs = ",abs(),acos(),acosh(),Ai(),arccos(),arcosh(),arcsin(),arsinh(),arctan(),artanh(),asin(),asinh(),ascii(),atan(),atanh(),avg(),bessel(),betheweizsaecker(),Bi(),binom(),cache(),char(),cmp(),cnt(),cos(),cosh(),cross(),data(),date(),dblfacul(),degree(),det(),diag(),diagonalize(),eigenvals(),eigenvects(),erf(),erfc(),exp(),faculty(),findfile(),findparam(),floor(),gamma(),gcd(),getfilelist(),getmatchingparens(),getopt(),heaviside(),hermite(),identity(),invert(),is_data(),is_nan(),is_string(),laguerre(),laguerre_a(),lcm(),legendre(),legendre_a(),ln(),log(),log10(),log2(),matfc(),matfcf(),matfl(),matflf(),max(),med(),min(),neumann(),norm(),num(),one(),pct(),phi(),prd(),radian(),rand(),range(),rect(),rint(),roof(),round(),sbessel(),sign(),sin(),sinc(),sinh(),sneumann(),solve(),split(),sqrt(),std(),strfnd(),strrfnd(),strlen(),student_t(),substr(),sum(),tan(),tanh(),theta(),time(),to_char(),to_cmd(),to_string(),to_value(),trace(),transpose(),valtostr(),Y(),zero()";
	sUserdefinedFuncs = "";
	sPredefinedCommands =  ";abort;about;audio;break;compose;cont;cont3d;continue;copy;credits;data;datagrid;define;delete;dens;dens3d;diff;draw;draw3d;edit;else;endcompose;endfor;endif;endprocedure;endwhile;eval;explicit;export;extrema;fft;find;fit;for;get;global;grad;grad3d;graph;graph3d;help;hist;hline;if;ifndef;ifndefined;info;integrate;list;load;matop;mesh;mesh3d;move;mtrxop;namespace;new;odesolve;plot;plot3d;procedure;pulse;quit;random;read;readline;regularize;remove;rename;replaceline;resample;return;save;script;set;smooth;sort;stats;stfa;str;surf;surf3d;swap;taylor;throw;undef;undefine;var;vect;vect3d;while;write;zeroes;";
	sPluginCommands = "";
	mCachesMap["cache"] = 0;
}

// --> Allgemeiner Konstruktor <--
Cache::Cache(long long int _nLines, long long int _nCols, long long int _nLayers) : FileSystem()
{
    Cache();
    nLayers = _nLayers;
	nLines = _nLines;
	nCols = _nCols;
    AllocateCache(_nLines, _nCols, _nLayers);
}

// --> Destruktor <--
Cache::~Cache()
{
	// --> Gib alle Speicher frei, sofern sie belegt sind! (Pointer != 0) <--
	if (dCache)
	{
		for (long long int i = 0; i < nLines; i++)
		{
            for (long long int j = 0; j < nCols; j++)
                delete[] dCache[i][j];
            delete[] dCache[i];
		}
		delete[] dCache;
	}
	if (sHeadLine)
	{
        for (long long int i = 0; i < nLayers; i++)
            delete[] sHeadLine[i];
		delete[] sHeadLine;
	}
	if (nAppendedZeroes)
	{
        for (long long int i = 0; i < nLayers; i++)
            delete[] nAppendedZeroes[i];
		delete[] nAppendedZeroes;
    }
    if (cache_file.is_open())
        cache_file.close();
}

// --> Generiere eine neue Matrix auf Basis der gesetzten Werte. Pruefe zuvor, ob nicht schon eine vorhanden ist <--
bool Cache::AllocateCache(long long int _nNLines, long long int _nNCols, long long int _nNLayers)
{
    //cerr << _nNLines << " " << _nNCols << " " << _nNLayers << endl;
	if (_nNCols * _nNLines * _nNLayers > 1e8)
	{
        throw TOO_LARGE_CACHE;
	}
	else if (!dCache && !nAppendedZeroes && !sHeadLine)// && !bValidElement)
	{
		sHeadLine = new string*[_nNLayers];
		for (long long int i = 0; i < _nNLayers; i++)
		{
            sHeadLine[i] = new string[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
                sHeadLine[i][j] = "Spalte_" + toString(j+1);
		}
		nAppendedZeroes = new long long int*[_nNLayers];
		for (long long int i = 0; i < _nNLayers; i++)
		{
            nAppendedZeroes[i] = new long long int[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
                nAppendedZeroes[i][j] = _nNLines;
		}
		/*bValidElement = new bool**[_nNLines];
		for (long long int i = 0; i < _nNLines; i++)
		{
			bValidElement[i] = new bool*[_nNCols];
			for (long long int j = 0; j < _nNCols; j++)
			{
                bValidElement[i][j] = new bool[_nNLayers];
                for (long long int k = 0; k < _nNLayers; k++)
                    bValidElement[i][j][k] = false;
			}
		}*/
		dCache = new double**[_nNLines];
		for (long long int i = 0; i < _nNLines; i++)
		{
			dCache[i] = new double*[_nNCols];
			for (long long int j = 0; j < _nNCols; j++)
			{
                dCache[i][j] = new double[_nNLayers];
                for (long long int k = 0; k < _nNLayers; k++)
                    dCache[i][j][k] = NAN;
			}
		}
		//cerr << nLines << " " << nCols << " " << nLayers << endl;

		nLines = _nNLines;
		nCols = _nNCols;
		nLayers = _nNLayers;


		//cerr << nLines << " " << nCols << " " << nLayers << endl;
		//cerr << "|-> Cache wurde erfolgreich vorbereitet!" << endl;
	}
	else if (nLines && nCols && nLayers && dCache && nAppendedZeroes)// && bValidElement)
	{
        long long int nCaches = mCachesMap.size();
        if (nLayers < nCaches)
            nCaches = nLayers;
        string** sNewHeadLine = new string*[_nNLayers];
        for (long long int i = 0; i < _nNLayers; i++)
        {
            sNewHeadLine[i] = new string[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
            {
                if (j < nCols && i < nCaches)
                    sNewHeadLine[i][j] = sHeadLine[i][j];
                else
                    sNewHeadLine[i][j] = "Spalte_"+toString(j+1);
            }
        }
        long long int** nNewAppendedZeroes = new long long int*[_nNLayers];
        for (long long int i = 0; i < _nNLayers; i++)
        {
            nNewAppendedZeroes[i] = new long long int[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
            {
                if (j < nCols && i < nCaches)
                    nNewAppendedZeroes[i][j] = nAppendedZeroes[i][j]+(_nNLines-nLines);
                else
                    nNewAppendedZeroes[i][j] = _nNLines;
            }
        }
        double*** dNewCache = new double**[_nNLines];
        for (long long int i = 0; i < _nNLines; i++)
        {
            dNewCache[i] = new double*[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
            {
                dNewCache[i][j] = new double[_nNLayers];
                for (long long int k = 0; k < _nNLayers; k++)
                {
                    if (i < nLines && j < nCols && k < nCaches)
                        dNewCache[i][j][k] = dCache[i][j][k];
                    else
                        dNewCache[i][j][k] = NAN;
                }
            }
        }
        /*bool*** bNewValidElement = new bool**[_nNLines];
        for (long long int i = 0; i < _nNLines; i++)
        {
            bNewValidElement[i] = new bool*[_nNCols];
            for (long long int j = 0; j < _nNCols; j++)
            {
                bNewValidElement[i][j] = new bool[_nNLayers];
                for (long long int k = 0; k < _nNLayers; k++)
                {
                    if (i < nLines && j < nCols && k < nCaches)
                        bNewValidElement[i][j][k] = bValidElement[i][j][k];
                    else
                        bNewValidElement[i][j][k] = false;
                }
            }
        }*/

        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                delete[] dCache[i][j];
                //delete[] bValidElement[i][j];
            }
            delete[] dCache[i];
            //delete[] bValidElement[i];
        }
        delete[] dCache;
        //delete[] bValidElement;
        for (long long int i = 0; i < nLayers; i++)
        {
            delete[] nAppendedZeroes[i];
            delete[] sHeadLine[i];
        }
        delete[] nAppendedZeroes;
        delete[] sHeadLine;

        nCols = _nNCols;
        nLines = _nNLines;
        nLayers = _nNLayers;

        dCache = dNewCache;
        //bValidElement = bNewValidElement;
        nAppendedZeroes = nNewAppendedZeroes;
        sHeadLine = sNewHeadLine;
	}
	else
	{
		NumeReKernel::print("FEHLER: Kann nicht in den Cache schreiben!");
		return false;
	}
	return true;
}


// --> Setzt nCols <--
bool Cache::resizeCache(long long int _nLines, long long int _nCols, long long int _nLayers)
{
	long long int _nNCols = nCols;
	long long int _nNLines = nLines;
	long long int _nNLayers = nLayers;

	while (_nLines > _nNLines)
		_nNLines *= 2;
	while (_nCols > _nNCols)
		_nNCols *= 2;
    if (_nLayers > 0 && (_nLayers > _nNLayers || !dCache) && _nLayers > 4)
        _nNLayers = _nLayers;
	if (!AllocateCache(_nNLines, _nNCols, _nNLayers))
        return false;
	return true;
}

// --> gibt nCols zurueck <--
long long int Cache::getCacheCols(const string& _sCache, bool bFull) const
{
    return getCacheCols(mCachesMap.at(_sCache), bFull);
}

long long int Cache::getCacheCols(long long int _nLayer, bool _bFull) const
{
	if (!_bFull && dCache && bValidData)
	{
        if (nAppendedZeroes)
        {
            long long int nReturn = nCols;
            /* --> Von oben runterzaehlen, damit nur die leeren Spalten rechts von den Daten
             *     ignoriert werden! <--
             */
            for (long long int i = nCols-1; i >= 0; i--)
            {
                if (nAppendedZeroes[_nLayer][i] == nLines)
                    nReturn--;
                // --> Findest du eine Spalte die nicht leer ist, dann breche die Schleife ab! <--
                if (nAppendedZeroes[_nLayer][i] != nLines)
                    break;
            }
            return nReturn;
        }
        else
            return 0;
	}
	else if (!dCache || !bValidData)
        return 0;
	else
		return nCols;
}

// --> gibt nLines zurueck <--
long long int Cache::getCacheLines(const string& _sCache, bool _bFull) const
{
    return getCacheLines(mCachesMap.at(_sCache), _bFull);
}

long long int Cache::getCacheLines(long long int _nLayer, bool _bFull) const
{
    if (!_bFull && dCache && bValidData)
    {
        if (nAppendedZeroes)
        {
            long long int nReturn = 0;
            /* --> Suche die Spalte, in der am wenigsten Nullen angehaengt sind, und gib deren
             *     Laenge zurueck <--
             */
            for (long long int i = 0; i < nCols; i++)
            {
                if (nLines - nAppendedZeroes[_nLayer][i] > nReturn)
                    nReturn = nLines - nAppendedZeroes[_nLayer][i];
            }
            return nReturn;
        }
        else
            return 0;
    }
    else if (!dCache || !bValidData)
        return 0;
    else
        return nLines;
}

double Cache::readFromCache(long long int _nLine, long long int _nCol, const string& _sCache) const
{
    return readFromCache(_nLine, _nCol, mCachesMap.at(_sCache));
}

// --> gibt das Element der _nLine-ten Zeile und der _nCol-ten Spalte zurueck <--
double Cache::readFromCache(long long int _nLine, long long int _nCol, long long int _nLayer) const
{
	if (_nLine < nLines && _nCol < nCols && _nLayer < nLayers && dCache && _nLine >= 0 && _nCol >= 0)
		return dCache[_nLine][_nCol][_nLayer];
	else
		return NAN;
}

vector<double> Cache::readFromCache(const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& _sCache) const
{
    return readFromCache(_vLine, _vCol, mCachesMap.at(_sCache));
}

vector<double> Cache::readFromCache(const vector<long long int>& _vLine, const vector<long long int>& _vCol, long long int _nLayer) const
{
    vector<double> vReturn;

    if ((_vLine.size() > 1 && _vCol.size() > 1) || !dCache || _nLayer >= nLayers)
        vReturn.push_back(NAN);
    else
    {
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            for (unsigned int j = 0; j < _vCol.size(); j++)
            {
                if (_vLine[i] < 0
                    || _vLine[i] >= getCacheLines(_nLayer, false)
                    || _vCol[j] < 0
                    || _vCol[j] >= getCacheCols(_nLayer, false)
                    || _vLine[i] >= nLines-nAppendedZeroes[_nLayer][_vCol[j]])
                    vReturn.push_back(NAN);
                else
                    vReturn.push_back(dCache[_vLine[i]][_vCol[j]][_nLayer]);
            }
        }
    }
    return vReturn;
}

bool Cache::isValidElement(long long int _nLine, long long int _nCol, const string& _sCache) const
{
    return isValidElement(_nLine, _nCol, mCachesMap.at(_sCache));
}

// --> gibt zurueck, ob das Element der _nLine-ten Zeile und _nCol-ten Spalte ueberhaupt gueltig ist <--
bool Cache::isValidElement(long long int _nLine, long long int _nCol, long long int _nLayer) const
{
	if (_nLine < nLines && _nLine >= 0 && _nCol < nCols && _nCol >= 0 && _nLayer < nLayers && dCache)// && bValidElement)
		return !isnan(dCache[_nLine][_nCol][_nLayer]);//bValidElement[_nLine][_nCol][_nLayer];
	else
		return false;
}

// --> loescht den Inhalt des Datenfile-Objekts, ohne selbiges zu zerstoeren <--
void Cache::removeCachedData()
{
	if(bValidData)	// Sind ueberhaupt Daten vorhanden?
	{
        if (bSaveMutex)
            return;
        bSaveMutex = true;
		// --> Speicher, wo denn noetig freigeben <--
		for (long long int i = 0; i < nLines; i++)
		{
            for (long long int j = 0; j < nCols; j++)
            {
                delete[] dCache[i][j];
            }
			delete[] dCache[i];
		}
		delete[] dCache;
		if (nAppendedZeroes)
		{
            for (long long int i = 0; i < nLayers; i++)
                delete[] nAppendedZeroes[i];
			delete[] nAppendedZeroes;
			nAppendedZeroes = 0;
		}
		if (sHeadLine)
		{
            for (long long int i = 0; i < nLayers; i++)
                delete[] sHeadLine[i];
			delete[] sHeadLine;
			sHeadLine = 0;
		}

		// --> Variablen zuruecksetzen <--
		nLines = 128;
		nCols = 8;
		nLayers = 4;
		dCache = 0;
		bValidData = false;
		bSaveMutex = false;
		bIsSaved = true;
		nLastSaved = time(0);
		mCachesMap.clear();
		mCachesMap["cache"] = 0;
	}
	return;
}

// --> gibt den Wert von bValidData zurueck <--
bool Cache::isValid() const
{
    if (!dCache)
        return false;

    for (long long int i = 0; i < nLayers; i++)
    {
        if (Cache::getCacheCols(i, false))
            return true;
    }
	return false;
}

// --> gibt den Wert von bIsSaved zurueck <--
bool Cache::getSaveStatus() const
{
    return bIsSaved;
}

string Cache::getCacheHeadLineElement(long long int _i, const string& _sCache) const
{
    //cerr << _sCache << endl;
    return getCacheHeadLineElement(_i, mCachesMap.at(_sCache));
}

// --> gibt das _i-te Element der gespeicherten Kopfzeile zurueck <--
string Cache::getCacheHeadLineElement(long long int _i, long long int _nLayer) const
{
    if (_i >= getCacheCols(_nLayer, true))
        return "Spalte " + toString((int)_i+1) + " (leer)";
    else
        return sHeadLine[_nLayer][_i];
}

vector<string> Cache::getCacheHeadLineElement(vector<long long int> _vCol, const string& _sCache) const
{
    return getCacheHeadLineElement(_vCol, mCachesMap.at(_sCache));
}

vector<string> Cache::getCacheHeadLineElement(vector<long long int> _vCol, long long int _nLayer) const
{
    vector<string> vHeadLines;

    for (unsigned int i = 0; i < _vCol.size(); i++)
    {
        if (_vCol[i] < 0)
            continue;
        if (_vCol[i] >= getCacheCols(_nLayer, false) || _vCol[i] < 0)
            vHeadLines.push_back("Spalte " + toString((int)_vCol[i]+1) + " (leer)");
        else
            vHeadLines.push_back(sHeadLine[_nLayer][_vCol[i]]);
    }

    return vHeadLines;
}

bool Cache::setCacheHeadLineElement(long long int _i, const string& _sCache, string _sHead)
{
    return setCacheHeadLineElement(_i, mCachesMap.at(_sCache), _sHead);
}

// --> schreibt _sHead in das _i-te Element der Kopfzeile <--
bool Cache::setCacheHeadLineElement(long long int _i, long long int _nLayer, string _sHead)
{
	if (_i < nCols && _nLayer < nLayers && dCache)
        sHeadLine[_nLayer][_i] = _sHead;
    else if (!dCache)
    {
        if (!resizeCache(nLines, _i+1, _nLayer+1))
            return false;
        sHeadLine[_nLayer][_i] = _sHead;
    }
    else
    {
        if (!resizeCache(nLines, _i+1, _nLayer+1))
            return false;
        sHeadLine[_nLayer][_i] = _sHead;
    }

    if (bIsSaved)
    {
        nLastSaved = time(0);
        bIsSaved = false;
    }
	return true;
}

long long int Cache::getAppendedZeroes(long long int _i, const string& _sCache) const
{
    return getAppendedZeroes(_i, mCachesMap.at(_sCache));
}

// --> gibt die Zahl der in der _i-ten Spalte angehaengten Nullzeilen zurueck <--
long long int Cache::getAppendedZeroes(long long int _i, long long int _nLayer) const
{
	if (nAppendedZeroes)
		return nAppendedZeroes[_nLayer][_i];
	else
		return 0;
}

bool Cache::writeToCache(long long int _nLine, long long int _nCol, const string& _sCache, double _dData)
{
    //cerr << "error" << endl;
    return writeToCache(_nLine, _nCol, mCachesMap.at(_sCache), _dData);
}

// --> Schreibt einen Wert an beliebiger Stelle in den Cache <--
bool Cache::writeToCache(long long int _nLine, long long int _nCol, long long int _nLayer, double _dData)
{
    //cerr << "Layer = " << _nLayer << endl;
	if (dCache && (_nLine < nLines) && (_nCol < nCols) && (_nLayer < nLayers))
	{
        if (isinf(_dData) || isnan(_dData))
        {
            //bValidElement[_nLine][_nCol][_nLayer] = false;
            dCache[_nLine][_nCol][_nLayer] = NAN;
        }
        else
        {
            //cerr << _dData << endl;
            dCache[_nLine][_nCol][_nLayer] = _dData;
            //bValidElement[_nLine][_nCol][_nLayer] = true;
            if (nLines - nAppendedZeroes[_nLayer][_nCol] <= _nLine)
            {
                nAppendedZeroes[_nLayer][_nCol] = nLines - _nLine - 1;
                //cerr << "nAppendedZeroes" << endl;
            }
            bValidData = true;
        }
	}
	else if (_nLayer > nLayers)
	{
        return false;
	}
	else
	{
        /* --> Ist der Cache zu klein? Verdoppele die fehlenden Dimensionen so lange,
         *     bis die fehlende Dimension groesser als das zu schreibende Matrixelement
         *     ist. <--
         */
        //cerr << "Write " << nLines << " " << nCols << " " << nLayers << endl;
		long long int _nNLines = nLines;
		long long int _nNCols = nCols;
		while (_nLine+1 >= _nNLines)
		{
            _nNLines = 2 * _nNLines;
		}
		while (_nCol+1 >= _nNCols)
		{
            _nNCols = 2 * _nNCols;
		}
		if (!AllocateCache(_nNLines, _nNCols, nLayers))
            return false;

        if (isinf(_dData) || isnan(_dData))
        {
            //bValidElement[_nLine][_nCol][_nLayer] = false;
            dCache[_nLine][_nCol][_nLayer] = NAN;
        }
        else
        {
            dCache[_nLine][_nCol][_nLayer] = _dData;
            //bValidElement[_nLine][_nCol][_nLayer] = true;
            if (nLines - nAppendedZeroes[_nLayer][_nCol] <= _nLine)
                nAppendedZeroes[_nLayer][_nCol] = nLines - _nLine - 1;
		}
		nLines = _nNLines;
		nCols = _nNCols;
        if (!isinf(_dData) && !isnan(_dData) && !bValidData)
            bValidData = true;
	}
	// --> Setze den Zeitstempel auf "jetzt", wenn der Cache eben noch gespeichert war <--
	if (bIsSaved)
	{
        nLastSaved = time(0);
        bIsSaved = false;
    }
	return true;
}

void Cache::setSaveStatus(bool _bIsSaved)
{
    bIsSaved = _bIsSaved;
    if (bIsSaved)
        nLastSaved = time(0);
    return;
}

long long int Cache::getLastSaved() const
{
    return nLastSaved;
}

void Cache::deleteEntry(long long int _nLine, long long int _nCol, const string& _sCache)
{
    deleteEntry(_nLine, _nCol, mCachesMap.at(_sCache));
    return;
}

void Cache::deleteEntry(long long int _nLine, long long int _nCol, long long int _nLayer)
{
    if (dCache)// && bValidElement)
    {
        if (!isnan(dCache[_nLine][_nCol][_nLayer]))//bValidElement[_nLine][_nCol][_nLayer])
        {
            dCache[_nLine][_nCol][_nLayer] = NAN;
            //bValidElement[_nLine][_nCol][_nLayer] = false;
            if (bIsSaved)
            {
                nLastSaved = time(0);
                bIsSaved = false;
            }
            for (long long int i = nLines-1; i >= 0; i--)
            {
                if (!isnan(dCache[i][_nCol][_nLayer]))
                {
                    nAppendedZeroes[_nLayer][_nCol] = nLines - i - 1;
                    break;
                }
                if (!i && isnan(dCache[i][_nCol][_nLayer]))
                {
                    nAppendedZeroes[_nLayer][_nCol] = nLines;
                    sHeadLine[_nLayer][_nCol] = "Spalte_"+toString((int)_nCol+1);
                }
            }
        }
    }
    return;
}

bool Cache::qSort(int* nIndex, int nElements, int nKey, int nLayer, int nLeft, int nRight, int nSign)
{
    //cerr << nLeft << "/" << nRight << endl;

    if (!nIndex || !nElements || nLeft < 0 || nRight > nElements || nRight < nLeft)
    {
        return false;
    }
    if (nRight == nLeft)
        return true;
    if (nRight - nLeft <= 1 && (nSign*dCache[nIndex[nLeft]][nKey][nLayer] <= nSign*dCache[nIndex[nRight]][nKey][nLayer] || isnan(dCache[nIndex[nRight]][nKey][nLayer])))
        return true;
    else if (nRight - nLeft <= 1 && (nSign*dCache[nIndex[nRight]][nKey][nLayer] <= nSign*dCache[nIndex[nLeft]][nKey][nLayer] || isnan(dCache[nIndex[nLeft]][nKey][nLayer])))
    {
        int nTemp = nIndex[nLeft];
        nIndex[nLeft] = nIndex[nRight];
        nIndex[nRight] = nTemp;
        return true;
    }
    while (isnan(dCache[nIndex[nRight]][nKey][nLayer]) && nRight >= nLeft)
    {
        nRight--;
    }
    double nPivot = nSign*dCache[nIndex[nRight]][nKey][nLayer];
    int i = nLeft;
    int j = nRight-1;
    do
    {
        while ((nSign*dCache[nIndex[i]][nKey][nLayer] <= nPivot && !isnan(dCache[nIndex[i]][nKey][nLayer])) && i < nRight)
            i++;
        while ((nSign*dCache[nIndex[j]][nKey][nLayer] >= nPivot || isnan(dCache[nIndex[j]][nKey][nLayer])) && j > nLeft)
            j--;
        if (i < j)
        {
            int nTemp = nIndex[i];
            nIndex[i] = nIndex[j];
            nIndex[j] = nTemp;
        }
    }
    while (i < j);

    if (nSign*dCache[nIndex[i]][nKey][nLayer] > nPivot || isnan(dCache[nIndex[i]][nKey][nLayer]))
    {
        int nTemp = nIndex[i];
        nIndex[i] = nIndex[nRight];
        nIndex[nRight] = nTemp;
    }
    if (isnan(dCache[nIndex[nRight-1]][nKey][nLayer]))
    {
        int nTemp = nIndex[nRight-1];
        nIndex[nRight-1] = nIndex[nRight];
        nIndex[nRight] = nTemp;
    }
    //cerr << nLeft << "/" << i << endl;
    if (i > nLeft)
    {
        if (!qSort(nIndex, nElements, nKey, nLayer, nLeft, i-1, nSign))
            return false;
    }
    if (i < nRight)
    {
        if (!qSort(nIndex, nElements, nKey, nLayer, i+1, nRight, nSign))
            return false;
    }
    return true;
}

bool Cache::sortElements(const string& sLine) // cache -sort[[=desc]] cols=1[2:3]4[5:9]10:
{
    if (!dCache)
        return false;
    int nIndex[nLines];
    int nLayer = 0;
    bool bError = false;
    //bool bSortVector[nLines];
    double dSortVector[nLines];
    for (int i = 0; i < nLines; i++)
        nIndex[i] = i;

    int nSign = 1;

    if (findCommand(sLine).sString != "sort")
    {
        nLayer = mCachesMap.at(findCommand(sLine).sString);
    }
    if (matchParams(sLine, "sort", '='))
    {
        if (getArgAtPos(sLine, matchParams(sLine, "sort", '=')+4) == "desc")
            nSign = -1;
    }
    else
    {
        for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
        {
            if (matchParams(sLine, iter->first, '='))
            {
                if (getArgAtPos(sLine, matchParams(sLine, iter->first, '=')+5) == "desc")
                    nSign = -1;
                nLayer = iter->second;
                break;
            }
            else if (matchParams(sLine, iter->first))
            {
                nLayer = iter->second;
                break;
            }
        }
    }

    if (!matchParams(sLine, "cols", '=') && !matchParams(sLine, "c", '='))
    {
        for (int i = 0; i < getCacheCols(nLayer, false); i++)
        {
            //cerr << "Sortiere Spalte " << i+1 << " ... " << nLines-nAppendedZeroes[i] << endl;
            if (!qSort(nIndex, nLines-nAppendedZeroes[nLayer][i], i, nLayer, 0, nLines-1-nAppendedZeroes[nLayer][i], nSign))
            {
                bError = true;
                break;
            }
            //cerr << "Werte Indexmap aus ..." << endl;
            for (int j = 0; j < nLines-nAppendedZeroes[nLayer][i]; j++)
            {
                //cerr << nIndex[j] << "; ";
                dSortVector[j] = dCache[nIndex[j]][i][nLayer];
                //bSortVector[j] = !isnan(dCache[nIndex[j]][i][nLayer]);
            }

            for (int j = 0; j < nLines-nAppendedZeroes[nLayer][i]; j++)
            {
                dCache[j][i][nLayer] = dSortVector[j];
                //bValidElement[j][i][nLayer] = bSortVector[j];
                //cerr << bSortVector[j] << "/" << bValidElement[j][i] << "; ";
            }
            for (int j = 0; j < nLines; j++)
                nIndex[j] = j;
            //cerr << endl;
        }
    }
    else
    {
        string sCols = "";
        if (matchParams(sLine, "cols", '='))
        {
            sCols = getArgAtPos(sLine, matchParams(sLine, "cols", '=')+4);
        }
        else
        {
            sCols = getArgAtPos(sLine, matchParams(sLine, "c", '=')+1);
        }

        if (sCols.find(':') == string::npos && sCols.find('[') == string::npos)
        {
            int nKey = StrToInt(sCols)-1;
            if (nKey >= 0 && nKey < getCacheCols(nLayer, false))
            {
                if (!qSort(nIndex, nLines-nAppendedZeroes[nLayer][nKey], nKey, nLayer, 0, nLines-nAppendedZeroes[nLayer][nKey]-1, nSign))
                {
                    bError = true;
                }

                if (!bError)
                {
                    for (int j = 0; j < nLines-nAppendedZeroes[nLayer][nKey]; j++)
                    {
                        dSortVector[j] = dCache[nIndex[j]][nKey][nLayer];
                        //bSortVector[j] = bValidElement[nIndex[j]][nKey][nLayer];
                    }
                    for (int j = 0; j < nLines-nAppendedZeroes[nLayer][nKey]; j++)
                    {
                        dCache[j][nKey][nLayer] = dSortVector[j];
                        //bValidElement[j][nKey][nLayer] = bSortVector[j];
                    }
                }
            }
        }
        else
        {
            unsigned int nLastIndex = 0;
            for (unsigned int n = 0; n < sCols.length(); n++)
            {
                if (sCols[n] == ':')
                {
                    int nKey_1 = 0;
                    if (n != nLastIndex)
                        nKey_1 = StrToInt(sCols.substr(nLastIndex, n-nLastIndex))-1;
                    n++;
                    int nKey_2 = n;
                    for (unsigned int i = n; n < sCols.length(); i++)
                    {
                        if (sCols[i] == '[')
                        {
                            nKey_2 = i;
                            break;
                        }
                    }
                    if (nKey_2 == (int)n)
                        nKey_2 = sCols.length()-1;
                    nLastIndex = nKey_2;
                    nKey_2 = StrToInt(sCols.substr(n, nKey_2-n));
                    if (!nKey_2)
                        nKey_2 = getCacheCols(nLayer, false);
                    for (int i = nKey_1; i < nKey_2; i++)
                    {
                        if (!qSort(nIndex, nLines-nAppendedZeroes[nLayer][i], i, nLayer, 0, nLines-nAppendedZeroes[nLayer][i]-1, nSign))
                        {
                            bError = true;
                            break;
                        }
                        for (int j = 0; j < nLines-nAppendedZeroes[nLayer][i]; j++)
                        {
                            dSortVector[j] = dCache[nIndex[j]][i][nLayer];
                            //bSortVector[j] = bValidElement[nIndex[j]][i][nLayer];
                        }
                        for (int j = 0; j < nLines-nAppendedZeroes[nLayer][i]; j++)
                        {
                            dCache[j][i][nLayer] = dSortVector[j];
                            //bValidElement[j][i][nLayer] = bSortVector[j];
                        }
                        for (int j = 0; j < nLines; j++)
                            nIndex[j] = j;
                    }
                    if (bError)
                        break;
                    n = nLastIndex;
                }
                else if (sCols[n] == '[' && sCols.find(']', n) != string::npos)
                {
                    int nKey = StrToInt(sCols.substr(nLastIndex, n-nLastIndex))-1;
                    int nKey_1 = n+1;
                    int nKey_2 = n+1;
                    for (unsigned int i = nKey_1; i < sCols.length(); i++)
                    {
                        if (sCols[i] == ']')
                        {
                            nKey_2 = i;
                            break;
                        }
                    }
                    nLastIndex = nKey_2+1;
                    if (sCols.substr(nKey_1, nKey_2-nKey_1).find(':') != string::npos)
                    {
                        string sColArray = sCols.substr(nKey_1, nKey_2-nKey_1);
                        //cerr << sColArray << endl;
                        nKey_1 = StrToInt(sColArray.substr(0, sColArray.find(':')))-1;
                        nKey_2 = StrToInt(sColArray.substr(sColArray.find(':')+1));
                        if (nKey_1 == -1)
                            nKey_1 = 0;
                        if (!nKey_2)
                            nKey_2 = getCacheCols(nLayer, false);
                    }
                    else
                    {
                        nKey_1 = StrToInt(sCols.substr(nKey_1, nKey_2-nKey_1))-1;
                        nKey_2 = nKey_1+1;
                    }
                    if (nKey < 0
                        || nKey > getCacheCols(nLayer, false)
                        || nKey_1 < 0
                        || nKey_1 > getCacheCols(nLayer, false)
                        || nKey_2 <= 0
                        || nKey_2 > getCacheCols(nLayer, false)
                        || nKey_1 > nKey_2)
                    {
                        bError = true;
                        break;
                    }
                    //cerr << nKey_1 << "; " << nKey_2 << endl;
                    if (!qSort(nIndex, nLines-nAppendedZeroes[nLayer][nKey], nKey, nLayer, 0, nLines-nAppendedZeroes[nLayer][nKey]-1, nSign))
                    {
                        bError = true;
                        break;
                    }
                    for (int j = 0; j < nLines-nAppendedZeroes[nLayer][nKey]; j++)
                    {
                        dSortVector[j] = dCache[nIndex[j]][nKey][nLayer];
                        //bSortVector[j] = bValidElement[nIndex[j]][nKey][nLayer];

                    }
                    for (int j = 0; j < nLines-nAppendedZeroes[nLayer][nKey]; j++)
                    {
                        dCache[j][nKey][nLayer] = dSortVector[j];
                        //bValidElement[j][nKey][nLayer] = bSortVector[j];
                    }
                    //cerr << nKey_1 << "; " << nKey_2 << endl;
                    for (int c = nKey_1; c < nKey_2; c++)
                    {
                        if (c == nKey)
                            continue;
                        for (int j = 0; j < nLines-nAppendedZeroes[nLayer][nKey]; j++)
                        {
                            dSortVector[j] = dCache[nIndex[j]][c][nLayer];
                            //bSortVector[j] = bValidElement[nIndex[j]][c][nLayer];
                        }
                        for (int j = 0; j < nLines-nAppendedZeroes[nLayer][nKey]; j++)
                        {
                            dCache[j][c][nLayer] = dSortVector[j];
                            //bValidElement[j][c][nLayer] = bSortVector[j];
                        }
                    }
                    for (int j = 0; j < nLines; j++)
                        nIndex[j] = j;
                    n = nLastIndex;
                }
                else if (sCols[n] == '[')
                {
                    bError = true;
                    break;
                }
                // --> Alle Leerzeichen und ':' ueberspringen <--
                while ((sCols[n+1] == ':' || sCols[n+1] == ' ') && n+1 < sCols.length())
                {
                    n++;
                    nLastIndex++;
                }
            }
        }
    }

    for (int i = 0; i < getCacheCols(nLayer, false); i++)
    {
        for (int j = nLines-1; j >= 0; j--)
        {
            if (!isnan(dCache[j][i][nLayer]))
            {
                nAppendedZeroes[nLayer][i] = nLines-j-1;
                break;
            }
        }
    }

    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }

    return !bError;
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
    long long int nSavingLayers = 4;
    long long int nSavingCols = 8;
    long long int nSavingLines = 128;
    long long int nColMax = 0;
    long long int nLineMax = 0;
    bool* bValidElement = 0;
    if (!bValidData)
    {
        bSaveMutex = false;
        return false;
    }
    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (getCacheCols(iter->second, false) > nColMax)
            nColMax = getCacheCols(iter->second, false);
        if (getCacheLines(iter->second, false) > nLineMax)
            nLineMax = getCacheLines(iter->second, false);
    }

    while (nSavingLayers < mCachesMap.size() && nSavingLayers < nLayers)
        nSavingLayers *= 2;
    while (nSavingCols < nColMax && nSavingCols < nCols)
        nSavingCols *= 2;
    while (nSavingLines < nLineMax && nSavingLines < nLines)
        nSavingLines *= 2;
    sCache_file = FileSystem::ValidFileName(sCache_file, ".cache");

    char*** cHeadLine = new char**[nSavingLayers];
    bValidElement = new bool[nSavingLayers];
    //cerr << 1 << endl;
    //cerr << nSavingCols << " " << nSavingLines << " " << nSavingLayers << endl;
    //cerr << nCols << " " << nLines << " " << nLayers << endl;
    //cerr << mCachesMap.size() << endl;
    char** cCachesList = new char*[mCachesMap.size()];
    long long int** nAppZeroesTemp = new long long int*[nSavingLayers];
    for (long long int i = 0; i < nSavingLayers; i++)
    {
        cHeadLine[i] = new char*[nSavingCols];
        nAppZeroesTemp[i] = new long long int[nSavingCols];
        for (long long int j = 0; j < nSavingCols; j++)
        {
            nAppZeroesTemp[i][j] = nAppendedZeroes[i][j] - (nLines - nSavingLines);
            cHeadLine[i][j] = new char[sHeadLine[i][j].length()+1];
            for (unsigned int k = 0; k < sHeadLine[i][j].length(); k++)
            {
                cHeadLine[i][j][k] = sHeadLine[i][j][k];
            }
            cHeadLine[i][j][sHeadLine[i][j].length()] = '\0';
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
    if (bValidData && cache_file.good())
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
                size_t nlength = sHeadLine[i][j].length()+1;
                //cerr << nlength << endl;
                cache_file.write((char*)&nlength, sizeof(size_t));
                cache_file.write(cHeadLine[i][j], sizeof(char)*(sHeadLine[i][j].length()+1));
            }
        }
        //cerr << 4 << endl;
        for (long long int i = 0; i < nSavingLayers; i++)
            cache_file.write((char*)nAppZeroesTemp[i], sizeof(long long int)*nSavingCols);
        for (long long int i = 0; i < nSavingLines; i++)
        {
            for (long long int j = 0; j < nSavingCols; j++)
                cache_file.write((char*)dCache[i][j], sizeof(double)*nSavingLayers);
        }
        //cerr << 5 << endl;
        for (long long int i = 0; i < nSavingLines; i++)
        {
            for (long long int j = 0; j < nSavingCols; j++)
            {
                for (long long int k = 0; k < nSavingLayers; k++)
                    bValidElement[k] = !isnan(dCache[i][j][k]);
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
    bool* bValidElement = 0;
    long int nMajor = 0;
    long int nMinor = 0;
    long int nBuild = 0;
    size_t nLength = 0;
    size_t cachemapssize = 0;
    long long int nLayerIndex = 0;
    if (!cache_file.is_open())
        cache_file.close();
    cache_file.open(sCache_file.c_str(), ios_base::in | ios_base::binary);

    if (!bValidData && cache_file.good())
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
        nLastSaved = tTime;
        cache_file.read((char*)&nLines, sizeof(long long int));
        cache_file.read((char*)&nCols, sizeof(long long int));

        if (nMajor*100+nMinor*10+nBuild >= 107 && nLines < 0 && nCols < 0)
        {
            nLines *= -1;
            nCols *= -1;
            cache_file.read((char*)&nLayers, sizeof(long long int));
            nLayers *= -1;
            cache_file.read((char*)&cachemapssize, sizeof(size_t));
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
            }
        }
        else
            nLayers = 1;

        AllocateCache(nLines, nCols, nLayers);
        cHeadLine = new char**[nLayers];
        bValidElement = new bool[nLayers];
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
                sHeadLine[i][j].resize(nLength-1);
                for (unsigned int k = 0; k < nLength-1; k++)
                {
                    sHeadLine[i][j][k] = cHeadLine[i][j][k];
                }
            }
        }
        for (long long int i = 0; i < nLayers; i++)
            cache_file.read((char*)nAppendedZeroes[i], sizeof(long long int)*nCols);
        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
                cache_file.read((char*)dCache[i][j], sizeof(double)*nLayers);
        }
        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                cache_file.read((char*)bValidElement, sizeof(bool)*nLayers);
                for (long long int k = 0; k < nLayers; k++)
                {
                    if (!bValidElement[k])
                        dCache[i][j][k] = NAN;
                }
            }
        }
        cache_file.close();
        setSaveStatus(true);
        bValidData = true;
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
    if (bValidElement)
        delete[] bValidElement;
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
    string sValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

    //cerr << sCache << endl;
    if ((sCacheName[0] >= '0' && sCacheName[0] <= '9') || sCacheName == "data" || sCacheName == "string")
        throw INVALID_CACHE_NAME;
    if (sPredefinedFuncs.find(","+sCacheName+"()") != string::npos)
    {
        sErrorToken = sCacheName+"()";
        throw FUNCTION_IS_PREDEFINED;
    }
    if (sUserdefinedFuncs.length() && sUserdefinedFuncs.find(";"+sCacheName+";") != string::npos)
    {
        sErrorToken = sCacheName;
        throw FUNCTION_ALREADY_EXISTS;
    }

    for (unsigned int i = 0; i < sCacheName.length(); i++)
    {
        if (sValidChars.find(sCacheName[i]) == string::npos)
            throw INVALID_CACHE_NAME;
    }

    for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        if (iter->first == sCacheName)
        {
            sErrorToken = sCacheName + "()";
            throw CACHE_ALREADY_EXISTS;
        }
    }

    if (sPredefinedCommands.find(";"+sCacheName+";") != string::npos)
        NumeReKernel::print(LineBreak(_lang.get("CACHE_WARNING_CMD_OVERLAP", sCacheName), _option));
    if (sPluginCommands.length() && sPluginCommands.find(";"+sCacheName+";") != string::npos)
        NumeReKernel::print(LineBreak(_lang.get("CACHE_WARNING_PLUGIN_OVERLAP"), _option));

    //cerr << "add" << mCachesMap.size() << " " << nLayers << endl;

    if (mCachesMap.size() >= nLayers)
    {
        if (!resizeCache(nLines, nCols, nLayers*2))
            return false;
    }
    long long int nIndex = mCachesMap.size();
    mCachesMap[sCacheName] = nIndex;
    //cerr << "add" << mCachesMap.size() << " " << nLayers << endl;
    /*for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
    {
        cerr << iter->first << " = " << iter->second << endl;
    }*/
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
            if (dCache)
            {
                for (long long int k = iter->second; k < nLayers-1; k++)
                {
                    for (long long int i = 0; i < nLines; i++)
                    {
                        for (long long int j = 0; j < nCols; j++)
                        {
                            if (!i)
                            {
                                sHeadLine[k][j] = sHeadLine[k+1][j];
                                nAppendedZeroes[k][j] = nAppendedZeroes[k+1][j];
                            }
                            dCache[i][j][k] = dCache[i][j][k+1];
                            //bValidElement[i][j][k] = bValidElement[i][j][k+1];
                        }
                    }
                }
                //cerr << 1 << endl;
                for (long long int i = 0; i < nLines; i++)
                {
                    for (long long int j = 0; j < nCols; j++)
                    {
                        if (!i)
                        {
                            sHeadLine[nLayers-1][j] = "Spalte_"+toString(j+1);
                            nAppendedZeroes[nLayers-1][j] = nLines;
                        }
                        dCache[i][j][nLayers-1] = NAN;
                        //bValidElement[i][j][nLayers-1] = false;
                    }
                }
            }
            //cerr << 2 << endl;
            for (auto iter2 = mCachesMap.begin(); iter2 != mCachesMap.end(); ++iter2)
            {
                if (iter2->second > iter->second)
                    mCachesMap[iter2->first] = iter2->second-1;
            }
            //cerr << 3 << endl;
            mCachesMap.erase(iter);
            //cerr << 4 << endl;
            if (bIsSaved && Cache::isValid())
            {
                bIsSaved = false;
                nLastSaved = time(0);
            }
            else if (!Cache::isValid())
            {
                bIsSaved = true;
                nLastSaved = time(0);
                if (fileExists(getProgramPath()+"/numere.cache"))
                {
                    string sCachefile = getProgramPath() + "/numere.cache";
                    NumeReKernel::printPreFmt(sCachefile+"\n");
                    remove(sCachefile.c_str());
                }
            }
            //cerr << 5 << endl;
            return true;
        }
    }
    return false;
}

bool Cache::saveLayer(string _sFileName, const string& sLayer)
{
    ofstream file_out;
    if (file_out.is_open())
        file_out.close();
    _sFileName = ValidFileName(_sFileName, ".ndat");
    file_out.open(_sFileName.c_str(), ios_base::binary | ios_base::trunc | ios_base::out);

    if (file_out.is_open() && file_out.good() && bValidData)
    {
        long long int nLayer = mCachesMap.at(sLayer);
        char** cHeadLine = new char*[nCols];
        long int nMajor = AutoVersion::MAJOR;
        long int nMinor = AutoVersion::MINOR;
        long int nBuild = AutoVersion::BUILD;
        long long int lines = getCacheLines(nLayer, false);
        long long int cols = getCacheCols(nLayer, false);
        long long int appendedzeroes[cols];
        double dDataValues[cols];
        bool bValidValues[cols];
        for (long long int i = 0; i < cols; i++)
        {
            appendedzeroes[i] = nAppendedZeroes[nLayer][i]-(nLines-lines);
        }

        for (long long int i = 0; i < cols; i++)
        {
            cHeadLine[i] = new char[sHeadLine[nLayer][i].length()+1];
            for (unsigned int j = 0; j < sHeadLine[nLayer][i].length(); j++)
            {
                cHeadLine[i][j] = sHeadLine[nLayer][i][j];
            }
            cHeadLine[i][sHeadLine[nLayer][i].length()] = '\0';
        }

        time_t tTime = time(0);
        file_out.write((char*)&nMajor, sizeof(long));
        file_out.write((char*)&nMinor, sizeof(long));
        file_out.write((char*)&nBuild, sizeof(long));
        file_out.write((char*)&tTime, sizeof(time_t));
        file_out.write((char*)&lines, sizeof(long long int));
        file_out.write((char*)&cols, sizeof(long long int));
        //cerr << lines << " " << cols << endl;
        for (long long int i = 0; i < cols; i++)
        {
            size_t nlength = sHeadLine[nLayer][i].length()+1;
            //cerr << nlength << endl;
            file_out.write((char*)&nlength, sizeof(size_t));
            file_out.write(cHeadLine[i], sizeof(char)*sHeadLine[nLayer][i].length()+1);
        }
        file_out.write((char*)appendedzeroes, sizeof(long long int)*cols);

        for (long long int i = 0; i < lines; i++)
        {
            for (long long int j = 0; j < cols; j++)
                dDataValues[j] = dCache[i][j][nLayer];
            file_out.write((char*)dDataValues, sizeof(double)*cols);
        }
        for (long long int i = 0; i < lines; i++)
        {
            for (long long int j = 0; j < cols; j++)
                bValidValues[j] = !isnan(dCache[i][j][nLayer]);
            file_out.write((char*)bValidValues, sizeof(bool)*cols);
        }
        file_out.close();
        for (long long int i = 0; i < cols; i++)
        {
            delete[] cHeadLine[i];
        }
        delete[] cHeadLine;

        return true;
    }
    else
    {
        file_out.close();
        return false;
    }

    return true;
}

void Cache::deleteBulk(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    Cache::deleteBulk(mCachesMap.at(_sCache), i1,i2,j1,j2);
    return;
}

void Cache::deleteBulk(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    //cerr << i1 << " " << i2 << " " << j1 << " " << j2 << endl;
    if (!Cache::getCacheCols(_nLayer, false))
        return;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;
    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            //bValidElement[i][j][_nLayer] = false;
            dCache[i][j][_nLayer] = NAN;
        }
    }
    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }
    for (long long int j = nCols-1; j >= 0; j--)
    {
        for (long long int i = nLines-1; i >= 0; i--)
        {
            if (!isnan(dCache[i][j][_nLayer]))
            {
                nAppendedZeroes[_nLayer][j] = nLines - i - 1;
                break;
            }
            if (!i && isnan(dCache[i][j][_nLayer]))
            {
                nAppendedZeroes[_nLayer][j] = nLines;
                sHeadLine[_nLayer][j] = "Spalte_"+toString((int)j+1);
            }
        }
    }
    return;
}

void Cache::deleteBulk(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    Cache::deleteBulk(mCachesMap.at(_sCache), _vLine, _vCol);
    return;
}

void Cache::deleteBulk(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] >= nCols || _vCol[j] < 0 || _vLine[i] >= nLines || _vLine[i] < 0)
                continue;
            //bValidElement[_vLine[i]][_vCol[j]][_nLayer] = false;
            dCache[_vLine[i]][_vCol[j]][_nLayer] = NAN;
        }
    }
    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }
    for (long long int j = nCols-1; j >= 0; j--)
    {
        for (long long int i = nLines-1; i >= 0; i--)
        {
            if (!isnan(dCache[i][j][_nLayer]))
            {
                nAppendedZeroes[_nLayer][j] = nLines - i - 1;
                break;
            }
            if (!i && isnan(dCache[i][j][_nLayer]))
            {
                nAppendedZeroes[_nLayer][j] = nLines;
                sHeadLine[_nLayer][j] = "Spalte_"+toString((int)j+1);
            }
        }
    }
    return;
}

double Cache::std(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return std(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::std(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    double dMean = 0.0;
    double dStd = 0.0;
    long long int nInvalid = 0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;
    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
            {
                nInvalid++;
                continue;
            }
            dMean += dCache[i][j][_nLayer];
        }
    }
    dMean /= (double)((i2-i1+1)*(j2-j1+1)-nInvalid);

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                continue;
            dStd += (dMean - dCache[i][j][_nLayer]) * (dMean - dCache[i][j][_nLayer]);
        }
    }
    dStd /= (double)((i2-i1+1)*(j2-j1+1)-1-nInvalid);
    dStd = sqrt(dStd);
    return dStd;
}

double Cache::std(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return std(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::std(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dAvg = avg(_nLayer, _vLine, _vCol);
    double dStd = 0.0;
    unsigned int nInvalid = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                nInvalid++;
            else if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                nInvalid++;
            else
                dStd += (dAvg - dCache[_vLine[i]][_vCol[j]][_nLayer])*(dAvg - dCache[_vLine[i]][_vCol[j]][_nLayer]);
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size()-1)
        return NAN;
    return sqrt(dStd / ((_vLine.size()*_vCol.size())-1-nInvalid));
}

double Cache::avg(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return avg(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::avg(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    double dMean = 0.0;
    long long int nInvalid = 0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
            {
                nInvalid++;
                continue;
            }
            dMean += dCache[i][j][_nLayer];
        }
    }
    dMean /= (double)((i2-i1+1)*(j2-j1+1) - nInvalid);
    return dMean;
}

double Cache::avg(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{;
    return avg(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::avg(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dAvg = 0.0;
    unsigned int nInvalid = 0;
    //cerr << "vectorfunktion" << endl;
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            //cerr << _vLine[i] << endl;
            //cerr << "j " << _vCol[j] << endl;
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                nInvalid++;
            else if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                nInvalid++;
            else
                dAvg += dCache[_vLine[i]][_vCol[j]][_nLayer];
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;
    return dAvg/(_vLine.size()*_vCol.size()-nInvalid);
}

double Cache::max(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return max(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::max(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    double dMax = 0.0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                continue;
            if (i == i1 && j == j1)
                dMax = dCache[i][j][_nLayer];
            else if (dCache[i][j][_nLayer] > dMax)
                dMax = dCache[i][j][_nLayer];
            else
                continue;
        }
    }
    return dMax;
}

double Cache::max(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return max(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::max(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dMax = NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                continue;
            if (isnan(dMax))
                dMax = dCache[_vLine[i]][_vCol[j]][_nLayer];
            if (dMax < dCache[_vLine[i]][_vCol[j]][_nLayer])
                dMax = dCache[_vLine[i]][_vCol[j]][_nLayer];
        }
    }
    return dMax;
}

double Cache::min(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return min(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::min(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    double dMin = 0.0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                continue;
            if (i == i1 && j == j1)
                dMin = dCache[i][j][_nLayer];
            else if (dCache[i][j][_nLayer] < dMin)
                dMin = dCache[i][j][_nLayer];
            else
                continue;
        }
    }
    return dMin;
}

double Cache::min(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return min(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::min(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dMin = NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                continue;
            if (isnan(dMin))
                dMin = dCache[_vLine[i]][_vCol[j]][_nLayer];
            if (dMin > dCache[_vLine[i]][_vCol[j]][_nLayer])
                dMin = dCache[_vLine[i]][_vCol[j]][_nLayer];
        }
    }
    return dMin;
}

double Cache::prd(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return prd(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::prd(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    double dPrd = 1.0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                continue;
            dPrd *= dCache[i][j][_nLayer];
        }
    }
    return dPrd;

}

double Cache::prd(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return prd(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::prd(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dPrd = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                continue;
            dPrd *= dCache[_vLine[i]][_vCol[j]][_nLayer];
        }
    }
    return dPrd;
}

double Cache::sum(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return sum(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::sum(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    double dSum = 0.0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                continue;
            dSum += dCache[i][j][_nLayer];
        }
    }
    return dSum;
}

double Cache::sum(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return sum(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::sum(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dSum = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                continue;
            dSum += dCache[_vLine[i]][_vCol[j]][_nLayer];
        }
    }
    return dSum;
}

double Cache::num(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return num(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::num(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    int nInvalid = 0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    //cerr << i1 << i2 << j1 << j2 << endl;
    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                nInvalid++;
        }
    }
    return (double)((i2-i1+1)*(j2-j1+1)-nInvalid);
}

double Cache::num(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return num(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::num(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    //double dNum = 0.0;
    int nInvalid = 0;

    //cerr << _vLine.size() << " " << _vCol.size() << endl;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                nInvalid++;
            else if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                nInvalid++;
        }
    }
    return (_vLine.size()*_vCol.size())-nInvalid;
}

double Cache::and_func(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return and_func(mCachesMap.at(sCache), i1, i2, j1, j2);
}

double Cache::and_func(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return 0.0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    double dRetVal = NAN;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dRetVal))
                dRetVal = 1.0;
            if (isnan(dCache[i][j][_nLayer]) || dCache[i][j][_nLayer] == 0)
                return 0.0;
        }
    }
    if (isnan(dRetVal))
        return 0.0;
    return 1.0;
}

double Cache::and_func(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return and_func(mCachesMap.at(sCache), _vLine, _vCol);
}

double Cache::and_func(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return 0.0;


    double dRetVal = NAN;
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dRetVal))
                dRetVal = 1.0;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]) || dCache[_vLine[i]][_vCol[j]][_nLayer] == 0)
                return 0.0;
        }
    }

    if (isnan(dRetVal))
        return 0.0;
    return 1.0;
}

double Cache::or_func(const string& sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return or_func(mCachesMap.at(sCache), i1, i2, j1, j2);
}

double Cache::or_func(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return 0.0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (!isnan(dCache[i][j][_nLayer]) && dCache[i][j][_nLayer] != 0.0)
                return 1.0;
        }
    }
    return 0.0;
}

double Cache::or_func(const string& sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return or_func(mCachesMap.at(sCache), _vLine, _vCol);
}

double Cache::or_func(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]) || dCache[_vLine[i]][_vCol[j]][_nLayer] != 0)
                return 1.0;
        }
    }
    return 0.0;
}


double Cache::cnt(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return cnt(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::cnt(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return 0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    if (j2 >= nCols)
        j2 = nCols-1;
    if (i2 >= nLines-getAppendedZeroes(j1, _nLayer))
        i2 = nLines-1-getAppendedZeroes(j1, _nLayer);

    return (double)((i2-i1+1)*(j2-j1+1));
}

double Cache::cnt(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return cnt(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::cnt(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    int nInvalid;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                nInvalid++;
        }
    }
    return (_vLine.size()*_vCol.size())-nInvalid;
}

double Cache::norm(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return norm(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::norm(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    double dNorm = 0.0;
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                continue;
            dNorm += dCache[i][j][_nLayer]*dCache[i][j][_nLayer];
        }
    }
    return sqrt(dNorm);
}

double Cache::norm(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return norm(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::norm(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dNorm = 0.0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                continue;
            dNorm += dCache[_vLine[i]][_vCol[j]][_nLayer]*dCache[_vLine[i]][_vCol[j]][_nLayer];
        }
    }
    return sqrt(dNorm);
}

double Cache::cmp(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2, double dRef, int nType)
{
    return cmp(mCachesMap.at(_sCache), i1, i2, j1, j2, dRef, nType);
}

double Cache::cmp(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2, double dRef, int nType)
{
    //cerr << _nLayer << " " << i1 << " " << i2 << " " << j1 << " " << j2 << " " << dRef << " " << nType << endl;
    if (!bValidData)
        return NAN;
    double dKeep = 0.0;
    int nKeep = -1;

    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (isnan(dCache[i][j][_nLayer]))
                continue;
            if (dCache[i][j][_nLayer] == dRef)
            {
                if (abs(nType) <= 1)
                {
                    if (i1 == i2)
                        return j+1;
                    else
                        return i+1;
                }
                else
                    return dCache[i][j][_nLayer];
            }
            else if ((nType == 1 || nType == 2) && dCache[i][j][_nLayer] > dRef)
            {
                if (nKeep == -1 || dCache[i][j][_nLayer] < dKeep)
                {
                    dKeep = dCache[i][j][_nLayer];
                    if (i1 == i2)
                        nKeep = j;
                    else
                        nKeep = i;
                }
                else
                    continue;
            }
            else if ((nType == -1 || nType == -2) && dCache[i][j][_nLayer] < dRef)
            {
                if (nKeep == -1 || dCache[i][j][_nLayer] > dKeep)
                {
                    dKeep = dCache[i][j][_nLayer];
                    if (i1 == i2)
                        nKeep = j;
                    else
                        nKeep = i;
                }
                else
                    continue;
            }
        }
    }
    if (nKeep == -1)
        return NAN;
    else if (abs(nType) == 2)
        return dKeep;
    else
        return nKeep+1;
}

double Cache::cmp(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef, int nType)
{
    return cmp(mCachesMap.at(_sCache), _vLine, _vCol, dRef, nType);
}

double Cache::cmp(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef, int nType)
{
    if (!bValidData)
        return NAN;
    double dKeep = 0.0;
    int nKeep = -1;

    for (long long int i = 0; i < _vLine.size(); i++)
    {
        for (long long int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                continue;
            if (dCache[_vLine[i]][_vCol[j]][_nLayer] == dRef)
            {
                if (abs(nType) <= 1)
                {
                    if (_vLine[0] == _vLine[_vLine.size()-1])
                        return _vCol[j]+1;
                    else
                        return _vLine[i]+1;
                }
                else
                    return dCache[_vLine[i]][_vCol[j]][_nLayer];
            }
            else if ((nType == 1 || nType == 2) && dCache[_vLine[i]][_vCol[j]][_nLayer] > dRef)
            {
                if (nKeep == -1 || dCache[_vLine[i]][_vCol[j]][_nLayer] < dKeep)
                {
                    dKeep = dCache[_vLine[i]][_vCol[j]][_nLayer];
                    if (_vLine[0] == _vLine[_vLine.size()-1])
                        nKeep = _vCol[j];
                    else
                        nKeep = _vLine[i];
                }
                else
                    continue;
            }
            else if ((nType == -1 || nType == -2) && dCache[_vLine[i]][_vCol[j]][_nLayer] < dRef)
            {
                if (nKeep == -1 || dCache[_vLine[i]][_vCol[j]][_nLayer] > dKeep)
                {
                    dKeep = dCache[_vLine[i]][_vCol[j]][_nLayer];
                    if (_vLine[0] == _vLine[_vLine.size()-1])
                        nKeep = _vCol[j];
                    else
                        nKeep = _vLine[i];
                }
                else
                    continue;
            }
        }
    }
    if (nKeep == -1)
        return NAN;
    else if (abs(nType) == 2)
        return dKeep;
    else
        return nKeep+1;
}

double Cache::med(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2)
{
    return med(mCachesMap.at(_sCache), i1, i2, j1, j2);
}

double Cache::med(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2)
{
    if (!bValidData)
        return NAN;
    Datafile _cache;
    _cache.setCacheStatus(true);
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (i1 != i2 && j1 != j2)
            {
                if (!isnan(dCache[i][j][_nLayer]))
                    _cache.writeToCache((j-j1)+(i-i1)*(j2-j1+1),0, "cache", dCache[i][j][_nLayer]);
            }
            else if (i1 != i2)
            {
                if (!isnan(dCache[i][j][_nLayer]))
                    _cache.writeToCache(i-i1,j-j1,"cache",dCache[i][j][_nLayer]);
            }
            else
            {
                if (!isnan(dCache[i][j][_nLayer]))
                    _cache.writeToCache(j-j1,i-i1,"cache",dCache[i][j][_nLayer]);
            }
        }
    }
    string sSortCommand = "cache -sort";
    _cache.sortElements(sSortCommand);
    if (_cache.getCacheLines(0, false) % 2)
    {
        return _cache.getElement(_cache.getCacheLines(0, false) / 2, 0, "cache");
    }
    else
    {
        return (_cache.getElement(_cache.getCacheLines(0, false) / 2, 0, "cache") + _cache.getElement(_cache.getCacheLines(0, false) / 2 - 1, 0, "cache")) / 2.0;
    }
}

double Cache::med(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    return med(mCachesMap.at(_sCache), _vLine, _vCol);
}

double Cache::med(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol)
{
    if (!bValidData)
        return NAN;
    double dMed = 0.0;
    unsigned int nInvalid = 0;
    unsigned int nCount = 0;
    double* dData = 0;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                nInvalid++;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                nInvalid++;
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;
    dData = new double[(_vLine.size()*_vCol.size())-nInvalid];
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            dData[nCount] = dCache[_vLine[i]][_vCol[j]][_nLayer];
            nCount++;
            if (nCount == (_vLine.size()*_vCol.size())-nInvalid)
                break;
        }
    }

    gsl_sort(dData, 1, nCount);
    dMed = gsl_stats_median_from_sorted_data(dData, 1, nCount);

    delete[] dData;

    return dMed;
}

double Cache::pct(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2, double dPct)
{
    return pct(mCachesMap.at(_sCache), i1, i2, j1, j2, dPct);
}

double Cache::pct(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2, double dPct)
{
    if (!bValidData)
        return NAN;
    if (dPct >= 1 || dPct <= 0)
        return NAN;
    Datafile _cache;
    _cache.setCacheStatus(true);
    if (i2 == -1)
        i2 = i1;
    else
        i2--;
    if (j2 == -1)
        j2 = j1;
    else
        j2--;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            if (i1 != i2 && j1 != j2)
            {
                if (!isnan(dCache[i][j][_nLayer]))
                    _cache.writeToCache((j-j1)+(i-i1)*(j2-j1+1),0, "cache", dCache[i][j][_nLayer]);
            }
            else if (i1 != i2)
            {
                if (!isnan(dCache[i][j][_nLayer]))
                    _cache.writeToCache(i-i1,j-j1,"cache",dCache[i][j][_nLayer]);
            }
            else
            {
                if (!isnan(dCache[i][j][_nLayer]))
                    _cache.writeToCache(j-j1,i-i1,"cache",dCache[i][j][_nLayer]);
            }
        }
    }
    string sSortCommand = "cache -sort";
    _cache.sortElements(sSortCommand);
    return (1-((_cache.getCacheLines(0,false)-1)*dPct-floor((_cache.getCacheLines(0,false)-1)*dPct)))*_cache.getElement(floor((_cache.getCacheLines(0,false)-1)*dPct),0,"cache")
        + ((_cache.getCacheLines(0,false)-1)*dPct-floor((_cache.getCacheLines(0,false)-1)*dPct))*_cache.getElement(floor((_cache.getCacheLines(0,false)-1)*dPct)+1,0,"cache");
}

double Cache::pct(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct)
{
    return pct(mCachesMap.at(_sCache), _vLine, _vCol, dPct);
}

double Cache::pct(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct)
{
    if (!bValidData)
        return NAN;
    unsigned int nInvalid = 0;
    unsigned int nCount = 0;
    double* dData = 0;

    if (dPct >= 1 || dPct <= 0)
        return NAN;

    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                nInvalid++;
            if (isnan(dCache[_vLine[i]][_vCol[j]][_nLayer]))// || !bValidElement[_vLine[i]][_vCol[j]][_nLayer])
                nInvalid++;
        }
    }
    if (nInvalid >= _vLine.size()*_vCol.size())
        return NAN;
    dData = new double[(_vLine.size()*_vCol.size())-nInvalid];
    for (unsigned int i = 0; i < _vLine.size(); i++)
    {
        for (unsigned int j = 0; j < _vCol.size(); j++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= getCacheLines(_nLayer, false) || _vCol[j] < 0 || _vCol[j] >= getCacheCols(_nLayer, false))
                continue;
            dData[nCount] = dCache[_vLine[i]][_vCol[j]][_nLayer];
            nCount++;
            if (nCount == (_vLine.size()*_vCol.size())-nInvalid)
                break;
        }
    }

    gsl_sort(dData, 1, nCount);
    dPct = gsl_stats_quantile_from_sorted_data(dData, 1, nCount, dPct);

    delete[] dData;

    return dPct;
}


bool Cache::retoque(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2, AppDir Direction)
{
    return retoque(mCachesMap.at(_sCache), i1, i2, j1, j2, Direction);
}

bool Cache::retoque(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2, AppDir Direction)
{
    bool bUseAppendedZeroes = false;
    if (!bValidData)
        return false;
    if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
        return false;
    if (i2 == -1)
        i2 = i1;
    else if (i2 == -2)
    {
        i2 = getCacheLines(_nLayer, true)-1;
        bUseAppendedZeroes = true;
    }
    if (j2 == -1)
        j2 = j1;
    else if (j2 == -2)
        j2 = getCacheCols(_nLayer, false)-1;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    if ((Direction == ALL || Direction == GRID) && i2 - i1 < 3)
        Direction = LINES;
    if ((Direction == ALL || Direction == GRID) && j2 - j1 < 3)
        Direction = COLS;

    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;
        for (long long int j = j1; j <= j2; j++)
        {
            if (nMax < nLines - nAppendedZeroes[_nLayer][j]-1)
                nMax = nLines - nAppendedZeroes[_nLayer][j]-1;
        }
        if (i2 > nMax)
            i2 = nMax;
    }


    if (Direction == GRID)
    {
        if (bUseAppendedZeroes)
        {
            if (!retoque(_nLayer, i1, -2, j1, -1, COLS) || !retoque(_nLayer, i1, -2, j1+1, -1, COLS))
                return false;
        }
        else
        {
            if (!retoque(_nLayer, i1, i2, j1, j1+1, COLS))
                return false;
        }
        j1 += 2;
    }

    if (Direction == ALL || Direction == GRID)
    {
        for (long long int i = i1; i <= i2; i++)
        {
            for (long long int j = j1; j <= j2; j++)
            {
                if (isnan(dCache[i][j][_nLayer]))
                {
                    if (i > i1 && i < i2 && j > j1 && j < j2 && isValidDisc(i-1, j-1, _nLayer, 2))
                    {
                        retoqueRegion(_nLayer, i-1, i+1, j-1, j+1, 1, ALL);
                        if (bIsSaved)
                        {
                            bIsSaved = false;
                            nLastSaved = time(0);
                        }
                    }
                }
            }
        }

        for (long long int i = i1; i <= i2; i++)
        {
            for (long long int j = j1; j <= j2; j++)
            {
                if (isnan(dCache[i][j][_nLayer]))
                {
                    if (i > i1 && i < i2 && j > j1 && j < j2 && isValidDisc(i-1, j-1, _nLayer, 2))
                    {
                        retoqueRegion(_nLayer, i-1, i+1, j-1, j+1, 1, ALL);
                    }
                    else if (i == i1 || i == i2 || j == j1 || j == j2)
                    {
                        unsigned int nOrder = 1;
                        long long int __i = i;
                        long long int __j = j;
                        if (i == i1)
                        {
                            if (j == j1)
                            {
                                while (__i+nOrder+1 <= i2 && __j+nOrder+1 <= j2
                                    && num(_nLayer, i+nOrder+1, i+nOrder+2, j, j+nOrder+1) != cnt(_nLayer, i+nOrder+1, i+nOrder+2, j, j+nOrder+1)
                                    && num(_nLayer, i, i+nOrder+2, j+nOrder+1, j+nOrder+2) != cnt(_nLayer, i, i+nOrder+2, j+nOrder+1, j+nOrder+2))
                                {
                                    nOrder++;
                                }
                                if (__i+nOrder+1 <= i2 && __j+nOrder+1 <= j2)
                                {
                                    //Datafile _cache;
                                    //_cache.setCacheStatus(true);
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                                    for (long long int _i = __i; _i <= __i+nOrder; _i++)
                                    {
                                        //_region.vDataArray[_i-__i].resize(nOrder+2);
                                        //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                        for (long long int _j = __j; _j <= __j+nOrder; _j++)
                                        {
                                            if (_i == __i+nOrder || _j == __j+nOrder)
                                            {
                                                _region.vDataArray[(_i == __i+nOrder ? 0 : _i-__i+1)][(_j == __j+nOrder ? 0 : _j-__j+1)] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[(_i == __i+nOrder ? 0 : _i-__i+1)][(_j == __j+nOrder ? 0 : _j-__j+1)] = true;
                                            }
                                            if (!isnan(dCache[_i][_j][_nLayer]))
                                            {
                                                _region.vDataArray[_i-__i+1][_j-__j+1] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[_i-__i+1][_j-__j+1] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i-__i+1][_j-__j+1] = NAN;
                                                _region.vValidationArray[_i-__i+1][_j-__j+1] = false;
                                            }
                                                //_cache.writeToCache(_i-__i+1, _j-__j+1, "cache", dCache[_i][_j][_nLayer]);
                                                //_cache.writeToCache((_i == __i+nOrder+1 ? 0 : _i-__i+1), (_j == __j+nOrder+1 ? 0 : _j-__j+1), "cache", dCache[_i][_j][_nLayer]);
                                        }
                                    }
                                    retoqueRegion(_region);
                                    //_cache.retoqueRegion(0, 0, nOrder+1, 0, nOrder+1, nOrder, 0);
                                    for (long long int _i = __i; _i <= __i+nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j+nOrder; _j++)
                                        {
                                            if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i+1][_j-__j+1])
                                            {
                                                dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i+1][_j-__j+1];
                                                //bValidElement[_i][_j][_nLayer] = true;
                                            }
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else if (j == j2)
                            {
                                __j--;
                                while (__i+nOrder+1 <= i2 && __j >= j1 && __j+nOrder+1 < j2
                                    && num(_nLayer, __i+nOrder+1, __i+nOrder+2, __j, __j+nOrder+1) != cnt(_nLayer, __i+nOrder+1, __i+nOrder+2, __j, __j+nOrder+1)
                                    && num(_nLayer, __i, __i+nOrder+2, __j, __j+1) != cnt(_nLayer, __i, __i+nOrder+2, __j, __j+1))
                                {
                                    nOrder++;
                                    if (__j > j1)
                                        __j--;
                                }
                                if (__i+nOrder+1 <= i2 && __j >= j1 && __j+nOrder+1 < j2)
                                {
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                                    //_region.vDataArray.resize(nOrder+2);
                                    //_region.vValidationArray.resize(nOrder+2);
                                    //Datafile _cache;
                                    //_cache.setCacheStatus(true);
                                    for (long long int _i = __i; _i <= __i+nOrder; _i++)
                                    {
                                        //_region.vDataArray[_i-__i].resize(nOrder+2);
                                        //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (!isnan(dCache[_i][_j][_nLayer]))
                                            {
                                                _region.vDataArray[_i-__i+1][_j-__j] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[_i-__i+1][_j-__j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i-__i+1][_j-__j] = NAN;
                                                _region.vValidationArray[_i-__i+1][_j-__j] = false;
                                            }
                                                //_cache.writeToCache(_i-__i+1, _j-__j, "cache", dCache[_i][_j][_nLayer]);
                                            if (_i == __i+nOrder || _j == __j)
                                            {
                                                _region.vDataArray[(_i == __i+nOrder ? 0 : _i-__i+1)][(_j == __j ? nOrder+1 : _j-__j)] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[(_i == __i+nOrder ? 0 : _i-__i+1)][(_j == __j ? nOrder+1 : _j-__j)] = true;
                                            }
                                                //_cache.writeToCache((_i == __i+nOrder+1 ? 0 : _i-__i+1), (_j == __j ? __j+nOrder+1 : _j-__j), "cache", dCache[_i][_j][_nLayer]);
                                        }
                                    }
                                    retoqueRegion(_region);
                                    for (long long int _i = __i; _i <= __i+nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i+1][_j-__j])
                                            {
                                                dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i+1][_j-__j];
                                                //bValidElement[_i][_j][_nLayer] = true;
                                            }
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else
                            {
                                while (__i+nOrder+1 <= i2 && __j+nOrder+1 <= j2 && __j >= j1
                                    && num(_nLayer, __i+nOrder+1, __i+nOrder+2, __j, __j+nOrder+2) != cnt(_nLayer, __i+nOrder+1, __i+nOrder+2, __j, __j+nOrder+2)
                                    && num(_nLayer, __i, __i+nOrder+2, __j+nOrder+1, __j+nOrder+2) != cnt(_nLayer, __i, __i+nOrder+2, __j+nOrder+1, __j+nOrder+2))
                                {
                                    if (__j > j1)
                                        nOrder += 2;
                                    else
                                        nOrder++;
                                    if (__j > j1)
                                        __j--;
                                }
                                if (__i+nOrder+1 <= i2 && __j+nOrder+1 <= j2 && __j >= j1)
                                {
                                    //Datafile _cache;
                                    //_cache.setCacheStatus(true);
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                                    //_region.vDataArray.resize(nOrder+2);
                                    //_region.vValidationArray.resize(nOrder+2);
                                    for (long long int _i = __i; _i <= __i+nOrder; _i++)
                                    {
                                        //_region.vDataArray[_i-__i].resize(nOrder+2);
                                        //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (!isnan(dCache[_i][_j][_nLayer]))
                                            {
                                                _region.vDataArray[_i-__i+1][_j-__j] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[_i-__i+1][_j-__j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i-__i+1][_j-__j] = NAN;
                                                _region.vValidationArray[_i-__i+1][_j-__j] = false;
                                            }
                                                //_cache.writeToCache(_i-__i+1, _j-__j, "cache", dCache[_i][_j][_nLayer]);
                                            if (_i == __i+nOrder)
                                            {
                                                _region.vDataArray[0][_j-__j] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[0][_j-__j] = true;
                                                continue;
                                            }
                                                //_cache.writeToCache(0, _j-__j, "cache", dCache[_i][_j][_nLayer]);
                                        }
                                    }

                                    retoqueRegion(_region);
                                    for (long long int _i = __i; _i <= __i+nOrder; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i+1][_j-__j])
                                            {
                                                dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i+1][_j-__j];
                                                //bValidElement[_i][_j][_nLayer] = true;
                                            }
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                        }
                        else if (i == i2)
                        {
                            __i--;
                            if (j == j2)
                            {
                                __j--;
                                while (__i >= i1 && __j >= j1
                                    && num(_nLayer, __i, __i+1, __j, __j+nOrder+1) != cnt(_nLayer, __i, __i+1, __j, __j+nOrder+1)
                                    && num(_nLayer, __i, __i+nOrder+1, __j, __j+1) != cnt(_nLayer, __i, __i+nOrder+1, __j, __j+1))
                                {
                                    if (__j > j1)
                                        __j--;
                                    if (__i > i1)
                                        __i--;
                                    nOrder++;
                                }
                                if (__i >= i1 && __j >= j1)
                                {
                                    //Datafile _cache;
                                    //_cache.setCacheStatus(true);
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                                    //_region.vDataArray.resize(nOrder+2);
                                    //_region.vValidationArray.resize(nOrder+2);
                                    for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                    {
                                        //_region.vDataArray[_i-__i].resize(nOrder+2);
                                        //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (!isnan(dCache[_i][_j][_nLayer]))
                                            {
                                                _region.vDataArray[_i-__i][_j-__j] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[_i-__i][_j-__j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i-__i][_j-__j] = NAN;
                                                _region.vValidationArray[_i-__i][_j-__j] = false;
                                            }
                                                //_cache.writeToCache(_i-__i, _j-__j, "cache", dCache[_i][_j][_nLayer]);
                                            if (_i == __i || _j == __j)
                                            {
                                                _region.vDataArray[(_i == __i ? nOrder+1 : _i-__i)][(_j == __j ? nOrder+1 : _j-__j)] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[(_i == __i ? nOrder+1 : _i-__i)][(_j == __j ? nOrder+1 : _j-__j)] = true;
                                            }
                                                //_cache.writeToCache((_i == __i ? _i+nOrder+1 : _i-__i), (_j == __j ? __j+nOrder+1 : _j-__j), "cache", dCache[_i][_j][_nLayer]);
                                        }
                                    }
                                    retoqueRegion(_region);
                                    for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i][_j-__j])
                                            {
                                                dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i][_j-__j];
                                                //bValidElement[_i][_j][_nLayer] = true;
                                            }
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else if (j == j1)
                            {
                                while (__i+nOrder+1 <= i2 && __j+nOrder+1 <= j2
                                    && num(_nLayer, __i+nOrder+1, __i+nOrder+2, __j, __j+nOrder+2) != cnt(_nLayer, __i+nOrder+1, __i+nOrder+2, __j, __j+nOrder+2)
                                    && num(_nLayer, __i, __i+nOrder+2, __j+nOrder+1, __j+nOrder+2) != cnt(_nLayer, __i, __i+nOrder+2, __j+nOrder+1, __j+nOrder+2))
                                {
                                    if (__i > i1)
                                        __i--;
                                    nOrder++;
                                }
                                if (__i+nOrder+1 <= i2 && __j+nOrder+1 <= j2)
                                {
                                    //Datafile _cache;
                                    //_cache.setCacheStatus(true);
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                                    //_region.vDataArray.resize(nOrder+2);
                                    //_region.vValidationArray.resize(nOrder+2);
                                    for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                    {
                                        //_region.vDataArray[_i-__i].resize(nOrder+2);
                                        //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                        for (long long int _j = __j; _j <= __j+nOrder; _j++)
                                        {
                                            if (!isnan(dCache[_i][_j][_nLayer]))
                                            {
                                                _region.vDataArray[_i-__i][_j-__j+1] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[_i-__i][_j-__j+1] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i-__i][_j-__j+1] = NAN;
                                                _region.vValidationArray[_i-__i][_j-__j+1] = false;
                                            }
                                                //_cache.writeToCache(_i-__i, _j-__j+1, "cache", dCache[_i][_j][_nLayer]);
                                            if (_i == __i || _j == __j+nOrder)
                                            {
                                                _region.vDataArray[(_i == __i ? nOrder+1 : _i-__i)][(_j == __j+nOrder ? 0 : _j-__j+1)] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[(_i == __i ? nOrder+1 : _i-__i)][(_j == __j+nOrder ? 0 : _j-__j+1)] = true;
                                            }
                                                //_cache.writeToCache((_i == __i ? _i+nOrder+1 : _i-__i), (_j == __j+nOrder+1 ? 0 : _j-__j+1), "cache", dCache[_i][_j][_nLayer]);
                                        }
                                    }
                                    retoqueRegion(_region);
                                    for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j+nOrder; _j++)
                                        {
                                            if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i][_j-__j+1])
                                            {
                                                dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i][_j-__j+1];
                                                //bValidElement[_i][_j][_nLayer] = true;
                                            }
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                            else
                            {
                                while (__i >= i1 && __j+nOrder+1 <= j2
                                    && num(_nLayer, __i, __i+1, __j, __j+nOrder+2) != cnt(_nLayer, __i, __i+1, __j, __j+nOrder+2)
                                    && num(_nLayer, __i, __i+nOrder+1, __j+nOrder+1, __j+nOrder+2) != cnt(_nLayer, __i, __i+nOrder+1, __j+nOrder+1, __j+nOrder+2))
                                {
                                    nOrder++;
                                    if (__j > j1)
                                        __j--;
                                    if (__i > i1)
                                        __i--;
                                }
                                if (__i >= i1 && __j+nOrder+1 <= j2)
                                {
                                    //Datafile _cache;
                                    //_cache.setCacheStatus(true);
                                    RetoqueRegion _region;
                                    prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder));
                                    //_region.vDataArray.resize(nOrder+2);
                                    //_region.vValidationArray.resize(nOrder+2);
                                    for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                    {
                                        //_region.vDataArray[_i-__i].resize(nOrder+2);
                                        //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (!isnan(dCache[_i][_j][_nLayer]))
                                            {
                                                _region.vDataArray[_i-__i][_j-__j] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[_i-__i][_j-__j] = true;
                                            }
                                            else
                                            {
                                                _region.vDataArray[_i-__i][_j-__j] = NAN;
                                                _region.vValidationArray[_i-__i][_j-__j] = false;
                                            }
                                                //_cache.writeToCache(_i-__i, _j-__j, "cache", dCache[_i][_j][_nLayer]);
                                            if (_i == __i)
                                            {
                                                _region.vDataArray[nOrder+1][_j-__j] = dCache[_i][_j][_nLayer];
                                                _region.vValidationArray[nOrder+1][_j-__j] = true;
                                            }
                                                //_cache.writeToCache(_i+nOrder+1, _j-__j, "cache", dCache[_i][_j][_nLayer]);
                                        }
                                    }
                                    retoqueRegion(_region);
                                    for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                    {
                                        for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                        {
                                            if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i][_j-__j])
                                            {
                                                dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i][_j-__j];
                                                //bValidElement[_i][_j][_nLayer] = true;
                                            }
                                        }
                                    }
                                }
                                else
                                    continue;
                            }
                        }
                        else if (j == j1)
                        {
                            while (__i+nOrder+1 <= i2 && __i >= i1 && __j+nOrder+1 <= j2
                                && num(_nLayer, __i, __i+1, __j, __j+nOrder+2) != cnt(_nLayer, __i, __i+1, __j, __j+nOrder+2)
                                && num(_nLayer, __i, __i+nOrder+2, __j+nOrder+1, __j+nOrder+2) != cnt(_nLayer, __i, __i+nOrder+2, __j+nOrder+1, __j+nOrder+2))
                            {
                                if (__i > i1)
                                    nOrder+=2;
                                else
                                    nOrder++;
                                if (__i > i1)
                                    __i--;
                            }
                            if (__i+nOrder+1 <= i2 && __i >= i1 && __j+nOrder+1 <= j2)
                            {
                                //Datafile _cache;
                                //_cache.setCacheStatus(true);
                                RetoqueRegion _region;
                                prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                                //_region.vDataArray.resize(nOrder+2);
                                //_region.vValidationArray.resize(nOrder+2);
                                for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                {
                                    //_region.vDataArray[_i-__i].resize(nOrder+2);
                                    //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                    for (long long int _j = __j; _j <= __j+nOrder; _j++)
                                    {
                                        if (!isnan(dCache[_i][_j][_nLayer]))
                                        {
                                            _region.vDataArray[_i-__i][_j-__j+1] = dCache[_i][_j][_nLayer];
                                            _region.vValidationArray[_i-__i][_j-__j+1] = true;
                                        }
                                        else
                                        {
                                            _region.vDataArray[_i-__i][_j-__j+1] = NAN;
                                            _region.vValidationArray[_i-__i][_j-__j+1] = false;
                                        }
                                            //_cache.writeToCache(_i-__i, _j-__j+1, "cache", dCache[_i][_j][_nLayer]);
                                        if (_j == __j+nOrder)
                                        {
                                            _region.vDataArray[_i-__i][0] = dCache[_i][_j][_nLayer];
                                            _region.vValidationArray[_i-__i][0] = true;
                                        }
                                            //_cache.writeToCache(_i-__i, 0, "cache", dCache[_i][_j][_nLayer]);
                                    }
                                }
                                retoqueRegion(_region);
                                for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                {
                                    for (long long int _j = __j; _j <= __j+nOrder; _j++)
                                    {
                                        if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i][_j-__j+1])
                                        {
                                            dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i][_j-__j+1];
                                            //bValidElement[_i][_j][_nLayer] = true;
                                        }
                                    }
                                }
                            }
                            else
                                continue;
                        }
                        else
                        {
                            __j--;
                            while (__i+nOrder+1 <= i2 && __i >= i1 && __j >= j1
                                && num(_nLayer, __i, __i+1, __j, __j+nOrder+1) != cnt(_nLayer, __i, __i+1, __j, __j+nOrder+1)
                                && num(_nLayer, __i, __i+nOrder+1, __j, __j+1) != cnt(_nLayer, __i, __i+nOrder+1, __j, __j+1))
                            {
                                nOrder++;
                                if (__j > j1)
                                    __j--;
                                if (__i > i1)
                                    __i--;
                            }
                            if (__i+nOrder+1 <= i2 && __i >= i1 && __j-nOrder-1 >= j1)
                            {
                                //Datafile _cache;
                                //_cache.setCacheStatus(true);
                                RetoqueRegion _region;
                                prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                                //_region.vDataArray.resize(nOrder+2);
                                //_region.vValidationArray.resize(nOrder+2);
                                for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                {
                                    //_region.vDataArray[_i-__i].resize(nOrder+2);
                                    //_region.vValidationArray[_i-__i].resize(nOrder+2);
                                    for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                    {
                                        if (!isnan(dCache[_i][_j][_nLayer]))
                                        {
                                            _region.vDataArray[_i-__i][_j-__j] = dCache[_i][_j][_nLayer];
                                            _region.vValidationArray[_i-__i][_j-__j] = true;
                                        }
                                        else
                                        {
                                            _region.vDataArray[_i-__i][_j-__j] = NAN;
                                            _region.vValidationArray[_i-__i][_j-__j] = false;
                                        }
                                            //_cache.writeToCache(_i-__i, _j-__j, "cache", dCache[_i][_j][_nLayer]);
                                        if (_j == __j)
                                        {
                                            _region.vDataArray[_i-__i][nOrder+1] = dCache[_i][_j][_nLayer];
                                            _region.vValidationArray[_i-__i][nOrder+1] = true;
                                        }
                                            //_cache.writeToCache(_i-__i, __j+nOrder+1, "cache", dCache[_i][_j][_nLayer]);
                                    }
                                }
                                retoqueRegion(_region);
                                for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                                {
                                    for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                                    {
                                        if (isnan(dCache[_i][_j][_nLayer]) && _region.vValidationArray[_i-__i][_j-__j])
                                        {
                                            dCache[_i][_j][_nLayer] = _region.vDataArray[_i-__i][_j-__j];
                                            //bValidElement[_i][_j][_nLayer] = true;
                                        }
                                    }
                                }
                            }
                            else
                                continue;
                        }
                    }
                    else
                    {
                        unsigned int nOrder = 1;
                        long long int __i = i-1;
                        long long int __j = j-1;
                        while (!isValidDisc(__i,__j,_nLayer, nOrder+1))
                        {
                            for (long long int _i = __i; _i <= __i+nOrder+1; _i++)
                            {
                                if (isnan(dCache[_i][__j][_nLayer]))
                                {
                                    __j--;
                                    break;
                                }
                                if (isnan(dCache[_i][__j+nOrder+1][_nLayer]))
                                {
                                    nOrder++;
                                    break;
                                }
                            }
                            if (__i < i1 || __i+nOrder+1 > i2 || __j < j1 || __j+nOrder+1 > j2)
                                break;
                            for (long long int _j = __j; _j <= __j+nOrder+1; _j++)
                            {
                                if (isnan(dCache[__i][_j][_nLayer]))
                                {
                                    __i--;
                                    break;
                                }
                                if (isnan(dCache[__i+nOrder+1][_j][_nLayer]))
                                {
                                    nOrder++;
                                    break;
                                }
                            }
                            if (__i < i1 || __i+nOrder+1 > i2 || __j < j1 || __j+nOrder+1 > j2)
                                break;
                        }
                        if (__i < i1 || __i+nOrder+1 > i2 || __j < j1 || __j+nOrder+1 > j2)
                            continue;
                        //Datafile _cache;
                        //_cache.setCacheStatus(true);
                        RetoqueRegion _region;
                        prepareRegion(_region, nOrder+2, med(_nLayer, __i, __i+nOrder+1, __j, __j+nOrder+1));
                        //_region.vDataArray.resize(nOrder+2);
                        //_region.vValidationArray.resize(nOrder+2);
                        for (long long int k = __i; k <= __i+nOrder+1; k++)
                        {
                            //_region.vDataArray[k-__i].resize(nOrder+2);
                            //_region.vValidationArray[k-__i].resize(nOrder+2);
                            for (long long int l = __j; l <= __j+nOrder+1; l++)
                            {
                                if (!isnan(dCache[k][l][_nLayer]))
                                {
                                    _region.vDataArray[k-__i][l-__j] = dCache[k][l][_nLayer];
                                    _region.vValidationArray[k-__i][l-__j] = true;
                                }
                                else
                                {
                                    _region.vDataArray[k-__i][l-__j] = NAN;
                                    _region.vValidationArray[k-__i][l-__j] = false;
                                }
                                    //_cache.writeToCache(k-__i, l-__j, "cache", dCache[k][l][_nLayer]);
                            }
                        }
                        retoqueRegion(_region);
                        for (long long int k = __i; k <= __i+nOrder+1; k++)
                        {
                            for (long long int l = __j; l <= __j+nOrder+1; l++)
                            {
                                if (isnan(dCache[k][l][_nLayer]) && _region.vValidationArray[k-__i][l-__j])
                                {
                                    dCache[k][l][_nLayer] = _region.vDataArray[k-__i][l-__j];
                                    //bValidElement[k][l][_nLayer] = true;
                                }
                            }
                        }

                    }
                    if (bIsSaved)
                    {
                        bIsSaved = false;
                        nLastSaved = time(0);
                    }
                }
            }
        }
    }
    else if (Direction == LINES)
    {
        for (long long int i = i1; i <= i2; i++)
        {
            for (long long int j = j1; j <= j2; j++)
            {
                if (isnan(dCache[i][j][_nLayer]))
                {
                    for (long long int _j = j; _j <= j2; _j++)
                    {
                        if (!isnan(dCache[i][_j][_nLayer]))
                        {
                            if (j != j1)
                            {
                                for (long long int __j = j; __j < _j; __j++)
                                {
                                    //bValidElement[i][__j][_nLayer] = true;
                                    dCache[i][__j][_nLayer] = (dCache[i][_j][_nLayer]-dCache[i][j-1][_nLayer])/(double)(_j-j)*(double)(__j-j+1) + dCache[i][j-1][_nLayer];
                                }
                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }
                                break;
                            }
                            else if (j == j1 && _j != j2)
                            {
                                for (long long int __j = j; __j < _j; __j++)
                                {
                                    //bValidElement[i][__j][_nLayer] = true;
                                    dCache[i][__j][_nLayer] = dCache[i][_j][_nLayer];
                                }
                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }
                                break;
                            }
                        }
                        if (j != j1 && _j == j2 && isnan(dCache[i][_j][_nLayer]))
                        {
                            for (long long int __j = j; __j <= j2; __j++)
                            {
                                //bValidElement[i][__j][_nLayer] = true;
                                dCache[i][__j][_nLayer] = dCache[i][j-1][_nLayer];
                            }
                            if (bIsSaved)
                            {
                                bIsSaved = false;
                                nLastSaved = time(0);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    else if (Direction == COLS)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            for (long long int i = i1; i <= i2; i++)
            {
                if (isnan(dCache[i][j][_nLayer]))
                {
                    for (long long int _i = i; _i <= i2; _i++)
                    {
                        if (!isnan(dCache[_i][j][_nLayer]))
                        {
                            if (i != i1)
                            {
                                for (long long int __i = i; __i < _i; __i++)
                                {
                                    //bValidElement[__i][j][_nLayer] = true;
                                    dCache[__i][j][_nLayer] = (dCache[_i][j][_nLayer]-dCache[i-1][j][_nLayer])/(double)(_i-i)*(double)(__i-i+1) + dCache[i-1][j][_nLayer];
                                }
                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }
                                break;
                            }
                            else if (i == i1 && _i != i2)
                            {
                                for (long long int __i = i; __i < _i; __i++)
                                {
                                    //bValidElement[__i][j][_nLayer] = true;
                                    dCache[__i][j][_nLayer] = dCache[_i][j][_nLayer];
                                }
                                if (bIsSaved)
                                {
                                    bIsSaved = false;
                                    nLastSaved = time(0);
                                }
                                break;
                            }
                        }
                        if (i != i1 && _i == i2 && isnan(dCache[_i][j][_nLayer]))
                        {
                            for (long long int __i = i; __i <= i2; __i++)
                            {
                                //bValidElement[__i][j][_nLayer] = true;
                                dCache[__i][j][_nLayer] = dCache[i-1][j][_nLayer];
                            }
                            if (bIsSaved)
                            {
                                bIsSaved = false;
                                nLastSaved = time(0);
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool Cache::retoqueRegion(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nOrder, AppDir Direction)
{
    bool bUseAppendedZeroes = false;
    if (!bValidData)
        return false;
    if (nOrder < 1)
        return false;
    if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
        return false;
    if (i2 == -1)
        i2 = i1;
    else if (i2 == -2)
    {
        i2 = getCacheLines(_nLayer, true)-1;
        bUseAppendedZeroes = true;
    }
    if (j2 == -1)
        j2 = j1;
    else if (j2 == -2)
        j2 = getCacheCols(_nLayer, false)-1;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;
        for (long long int j = j1; j <= j2; j++)
        {
            if (nMax < nLines - nAppendedZeroes[_nLayer][j]-1)
                nMax = nLines - nAppendedZeroes[_nLayer][j]-1;
        }
        if (i2 > nMax)
            i2 = nMax;
    }

    if ((Direction == ALL || Direction == GRID) && nOrder > 1)
    {
        if (bUseAppendedZeroes)
        {
            Cache::smooth(_nLayer, i1, -2, j1, j1, nOrder, COLS);
            Cache::smooth(_nLayer, i1, -2, j2, j2, nOrder, COLS);
            Cache::smooth(_nLayer, i1, i1, j1, j2, nOrder, LINES);
            Cache::smooth(_nLayer, i2, i2, j1, j2, nOrder, LINES);
        }
        else
        {
            Cache::smooth(_nLayer, i1, i2, j1, j1, nOrder, COLS);
            Cache::smooth(_nLayer, i1, i2, j2, j2, nOrder, COLS);
            Cache::smooth(_nLayer, i1, i1, j1, j2, nOrder, LINES);
            Cache::smooth(_nLayer, i2, i2, j1, j2, nOrder, LINES);
        }
    }

    //cerr << i1 << " " << i2 << " " << j1 << " " << j2 << " " << nOrder << endl;
    if (Direction == LINES)
    {
        for (long long int i = i1; i <= i2; i++)
        {
            for (long long int j = j1+1; j <= j2-nOrder; j++)
            {
                for (unsigned int n = 0; n < nOrder; n++)
                {
                    if (!isnan(dCache[i][j-1][_nLayer]) && !isnan(dCache[i][j+nOrder][_nLayer]) && !isnan(dCache[i][j+n][_nLayer]))
                        dCache[i][j+n][_nLayer] = 0.5 * dCache[i][j+n][_nLayer] + 0.5 * (dCache[i][j-1][_nLayer] + (dCache[i][j+nOrder][_nLayer] - dCache[i][j-1][_nLayer])/(double)(nOrder+1)*(double)(n+1));
                }
            }
        }
    }
    else if (Direction == COLS)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            for (long long int i = i1+1; i <= i2-nOrder; i++)
            {
                for (unsigned int n = 0; n < nOrder; n++)
                {
                    if (!isnan(dCache[i-1][j][_nLayer]) && !isnan(dCache[i+nOrder][j][_nLayer]) && !isnan(dCache[i+n][j][_nLayer]))
                        dCache[i+n][j][_nLayer] = 0.5 * dCache[i+n][j][_nLayer] + 0.5 * (dCache[i-1][j][_nLayer] + (dCache[i+nOrder][j][_nLayer] - dCache[i-1][j][_nLayer])/(double)(nOrder+1)*(double)(n+1));
                }
            }
        }
    }
    else if ((Direction == ALL || Direction == GRID) && i2-i1 > 1 && j2-j1 > 1)
    {
        for (long long int j = j1; j <= j2-nOrder-1; j++)
        {
            for (long long int i = i1; i <= i2-nOrder-1; i++)
            {
                for (unsigned int nj = 1; nj <= nOrder; nj++)
                {
                    for (unsigned int ni = 1; ni <= nOrder; ni++) // nOrder-nj+1: Dreieckig glaetten => weniger Glaettungen je Punkt
                    {
                        if (nOrder == 1)
                        {
                            if (!isnan(dCache[i+ni][j+nOrder+1][_nLayer])
                                && !isnan(dCache[i+ni][j][_nLayer])
                                && !isnan(dCache[i+nOrder+1][j+nj][_nLayer])
                                && !isnan(dCache[i][j+nj][_nLayer]))
                            {
                                //cerr << dCache[i+ni][j+nj][_nLayer];
                                dCache[i+ni][j+nj][_nLayer] = 0.5*med(_nLayer, i1, i2+1, j1, j2+1) + 0.25*(
                                    dCache[i][j+nj][_nLayer]+(dCache[i+nOrder+1][j+nj][_nLayer]-dCache[i][j+nj][_nLayer])/(double)(nOrder+1)*(double)ni
                                    +dCache[i+ni][j][_nLayer]+(dCache[i+ni][j+nOrder+1][_nLayer]-dCache[i+ni][j][_nLayer])/(double)(nOrder+1)*(double)nj);
//                                dCache[i+ni][j+nj][_nLayer] = 0.5*(
//                                    dCache[i][j+nj][_nLayer]+(dCache[i+nOrder+1][j+nj][_nLayer]-dCache[i][j+nj][_nLayer])/(double)(nOrder+1)*(double)ni
//                                    +dCache[i+ni][j][_nLayer]+(dCache[i+ni][j+nOrder+1][_nLayer]-dCache[i+ni][j][_nLayer])/(double)(nOrder+1)*(double)nj);
                                /*if (!isnan(dCache[i+ni][j+nj][_nLayer]) && !isinf(dCache[i+ni][j+nj][_nLayer]))
                                    bValidElement[i+ni][j+nj][_nLayer] = true;*/
                                //cerr << " / " << dCache[i+ni][j+nj][_nLayer] << endl;
                            }
                        }
                        else
                        {
                            if (isValidDisc(i, j, _nLayer, nOrder+1))
                            {
                                double dAverage = dCache[i][j+nj][_nLayer]
                                        +(dCache[i+nOrder+1][j+nj][_nLayer]-dCache[i][j+nj][_nLayer])/(double)(nOrder+1)*(double)ni
                                    +dCache[i+ni][j][_nLayer]
                                        +(dCache[i+ni][j+nOrder+1][_nLayer]-dCache[i+ni][j][_nLayer])/(double)(nOrder+1)*(double)nj;
                                dAverage *= 2;
                                if (ni >= nj)
                                {
                                    dAverage += dCache[i][j+(ni-nj)][_nLayer]
                                        +(dCache[i+nOrder+1-(ni-nj)][j+nOrder+1][_nLayer]-dCache[i][j+(ni-nj)][_nLayer])/(double)(nOrder-(ni-nj)+1)*(double)ni;
                                }
                                else
                                {
                                    dAverage += dCache[i][j+(nj-ni)][_nLayer]
                                        +(dCache[i+nOrder+1][j+nOrder+1-(nj-ni)][_nLayer]-dCache[i+(nj-ni)][j][_nLayer])/(double)(nOrder-(nj-ni)+1)*(double)ni;
                                }
                                if (ni + nj <= nOrder+1)
                                {
                                    dAverage += dCache[i+ni+nj][j][_nLayer]
                                        +(dCache[i][j+ni+nj][_nLayer]-dCache[i+ni+nj][j][_nLayer])/(double)(ni+nj)*(double)(nj);
                                }
                                else
                                {
                                    dAverage += dCache[i+nOrder+1][j+(ni+nj-nOrder-1)][_nLayer]
                                        +(dCache[i+(ni+nj-nOrder-1)][j+nOrder+1][_nLayer]-dCache[i+nOrder+1][j+(ni+nj-nOrder-1)][_nLayer])/(double)(2*nOrder+2-(ni+nj))*(double)(nj-(ni+nj-nOrder-1));
                                }
                                dAverage /= 6.0;
                                if (!isnan(dCache[i+ni][j+nj][_nLayer]))
                                {
                                    //dCache[i+ni][j+nj][_nLayer] = 0.5*dCache[i+ni][j+nj][_nLayer] + 0.5*dAverage;
                                    if (nOrder % 2)
                                    {
                                        dCache[i+ni][j+nj][_nLayer] =
                                            0.5*(1.0-0.5*hypot(ni-(nOrder+1)/2,nj-(nOrder+1)/2)/hypot(1-(nOrder+1)/2,1-(nOrder+1)/2))
                                                *dCache[i+ni][j+nj][_nLayer]
                                            + 0.5*(1.0+0.5*hypot(ni-(nOrder+1)/2,nj-(nOrder+1)/2)/hypot(1-(nOrder+1)/2,1-(nOrder+1)/2))*dAverage;
                                    }
                                    else
                                    {
                                        dCache[i+ni][j+nj][_nLayer] =
                                            0.5*(1.0-0.5*hypot(ni-(nOrder)/2,nj-(nOrder)/2)/hypot(nOrder/2,nOrder/2))
                                                *dCache[i+ni][j+nj][_nLayer]
                                            + 0.5*(1.0+0.5*hypot(ni-(nOrder)/2,nj-(nOrder)/2)/hypot(nOrder/2,nOrder/2))*dAverage;
                                    }
                                }
                                else
                                {
                                    dCache[i+ni][j+nj][_nLayer] = dAverage;
                                    //bValidElement[i+ni][j+nj][_nLayer] = true;
                                }
                            }
                        }
                    }
                }
                i += nOrder / 2;
            }
            j += nOrder / 2;
        }
    }
    return true;
}

bool Cache::retoqueRegion(RetoqueRegion& _region)
{
    int nOrder = _region.vDataArray.size()-2;
    for (unsigned int i = 0; i < _region.vDataArray.size(); i++)
    {
        for (unsigned int j = 0; j < _region.vDataArray.size(); j++)
        {
            if (!_region.vValidationArray[i][j])
            {
                double dAverage = _region.vDataArray[0][j]
                        +(_region.vDataArray[nOrder+1][j]-_region.vDataArray[0][j])/(double)(nOrder+1)*(double)i
                    +_region.vDataArray[i][0]
                        +(_region.vDataArray[i][nOrder+1]-_region.vDataArray[i][0])/(double)(nOrder+1)*(double)j;
                dAverage *= 2.0;
                if (i >= j)
                {
                    dAverage += _region.vDataArray[0][(i-j)]
                        +(_region.vDataArray[nOrder+1-(i-j)][nOrder+1]-_region.vDataArray[0][(i-j)])/(double)(nOrder-(i-j)+1)*(double)i;
                }
                else
                {
                    dAverage += _region.vDataArray[0][(j-i)]
                        +(_region.vDataArray[nOrder+1][nOrder+1-(j-i)]-_region.vDataArray[(j-i)][0])/(double)(nOrder-(j-i)+1)*(double)i;
                }
                if (i + j <= (unsigned)nOrder+1)
                {
                    dAverage += _region.vDataArray[i+j][0]
                        +(_region.vDataArray[0][i+j]-_region.vDataArray[i+j][0])/(double)(i+j)*(double)j;
                }
                else
                {
                    dAverage += _region.vDataArray[nOrder+1][(i+j-nOrder-1)]
                        +(_region.vDataArray[(i+j-nOrder-1)][nOrder+1]-_region.vDataArray[nOrder+1][(i+j-nOrder-1)])/(double)(2*nOrder+2-(i+j))*(double)(j-(i+j-nOrder-1));
                }
                dAverage /= 6.0;

                if (isnan(_region.dMedian))
                    _region.vDataArray[i][j] = dAverage;
                else
                {
                    _region.vDataArray[i][j] =
                        0.5*(1.0-0.5*hypot(i-(nOrder)/2.0,j-(nOrder)/2.0)/(M_SQRT2*(nOrder/2.0)))
                            *_region.dMedian
                        + 0.5*(1.0+0.5*hypot(i-(nOrder)/2.0,j-(nOrder)/2.0)/(M_SQRT2*(nOrder/2.0)))*dAverage;
                }
                if (!isnan(dAverage) && !isinf(dAverage) && !isnan(_region.vDataArray[i][j]) && !isinf(_region.vDataArray[i][j]))
                    _region.vValidationArray[i][j] = true;
                else
                    _region.vValidationArray[i][j] = false;
            }
        }
    }
    return true;
}

bool Cache::smooth(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nOrder, AppDir Direction)
{
    return smooth(mCachesMap.at(_sCache), i1, i2, j1, j2, nOrder, Direction);
}

bool Cache::smooth(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nOrder, AppDir Direction)
{
    bool bUseAppendedZeroes = false;
    if (!bValidData)
        return false; //Cols == 2
    if (nOrder < 1 || (nOrder >= nLines && Direction == COLS) || (nOrder >= nCols && Direction == LINES) || ((nOrder >= nLines || nOrder >= nCols) && !(Direction == ALL || Direction == GRID)))
        return false;
    if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
        return false;
    if (i2 == -1)
        i2 = i1;
    else if (i2 == -2)
    {
        i2 = getCacheLines(_nLayer, true)-1;
        bUseAppendedZeroes = true;
    }
    if (j2 == -1)
        j2 = j1;
    else if (j2 == -2)
        j2 = getCacheCols(_nLayer, false)-1;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return NAN;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;


    if ((Direction == ALL || Direction == GRID) && i2 - i1 < 3)
        Direction = LINES;
    if ((Direction == ALL || Direction == GRID) && j2 - j1 < 3)
        Direction = COLS;

    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;
        for (long long int j = j1; j <= j2; j++)
        {
            if (nMax < nLines - nAppendedZeroes[_nLayer][j]-1)
                nMax = nLines - nAppendedZeroes[_nLayer][j]-1;
        }
        if (i2 > nMax)
            i2 = nMax;
    }


    if (Direction == GRID)
    {
        if (bUseAppendedZeroes)
        {
            if (!smooth(_nLayer, i1, -2, j1, -1, nOrder, COLS) || !smooth(_nLayer, i1, -2, j1+1, -1, nOrder, COLS))
                return false;
        }
        else
        {
            if (!smooth(_nLayer, i1, i2, j1, j1+1, nOrder, COLS))
                return false;
        }
        j1 += 2;
    }

    if (Direction == ALL || Direction == GRID)
    {
        Cache::retoque(_nLayer, i1, i2+1, j1, j2+1, ALL);
        Cache::smooth(_nLayer, i1, i2, j1, j1, nOrder, COLS);
        Cache::smooth(_nLayer, i1, i2, j2, j2, nOrder, COLS);
        Cache::smooth(_nLayer, i1, i1, j1, j2, nOrder, LINES);
        Cache::smooth(_nLayer, i2, i2, j1, j2, nOrder, LINES);
    }

    //cerr << i1 << " " << i2 << " " << j1 << " " << j2 << " " << nOrder << endl;
    if (Direction == LINES)
    {
        for (long long int i = i1; i <= i2; i++)
        {
            for (long long int j = j1+1; j <= j2-nOrder; j++)
            {
                for (unsigned int n = 0; n < nOrder; n++)
                {
                    if (!isnan(dCache[i][j-1][_nLayer]) && !isnan(dCache[i][j+nOrder][_nLayer]) && !isnan(dCache[i][j+n][_nLayer]))
                        dCache[i][j+n][_nLayer] = 0.5 * dCache[i][j+n][_nLayer] + 0.5 * (dCache[i][j-1][_nLayer] + (dCache[i][j+nOrder][_nLayer] - dCache[i][j-1][_nLayer])/(double)(nOrder+1)*(double)(n+1));
                }
            }
        }
    }
    else if (Direction == COLS)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            for (long long int i = i1+1; i <= i2-nOrder; i++)
            {
                for (unsigned int n = 0; n < nOrder; n++)
                {
                    if (!isnan(dCache[i-1][j][_nLayer]) && !isnan(dCache[i+nOrder][j][_nLayer]) && !isnan(dCache[i+n][j][_nLayer]))
                        dCache[i+n][j][_nLayer] = 0.5 * dCache[i+n][j][_nLayer] + 0.5 * (dCache[i-1][j][_nLayer] + (dCache[i+nOrder][j][_nLayer] - dCache[i-1][j][_nLayer])/(double)(nOrder+1)*(double)(n+1));
                }
            }
        }
    }
    else if ((Direction == ALL || Direction == GRID) && i2-i1 > 1 && j2-j1 > 1)
    {
        for (long long int j = j1; j <= j2-nOrder-1; j++)
        {
            for (long long int i = i1; i <= i2-nOrder-1; i++)
            {
                for (unsigned int nj = 1; nj <= nOrder; nj++)
                {
                    for (unsigned int ni = 1; ni <= nOrder; ni++) // nOrder-nj+1: Dreieckig glaetten => weniger Glaettungen je Punkt
                    {
                        if (nOrder == 1)
                        {
                            if (!isnan(dCache[i+ni][j+nOrder+1][_nLayer])
                                && !isnan(dCache[i+ni][j][_nLayer])
                                && !isnan(dCache[i+nOrder+1][j+nj][_nLayer])
                                && !isnan(dCache[i][j+nj][_nLayer]))
                            {
                                //cerr << dCache[i+ni][j+nj][_nLayer];
                                if (!isnan(dCache[i+ni][j+nj][_nLayer]))
                                    dCache[i+ni][j+nj][_nLayer] = 0.5*dCache[i+ni][j+nj][_nLayer] + 0.25*(
                                        dCache[i][j+nj][_nLayer]+(dCache[i+nOrder+1][j+nj][_nLayer]-dCache[i][j+nj][_nLayer])/(double)(nOrder+1)*(double)ni
                                        +dCache[i+ni][j][_nLayer]+(dCache[i+ni][j+nOrder+1][_nLayer]-dCache[i+ni][j][_nLayer])/(double)(nOrder+1)*(double)nj);
                                else
                                {
                                    dCache[i+ni][j+nj][_nLayer] = 0.5*(
                                        dCache[i][j+nj][_nLayer]+(dCache[i+nOrder+1][j+nj][_nLayer]-dCache[i][j+nj][_nLayer])/(double)(nOrder+1)*(double)ni
                                        +dCache[i+ni][j][_nLayer]+(dCache[i+ni][j+nOrder+1][_nLayer]-dCache[i+ni][j][_nLayer])/(double)(nOrder+1)*(double)nj);
                                    //bValidElement[i+ni][j+nj][_nLayer] = true;
                                }
                                //cerr << " / " << dCache[i+ni][j+nj][_nLayer] << endl;
                            }
                        }
                        else
                        {
                            if (isValidDisc(i, j, _nLayer, nOrder+1))
                            {
                                double dAverage = dCache[i][j+nj][_nLayer]
                                        +(dCache[i+nOrder+1][j+nj][_nLayer]-dCache[i][j+nj][_nLayer])/(double)(nOrder+1)*(double)ni
                                    +dCache[i+ni][j][_nLayer]
                                        +(dCache[i+ni][j+nOrder+1][_nLayer]-dCache[i+ni][j][_nLayer])/(double)(nOrder+1)*(double)nj;
                                dAverage *= 2.0;
                                if (ni >= nj)
                                {
                                    dAverage += dCache[i][j+(ni-nj)][_nLayer]
                                        +(dCache[i+nOrder+1-(ni-nj)][j+nOrder+1][_nLayer]-dCache[i][j+(ni-nj)][_nLayer])/(double)(nOrder-(ni-nj)+1)*(double)ni;
                                }
                                else
                                {
                                    dAverage += dCache[i][j+(nj-ni)][_nLayer]
                                        +(dCache[i+nOrder+1][j+nOrder+1-(nj-ni)][_nLayer]-dCache[i+(nj-ni)][j][_nLayer])/(double)(nOrder-(nj-ni)+1)*(double)ni;
                                }
                                if (ni + nj <= nOrder+1)
                                {
                                    dAverage += dCache[i+ni+nj][j][_nLayer]
                                        +(dCache[i][j+ni+nj][_nLayer]-dCache[i+ni+nj][j][_nLayer])/(double)(ni+nj)*(double)(nj);
                                }
                                else
                                {
                                    dAverage += dCache[i+nOrder+1][j+(ni+nj-nOrder-1)][_nLayer]
                                        +(dCache[i+(ni+nj-nOrder-1)][j+nOrder+1][_nLayer]-dCache[i+nOrder+1][j+(ni+nj-nOrder-1)][_nLayer])/(double)(2*nOrder+2-(ni+nj))*(double)(nj-(ni+nj-nOrder-1));
                                }
                                dAverage /= 6.0;
                                if (!isnan(dCache[i+ni][j+nj][_nLayer]))
                                {
                                    //dCache[i+ni][j+nj][_nLayer] = 0.5*dCache[i+ni][j+nj][_nLayer] + 0.5*dAverage;
//                                    if (nOrder % 2)
//                                    {
//                                        dCache[i+ni][j+nj][_nLayer] =
//                                            0.5*(1.0-0.5*hypot(ni-(nOrder+1)/2,nj-(nOrder+1)/2)/(M_SQRT2*((nOrder+1)/2)))//hypot(1-(nOrder+1)/2,1-(nOrder+1)/2))
//                                                *dCache[i+ni][j+nj][_nLayer]
//                                            + 0.5*(1.0+0.5*hypot(ni-(nOrder+1)/2,nj-(nOrder+1)/2)/(M_SQRT2*(nOrder/2)))*dAverage;//hypot(1-(nOrder+1)/2,1-(nOrder+1)/2))*dAverage;
//                                    }
//                                    else
//                                    {
                                        dCache[i+ni][j+nj][_nLayer] =
                                            0.5*(1.0-0.5*hypot(ni-(nOrder)/2.0,nj-(nOrder)/2.0)/(M_SQRT2*((nOrder)/2.0)))//hypot(nOrder/2,nOrder/2))
                                                *dCache[i+ni][j+nj][_nLayer]
                                            + 0.5*(1.0+0.5*hypot(ni-(nOrder)/2.0,nj-(nOrder)/2.0)/(M_SQRT2*(nOrder/2.0)))*dAverage;//hypot(nOrder/2,nOrder/2))*dAverage;
//                                    }
                                }
                                else
                                {
                                    dCache[i+ni][j+nj][_nLayer] = dAverage;
                                    //bValidElement[i+ni][j+nj][_nLayer] = true;
                                }
                            }
                        }
                    }
                }
                i += nOrder / 2;
            }
            j += nOrder / 2;
        }
    }
    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }
    return true;
}

bool Cache::resample(const string& _sCache, long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nSamples, AppDir Direction)
{
    return resample(mCachesMap.at(_sCache), i1, i2, j1, j2, nSamples, Direction);
}

bool Cache::resample(long long int _nLayer, long long int i1, long long int i2, long long int j1, long long int j2, unsigned int nSamples, AppDir Direction)
{
    //cerr << i1 << " " << i2 << " " << j1 << " " << j2 << " " << endl;
    bool bUseAppendedZeroes = false;
    //vector<double> vTemp;
    const long long int __nLines = nLines;
    const long long int __nCols = nCols;

    if (!bValidData)
        return false;
    /*if (nSamples == getCacheLines(_nLayer, false))
        return true;*/
    if (!nSamples)
        return false;
    if (i1 == -1 && i2 == -1 && j1 == -1 && j2 == -1)
        return false;
    if (i2 == -1)
        i2 = i1;
    else if (i2 == -2)
    {
        i2 = getCacheLines(_nLayer, false)-1;
        bUseAppendedZeroes = true;
    }
    if (j2 == -1)
        j2 = j1;
    else if (j2 == -2)
        j2 = getCacheCols(_nLayer, false)-1;

    if (i1 > i2)
    {
        long long int nTemp = i1;
        i1 = i2;
        i2 = nTemp;
    }
    if (j1 > j2)
    {
        long long int nTemp = j1;
        j1 = j2;
        j2 = nTemp;
    }
    if (i1 >= getCacheLines(_nLayer, false) || j1 >= getCacheCols(_nLayer, false))
        return false;
    if (i2 >= getCacheLines(_nLayer, false))
        i2 = getCacheLines(_nLayer, false)-1;
    if (j2 >= getCacheCols(_nLayer, false))
        j2 = getCacheCols(_nLayer, false)-1;

    if (bUseAppendedZeroes)
    {
        long long int nMax = 0;
        for (long long int j = j1; j <= j2; j++)
        {
            if (nMax < nLines - nAppendedZeroes[_nLayer][j]-1)
                nMax = nLines - nAppendedZeroes[_nLayer][j]-1;
        }
        if (i2 > nMax)
            i2 = nMax;
    }
    if (Direction == GRID)
    {
        if (j2-j1-2 != i2-i1 && !bUseAppendedZeroes)
            return false;
        else if (j2-j1-2 != (nLines-nAppendedZeroes[_nLayer][j1+1]-1)-i1 && bUseAppendedZeroes)
            return false;
    }
    //cerr << i2 << endl;
    //cerr << i2-i1 << endl;
    //cerr << __nLines << " " << __nCols << endl;
    //double dTemp[__nLines][__nCols];
    double** dTemp = new double*[__nLines];
    for (long long int i = 0; i < __nLines; i++)
        dTemp[i] = new double[__nCols];
    //cerr << "dTemp" << endl;
    Resampler* _resampler = 0;
    //cerr << "_resampler-pointer" << endl;
    if (Direction == ALL || Direction == GRID) // 2D
    {
        if (Direction == GRID)
        {
            if (bUseAppendedZeroes)
            {
                if (!resample(_nLayer, i1, -2, j1, -1, nSamples, COLS) || !resample(_nLayer, i1, -2, j1+1, -1, nSamples, COLS))
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dTemp[i];
                    delete[] dTemp;
                    return false;
                }
            }
            else
            {
                if (!resample(_nLayer, i1, i2, j1, j1+1, nSamples, COLS)) // Achsenwerte getrennt resamplen
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dTemp[i];
                    delete[] dTemp;
                    return false;
                }
            }
            j1 += 2;
        }
        if (bUseAppendedZeroes)
        {
            long long int nMax = 0;
            for (long long int j = j1; j <= j2; j++)
            {
                if (nMax < nLines - nAppendedZeroes[_nLayer][j]-1)
                    nMax = nLines - nAppendedZeroes[_nLayer][j]-1;
            }
            if (i2 > nMax)
                i2 = nMax;
        }
        //cerr << "initializing grid resampler: " << j2-j1+1 << " " << i2-i1+1 << " " << nSamples << endl;
        _resampler = new Resampler(j2-j1+1, i2-i1+1, nSamples, nSamples, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
        if (nSamples > i2-i1+1 || nSamples > j2-j1+1)
            resizeCache(nLines+nSamples-(i2-i1+1), nCols+nSamples-(j2-j1+1), -1);
    }
    else if (Direction == COLS) // cols
    {
        //cerr << "initializing cols resampler: " << j2-j1+1 << " " << i2-i1+1 << " " << nSamples << endl;
        _resampler = new Resampler(j2-j1+1, i2-i1+1, j2-j1+1, nSamples, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
        if (nSamples > i2-i1+1)
            resizeCache(nLines+nSamples-(i2-i1+1), nCols-1, -1);
    }
    else if (Direction == LINES)// lines
    {
        _resampler = new Resampler(j2-j1+1, i2-i1+1, nSamples, i2-i1+1, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
        if (nSamples > j2-j1+1)
            resizeCache(nLines-1, nCols+nSamples-(j2-j1+1), -1);
    }

    if (!_resampler)
    {
        for (long long int i = 0; i < __nLines; i++)
            delete[] dTemp[i];
        delete[] dTemp;
        return false;
    }
    //cerr << "_resampler initialized" << endl;

    const double* dOutputSamples = 0;
    double* dInputSamples = new double[j2-j1+1];

    for (long long int i = i1; i <= i2; i++)
    {
        for (long long int j = j1; j <= j2; j++)
        {
            dInputSamples[j-j1] = dCache[i][j][_nLayer];
        }
        if (!_resampler->put_line(dInputSamples))
        {
            //cerr << "STATUS: " << _resampler->status() << endl;
            delete _resampler;
            for (long long int i = 0; i < __nLines; i++)
                delete[] dTemp[i];
            delete[] dTemp;
            return false;
        }
    }
    delete[] dInputSamples;


    for (long long int i = 0; i < __nLines; i++)
    {
        for (long long int j = 0; j < __nCols; j++)
        {
            dTemp[i][j] = dCache[i][j][_nLayer];
        }
    }

    long long int _ret_line = 0;
    long long int _final_cols = 0;
    if (Direction == ALL || Direction == GRID || Direction == LINES)
        _final_cols = nSamples;
    else
        _final_cols = j2-j1+1;

    while (true)
    {
        dOutputSamples = _resampler->get_line();
        if (!dOutputSamples)
            break;
        for (long long int _fin = 0; _fin < _final_cols; _fin++)
        {
            //cerr << dOutputSamples[_fin] << " ";
            if (isnan(dOutputSamples[_fin]))
            {
                dCache[i1+_ret_line][j1+_fin][_nLayer] = NAN;
                //bValidElement[i1+_ret_line][j1+_fin][_nLayer] = false;
                continue;
            }
            dCache[i1+_ret_line][j1+_fin][_nLayer] = dOutputSamples[_fin];
            //bValidElement[i1+_ret_line][j1+_fin][_nLayer] = true;
        }
        //cerr << endl;
        _ret_line++;

    }
    _ret_line++;
    delete _resampler;
    //cerr << "_resampler deleted" << endl;
    // Block unter dem resampleten kopieren
    if (i2-i1+1 < nSamples && (Direction == ALL || Direction == GRID || Direction == COLS))
    {
        for (long long int i = i2+1; i < __nLines; i++)
        {
            for (long long int j = j1; j <= j2; j++)
            {
                if (_ret_line+i-(i2+1)+i1 >= nLines)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dTemp[i];
                    delete[] dTemp;
                    return false;
                }
                if (isnan(dTemp[i][j]))
                {
                    dCache[_ret_line+i-(i2+1)+i1][j][_nLayer] = NAN;
                    //bValidElement[_ret_line+i-(i2+1)+i1][j][_nLayer] = false;
                }
                else
                {
                    dCache[_ret_line+i-(i2+1)+i1][j][_nLayer] = dTemp[i][j];
                    //bValidElement[_ret_line+i-(i2+1)+i1][j][_nLayer] = true;
                }
            }
        }
    }
    else if (i2-i1+1 > nSamples && (Direction == ALL || Direction == GRID || Direction == COLS))
    {
        for (long long int i = i1+nSamples-1; i <= i2; i++)
        {
            for (long long int j = j1; j <= j2; j++)
            {
                if (i >= nLines)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dTemp[i];
                    delete[] dTemp;
                    return false;
                }
                dCache[i][j][_nLayer] = NAN;
                //bValidElement[i][j][_nLayer] = false;
            }
        }
    }
    //cerr << "Lower block" << endl;
    // Block rechts kopieren
    if (j2-j1+1 < nSamples && (Direction == ALL || Direction == GRID || Direction == LINES))
    {
        for (long long int i = 0; i < __nLines; i++)
        {
            for (long long int j = j2+1; j < __nCols; j++)
            {
                if (_final_cols+j-(j2+1)+j1 >= nCols)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dTemp[i];
                    delete[] dTemp;
                    return false;
                }
                if (isnan(dTemp[i][j]))
                {
                    dCache[i][_final_cols+j-(j2+1)+j1][_nLayer] = NAN;
                    //bValidElement[i][_final_cols+j-(j2+1)+j1][_nLayer] = false;
                }
                else
                {
                    dCache[i][_final_cols+j-(j2+1)+j1][_nLayer] = dTemp[i][j];
                    //bValidElement[i][_final_cols+j-(j2+1)+j1][_nLayer] = true;
                }
            }
        }
    }
    else if (j2-j1+1 > nSamples && (Direction == ALL || Direction == GRID || Direction == LINES))
    {
        for (long long int i = i1; i < i2; i++)
        {
            for (long long int j = j1+nSamples-1; j <= j2; j++)
            {
                if (j >= nCols)
                {
                    for (long long int i = 0; i < __nLines; i++)
                        delete[] dTemp[i];
                    delete[] dTemp;
                    return false;
                }
                dCache[i][j][_nLayer] = NAN;
                //bValidElement[i][j][_nLayer] = false;
            }
        }
    }

    // appended zeroes zaehlen
    for (long long int j = 0; j < nCols; j++)
    {
        for (long long int i = nLines; i >= 0; i--)
        {
            if (i == nLines)
                nAppendedZeroes[_nLayer][j] = 0;
            else if (isnan(dCache[i][j][_nLayer]))
                nAppendedZeroes[_nLayer][j]++;
            else
                break;

        }
    }
    for (long long int i = 0; i < __nLines; i++)
        delete[] dTemp[i];
    delete[] dTemp;
    /*double dValues[nSamples];
    double dDelta = 1.0;
    if (!bUseAppendedZeroes)
        dDelta = (double)(i2-i1) / (double)(nSamples-1);

    for (unsigned int i = 0; i < nSamples; i++)
        dValues[i] = 0.0;
    //cerr << dDelta << endl;
    //cerr << i1 << " " << i2 << " " << j1 << " " << j2 << " " << dDelta << endl;
    //cerr << nSamples << " " << nLines << endl;
    if ((nSamples > (i2-i1) && !bUseAppendedZeroes) || (nSamples >= (i2-i1) && bUseAppendedZeroes))
    {
        long long int nNLines = nLines;
        while (nNLines <= (i1 + nSamples + getCacheCols(_nLayer, false)-i2))
            nNLines *= 2;
        resizeCache(nNLines, nCols, nLayers);
    }
    //cerr << "|-> Resampling ... ";
    long long int I = 0;
    for (long long int j = j1; j <= j2; j++)
    {
        dValues[0] = dCache[i1][j][_nLayer];
        if (bUseAppendedZeroes)
            dDelta = (i2-i1-nAppendedZeroes[_nLayer][j])/(double)(nSamples-1);
        //cerr << "blubb";
        for (unsigned int i = 1; i < nSamples; i++)
        {
            I = (int)((double)i*dDelta+(double)i1);
            //cerr << (double)i*dDelta+(double)i1 << " / " << I << ", " << I+1 << endl;

            if (I == i2 || (I+1 == nLines - nAppendedZeroes[_nLayer][j] && bUseAppendedZeroes) || (I+1 == nLines && !bUseAppendedZeroes))
            {
                dValues[i] = dCache[I][j][_nLayer]; //<< SEGFAULT!
                if (!bUseAppendedZeroes)
                {
                    for (unsigned int k = i2; k < (unsigned)(getCacheLines(_nLayer, true)-nAppendedZeroes[_nLayer][j]); k++)
                    {
                        if (!bValidElement[k][j][_nLayer])
                            vTemp.push_back(NAN);
                        else
                            vTemp.push_back(dCache[k][j][_nLayer]);
                    }
                }
                break;
            }
            else if (!bValidElement[I][j][_nLayer] && !bValidElement[I+1][j][_nLayer])
                dValues[i] = NAN;
            else if (I == (double)i*dDelta+(double)i1 || !bValidElement[I+1][j][_nLayer])
                dValues[i] = dCache[I][j][_nLayer];
            else if (!bValidElement[I][j][_nLayer])
                dValues[i] = dCache[I+1][j][_nLayer];
            else
            {
                dValues[i] = dCache[I][j][_nLayer] + (dCache[I+1][j][_nLayer]-dCache[I][j][_nLayer]) * ((double)i*dDelta+(double)i1 - (double)I);
                //cerr << ((double)i*dDelta+(double)i1 - (double)I) << " => " << dValues[i] << endl;
            }
            //cerr << "TEST" << i << endl;
        }
        //cerr << "TEST" << endl;
        nAppendedZeroes[_nLayer][j] = 0;
        for (long long int i = i1; i < nLines; i++)
        {
            if (i-i1 < nSamples)
            {
                if (isnan(dValues[i-i1]))
                {
                    dCache[i][j][_nLayer] = NAN;
                    bValidElement[i][j][_nLayer] = false;
                }
                else
                {
                    dCache[i][j][_nLayer] = dValues[i-i1];
                    bValidElement[i][j][_nLayer] = true;
                }
            }
            else if (i-i1-nSamples < vTemp.size())
            {
                if (isnan(vTemp[i-i1-nSamples]))
                {
                    dCache[i][j][_nLayer] = NAN;
                    bValidElement[i][j][_nLayer] = false;
                }
                else
                {
                    dCache[i][j][_nLayer] = vTemp[i-i1-nSamples];
                    bValidElement[i][j][_nLayer] = true;
                }
            }
            else
            {
                dCache[i][j][_nLayer] = NAN;
                bValidElement[i][j][_nLayer] = false;
                nAppendedZeroes[_nLayer][j]++;
            }
        }
    }*/
    if (bIsSaved)
    {
        bIsSaved = false;
        nLastSaved = time(0);
    }
    //cerr << "Erfolg!" << endl;
    return true;
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
    else if (_sString.length())
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
        throw STRINGVARS_MUSTNT_BEGIN_WITH_A_NUMBER;
    for (unsigned int i = 0; i < sVar.length(); i++)
    {
        if (sValidChars.find(sVar[i]) == string::npos)
        {
            sErrorToken = sVar[i];
            throw STRINGVARS_MUSTNT_CONTAIN;
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

bool Cache::isValidDisc(long long int _nLine, long long int _nCol, long long int _nLayer, unsigned int nSize)
{
    if (_nLine >= Cache::getCacheLines(_nLayer, false)-nSize
        || _nCol >= Cache::getCacheCols(_nLayer, false)-nSize
        || !bValidData)
        return false;
    for (long long int i = _nLine; i <= _nLine+nSize; i++)
    {
        if (isnan(dCache[i][_nCol][_nLayer]) || isnan(dCache[i][_nCol+nSize][_nLayer]))
            return false;
    }
    for (long long int j = _nCol; j <= _nCol+nSize; j++)
    {
        if (isnan(dCache[_nLine][j][_nLayer]) || isnan(dCache[_nLine+nSize][j][_nLayer]))
            return false;
    }

    return true;
}
