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


#include <string>
#include <boost/tokenizer.hpp>
#include <vector>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

#include "../ui/error.hpp"
#include "../settings.hpp"
#include "cache.hpp"
#include "../IgorLib/ReadWave.h"
#include "../utils/zip++.hpp"
#include "../utils/BasicExcel.hpp"
#include "../utils/tinyxml2.h"


#ifndef DATAFILE_HPP
#define DATAFILE_HPP

string toString(int);
string toLowerCase(const string&);

using namespace std;
using namespace boost;

int matchParams(const string& sCmd, const string& sParam, const char cFollowing);
string getArgAtPos(const string& sCmd, unsigned int nPos);
void StripSpaces(string& sToStrip);
string getClipboardText();
string utf8parser(const string& sString);
int StrToInt(const string&);
/*
 * Header zur Datafile-Klasse
 */

class Datafile : public Cache		//	Diese Klasse ist ein CHILD von FileSystem und von Cache
{
	private:
		long long int nLines;							// Zeilen des Datenfiles
		long long int nCols;							// Spalten des Datenfiles
		long long int* nAppendedZeroes;					// Pointer auf ein Array von ints, die fuer jede Spalte die Zahl der angehaengten Nullen beinhaelt
		double** dDatafile;								// Pointer auf Pointer auf die Datenfile-double-Matrix
		ifstream file_in;								// ifstream, der zum Einlesen eines Datenfiles verwendet wird
		ofstream file_out;                              // ofstream, der zum Schreiben des Datenfiles verwendet wird
		bool bValidData;								// TRUE, wenn die Instanz der Klasse auch Daten enthaelt
		//bool** bValidEntry;							// Pointer auf Pointer auf Valid-Element-Bool-Matrix
		bool bUseCache;									// TRUE, wenn die Elemente des Caches verwendet werden sollen
		bool bLoadEmptyCols;
		bool bLoadEmptyColsInNextFile;
		string sDataFile;								// string, in dem der Name des eingelesenen Files gespeichert wird
		string sOutputFile;                             // string, der den letzten Output speichert
		string* sHeadLine; 								// Pointer auf ein string-Array fuer die Tabellenkoepfe
		string sSavePath;
		string sPrefix;
		bool bPauseOpening;

		string getDate();

		void Allocate();								// Methode, um dem Pointer dDatafile die finale Matrix zuzuweisen
		void setLines(long long int _nLines);			// Methode, um nLines zu setzen
		void setCols(long long int _nCols);				// Methode, um nCols zu setzen
		inline void replaceDecimalSign(string& _sToReplace)
            {
                if (_sToReplace.find(',') == string::npos)
                    return;
                else
                {
                    for (unsigned int i = 0; i < _sToReplace.length(); i++)
                    {
                        if (_sToReplace[i] == ',')
                            _sToReplace[i] = '.';
                    }
                    return;
                }
            }
        inline void replaceTabSign(string& _sToReplace, bool bAddPlaceholders = false)
            {
                if (_sToReplace.find('\t') == string::npos)
                    return;
                else
                {
                    for (unsigned int i = 0; i < _sToReplace.length(); i++)
                    {
                        if (_sToReplace[i] == '\t')
                        {
                            _sToReplace[i] = ' ';
                            if (bAddPlaceholders)
                            {
                                if (!i)
                                    _sToReplace.insert(0,1,'_');
                                else if (_sToReplace[i-1] == ' ')
                                    _sToReplace.insert(i,1,'_');
                                if (i+1 == _sToReplace.length())
                                    _sToReplace += "_";
                            }
                        }
                    }
                    return;
                }
            }
        inline void stripTrailingSpaces(string& _sToStrip)
            {
                if (_sToStrip.find(' ') == string::npos && _sToStrip.find('\t') == string::npos)
                    return;
                else
                {
                    while (_sToStrip[_sToStrip.length()-1] == ' ' || _sToStrip[_sToStrip.length()-1] == '\t')
                    {
                        _sToStrip = _sToStrip.substr(0,_sToStrip.length()-1);
                    }
                }
                return;
            }
        bool isNumeric(const string& _sString);
        bool qSortWrapper(int* nIndex, int nElements, int nKey, int nLeft, int nRight, int nSign = 1);
        bool qSort(int* nIndex, int nElements, int nKey, int nLeft, int nRight, int nSign = 1);
        void openLabx(Settings& _option);
        void openCSV(Settings& _option);
        void openNDAT(Settings& _option);
        void openJDX(Settings& _option);
        void openIBW(Settings& _option, bool bXZSlice = false, bool bYZSlice = false);
        void openODS(Settings& _option);
        void openXLS(Settings& _option);
        void openXLSX(Settings& _option);
        void countAppendedZeroes(long long int nCol = -1);
        void condenseDataSet();
        string expandODSLine(const string& sLine);
        void evalExcelIndices(const string& sIndices, int& nLine, int& nCol);
        vector<double> parseJDXLine(const string& sLine);
        vector<string> getPastedDataFromCmdLine(const Settings& _option, bool& bKeepEmptyTokens);
        void reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
        bool sortSubList(vector<int>& vIndex, ColumnKeys* KeyList, long long int i1, long long int i2, long long int j1, int nSign);

