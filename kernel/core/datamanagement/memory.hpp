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

#include <string>
#include <vector>

#include "table.hpp"
#include "sorter.hpp"
#include "../maths/filtering.hpp"

#ifndef MEMORY_HPP
#define MEMORY_HPP

using namespace std;

// forward declaration for using the memory manager as friend
class MemoryManager;
namespace NumeRe
{
    class FileAdapter;
}

struct RetouchBoundary
{
    long long int n;
    long long int m;
    size_t rows;
    size_t cols;

    RetouchBoundary(long long int i, long long int j, size_t _row, size_t _col) : n(i), m(j), rows(_row), cols(_col) {}

    long long int rf()
    {
        return n;
    }

    long long int re()
    {
        return n+rows;
    }

    long long int cf()
    {
        return m;
    }

    long long int ce()
    {
        return m+cols;
    }
};

/////////////////////////////////////////////////
/// \brief This class represents a single table
/// in memory, or a - so to say - single memory
/// page to be handled by the MemoryManager class
/// instance.
/////////////////////////////////////////////////
class Memory : public Sorter
{
    public:
        enum AppDir {LINES, COLS, GRID, ALL};

	private:
	    friend class MemoryManager;
	    friend class NumeRe::FileAdapter;

		long long int nLines;
		long long int nCols;
		long long int nWrittenHeadlines;
		long long int* nAppendedZeroes;
		double** dMemTable;
		bool bValidData;
		string* sHeadLine;
		bool bIsSaved;
		bool bSaveMutex;
		long long int nLastSaved;

		bool Allocate(long long int _nNLines, long long int _nNCols, bool shrink = false);
		void createTableHeaders();
		bool clear();
		RetouchBoundary findValidBoundary(const VectorIndex& _vLine, const VectorIndex& _vCol, long long int i, long long int j);
		bool retouch1D(const VectorIndex& _vLine, const VectorIndex& _vCol, AppDir Direction);
		bool retouch2D(const VectorIndex& _vLine, const VectorIndex& _vCol);
		bool onlyValidValues(const VectorIndex& _vLine, const VectorIndex& _vCol);
		void reorderColumn(const vector<int>& vIndex, long long int i1, long long int i2, long long int j1 = 0);
		virtual int compare(int i, int j, int col) override;
        virtual bool isValue(int line, int col) override;
		void countAppendedZeroes();
		void smoothingWindow1D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter, bool smoothLines);
		void smoothingWindow2D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter);

    public:
		Memory();
		Memory(long long int _nLines, long long int _nCols);
		~Memory();

		bool resizeMemory(long long int _nLines, long long int _nCols);
		bool isValid() const;
		bool isValidElement(long long int _nLine, long long int _nCol) const;
		bool shrink();
		long long int getLines(bool _bFull = false) const;
		long long int getCols(bool _bFull = false) const;
        inline int getSize() const
            {
                if (bValidData)
                    return nLines * nCols * sizeof(double);
                else
                    return 0;
            }

        // READ ACCESS METHODS
		double readMem(long long int _nLine, long long int _nCol) const;
		vector<double> readMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		void copyElementsInto(vector<double>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		string getHeadLineElement(long long int _i) const;
		vector<string> getHeadLineElement(const VectorIndex& _vCol) const;
		long long int getAppendedZeroes(long long int _i) const;
		int getHeadlineCount() const;

		// WRITE ACCESS METHODS
		bool writeSingletonData(Indices& _idx, double _dData);
		bool writeData(long long int _Line, long long int _nCol, double _dData);
		bool writeData(Indices& _idx, double* _dData, unsigned int _nNum);
		bool setHeadLineElement(long long int _i, string _sHead);

		bool save(string _sFileName, const string& sTableName, unsigned short nPrecision);
        bool getSaveStatus() const;
        void setSaveStatus(bool _bIsSaved);
        long long int getLastSaved() const;
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
        bool retouch(VectorIndex _vLine, VectorIndex _vCol, AppDir Direction = ALL);
        bool resample(VectorIndex _vLine, VectorIndex _vCol, unsigned int nSamples = 0, AppDir Direction = ALL);

};

#endif

