/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024 Erik Haenel et al.

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

#include "anovaimpl.hpp"
#include "../datamanagement/memory.hpp"

/////////////////////////////////////////////////
/// \brief This function calculates the mean and num values for the given node by
///  first splitting up all values into factor groups and then caluclation of means of these gropus
///
/// \param node FactorNode*
/// \param facIdx size_t
/// \return void
///
/////////////////////////////////////////////////
void FactorNode::calculateMean(const Memory *mem, const std::vector<std::vector<std::string>> &factors, size_t facIdx)
{
    Memory tmp_mem(0);
    for (size_t i = 0; i < factors[facIdx].size(); i++)
    {
        //positions of all elements, which correspond to the passed values
        std::vector<std::complex<double>> catIndex1 = mem->getIndex(facIdx+1, std::vector<std::complex<double>>(), std::vector<std::string>(1, factors[facIdx][i]));

        if (mu::isnan(catIndex1.front()))
            continue;

        if (parent == nullptr)
        {
            tmp_mem.memArray.push_back(TblColPtr(mem->memArray[0]->copy(VectorIndex(&catIndex1[0], catIndex1.size(), 0))));
            catIndex.push_back(catIndex1);
        } else
        {
            // Intersect with parent groups
            std::vector<std::vector<std::complex<double>>> catIndicesParent = parent->catIndex;
            for(std::vector<std::complex<double>> catIndex2 : catIndicesParent)
            {
                std::vector<std::complex<double>> intersection;
                for(auto a : catIndex1)
                    for(auto b : catIndex2)
                        if(a == b)
                            intersection.push_back(a);

                if (intersection.size() == 0)
                    continue;

                tmp_mem.memArray.push_back(TblColPtr(mem->memArray[0]->copy(VectorIndex(&intersection[0], intersection.size(), 0))));
                catIndex.push_back(intersection);
            }
        }
    }
    for (size_t i = 0; i < tmp_mem.memArray.size(); i++)
    {
        means.push_back(tmp_mem.avg(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(i)));
        nums.push_back(tmp_mem.num(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(i)));
    }
}

/////////////////////////////////////////////////
/// \brief calculation of Sum of Squares of given node
///
/// \param node FactorNode*
/// \return void
///
/////////////////////////////////////////////////
void FactorNode::calculateSS(const std::complex<double> overallMean)
{
    for(size_t i = 0; i < means.size(); i++)
        SS += nums[i] * intPower(means[i]-overallMean,2);
}

/////////////////////////////////////////////////
/// \brief calculation of Degrees of fredom of given node
///
/// \param node FactorNode*
/// \param factorCnt size_t
///
/////////////////////////////////////////////////
void FactorNode::calculateDof(size_t factorCnt)
{
    dof = (factorCnt-1);
    if(parent != nullptr)
        dof *= parent->dof;
}

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
void AnovaCalculationStructure::buildTreeHelper(FactorNode* node, int start, int n, std::vector<size_t>& currentSet)
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

        //calculate SS for new child
        child->calculateMean(mem, factors, i);
        child->calculateSS(overallMean);
        child->calculateDof(factors[i].size());

        node->addChild(child);
        buildTreeHelper(child, i + 1, n, newSubset);
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
void AnovaCalculationStructure::calculateLevel(FactorNode* node,  size_t depth)
{
    if(node->subset.size() == max_depth)
        calculateSSWithin(node);

    if(node->subset.size() == depth)
    {
        calculateSSInteraction(node);
        return;
    }

    for (auto c : node->children)
        calculateLevel(c, depth);

    return;
}

/////////////////////////////////////////////////
/// \brief calculation of SS Interaction of given node
///
/// \param node FactorNode*
/// \return void
///
/////////////////////////////////////////////////
void AnovaCalculationStructure::calculateSSInteraction(FactorNode* node)
{
    //no interaction effect if level 1
    if(node->parent == nullptr)
        return;

    std::vector<std::complex<double>> child_SS = getAllSubSetSS(node->subset);
    node->SS_interaction = node->SS - std::accumulate(child_SS.begin(), child_SS.end(),std::complex<double>());
}

/////////////////////////////////////////////////
/// \brief calculation of SSWithing using the provided node
///
/// \param node FactorNode*
/// \return void
///
/////////////////////////////////////////////////
void AnovaCalculationStructure::calculateSSWithin(FactorNode* node)
{
    //make sure it is 0
    SS_Within = 0;
    dof_within = overallNum.real() - node->catIndex.size();

    for(size_t i = 0; i < node->catIndex.size(); i++)
    {
        for(std::complex<double> idx : node->catIndex[i])
        {
            // these indeces start at 1 ?
            std::complex<double> val_j = mem->memArray[0].get()->getValue(idx.real()-1);
            SS_Within +=  intPower(val_j - node->means[i],2);
        }
    }
}

/////////////////////////////////////////////////
/// \brief get all SS of elements which are subset of set
///
/// \param set std::vector<size_t>
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> AnovaCalculationStructure::getAllSubSetSS(std::vector<size_t> set)
{
    std::vector<std::complex<double>> retvec;
    getAllChild_SS_helper(root, set, retvec);
    return retvec;
}

/////////////////////////////////////////////////
/// \brief helper function to get all SS of elements which are subset of set
///
/// \param node FactorNode*
/// \param set std::vector<size_t>
/// \param retvec std::vector<std::complex<double>>&
/// \return void
///
/////////////////////////////////////////////////
void AnovaCalculationStructure::getAllChild_SS_helper(FactorNode* node, std::vector<size_t> set, std::vector<std::complex<double>> &retvec)
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
bool AnovaCalculationStructure::isSubSet(std::vector<size_t> set, std::vector<size_t> subSet)  //todo static outside ?
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
void AnovaCalculationStructure::getResultsHelper(FactorNode* node, std::vector<AnovaResult>& res, size_t depth)
{
    if(node->subset.size() == depth)
    {
        AnovaResult r;
        std::complex<double> SS = node->subset.size() > 1 ? node->SS_interaction : node->SS ;
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

/////////////////////////////////////////////////
/// \brief Constructor of AnovaCalculationStructure
///
///
/////////////////////////////////////////////////
AnovaCalculationStructure::AnovaCalculationStructure()
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
void AnovaCalculationStructure::buildTree(std::vector<std::vector<std::string>> _factors, Memory* _mem, double _significance)
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
void AnovaCalculationStructure::calculateResults()
{
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
std::vector<AnovaResult> AnovaCalculationStructure::getResults()
{
    std::vector<AnovaResult> res;
    for(size_t d = 1; d <= max_depth; d++)
        getResultsHelper(root, res, d);
    return res;
}
