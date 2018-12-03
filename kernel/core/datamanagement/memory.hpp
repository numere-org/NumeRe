/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include "../ui/error.hpp"
#include "../settings.hpp"
#include "../structures.hpp"
#include "../utils/tools.hpp"
#include "../version.h"
#include "../maths/resampler.h"
#include "table.hpp"


#ifndef MEMORY_HPP
#define MEMORY_HPP

using namespace std;


/*
 * Header zur Memory-Klasse
 */

class Memory
{
    public:
        enum AppDir {LINES, COLS, GRID, ALL};

	private:
		long long int nLines;							// Zeilen des Caches
		long long int nCols;							// Spalten des Caches
		long long int nWrittenHeadlines;
		long long int* nAppendedZeroes;			    	// Pointer auf ein Array von ints, die fuer jede Spalte die Zahl der angehaengten Nullen beinhaelt
		double** dMemTable;								// Pointer auf Pointer auf die Datenfile-double-Matrix
		bool bValidData;								// TRUE, wenn die Instanz der Klasse auch Daten enthaelt
		string* sHeadLine;								// Pointer auf ein string-Array fuer die Tabellenkoepfe
		bool bIsSaved;                                  // Boolean: TRUE fuer gespeicherte Daten
		bool bSaveMutex;
		long long int nLastSaved;                       // Integer, der den Zeitpunkt des letzten Speicherns speichert

		bool Allocate(long long int _nNLines, long long int _nNCols, bool shrink = false);	// Methode, um dem Pointer dMemTable die finale Matrix zuzuweisen

		bool isValidDisc(long long int _nLine, long long int _nCol, unsigned int nSize);
		bool retoqueRegion(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nOrder = 1, AppDir Direction = ALL);
		bool retoqueRegion(RetoqueRegion& _region);

		void reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
		bool qSortWrapper(int* nIndex, int nElements, int nKey, long long int nLeft, long long int nRight, int nSign = 1); // wendet den Quicksort-Algorithmus an
		bool qSort(int* nIndex, int nElements, int nKey, long long int nLeft, long long int nRight, int nSign = 1); // wendet den Quicksort-Algorithmus an
		ColumnKeys* evaluateKeyList(string& sKeyList, long long int nMax);
		bool sortSubList(vector<int>& vIndex, ColumnKeys* KeyList, long long int i1, long long int i2, long long int j1, int nSign);

		bool evaluateIndices(long long int& i1, long long int& i2, long long int& j1, long long int& j2);

    public:
		Memory();										// Standard-Konstruktor
		Memory(long long int _nLines, long long int _nCols);	    				// Allgemeiner Konstruktor (generiert zugleich die Matrix dMemTable und die Arrays
														// 		auf Basis der uebergeben Werte)
		~Memory();										// Destruktor (wendet delete[] auf die Matrix und alle Arrays an, sofern es noetig ist)

		bool resizeMemory(long long int _nLines, long long int _nCols);	        // setzt nCols auf _nCols, nLines auf _nLines und ruft AllocateCache(int,int) auf
		bool isValid() const;							                        // gibt den Wert von bValidData zurueck
		bool isValidElement(long long int _nLine, long long int _nCol) const;	// gibt zurueck, ob an diesem Speicherpunkt ueberhaupt etwas existiert

		bool shrink();

		long long int getLines(bool _bFull = false) const;                 // gibt nLines zurueck
		long long int getCols(bool _bFull = false) const;			             // gibt nCols zurueck
        inline int getSize() const
            {
                if (bValidData)
                    return nLines * nCols * sizeof(double);
                else
                    return 0;
            }

		double readMem(long long int _nLine, long long int _nCol) const;	// Methode, um auf ein Element von dMemTable zuzugreifen
		vector<double> readMem(const vector<long long int>& _vLine, const vector<long long int>& _vCol) const;
		void copyElementsInto(vector<double>* vTarget, const vector<long long int>& _vLine, const vector<long long int>& _vCol) const;
		long long int getAppendedZeroes(long long int _i) const;			    // gibt die Zahl der angehaengten Nullen der _i-ten Spalte zurueck
		int getHeadlineCount() const;

		bool writeSingletonData(Indices& _idx, double* _dData);	// Methode, um ein Element zu schreiben
		bool writeData(long long int _Line, long long int _nCol, double _dData);	// Methode, um ein Element zu schreiben
		bool writeData(Indices& _idx, double* _dData, unsigned int _nNum);	// Methode, um ein Element zu schreiben

		string getHeadLineElement(long long int _i) const;		            // gibt das _i-te Element der Kopfzeile zurueck
		vector<string> getHeadLineElement(vector<long long int> _vCol) const;

		bool setHeadLineElement(long long int _i, string _sHead);	        // setzt das _i-te Element der Kopfzeile auf _sHead

		bool save(string _sFileName);
        bool getSaveStatus() const;                     // gibt bIsSaved zurueck
        void setSaveStatus(bool _bIsSaved);             // setzt bIsSaved
        long long int getLastSaved() const;             // gibt nLastSaved zurueck

        vector<int> sortElements(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0, const string& sSortingExpression = "");

        void deleteEntry(long long int _nLine, long long int _nCol);
        void deleteBulk(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0);
        void deleteBulk(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        Table extractTable(const string& _sTable = "");

        // MAFIMPLEMENTATIONS
        double std(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double std(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        double avg(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double avg(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        double max(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double max(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        double min(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double min(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        double prd(const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double prd(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);

        double sum(const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double sum(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);

        double num(const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double num(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);

        double and_func(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double and_func(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        double or_func(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double or_func(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        double xor_func(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);
        double xor_func(const vector<long long int>& _vLine, const vector<long long int>& _vCol);

        double cnt(const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double cnt(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);

        double norm(const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double norm(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);

        double cmp(const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dRef = 0.0, int nType = 0);
        double cmp(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dRef = 0.0, int nType = 0);

        double med(const vector<long long int>& _vLine, const vector<long long int>& _vCol);
        double med(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1);

        double pct(const vector<long long int>& _vLine, const vector<long long int>& _vCol, double dPct = 0.5);
        double pct(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, double dPct = 0.5);

        bool smooth(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nOrder = 1, AppDir Direction = ALL);
        bool retoque(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, AppDir Direction = ALL);
        bool resample(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nSamples = 0, AppDir Direction = ALL);

};

#endif

