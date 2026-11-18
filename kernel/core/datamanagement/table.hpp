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

#include "tableview.hpp"

#include "tabledata.hpp"
#include "../maths/filtering.hpp"
#include "../maths/anovaimpl.hpp"
#include "../maths/units.hpp"

#ifndef TABLE_HPP
#define TABLE_HPP

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
typedef mu::Array ValueVector;

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
class Table
{
    public:
        enum AppDir
        {
            ALL = 0x0,
            ROWS = 0x1,
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

        TableData m_data;
        bool bSaveMutex;
        bool bSortCaseInsensitive;


        bool Allocate(size_t nCols, size_t nSheets, bool shrink = false);
        void createTableHeaders();
        bool clear();
        Boundary findValidBoundary(const VectorIndex& _vRow,
                                   const VectorIndex& _vCol,
                                   size_t inSheet,
                                   int i,
                                   int j) const;
        bool retouch1D(const VectorIndex& _vRow,
                       const VectorIndex& _vCol,
                       size_t inSheet,
                       AppDir Direction);
        bool retouch2D(const VectorIndex& _vRow,
                       const VectorIndex& _vCol,
                       size_t inSheet);
        bool onlyValidValues(const VectorIndex& _vRow,
                             const VectorIndex& _vCol,
                             const VectorIndex& _vSheet) const;
        void smoothingWindow1D(const VectorIndex& _vRow,
                               const VectorIndex& _vCol,
                               size_t inSheet,
                               size_t i,
                               size_t j,
                               NumeRe::Filter* _filter,
                               bool smoothLines);
        void smoothingWindow2D(const VectorIndex& _vRow,
                               const VectorIndex& _vCol,
                               size_t inSheet,
                               size_t i,
                               size_t j,
                               NumeRe::Filter* _filter);
        void calculateStats(const VectorIndex& _vRow,
                            const VectorIndex& _vCol,
                            const VectorIndex& _vSheet,
                            std::vector<StatsLogic>& operation) const;

    public:
        Table();
        Table(size_t _nCols, size_t _nSheets = 1);
        ~Table();

        Table& operator=(const Table& other);

        bool reserve(size_t _nCols, size_t _nSheets);
        bool isValid() const;
        bool isValidElement(size_t _nRow, size_t _nCol, size_t _nSheet) const;
        bool shrink(const VectorIndex& _vCol = VectorIndex(0, VectorIndex::OPEN_END),
                    const VectorIndex& _vSheets = VectorIndex(0, VectorIndex::OPEN_END));
        void convert();
        bool convertColumns(const VectorIndex& _vCol, const VectorIndex& _vSheets, const std::string& _sType);
        bool setCategories(const VectorIndex& _vCol, const VectorIndex& _vSheets, const std::vector<std::string>& vCategories);
        int getRows(size_t sheet = (size_t)-1) const;
        int getElemsInColumn(size_t col, size_t sheet) const;
        int getFilledElemsInColumn(size_t col, size_t sheet) const;
        int getCols(size_t sheet = (size_t)-1) const;
        int getSheets() const;
        size_t getSize() const;
        std::string printDims() const;


        // READ ACCESS METHODS
        mu::Value read(size_t _nRow, size_t _nCol, size_t _nSheet) const;
        std::complex<double> readMemInterpolated(double _dLine, double _dCol, size_t sheet) const;
        mu::Array read(const VectorIndex& _vRow,
                       const VectorIndex& _vCol,
                       const VectorIndex& _vSheet) const;
        Matrix readMemAsMatrix(const VectorIndex& _vRow,
                               const VectorIndex& _vCol) const;
        TableColumn::ColumnType getType(const VectorIndex& _vCol,
                                        const VectorIndex& _vSheets) const;
        bool isValueLike(const VectorIndex& _vCol,
                         const VectorIndex& _vSheets) const;
        mu::Array getCategoryList(const VectorIndex& _vCol,
                                  const VectorIndex& _vSheets) const;
        Table* extractRange(const VectorIndex& _vRow,
                            const VectorIndex& _vCol,
                            const VectorIndex& _vSheets) const;
        void copyElementsInto(mu::Variable* vTarget,
                              const VectorIndex& _vRow,
                              const VectorIndex& _vCol,
                              const VectorIndex& _vSheet) const;
        std::string getSheetName(size_t sheet) const;
        std::string getHeadLineElement(size_t _i, size_t sheet) const;
        std::vector<std::string> getHeadLineElement(const VectorIndex& _vCol,
                                                    const VectorIndex& _vSheet) const;
        std::string getUnit(int nCol, size_t sheet) const;
        std::vector<std::complex<double>> asSiUnits(size_t nCol, size_t nSheet, UnitConversionMode mode) const;
        std::string showUnitConversion(size_t nCol, size_t nSheet, UnitConversionMode mode) const;
        size_t getHeadlineCount(size_t sheet) const;
        std::string getComment() const;
        NumeRe::TableMetaData getMetaData() const;

        // WRITE ACCESS METHODS
        void write(int _nRow, int _nCol, int _nSheet, const mu::Value& _value);
        void writeDirect(int _nRow, int _nCol, int _nSheet, const std::complex<double>& _dData);
        void writeUnsafe(int _nRow, int _nCol, int _nSheet, const std::complex<double>& _dData);
        void write(Indices& _idx, const mu::Array& _values);
        bool setSheetName(size_t sheet, const std::string& _sName);
        bool setHeadLineElement(size_t _i, size_t sheet, const std::string& _sHead);
        bool setUnit(int nCol, size_t sheet, const std::string& sUnit);
        std::vector<std::string> toSiUnits(const VectorIndex& _vCols, const VectorIndex& _vSheets, UnitConversionMode mode);
        void writeComment(const std::string& comment);
        void setMetaData(const NumeRe::TableMetaData& meta);
        void markModified();

