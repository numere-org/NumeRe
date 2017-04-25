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


#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cmath>
#include <vector>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

#include "error.hpp"
#include "settings.hpp"
#include "tools.hpp"
#include "filesystem.hpp"
#include "version.h"
#include "resampler.h"


#ifndef CACHE_HPP
#define CACHE_HPP

using namespace std;


struct RetoqueRegion
{
    vector<vector<double> > vDataArray;
    vector<vector<bool> > vValidationArray;
    double dMedian;
};

inline void prepareRegion(RetoqueRegion& _region, unsigned int nSize, double _dMedian = NAN)
{
    _region.vDataArray.resize(nSize);
    _region.vValidationArray.resize(nSize);
    for (unsigned int i = 0; i < nSize; i++)
    {
        _region.vDataArray[i].resize(nSize, NAN);
        _region.vValidationArray[i].resize(nSize, false);
    }
    _region.dMedian = _dMedian;
    return;
}

/*
 * Header zur Cache-Klasse --> PARENT zur Datafile-Klasse
 */

class Cache : public FileSystem
{
    public:
        enum AppDir {LINES,COLS,GRID,ALL};
	private:
		long long int nLines;							// Zeilen des Caches
		long long int nCols;							// Spalten des Caches
		long long int nLayers;
		long long int** nAppendedZeroes;			    	// Pointer auf ein Array von ints, die fuer jede Spalte die Zahl der angehaengten Nullen beinhaelt
		//bool*** bValidElement;							// Pointer auf Pointer auf die Valid-Data-bool-Matrix
		double*** dCache;								// Pointer auf Pointer auf die Datenfile-double-Matrix
		bool bValidData;								// TRUE, wenn die Instanz der Klasse auch Daten enthaelt
		string** sHeadLine;								// Pointer auf ein string-Array fuer die Tabellenkoepfe
		bool AllocateCache(long long int _nNLines, long long int _nNCols, long long int _nNLayers);	// Methode, um dem Pointer dCache die finale Matrix zuzuweisen
		bool bIsSaved;                                  // Boolean: TRUE fuer gespeicherte Daten
		bool bSaveMutex;
		long long int nLastSaved;                       // Integer, der den Zeitpunkt des letzten Speicherns speichert
		fstream cache_file;
		string sCache_file;

		string sPredefinedFuncs;
		string sUserdefinedFuncs;
		string sPredefinedCommands;
		string sPluginCommands;

		vector<vector<string> > sStrings;
		map<string,string> sStringVars;

		bool isValidDisc(long long int _nLine, long long int _nCol, long long int _nLayer, unsigned int nSize);
		bool retoqueRegion(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nOrder = 1, AppDir Direction = ALL);
		bool retoqueRegion(RetoqueRegion& _region);
		bool checkStringvarDelimiter(const string& sToken) const;
		void replaceStringMethod(string& sLine, size_t nPos, size_t nLength, const string& sVarValue);


	protected:
		bool resizeCache(long long int _nLines, long long int _nCols, long long int _nLayers);	        // setzt nCols auf _nCols, nLines auf _nLines und ruft AllocateCache(int,int) auf
		bool isValid() const;							                        // gibt den Wert von bValidData zurueck
		double readFromCache(long long int _nLine, long long int _nCol, long long int _nLayers) const;	// Methode, um auf ein Element von dCache zuzugreifen
		double readFromCache(long long int _nLine, long long int _nCol, const string& _sCache) const;	// Methode, um auf ein Element von dCache zuzugreifen
		vector<double> readFromCache(const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& _sCache) const;
		vector<double> readFromCache(const vector<long long int>& _vLine, const vector<long long int>& _vCol, long long int _nLayer) const;
		void removeCachedData();						                        // Loescht den Inhalt von dCache, allen Arrays und setzt das Objekt auf den Urzustand zurueck
		string getCacheHeadLineElement(long long int _i, long long int _nLayer) const;		            // gibt das _i-te Element der Kopfzeile zurueck
		string getCacheHeadLineElement(long long int _i, const string& _sCache) const;		            // gibt das _i-te Element der Kopfzeile zurueck
		vector<string> getCacheHeadLineElement(vector<long long int> _vCol, const string& _sCache) const;
		vector<string> getCacheHeadLineElement(vector<long long int> _vCol, long long int _nLayer) const;
		bool setCacheHeadLineElement(long long int _i, long long int _nLayer, string _sHead);	        // setzt das _i-te Element der Kopfzeile auf _sHead
		bool setCacheHeadLineElement(long long int _i, const string& _sCache, string _sHead);	        // setzt das _i-te Element der Kopfzeile auf _sHead
		long long int getAppendedZeroes(long long int _i, long long int _nLayer) const;			    // gibt die Zahl der angehaengten Nullen der _i-ten Spalte zurueck
		long long int getAppendedZeroes(long long int _i, const string& _sCache) const;			    // gibt die Zahl der angehaengten Nullen der _i-ten Spalte zurueck
		bool isValidElement(long long int _nLine, long long int _nCol, long long int _nLayer) const;	// gibt zurueck, ob an diesem Speicherpunkt ueberhaupt etwas existiert
		bool isValidElement(long long int _nLine, long long int _nCol, const string& _sCache) const;	// gibt zurueck, ob an diesem Speicherpunkt ueberhaupt etwas existiert
		bool qSort(int* nIndex, int nElements, int nKey, int nLayer, int nLeft, int nRight, int nSign = 1); // wendet den Quicksort-Algorithmus an
		bool saveLayer(string _sFileName, const string& sLayer);
//		void melt(Cache& _cache);						// Methode, um die Daten einer anderen Instanz dieser Klasse den Daten dieser Klasse
														//		(als weitere Spalten) hinzu zu fuegen

