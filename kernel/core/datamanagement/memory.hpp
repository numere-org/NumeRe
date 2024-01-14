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
#include <gsl/gsl_cdf.h>

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

/////////////////////////////////////////////////
/// \brief Contains the relevant results of the
/// ANOVA F test.
/////////////////////////////////////////////////
struct AnovaResult
{
    std::string prefix;
    mu::value_type m_FRatio;
    mu::value_type m_significanceVal;
    mu::value_type m_significance;
    bool m_isSignificant;
    size_t m_numCategories;
};

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

        enum RankingStrategy
        {
            RANK_DENSE,
            RANK_COMPETETIVE,
            RANK_FRACTIONAL
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
		bool convertEmptyColumns(const VectorIndex& _vCol, const std::string& _sType);
		bool setCategories(const VectorIndex& _vCol, const std::vector<std::string>& vCategories);
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
		ValueVector readMemAsString(const VectorIndex& _vLine, const VectorIndex& _vCol) const;
		TableColumn::ColumnType getType(const VectorIndex& _vCol) const;
		bool isValueLike(const VectorIndex& _vCol) const;
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
		void writeData(Indices& _idx, mu::value_type* _dData, size_t _nNum);
		void writeData(Indices& _idx, const ValueVector& _values);
		bool setHeadLineElement(size_t _i, const std::string& _sHead);
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
        std::vector<mu::value_type> findCols(const std::vector<std::string>& vColNames, bool enableRegEx) const;
        std::vector<mu::value_type> countIfEqual(const VectorIndex& _vCols, const std::vector<mu::value_type>& vValues, const std::vector<std::string>& vStringValues) const;
        std::vector<mu::value_type> getIndex(size_t col, const std::vector<mu::value_type>& vValues, const std::vector<std::string>& vStringValues) const;
        mu::value_type getCovariance(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2) const;
        mu::value_type getPearsonCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2) const;
        mu::value_type getSpearmanCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2) const;
        std::vector<mu::value_type> getRank(size_t col, const VectorIndex& _vIndex, RankingStrategy _strat) const;
        std::vector<mu::value_type> getZScore(size_t col, const VectorIndex& _vIndex) const;
        std::vector<mu::value_type> getBins(size_t col, size_t nBins) const;

        bool smooth(VectorIndex _vLine, VectorIndex _vCol, NumeRe::FilterSettings _settings, AppDir Direction = ALL);
        bool retouch(VectorIndex _vLine, VectorIndex _vCol, AppDir Direction = ALL);
        bool resample(VectorIndex _vLine, VectorIndex _vCol, std::pair<size_t,size_t> samples, AppDir Direction = ALL, std::string sFilter = "lanczos3");


        class FactorNode
        {
        public:
            FactorNode* parent = nullptr;
            std::vector<FactorNode*> children;

            // this is used for "navigation" in the tree, to access correct elements
            std::vector<size_t> subset;

            std::vector<std::vector<mu::value_type>> catIndex;
            std::vector<mu::value_type> means;
            std::vector<mu::value_type> nums;
            mu::value_type SS;
            mu::value_type SS_interaction;
            double dof;

            /////////////////////////////////////////////////
            /// \brief Constructor of FactorNode
            ///
            /// \param s
            /// \return FactorNode(const std::vector<size_t> s):
            ///
            /////////////////////////////////////////////////
            FactorNode(const std::vector<size_t> s) : subset(s) {}

            /////////////////////////////////////////////////
            /// \brief This function adds a chid element to the FactorNode
            ///
            /// \param child FactorNode*
            /// \return void
            ///
            /////////////////////////////////////////////////
            void addChild(FactorNode* child)
            {
                children.push_back(child);
            }

            /////////////////////////////////////////////////
            /// \brief Destructor of FactorNode
            ///
            ///
            /////////////////////////////////////////////////
            ~FactorNode()
            {
                for (auto child : children)
                    delete child;

                // ne wir kommen ja vom parent ?
                //if(parent != nullptr)
                //    delete parent;
            }
        };

        class AnovaCalculationStructure
        {
        private:
            FactorNode* root;
            Memory* mem;
            double significance = 0;
            std::vector<std::vector<std::string>> factors;

            mu::value_type overallMean;
            mu::value_type overallNum;
            mu::value_type SS_Within;
            double dof_within = 0;
            size_t max_depth = 0;

            /////////////////////////////////////////////////
            /// \brief This helper function does build the Tree up to a given level n
            ///
            /// \param node FactorNode*
            /// \param start int
            /// \param n int
            /// \param currentSet std::vector<size_t>&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void buildTreeHelper(FactorNode* node, int start, int n, std::vector<size_t>& currentSet)
            {
                if (start > n) return; // Base case: no more elements to add

                //depth is given by size of subset
                if(max_depth < currentSet.size()+1)
                    max_depth = currentSet.size()+1;

                for (size_t i = start; i <= n; ++i)
                {
                    std::vector<size_t> newSubset = currentSet;
                    newSubset.push_back(i);
                    FactorNode* child = new FactorNode(newSubset);

                    // we are at level > 2
                    if(currentSet.size() > 0)
                        child->parent = node;

                    std::stringstream ss;
                    for(auto i : newSubset)
                        ss << i << ",";
                    std::string test = ss.str();

                    //calculate SS for new child
                    calculateMean(child, i);
                    calculateDof(child, factors[i].size());

                    node->addChild(child);
                    buildTreeHelper(child, i + 1, n, newSubset);
                }
            }

            /////////////////////////////////////////////////
            /// \brief This function calculates the mean and num values for the given node by
            ///  first splitting up all values into factor groups and then caluclation of means of these gropus
            ///
            /// \param node FactorNode*
            /// \param facIdx size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void calculateMean(FactorNode* node, size_t facIdx)
            {
                Memory tmp_mem(0);
                for (size_t i = 0; i < factors[facIdx].size(); i++)
                {
                    //positions of all elements, which correspond to the passed values
                    std::vector<mu::value_type> catIndex1 = mem->getIndex(facIdx+1, std::vector<mu::value_type>(), std::vector<std::string>(1, factors[facIdx][i]));

                    if (mu::isnan(catIndex1.front()))
                        continue;

                    if (node->parent == nullptr)
                    {
                        tmp_mem.memArray.push_back(TblColPtr(mem->memArray[0]->copy(VectorIndex(&catIndex1[0], catIndex1.size(), 0))));
                        node->catIndex.push_back(catIndex1);
                    } else
                    {
                        // Intersect with parent groups
                        std::vector<std::vector<mu::value_type>> catIndicesParent = node->parent->catIndex;
                        for(std::vector<mu::value_type> catIndex2 : catIndicesParent)
                        {
                            std::vector<mu::value_type> intersection;
                            for(auto a : catIndex1)
                                for(auto b : catIndex2)
                                    if(a == b)
                                        intersection.push_back(a);

                            int siz = intersection.size();
                            if (intersection.size() == 0)
                                continue;

                            tmp_mem.memArray.push_back(TblColPtr(mem->memArray[0]->copy(VectorIndex(&intersection[0], intersection.size(), 0))));
                            node->catIndex.push_back(intersection);
                        }
                    }
                }
                for (size_t i = 0; i < tmp_mem.memArray.size(); i++)
                {
                    node->means.push_back(tmp_mem.avg(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(i)));
                    node->nums.push_back(tmp_mem.num(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(i)));
                    //no need for variance ?
                    //vVar_[i].push_back(intPower(groupedValues[i].std(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(j)), 2));
                }
            }

            /////////////////////////////////////////////////
            /// \brief This function is used to trigger calculations for depth after depth level, since values of
            ///  previous depths are needed
            ///
            /// \param node FactorNode*
            /// \param depth size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void calculateLevel(FactorNode* node,  size_t depth)
            {
                if(node->subset.size() == max_depth)
                    calculateSSWithin(node);

                if(node->subset.size() == depth)
                {
                    calculateSS(node);
                    calculateSSInteraction(node);
                    return;
                }

                for (auto c : node->children)
                    calculateLevel(c, depth);

                return;
            }

            /////////////////////////////////////////////////
            /// \brief calculation of Sum of Squares of given node
            ///
            /// \param node FactorNode*
            /// \return void
            ///
            /////////////////////////////////////////////////
            void calculateSS(FactorNode* node)
            {
                for(size_t i = 0; i < node->means.size(); i++)
                    node->SS += node->nums[i] * intPower(node->means[i]-overallMean,2);
            }

            /////////////////////////////////////////////////
            /// \brief calculation of SS Interaction of given node
            ///
            /// \param node FactorNode*
            /// \return void
            ///
            /////////////////////////////////////////////////
            void calculateSSInteraction(FactorNode* node)
            {
                //no interaction effect if level 1
                if(node->parent == nullptr)
                    return;

                std::vector<mu::value_type> child_SS = getAllSubSetSS(node->subset);
                node->SS_interaction = node->SS - std::accumulate(child_SS.begin(), child_SS.end(),mu::value_type());
            }

            /////////////////////////////////////////////////
            /// \brief calculation of Degrees of fredom of given node
            ///
            /// \param node FactorNode*
            /// \param factorCnt size_t
            ///
            /////////////////////////////////////////////////
            calculateDof(FactorNode* node, size_t factorCnt)
            {
                node->dof = (factorCnt-1);
                if(node->parent != nullptr)
                    node->dof *= node->parent->dof;
            }

            /////////////////////////////////////////////////
            /// \brief calculation of SSWithing using the provided node
            ///
            /// \param node FactorNode*
            /// \return void
            ///
            /////////////////////////////////////////////////
            void calculateSSWithin(FactorNode* node)
            {
                //make sure it is 0
                SS_Within = 0;
                dof_within = overallNum.real() - node->catIndex.size();

                for(size_t i = 0; i < node->catIndex.size(); i++)
                {
                    for(mu::value_type idx : node->catIndex[i])
                    {
                        // these indeces start at 1 ?
                        mu::value_type val_j = mem->memArray[0].get()->getValue(idx.real()-1);
                        SS_Within +=  intPower(val_j - node->means[i],2);
                    }
                }
            }


            /////////////////////////////////////////////////
            /// \brief get all SS of elements which are subset of set
            ///
            /// \param set std::vector<size_t>
            /// \return std::vector<mu::value_type>
            ///
            /////////////////////////////////////////////////
            std::vector<mu::value_type> getAllSubSetSS(std::vector<size_t> set)
            {
                std::vector<mu::value_type> retvec;
                getAllChild_SS_helper(root, set, retvec);
                return retvec;
            }

            /////////////////////////////////////////////////
            /// \brief helper function to get all SS of elements which are subset of set
            ///
            /// \param node FactorNode*
            /// \param set std::vector<size_t>
            /// \param retvec std::vector<mu::value_type>&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void getAllChild_SS_helper(FactorNode* node, std::vector<size_t> set, std::vector<mu::value_type> &retvec)
            {
                if (isSubSet(set, node->subset))
                {
                    retvec.push_back(node->subset.size() == 1 ? node->SS : node->SS_interaction);
                    for (auto c : node->children)
                        getAllChild_SS_helper(c, set, retvec);
                }
                return;
            }


            /////////////////////////////////////////////////
            /// \brief This function will checks if the given subSet is actually a valid subset of set
            ///
            /// \param set std::vector<size_t>
            /// \param subSet std::vector<size_t>
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isSubSet(std::vector<size_t> set, std::vector<size_t> subSet)  //todo static outside ?
            {
                for (auto s : subSet) {
                    if (std::find(set.begin(), set.end(), s) == set.end())
                        return false;
                    }
                return true;
            }


            /////////////////////////////////////////////////
            /// \brief This is the helper function to navigate trhough the Tree and
            ///     get all needed Results for the given depth level
            ///
            /// \param node FactorNode*
            /// \param res std::vector<AnovaResult>&
            /// \param depth size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void getResultsHelper(FactorNode* node, std::vector<AnovaResult>& res, size_t depth)
            {
                if(node->subset.size() == depth)
                {
                    AnovaResult r;
                    mu::value_type SS = node->subset.size() > 1 ? node->SS_interaction : node->SS ;
                    double dof = node->dof;
                    SS /= dof;
                    SS /= SS_Within;

                    std::stringstream ss;
                    if(node->subset.size() == 0)
                        return;

                    ss << "Factor" << node->subset[0];
                    for(int i = 1; i < node->subset.size(); i++)
                        ss << "x" << node->subset[i];

                    r.prefix = ss.str();
                    r.m_FRatio = SS;
                    r.m_significanceVal = gsl_cdf_fdist_Pinv(1.0 - significance, dof, dof_within);
                    r.m_significance = significance;
                    r.m_isSignificant = SS.real() >= r.m_significanceVal.real();
                    r.m_numCategories = node->means.size();
                    res.push_back(r);
                    return;
                }

                for (auto c : node->children)
                    getResultsHelper(c, res, depth);

                return;
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Constructor of AnovaCalculationStructure
            ///
            ///
            /////////////////////////////////////////////////
            AnovaCalculationStructure()
            {
                root = new FactorNode({});
            }

            /////////////////////////////////////////////////
            /// \brief This function will construct the Tree structure from the
            ///     given factors and the provided memory from the table columns the anova
            ///     is performed on.
            ///
            /// \param _factors std::vector<std::vector<std::string>>
            /// \param _mem Memory*
            /// \param _significance double
            /// \return void
            ///
            /////////////////////////////////////////////////
            void buildTree(std::vector<std::vector<std::string>> _factors, Memory* _mem, double _significance)
            {
                significance = _significance;
                factors = _factors;
                mem = _mem;
                overallMean = _mem->avg(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0));
                overallNum = _mem->num(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0));

                size_t n = factors.size()-1;
                std::vector<size_t> startSet;
                buildTreeHelper(root, 0, n, startSet);
            }

            /////////////////////////////////////////////////
            /// \brief This function will trigger all calculations to perform the Anova
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void calculateResults()
            {
                overallMean = mem->avg(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0));
                overallNum = mem->num(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0));
                for(int l = 1; l <= max_depth; l++)
                    calculateLevel(root, l);

                SS_Within /= dof_within;
            }

            /////////////////////////////////////////////////
            /// \brief This function will return the result of the anova calculation.
            ///
            /// \return std::vector<AnovaResult>
            ///
            /////////////////////////////////////////////////
            std::vector<AnovaResult> getResults()
            {
                std::vector<AnovaResult> res;
                for(size_t d = 1; d <= max_depth; d++)
                    getResultsHelper(root, res, d);
                return res;
            }
        };


        std::vector<AnovaResult> getAnova(const VectorIndex& colCategories, size_t colValues, const VectorIndex& _vIndex, double significance) const;

};

#endif