        inline void parseJDXDataLabel(string& sLine)
            {
                if (sLine.find("##") == string::npos || sLine.find('=') == string::npos)
                    return;
                for (unsigned int i = 0; i < sLine.length(); i++)
                {
                    if (sLine[i] == ' ')
                    {
                        sLine.erase(i,1);
                        i--;
                    }
                    if (sLine[i] == '-')
                    {
                        sLine.erase(i,1);
                        i--;
                    }
                    if (sLine[i] == '_')
                    {
                        sLine.erase(i,1);
                        i--;
                    }
                    if (sLine[i] >= 'a' && sLine[i] <= 'z')
                        sLine[i] += 'A'-'a';
                    if (sLine[i] == '=')
                        break;
                }
                return;
            }
        inline void parseEvery(long long int& nFirst, long long int& nEvery, string& sDir)
            {
                if (sDir.find("every=") != string::npos)
                {
                    string sEvery = getArgAtPos(sDir, sDir.find("every=")+6);
                    if (sEvery.find(',') != string::npos)
                    {
                        nFirst = StrToInt(sEvery.substr(0, sEvery.find(',')))-1;
                        nEvery = StrToInt(sEvery.substr(sEvery.find(',')+1));
                    }
                    else
                    {
                        nEvery = StrToInt(sEvery);
                        nFirst = nEvery-1;
                    }
                    sDir.erase(sDir.find("every="));
                    if (nEvery < 1)
                        nEvery = 1;
                    if (nFirst < 0)
                        nFirst = 0;
                }
                return;
            }

	public:
		Datafile();										// Standard-Konstruktor
		Datafile(long long int _nLines, long long int _nCols);	// Allgemeiner Konstruktor (generiert zugleich die Matrix dDatafile und die Arrays
                                                                // 		auf Basis der uebergeben Werte)
		~Datafile();									// Destruktor (wendet delete[] auf die Matrix und alle Arrays an, sofern es noetig ist)

		long long int getLines(const string& sCache, bool _bFull = false) const;	// gibt nLines zurueck
		long long int getCols(const string& sCache, bool _bFull = false) const;					// gibt nCols zurueck
		bool isValid() const;							// gibt den Wert von bValidData zurueck
		double getElement(long long int _nLine, long long int _nCol, const string& sCache) const;	// Methode, um auf ein Element von dDatafile zuzugreifen
		vector<double> getElement(const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& sCache) const;
		void copyElementsInto(vector<double>* vTarget, const vector<long long int>& _vLine, const vector<long long int>& _vCol, const string& sCache) const;
		void openFile(string _sFile,
                    Settings& _option,
                    bool bAutoSave = false,
                    bool bIgnore = false,
                    int _nHeadline = 0);	                // zentrale Methode: Oeffnet ein Datenfile, liest die Daten und interpretiert sie als double.
                                                        //		Ist auch in der Lage, Tabellenkoepfe aus Kommentarzeilen zu extrahieren.
        void pasteLoad(const Settings& _option);
		void openAutosave(string _sFile, Settings& _option);
		void removeData(bool bAutoSave = false);		// Loescht den Inhalt von dDatafile, allen Arrays und setzt das Objekt auf den Urzustand zurueck
		string getDataFileName(const string& sCache) const;					// gibt den Wert von sDataFile zurueck
		string getDataFileNameShort() const;
		string getHeadLineElement(long long int _i, const string& sCache) const;		// gibt das _i-te Element der Kopfzeile zurueck
		inline string getTopHeadLineElement(long long int _i, const string& sCache) const
            {
                return getHeadLineElement(_i, sCache).substr(0, getHeadLineElement(_i, sCache).find("\\n"));
            }
		vector<string> getHeadLineElement(vector<long long int> _vCol, const string& sCache) const;		// gibt das _i-te Element der Kopfzeile zurueck
		bool setHeadLineElement(long long int _i, const string& sCache, string _sHead);	// setzt das _i-te Element der Kopfzeile auf _sHead
		long long int getAppendedZeroes(long long int _i, const string& sCache) const;			// gibt die Zahl der angehaengten Nullen der _i-ten Spalte zurueck
		void melt (Datafile& _cache);					// Methode, um die Daten einer anderen Instanz dieser Klasse den Daten dieser Klasse
														//		(als weitere Spalten) hinzu zu fuegen
		bool isValidEntry(long long int _nLine, long long int _nCol, const string& sCache) const;	// gibt zurueck, ob der Datenpunkt ueberhaupt gueltig ist
		void setCacheStatus(bool _bCache);				// Setzt bUseCache
		bool getCacheStatus() const;					// gibt bUseCache zurueck
		bool isValidCache() const;						// gibt bValidData von Cache zurueck
		void clearCache();								// loest die Methode Cache::removeCachedData() auf
		bool setCacheSize(long long int _nLines, long long int _nCols, long long int _nLayers);		// Setzt die Anfangsgroesse des Caches
		vector<int> sortElements(const string& sLine);
		vector<int> sortElements(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0, const string& sSortingExpression = "");
        inline int getDataSize() const
            {
                if (bValidData)
                    return nLines * nCols * sizeof(double);
                else
                    return 0;
            }
        bool saveFile(const string& sCache, string _sFileName);
        inline string getOutputFileName() const
            {
                return sOutputFile;
            }
        inline void setSavePath(const string& _sPath)
        {
            sSavePath = _sPath;
            return;
        }
        inline void setPrefix(const string& _sPrefix)
        {
            sPrefix = _sPrefix;
            return;
        }
        inline void setbLoadEmptyCols(bool _bLoadEmptyCols)
            {
                bLoadEmptyCols = _bLoadEmptyCols;
                return;
            }
        void generateFileName();
        inline void setbLoadEmptyColsInNextFile(bool _bLoadEmptyCols)
            {
                bLoadEmptyColsInNextFile = _bLoadEmptyCols;
                return;
            }
        void openFromCmdLine(Settings& _option, string sFile = "", bool bOpen = false);
        inline bool pausedOpening() const
            {
                return bPauseOpening;
            }

