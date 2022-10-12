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
#include "tablecolumn.hpp"
#include "../maths/filtering.hpp"

#ifndef MEMORY_HPP
#define MEMORY_HPP

// forward declaration for using the memory manager as friend
class MemoryManager;
class Matrix;
struct StatsLogic;

namespace NumeRe
{
    class FileAdapter;
}


/////////////////////////////////////////////////
/// \brief This type defines a generic value
/// vector.
/////////////////////////////////////////////////
typedef std::vector<std::string> ValueVector;


/////////////////////////////////////////////////
/// \brief This class represents a single table
/// in memory, or a - so to say - single memory
/// page to be handled by the MemoryManager class
/// instance.
/////////////////////////////////////////////////
class Memory : public Sorter
{
    public:
        enum AppDir
        {
            ALL = 0x0,
            LINES = 0x1,
            COLS = 0x2,
            GRID = 0x4
        };

	private:
	    friend class MemoryManager;
	    friend class NumeRe::FileAdapter;

	    TableColumnArray memArray;
		NumeRe::TableMetaData m_meta;

		mutable int nCalcLines;

		bool bSaveMutex;
		bool bSortCaseInsensitive;


		bool Allocate(size_t _nNCols, bool shrink = false);
		void createTableHeaders();
		bool clear();
		Boundary findValidBoundary(const VectorIndex& _vLine, const VectorIndex& _vCol, int i, int j) const;
		bool retouch1D(const VectorIndex& _vLine, const VectorIndex& _vCol, AppDir Direction);
		bool retouch2D(const VectorIndex& _vLine, const VectorIndex& _vCol);
		bool onlyValidValues(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		void reorderColumn(const VectorIndex& vIndex, int i1, int i2, int j1 = 0);
		virtual int compare(int i, int j, int col) override;
        virtual bool isValue(int line, int col) override;
		void smoothingWindow1D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter, bool smoothLines);
		void smoothingWindow2D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter);
		void calculateStats(const VectorIndex& _vLine, const VectorIndex& _vCol, std::vector<StatsLogic>& operation) const;

    public:
		Memory();
		Memory(size_t _nCols);
		~Memory();

		Memory& operator=(const Memory& other);

		bool resizeMemory(size_t _nLines, size_t _nCols);
		bool isValid() const;
		bool isValidElement(size_t _nLine, size_t _nCol) const;
		bool shrink();
		void convert();
		bool convertColumns(const VectorIndex& _vCol, const std::string& _sType);
		int getLines(bool _bFull = false) const;
		int getElemsInColumn(size_t col) const;
		int getFilledElemsInColumn(size_t col) const;
		int getCols(bool _bFull = false) const;
        size_t getSize() const;


        // READ ACCESS METHODS
		mu::value_type readMem(size_t _nLine, size_t _nCol) const;
		mu::value_type readMemInterpolated(double _dLine, double _dCol) const;
		std::vector<mu::value_type> readMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		Matrix readMemAsMatrix(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		ValueVector readMixedMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		TableColumn::ColumnType getType(const VectorIndex& _vCol) const;
		ValueVector getCategoryList(const VectorIndex& _vCol) const;
		Memory* extractRange(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		void copyElementsInto(std::vector<mu::value_type>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		std::string getHeadLineElement(size_t _i) const;
		std::vector<std::string> getHeadLineElement(const VectorIndex& _vCol) const;
		size_t getAppendedZeroes(size_t _i) const;
		size_t getHeadlineCount() const;
		std::string getComment() const;
		NumeRe::TableMetaData getMetaData() const;

		// WRITE ACCESS METHODS
		void writeSingletonData(Indices& _idx, const mu::value_type& _dData);
		void writeSingletonData(Indices& _idx, const std::string& _sValue);
		void writeData(int _nLine, int _nCol, const mu::value_type& _dData);
		void writeDataDirect(int _nLine, int _nCol, const mu::value_type& _dData);
		void writeDataDirectUnsafe(int _nLine, int _nCol, const mu::value_type& _dData);
		void writeData(int _nLine, int _nCol, const std::string& sValue);
		void writeData(Indices& _idx, mu::value_type* _dData, unsigned int _nNum);
		void writeData(Indices& _idx, const ValueVector& _values);
		bool setHeadLineElement(size_t _i, const std::string& _sHead);
		void writeComment(const std::string& comment);
		void setMetaData(const NumeRe::TableMetaData& meta);
		void markModified();

		bool save(std::string _sFileName, const std::string& sTableName, unsigned short nPrecision);
        bool getSaveStatus() const;
        void setSaveStatus(bool _bIsSaved);
        long long int getLastSaved() const;
        std::vector<int> sortElements(int i1, int i2, int j1 = 0, int j2 = 0, const std::string& sSortingExpression = "");
        void deleteEntry(int _nLine, int _nCol);
        void deleteBulk(const VectorIndex& _vLine, const VectorIndex& _vCol);
        NumeRe::Table extractTable(const std::string& _sTable, const VectorIndex& lines, const VectorIndex& cols);
        void importTable(NumeRe::Table _table, const VectorIndex& lines, const VectorIndex& cols);

        // MAFIMPLEMENTATIONS
        mu::value_type std(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type avg(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type max(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type min(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type prd(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type sum(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type num(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type and_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type or_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type xor_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type cnt(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type norm(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type cmp(const VectorIndex& _vLine, const VectorIndex& _vCol, mu::value_type dRef = 0.0, int _nType = 0) const;
        mu::value_type med(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        mu::value_type pct(const VectorIndex& _vLine, const VectorIndex& _vCol, mu::value_type dPct = 0.5) const;
        std::vector<mu::value_type> size(const VectorIndex& _vIndex, int dir) const;
        std::vector<mu::value_type> minpos(const VectorIndex& _vIndex, int dir) const;
        std::vector<mu::value_type> maxpos(const VectorIndex& _vIndex, int dir) const;

        bool smooth(VectorIndex _vLine, VectorIndex _vCol, NumeRe::FilterSettings _settings, AppDir Direction = ALL);
        bool retouch(VectorIndex _vLine, VectorIndex _vCol, AppDir Direction = ALL);
        bool resample(VectorIndex _vLine, VectorIndex _vCol, std::pair<size_t,size_t> samples, AppDir Direction = ALL, std::string sFilter = "lanczos3");

};

#endif

