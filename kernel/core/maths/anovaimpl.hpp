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
#include <gsl/gsl_cdf.h>
#include "../maths/anova.hpp"

#include <vector>

#ifndef ANOVAIMPL_HPP
#define ANOVAIMPL_HPP

class Memory;

/////////////////////////////////////////////////
/// \brief This class is representing a Node in the Anova
/// calculation Tree.
/////////////////////////////////////////////////
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
    }

    void calculateSS(const mu::value_type overallMean);
    void calculateMean(const Memory *mem, const std::vector<std::vector<std::string>> &factors, size_t facIdx);
    void calculateDof(size_t factorCnt);
};

/////////////////////////////////////////////////
/// \brief This class provides the functionallity to calculate n way anova
///  in an recursive Tree structure.
/////////////////////////////////////////////////
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

    void buildTreeHelper(FactorNode* node, int start, int n, std::vector<size_t>& currentSet);
    void calculateLevel(FactorNode* node,  size_t depth);

    void calculateSSInteraction(FactorNode* node);
    void calculateSSWithin(FactorNode* node);
    std::vector<mu::value_type> getAllSubSetSS(std::vector<size_t> set);
    void getAllChild_SS_helper(FactorNode* node, std::vector<size_t> set, std::vector<mu::value_type> &retvec);
    bool isSubSet(std::vector<size_t> set, std::vector<size_t> subSet);
    void getResultsHelper(FactorNode* node, std::vector<AnovaResult>& res, size_t depth);

public:

    AnovaCalculationStructure();
    void buildTree(std::vector<std::vector<std::string>> _factors, Memory* _mem, double _significance);
    void calculateResults();
    std::vector<AnovaResult> getResults();
};
#endif