        inline vector<double> std(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(std(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(std(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(std(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(std(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(std(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(std(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> avg(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(avg(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(avg(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(avg(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(avg(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(avg(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(avg(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> max(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(max(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(max(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(max(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(max(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(max(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(max(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> min(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(min(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(min(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(min(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(min(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(min(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(min(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> prd(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(prd(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(prd(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(prd(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(prd(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(prd(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(prd(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> sum(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(sum(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(sum(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(sum(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(sum(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(sum(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(sum(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> num(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(num(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(num(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(num(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(num(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(num(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(num(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                return vResults;
            }
        inline vector<double> and_func(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(and_func(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(and_func(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(and_func(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(and_func(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(and_func(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(and_func(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                return vResults;
            }
        inline vector<double> or_func(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(or_func(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(or_func(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(or_func(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(or_func(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(or_func(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(or_func(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                return vResults;
            }
        inline vector<double> xor_func(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(xor_func(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(xor_func(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(xor_func(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(xor_func(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(xor_func(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(xor_func(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                return vResults;
            }
        inline vector<double> cnt(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(cnt(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(cnt(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(cnt(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(cnt(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(cnt(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(cnt(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> norm(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(norm(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(norm(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(norm(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(norm(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(norm(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(norm(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> cmp(const string& sCache, string sDir, double dRef = 0.0, int nType = 0)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(cmp(sCache, 0, getLines(sCache), i, -1, dRef, nType));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(cmp(sCache, i,-1,0,getCols(sCache), dRef, nType));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(cmp(sCache, 0, getLines(sCache), i, -1, dRef, nType));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(cmp(sCache, i,-1,2,getCols(sCache), dRef, nType));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> med(const string& sCache, string sDir)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(med(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(med(sCache, i,-1,0,getCols(sCache)));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(med(sCache, 0, getLines(sCache), i));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(med(sCache, i,-1,2,getCols(sCache)));
                }
                else if (sDir == "grid")
                {
                    vResults.push_back(med(sCache, 0,getLines(sCache),2,getCols(sCache)));
                }
                else
                {
                    vResults.push_back(med(sCache, 0,getLines(sCache),0,getCols(sCache)));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        inline vector<double> pct(const string& sCache, string sDir, double dPct = 0.5)
            {
                vector<double> vResults;

                long long int nEvery = 1;
                long long int nFirst = 0;

                parseEvery(nFirst, nEvery, sDir);

                if (sDir == "cols")
                {
                    if (nFirst >= getCols(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(pct(sCache, 0, getLines(sCache), i, dPct));
                }
                else if (sDir == "lines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(pct(sCache, i,-1,0,getCols(sCache), dPct));
                }
                else if (sDir == "gridcols")
                {
                    if (nFirst >= getCols(sCache)-2)
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = 2+nFirst; i < getCols(sCache); i += nEvery)
                        vResults.push_back(pct(sCache, 0, getLines(sCache), i, dPct));
                }
                else if (sDir == "gridlines")
                {
                    if (nFirst >= getLines(sCache))
                    {
                        vResults.push_back(NAN);
                        return vResults;
                    }
                    for (long long int i = nFirst; i < getLines(sCache, false); i += nEvery)
                        vResults.push_back(pct(sCache, i,-1,2,getCols(sCache), dPct));
                }
                if (!vResults.size())
                    vResults.push_back(NAN);
                return vResults;
            }
        double std(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double avg(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double max(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double min(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double prd(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double sum(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double num(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double and_func(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double or_func(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double xor_func(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double cnt(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double norm(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double cmp(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dRef = 0.0, int nType = 0);
        double med(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double pct(const string& sCache, long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dPct = 0.5);

        double std(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double avg(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double max(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double min(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double prd(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double sum(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double num(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double and_func(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double or_func(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double xor_func(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double cnt(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double norm(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double cmp(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef = 0.0, int nType = 0);
        double med(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double pct(const string& _sCache, const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct = 0.5);


};

#endif