        bool save(std::string _sFileName, const std::string& sTableName, unsigned short nPrecision, std::string sExt = "");
        bool getSaveStatus() const;
        void setSaveStatus(bool _bIsSaved);
        long long int getLastSaved() const;
        std::vector<int> sortElements(const VectorIndex& _vRow,
                                      const VectorIndex& _vCol,
                                      const VectorIndex& _vSheet,
                                      const std::string& sSortingExpression = "");
        void deleteBulk(const VectorIndex& _vRow,
                        const VectorIndex& _vCol,
                        const VectorIndex& _vSheet);
        NumeRe::TableView extractView(const std::string& _sTable,
                                      const VectorIndex& rows,
                                      const VectorIndex& cols,
                                      const VectorIndex& sheets);
        void importView(NumeRe::TableView _table,
                        const VectorIndex& rows,
                        const VectorIndex& cols,
                        const VectorIndex& sheets);
        void insertCopiedView(NumeRe::TableView _table,
                              const VectorIndex& rows,
                              const VectorIndex& cols,
                              const VectorIndex& sheets,
                              bool transpose);

        bool insertBlock(size_t atRow, size_t atCol, size_t atSheet, size_t rows, size_t cols);
        bool insertCols(size_t atCol, size_t atSheet, size_t num);
        bool insertRows(size_t atRow, size_t atSheet, size_t num);
        bool insertSheets(size_t atSheet, size_t num);
        bool removeBlock(size_t atRow, size_t atCol, size_t atSheet, size_t rows, size_t cols);
        bool removeCols(const VectorIndex& _vCols, size_t atSheet);
        bool removeRows(const VectorIndex& _vRows, size_t atSheet);
        bool removeSheets(const VectorIndex& _vSheets);
        bool reorderCols(const VectorIndex& _vCols, size_t atSheet, const VectorIndex& _vNewOrder);
        bool reorderRows(const VectorIndex& _vRows, size_t atSheet, const VectorIndex& _vNewOrder);
        bool reorderSheets(const VectorIndex& _vSheets, const VectorIndex& _vNewOrder);
        bool replaceVals(const VectorIndex& _vCols, size_t atSheet, const mu::Array& _oldVals, const mu::Array& _newVals, bool enableRegEx);

        std::pair<double, double> minmax() const;

        // MAFIMPLEMENTATIONS
        std::complex<double> std(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> avg(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> max(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> min(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> prd(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> sum(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> num(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> and_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> or_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> xor_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> cnt(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> norm(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> med(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> exc(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> skew(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> stderr_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;
        std::complex<double> rms(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const;

        std::complex<double> pct(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet, std::complex<double> dPct) const;
        std::complex<double> cmp(const VectorIndex& _vRow, const VectorIndex& _vCol, size_t sheet, std::complex<double> dRef, int _nType) const;

        std::vector<std::complex<double>> size(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, const VectorIndex& _slicesIdx, int dir) const;
        std::vector<std::complex<double>> minpos(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, size_t sheet, int dir) const;
        std::vector<std::complex<double>> maxpos(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, size_t sheet, int dir) const;
        std::vector<size_t> findCols(const std::vector<std::string>& vColNames, size_t inSheet, bool enableRegEx, bool autoCreate);
        std::vector<size_t> findSheets(const std::vector<std::string>& vSheetNames, bool enableRegEx, bool autoCreate);
        std::vector<size_t> countIfEqual(const VectorIndex& _vCols, size_t inSheet, const mu::Array& vValues, bool enableRegEx) const;
        mu::Array getIndex(size_t col, size_t inSheet, const mu::Array& vValues, bool enableRegEx) const;
        double getCovariance(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2, size_t inSheet) const;
        double getPearsonCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2, size_t inSheet) const;
        double getSpearmanCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2, size_t inSheet) const;
        std::vector<double> getRank(size_t col, const VectorIndex& _vIndex, size_t inSheet, RankingStrategy _strat) const;
        std::vector<std::complex<double>> getZScore(size_t col, const VectorIndex& _vIndex, size_t inSheet) const;
        std::vector<int64_t> getBins(size_t col, size_t inSheet, size_t nBins) const;

        bool smooth(VectorIndex _vRow,
                    VectorIndex _vCol,
                    size_t inSheet,
                    NumeRe::FilterSettings _settings,
                    AppDir Direction = ALL);
        bool retouch(VectorIndex _vRow,
                     VectorIndex _vCol,
                     size_t inSheet,
                     AppDir Direction = ALL);
        bool resample(VectorIndex _vRow,
                      VectorIndex _vCol,
                      size_t inSheet,
                      std::pair<size_t, size_t> samples,
                      AppDir Direction = ALL,
                      std::string sFilter = "lanczos3");

        std::vector<AnovaResult> getAnova(const VectorIndex& colCategories,
                                          size_t colValues,
                                          const VectorIndex& _vIndex,
                                          size_t inSheet,
                                          double significance) const;
        static KmeansInit stringToKmeansInit(const std::string& init_type);
        KMeansResult getKMeans(const VectorIndex& colCategories,
                               size_t inSheets,
                               size_t nClusters,
                               size_t maxIterations,
                               Table::KmeansInit init_method) const;
};

#endif