	public:
		Cache();										// Standard-Konstruktor
		Cache(long long int _nLines, long long int _nCols, long long int _nLayers);	    				// Allgemeiner Konstruktor (generiert zugleich die Matrix dCache und die Arrays
														// 		auf Basis der uebergeben Werte)
		~Cache();										// Destruktor (wendet delete[] auf die Matrix und alle Arrays an, sofern es noetig ist)


		map<string,long long int> mCachesMap;

		inline void setPredefinedFuncs(const string& sFuncs)
            {
                sPredefinedFuncs = sFuncs;
                return;
            }
        inline void setUserdefinedFuncs(const string& sUserFuncs)
            {
                sUserdefinedFuncs = sUserFuncs;
                return;
            }
        inline void setPluginCommands(const string& sPluginCmds)
            {
                sPluginCommands = sPluginCmds;
                return;
            }
		bool writeToCache(long long int _Line, long long int _nCol, long long int _nLayer, double _dData);	// Methode, um ein Element zu schreiben
		bool writeToCache(long long int _Line, long long int _nCol, const string& _sCache, double _dData);	// Methode, um ein Element zu schreiben
		long long int getCacheLines(long long int _nLayer, bool _bFull = false) const;                 // gibt nLines zurueck
		long long int getCacheLines(const string& _sCache, bool _bFull = false) const;                 // gibt nLines zurueck
		long long int getCacheCols(long long int _nLayer, bool _bFull) const;			             // gibt nCols zurueck
		long long int getCacheCols(const string& _sCache, bool _bFull) const;			             // gibt nCols zurueck
        bool getSaveStatus() const;                     // gibt bIsSaved zurueck
        void setSaveStatus(bool _bIsSaved);             // setzt bIsSaved
        long long int getLastSaved() const;             // gibt nLastSaved zurueck
        inline int getSize(long long int _nLayer) const
            {
                if (bValidData)
                    return nLines * nCols * sizeof(double);
                else
                    return 0;
            }
        inline string matchCache(const string& sExpression, char cFollowing = ' ')
            {
                for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
                {
                    if (matchParams(sExpression, iter->first, cFollowing))
                        return iter->first;
                }
                return "";
            }
        bool isCacheElement(const string& sCache);
        bool containsCacheElements(const string& sExpression);
        bool addCache(const string& sCache, const Settings& _option);
        bool deleteCache(const string& sCache);
        void deleteEntry(long long int _nLine, long long int _nCol, long long int _nLayer);
        void deleteEntry(long long int _nLine, long long int _nCol, const string& _sCache);
        void deleteBulk(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0);
        void deleteBulk(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0);
        void deleteBulk(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        void deleteBulk(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        inline unsigned int getCacheCount() const
            {
                return mCachesMap.size();
            }
        inline map<string,long long int> getCacheList() const
            {
                return mCachesMap;
            }
        inline string getCacheNames() const
            {
                string sReturn = ";";
                for (auto iter = mCachesMap.begin(); iter != mCachesMap.end(); ++iter)
                {
                    sReturn += iter->first+";";
                }
                return sReturn;
            }
        inline void renameCache(const string& sCache, const string& sNewName, bool bForceRenaming = false)
            {
                if (isCacheElement(sNewName))
                {
                    sErrorToken = sNewName+"()";
                    throw CACHE_ALREADY_EXISTS;
                }
                if (!isCacheElement(sCache))
                {
                    sErrorToken = sCache;
                    throw CACHE_DOESNT_EXIST;
                }
                if (sCache == "cache" && !bForceRenaming)
                    throw CACHE_CANNOT_BE_RENAMED;
                mCachesMap[sNewName] = mCachesMap[sCache];
                mCachesMap.erase(sCache);
                return;
            }
        inline void swapCaches(const string& sCache1, const string& sCache2)
            {
                if (!isCacheElement(sCache1))
                {
                    sErrorToken = sCache1;
                    throw CACHE_DOESNT_EXIST;
                }
                if (!isCacheElement(sCache2))
                {
                    sErrorToken = sCache2;
                    throw CACHE_DOESNT_EXIST;
                }
                long long int nTemp = mCachesMap[sCache1];
                mCachesMap[sCache1] = mCachesMap[sCache2];
                mCachesMap[sCache2] = nTemp;
                return;
            }

        bool sortElements(const string& sLine);               // wandelt das Kommando in einen Ausdruck um, startet qSort und fuehrt die Sortierung aus
        void setCacheFileName(string _sFileName);
        bool saveCache();
        bool loadCache();

        // STRINGFUNCS
        bool writeString(const string& _sString, unsigned int _nthString = string::npos, unsigned int nCol = 0);
        string readString(unsigned int _nthString = string::npos, unsigned int nCol = 0);
        string maxString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
        string minString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
        string sumString(unsigned int i1 = 0, unsigned int i2 = string::npos, unsigned int nCol = 0);
        inline unsigned int getStringElements(unsigned int nCol = string::npos) const
            {
                if (nCol == string::npos)
                {
                    unsigned int nCnt = 0;
                    for (unsigned int i = 0; i < sStrings.size(); i++)
                    {
                        if (nCnt < sStrings[i].size())
                            nCnt = sStrings[i].size();
                    }
                    return nCnt;
                }
                else if (nCol >= sStrings.size())
                    return 0;
                else
                    return sStrings[nCol].size();
                return 0;
            }
        inline unsigned int getStringCols() const
            {
                return sStrings.size();
            }
        inline bool removeStringElements(unsigned int nCol = 0)
            {
                if (nCol < sStrings.size())
                {
                    if (sStrings[nCol].size())
                        sStrings[nCol].clear();
                    return true;
                }
                return false;
            }
        inline bool clearStringElements()
            {
                if (sStrings.size())
                {
                    sStrings.clear();
                    return true;
                }
                return false;
            }
        inline int getStringSize(unsigned int nCol = string::npos) const
            {
                if (nCol == string::npos)
                {
                    unsigned int nSize = 0;
                    for (unsigned int i = 0; i < sStrings.size(); i++)
                    {
                        nSize += getStringSize(i);
                    }
                    return nSize;
                }
                else if (nCol < sStrings.size())
                {
                    if (sStrings[nCol].size())
                    {
                        int nSize = 0;
                        for (unsigned int i = 0; i < sStrings[nCol].size(); i++)
                            nSize += sStrings[nCol][i].size() * sizeof(char);
                        return nSize;
                    }
                    else
                        return 0;
                }
                else
                    return 0;
            }
        // STRINGVARFUNCS
        bool containsStringVars(const string& sLine) const;
        void getStringValues(string& sLine, unsigned int nPos = 0);
        void setStringValue(const string& sVar, const string& sValue);
        void removeStringVar(const string& sVar);
        inline const map<string,string>& getStringVars() const
            {
                return sStringVars;
            }

        // MAFIMPLEMENTATIONS
        double std(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double std(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double std(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double std(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double avg(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double avg(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double avg(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double avg(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double max(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double max(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double max(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double max(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double min(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double min(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double min(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double min(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double prd(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double prd(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double prd(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double prd(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double sum(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double sum(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double sum(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double sum(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double num(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double num(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double num(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double num(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double cnt(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double cnt(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double cnt(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double cnt(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double norm(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double norm(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double norm(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double norm(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double cmp(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef = 0.0, int nType = 0);
        double cmp(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef = 0.0, int nType = 0);
        double cmp(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dRef = 0.0, int nType = 0);
        double cmp(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dRef = 0.0, int nType = 0);
        double med(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double med(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double med(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double med(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double pct(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct = 0.5);
        double pct(long long int _nLayer, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct = 0.5);
        double pct(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dPct = 0.5);
        double pct(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dPct = 0.5);


        bool smooth(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nOrder = 1, AppDir Direction = ALL);
        bool smooth(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nOrder = 1, AppDir Direction = ALL);
        bool retoque(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, AppDir Direction = ALL);
        bool retoque(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, AppDir Direction = ALL);
        bool resample(long long int _nLayer, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nSamples = 0, AppDir Direction = ALL);
        bool resample(const string& _sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nSamples = 0, AppDir Direction = ALL);
};

#endif
