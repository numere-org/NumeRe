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
#include "../maths/anovaimpl.hpp"
#include "../maths/units.hpp"

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

struct KMeansResult
{
    std::vector<std::complex<double>> cluster_labels;
    long double inertia;
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
        enum AppDir
        {
            ALL = 0x0,
            LINES = 0x1,
            COLS = 0x2,
            GRID = 0x4
        };

        enum RankingStrategy
        {
            RANK_DENSE,
            RANK_COMPETETIVE,
            RANK_FRACTIONAL
        };

        enum KmeansInit
        {
            INVALID,
            INIT_RANDOM,
            INIT_KMEANSPP
        };

	private:
	    friend class MemoryManager;
	    friend class NumeRe::FileAdapter;
        friend AnovaCalculationStructure;
        friend FactorNode;

		NumeRe::TableMetaData m_meta;

		mutable int nCalcLines;
        TableColumnArray memArray;
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
		bool convertEmptyColumns(const VectorIndex& _vCol, const std::string& _sType);
		bool setCategories(const VectorIndex& _vCol, const std::vector<std::string>& vCategories);
		int getLines(bool _bFull = false) const;
		int getElemsInColumn(size_t col) const;
		int getFilledElemsInColumn(size_t col) const;
		int getCols(bool _bFull = false) const;
        size_t getSize() const;


        // READ ACCESS METHODS
		std::complex<double> readMem(size_t _nLine, size_t _nCol) const;
		std::complex<double> readMemInterpolated(double _dLine, double _dCol) const;
		std::vector<std::complex<double>> readMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		Matrix readMemAsMatrix(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		ValueVector readMixedMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		ValueVector readMemAsString(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		TableColumn::ColumnType getType(const VectorIndex& _vCol) const;
		bool isValueLike(const VectorIndex& _vCol) const;
		ValueVector getCategoryList(const VectorIndex& _vCol) const;
		Memory* extractRange(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		void copyElementsInto(std::vector<std::complex<double>>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		std::string getHeadLineElement(size_t _i) const;
		std::vector<std::string> getHeadLineElement(const VectorIndex& _vCol) const;
		std::string getUnit(int nCol) const;
		std::vector<std::complex<double>> asSiUnits(size_t nCol) const;
		std::string showUnitConversion(size_t nCol, UnitConversionMode mode) const;
		size_t getAppendedZeroes(size_t _i) const;
		size_t getHeadlineCount() const;
		std::string getComment() const;
		NumeRe::TableMetaData getMetaData() const;

		// WRITE ACCESS METHODS
		void writeSingletonData(Indices& _idx, const std::complex<double>& _dData);
		void writeSingletonData(Indices& _idx, const std::string& _sValue);
		void writeData(int _nLine, int _nCol, const std::complex<double>& _dData);
		void writeDataDirect(int _nLine, int _nCol, const std::complex<double>& _dData);
		void writeDataDirectUnsafe(int _nLine, int _nCol, const std::complex<double>& _dData);
		void writeData(int _nLine, int _nCol, const std::string& sValue);
		void writeData(Indices& _idx, std::complex<double>* _dData, size_t _nNum);
		void writeData(Indices& _idx, const ValueVector& _values);
		bool setHeadLineElement(size_t _i, const std::string& _sHead);
		bool setUnit(int nCol, const std::string& sUnit);
		std::vector<std::string> toSiUnits(const VectorIndex& _vCols, UnitConversionMode mode);
		void writeComment(const std::string& comment);
		void setMetaData(const NumeRe::TableMetaData& meta);
		void markModified();

		bool save(std::string _sFileName, const std::string& sTableName, unsigned short nPrecision, std::string sExt = "");
        bool getSaveStatus() const;
        void setSaveStatus(bool _bIsSaved);
        long long int getLastSaved() const;
        std::vector<int> sortElements(int i1, int i2, int j1 = 0, int j2 = 0, const std::string& sSortingExpression = "");
        void deleteEntry(int _nLine, int _nCol);
        void deleteBulk(const VectorIndex& _vLine, const VectorIndex& _vCol);
        NumeRe::Table extractTable(const std::string& _sTable, const VectorIndex& lines, const VectorIndex& cols);
        void importTable(NumeRe::Table _table, const VectorIndex& lines, const VectorIndex& cols);
        void insertCopiedTable(NumeRe::Table _table, const VectorIndex& lines, const VectorIndex& cols, bool transpose);

        bool insertBlock(size_t atRow, size_t atCol, size_t rows, size_t cols);
        bool insertCols(size_t atCol, size_t num);
        bool insertRows(size_t atRow, size_t num);
        bool removeBlock(size_t atRow, size_t atCol, size_t rows, size_t cols);
        bool removeCols(const VectorIndex& _vCols);
        bool removeRows(const VectorIndex& _vRows);
        bool reorderCols(const VectorIndex& _vCols, const VectorIndex& _vNewOrder);
        bool reorderRows(const VectorIndex& _vRows, const VectorIndex& _vNewOrder);

        // MAFIMPLEMENTATIONS
        std::complex<double> std(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> avg(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> max(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> min(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> prd(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> sum(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> num(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> and_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> or_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> xor_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> cnt(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> norm(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> cmp(const VectorIndex& _vLine, const VectorIndex& _vCol, std::complex<double> dRef = 0.0, int _nType = 0) const;
        std::complex<double> med(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
        std::complex<double> pct(const VectorIndex& _vLine, const VectorIndex& _vCol, std::complex<double> dPct = 0.5) const;
        std::vector<std::complex<double>> size(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, int dir) const;
        std::vector<std::complex<double>> minpos(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, int dir) const;
        std::vector<std::complex<double>> maxpos(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, int dir) const;
        std::vector<std::complex<double>> findCols(const std::vector<std::string>& vColNames, bool enableRegEx, bool autoCreate);
        std::vector<std::complex<double>> countIfEqual(const VectorIndex& _vCols, const std::vector<std::complex<double>>& vValues, const std::vector<std::string>& vStringValues) const;
        std::vector<std::complex<double>> getIndex(size_t col, const std::vector<std::complex<double>>& vValues, const std::vector<std::string>& vStringValues) const;
        std::complex<double> getCovariance(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2) const;
        std::complex<double> getPearsonCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2) const;
        std::complex<double> getSpearmanCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2) const;
        std::vector<std::complex<double>> getRank(size_t col, const VectorIndex& _vIndex, RankingStrategy _strat) const;
        std::vector<std::complex<double>> getZScore(size_t col, const VectorIndex& _vIndex) const;
        std::vector<std::complex<double>> getBins(size_t col, size_t nBins) const;

        bool smooth(VectorIndex _vLine, VectorIndex _vCol, NumeRe::FilterSettings _settings, AppDir Direction = ALL);
        bool retouch(VectorIndex _vLine, VectorIndex _vCol, AppDir Direction = ALL);
        bool resample(VectorIndex _vLine, VectorIndex _vCol, std::pair<size_t,size_t> samples, AppDir Direction = ALL, std::string sFilter = "lanczos3");

        std::vector<AnovaResult> getAnova(const VectorIndex& colCategories, size_t colValues, const VectorIndex& _vIndex, double significance) const;


        static KmeansInit stringToKmeansInit(const std::string& init_type);
        KMeansResult getKMeans(const VectorIndex& colCategories, size_t nClusters, size_t maxIterations, Memory::KmeansInit init_method) const;
};

#endif

