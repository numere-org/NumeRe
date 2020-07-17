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
//#include "../structures.hpp"
#include "../utils/tools.hpp"
#include "../version.h"
#include "../maths/resampler.h"
#include "../maths/filtering.hpp"
#include "table.hpp"
#include "sorter.hpp"


#ifndef MEMORY_HPP
#define MEMORY_HPP

using namespace std;

// forward declaration for using the memory manager as friend
class MemoryManager;

/*
 * Header zur Memory-Klasse
 */

class Memory : public Sorter
{
    public:
        enum AppDir {LINES, COLS, GRID, ALL};

	private:
	    friend class MemoryManager;

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
		bool isValidDisc(VectorIndex _vLine, VectorIndex _vCol);
		bool retoqueRegion(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = -1, unsigned int nOrder = 1, AppDir Direction = ALL);
		bool retoqueRegion(RetoqueRegion& _region);

		void reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
		virtual int compare(int i, int j, int col);
        virtual bool isValue(int line, int col);
		bool evaluateIndices(long long int& i1, long long int& i2, long long int& j1, long long int& j2);
		void countAppendedZeroes();
		void smoothingWindow1D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter, bool smoothLines);
		void smoothingWindow2D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter);

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
		vector<double> readMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		void copyElementsInto(vector<double>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		long long int getAppendedZeroes(long long int _i) const;			    // gibt die Zahl der angehaengten Nullen der _i-ten Spalte zurueck
		int getHeadlineCount() const;

		bool writeSingletonData(Indices& _idx, double* _dData);	// Methode, um ein Element zu schreiben
		bool writeData(long long int _Line, long long int _nCol, double _dData);	// Methode, um ein Element zu schreiben
		bool writeData(Indices& _idx, double* _dData, unsigned int _nNum);	// Methode, um ein Element zu schreiben

		string getHeadLineElement(long long int _i) const;		            // gibt das _i-te Element der Kopfzeile zurueck
		vector<string> getHeadLineElement(const VectorIndex& _vCol) const;

		bool setHeadLineElement(long long int _i, string _sHead);	        // setzt das _i-te Element der Kopfzeile auf _sHead

		bool save(string _sFileName, const string& sTableName, unsigned short nPrecision);
        bool getSaveStatus() const;                     // gibt bIsSaved zurueck
        void setSaveStatus(bool _bIsSaved);             // setzt bIsSaved
        long long int getLastSaved() const;             // gibt nLastSaved zurueck

        vector<int> sortElements(long long int i1, long long int i2, long long int j1 = 0, long long int j2 = 0, const string& sSortingExpression = "");

        void deleteEntry(long long int _nLine, long long int _nCol);
        void deleteBulk(const VectorIndex& _vLine, const VectorIndex& _vCol);

        NumeRe::Table extractTable(const string& _sTable = "");
        void importTable(NumeRe::Table _table);

        // MAFIMPLEMENTATIONS
        double std(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double avg(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double max(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double min(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double prd(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double sum(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double num(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double and_func(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double or_func(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double xor_func(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double cnt(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double norm(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double cmp(const VectorIndex& _vLine, const VectorIndex& _vCol, double dRef = 0.0, int nType = 0);
        double med(const VectorIndex& _vLine, const VectorIndex& _vCol);
        double pct(const VectorIndex& _vLine, const VectorIndex& _vCol, double dPct = 0.5);

        bool smooth(VectorIndex _vLine, VectorIndex _vCol, NumeRe::FilterSettings _settings, AppDir Direction = ALL);
        bool retoque(VectorIndex _vLine, VectorIndex _vCol, AppDir Direction = ALL);
        bool resample(VectorIndex _vLine, VectorIndex _vCol, unsigned int nSamples = 0, AppDir Direction = ALL);

};

#endif

