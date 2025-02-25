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

#include "../io/logger.hpp"

mu::Value overall;

/////////////////////////////////////////////////
/// \brief This function calculates the mean and
/// num values for the given node by first
/// splitting up values into factor groups and
/// then calculating the means of these groups.
///
/// \param mem const Memory*
/// \param factorSet const std::vector<std::string>&
/// \param facIdx size_t
/// \return void
///
/////////////////////////////////////////////////
void FactorNode::calculateMean(const Memory* mem, const std::vector<std::string>& factorSet, size_t facIdx)
{
    for (size_t i = 0; i < factorSet.size(); i++)
    {
        // positions of all elements, which correspond to the passed values
        mu::Array catIndex1 = mem->getIndex(facIdx+1, std::vector<std::string>(1, factorSet[i]));

        if (!catIndex1.front().isValid())
            continue;

        if (parent == nullptr)
        {
            means.push_back(mem->avg(catIndex1, VectorIndex(0)));
            nums.push_back(mem->num(catIndex1, VectorIndex(0)));

            catIndex.push_back(catIndex1);
        }
        else
        {
            // Intersect with parent groups
            const std::vector<mu::Array>& catIndicesParent = parent->catIndex;

            for (const mu::Array& catIndex2 : catIndicesParent)
            {
                mu::Array intersection;

                for (const auto& a : catIndex1)
                    for (const auto& b : catIndex2)
                        if (a == b)
                            intersection.push_back(a);

                if (intersection.size() == 0)
                    continue;

                means.push_back(mem->avg(intersection, VectorIndex(0)));
                nums.push_back(mem->num(intersection, VectorIndex(0)));


                catIndex.push_back(intersection);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief calculation of Sum of Squares of given node
///
/// \param overallMean const std::complex<double>&
/// \return void
///
/////////////////////////////////////////////////
void FactorNode::calculateSS(const std::complex<double>& overallMean)
{
    for (size_t i = 0; i < means.size(); i++)
    {
        if (!mu::isnan(means[i]))
            SS += nums[i] * intPower(means[i] - overallMean, 2); // S_B
    }
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

    if (parent != nullptr)
        dof *= parent->dof;
}

/////////////////////////////////////////////////
/// \brief This helper function does build the Tree up to a given level n
///
/// \param node FactorNode*
/// \param start size_t
/// \param n size_t
/// \param currentSet std::vector<size_t>&
/// \return void
///
/////////////////////////////////////////////////
void AnovaCalculationStructure::buildTreeHelper(FactorNode* node, size_t start, size_t n, std::vector<size_t>& currentSet)
{
    if (start > n)
        return; // Base case: no more elements to add

    // depth is given by size of subset
    if (max_depth < currentSet.size()+1)
        max_depth = currentSet.size()+1;

    for (size_t i = start; i <= n; ++i)
    {
        std::vector<size_t> newSubset = currentSet;
        newSubset.push_back(i);
        FactorNode* child = new FactorNode(newSubset);

        // we are at level > 2
        if (currentSet.size() > 0)
            child->parent = node;

        // calculate SS for new child
        child->calculateMean(mem, factors[i], i);
        child->calculateSS(overallMean);
        child->calculateDof(factors[i].size());

        // Use the category column names as prefixes
        child->name = mem->memArray[child->subset[0]+1]->m_sHeadLine;

        for (size_t i = 1; i < child->subset.size(); i++)
            child->name += " x " + mem->memArray[child->subset[i]+1]->m_sHeadLine;

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
    if (node->subset.size() == max_depth)
        calculateSSWithin(node);

    if (node->subset.size() == depth)
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
    if (node->parent == nullptr)
        return;

    auto nanAcc = [](const std::complex<double>& acc, const std::complex<double>& i) {return mu::isnan(i) ? acc : acc+i;};

    std::vector<std::complex<double>> child_SS = getAllSubSetSS(node->subset);
    node->SS_interaction = node->SS - std::accumulate(child_SS.begin(), child_SS.end(), std::complex<double>(), nanAcc);

    // Just a guess how to construct the correlation term (equals to 1 for a two-way)
    // Seems to correspond to the table at end of this article: https://en.wikipedia.org/wiki/Two-way_analysis_of_variance
    double correlation = std::count_if(child_SS.begin(), child_SS.end(), [](const std::complex<double>& i) {return i != 0.0;}) - 1.0;
    node->SS_interaction += correlation * overallNum * overallMean;
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

    for (size_t i = 0; i < node->catIndex.size(); i++)
    {
        if (mu::isnan(node->means[i]))
            continue;

        for (const auto& idx : node->catIndex[i])
        {
            // these indeces start at 1 ?
            std::complex<double> val_j = mem->readMem(idx.getNum().asI64()-1, 0).as_cmplx();

            if (!mu::isnan(val_j))
                SS_Within += intPower(val_j - node->means[i], 2); // S_W
        }
    }
}

/////////////////////////////////////////////////
/// \brief get all SS of elements which are subset of set
///
/// \param set const std::vector<size_t>&
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> AnovaCalculationStructure::getAllSubSetSS(const std::vector<size_t>& set)
{
    std::vector<std::complex<double>> retvec;
    getAllChild_SS_helper(root, set, retvec);
    return retvec;
}

/////////////////////////////////////////////////
/// \brief helper function to get all SS of elements which are subset of set
///
/// \param node FactorNode*
/// \param set const std::vector<size_t>&
/// \param retvec std::vector<std::complex<double>>&
/// \return void
///
/////////////////////////////////////////////////
void AnovaCalculationStructure::getAllChild_SS_helper(FactorNode* node, const std::vector<size_t>& set, std::vector<std::complex<double>>& retvec)
{
    if (isSubSet(set, node->subset))
    {
        retvec.push_back(node->subset.size() == 1 ? node->SS : node->SS_interaction);
        g_logger.info(node->name + ": " + toString(node->subset.size() == 1) + " -> " + toString(retvec.back(), 7));

        for (auto c : node->children)
            getAllChild_SS_helper(c, set, retvec);
    }
}


/////////////////////////////////////////////////
/// \brief This function will checks if the given subSet is actually a valid subset of set
///
/// \param set const std::vector<size_t>&
/// \param subSet const std::vector<size_t>&
/// \return bool
///
/////////////////////////////////////////////////
bool AnovaCalculationStructure::isSubSet(const std::vector<size_t>& set, const std::vector<size_t>& subSet)  //todo static outside ?
{
    for (auto s : subSet)
    {
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
    if (node->subset.size() == depth)
    {
        AnovaResult r;
        std::complex<double> SS = node->subset.size() > 1 ? node->SS_interaction : node->SS ;
        double dof = node->dof;
        SS /= dof; // MS_B
        SS /= SS_Within; //MS_B / MS_W

        if (node->subset.size() == 0)
            return;

        r.name = node->name;
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
    root->name = "root";
}

/////////////////////////////////////////////////
/// \brief Destructor of AnovaCalculationStructure
///
///
/////////////////////////////////////////////////
AnovaCalculationStructure::~AnovaCalculationStructure()
{
    delete root;
}

/////////////////////////////////////////////////
/// \brief This function will construct the Tree structure from the
///     given factors and the provided memory from the table columns the anova
///     is performed on.
///
/// \param _factors const std::vector<std::vector<std::string>>&
/// \param _mem Memory*
/// \param _significance double
/// \return void
///
/////////////////////////////////////////////////
void AnovaCalculationStructure::buildTree(const std::vector<std::vector<std::string>>& _factors, Memory* _mem, double _significance)
{
    significance = _significance;
    factors = _factors;
    mem = _mem;
    overallMean = _mem->avg(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0));
    overallNum = _mem->num(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0));

    overall = overallMean;

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
    for (size_t l = 1; l <= max_depth; l++)
        calculateLevel(root, l);

    SS_Within /= dof_within; // MS_W
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

    for (size_t d = 1; d <= max_depth; d++)
        getResultsHelper(root, res, d);

    return res;
}

