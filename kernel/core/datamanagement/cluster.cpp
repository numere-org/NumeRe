/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include <gsl/gsl_statistics.h>

#include "cluster.hpp"
#include "../../kernel.hpp"
#include "../ui/error.hpp"

namespace NumeRe
{
    //
    // class CLUSTER
    //
    //

    /////////////////////////////////////////////////
    /// \brief Private cluster copy assignment
    /// function.
    ///
    /// \param cluster const Cluster&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::assign(const Cluster& cluster)
    {
        bSortCaseInsensitive = false;
        mu::Array::operator=(cluster);
    }


    /////////////////////////////////////////////////
    /// \brief Private result assignment function for
    /// values using vectors as indices.
    ///
    /// \param _idx Indices
    /// \param data const mu::Array&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::assignVectorResults(Indices _idx, const mu::Array& data)
    {
        if (getCommonType() != data.getCommonType())
            m_commonType = mu::TYPE_VOID;

        if (data.size() == 1)
            _idx.row.setOpenEndIndex(std::max((int64_t)_idx.row.front(), (int64_t)size() - 1));

        // Do we want to create a hierarchy?
        if (_idx.row.size() == 1 && data.size() > 1)
        {
            // Expand the current cluster on-the-fly
            if (_idx.row.front() >= (int)size())
                resize(_idx.row.front()+1, mu::Value(NAN));

            mu::Array::get(_idx.row.front()) = mu::Value(data);
            dereference();
            return;
        }

        // Assign the single results
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (data.size() > 1 && data.size() <= i)
                return;

            // Expand the current cluster on-the-fly
            if (_idx.row[i] >= (int)size())
                resize(_idx.row[i]+1, mu::Value(NAN));

            mu::Array::get(_idx.row[i]) = data.mu::Array::get(i);
        }

        dereference();
    }


    /////////////////////////////////////////////////
    /// \brief This private member function is an
    /// override for the sorter object.
    ///
    /// \param i int
    /// \param j int
    /// \param col int
    /// \return int
    ///
    /////////////////////////////////////////////////
    int Cluster::compare(int i, int j, int col)
    {
        // Handle case insensitive
        if (bSortCaseInsensitive && mu::Array::get(i).isString() && mu::Array::get(j).isString())
        {
            if (toLowerCase(mu::Array::get(i).getStr()) < toLowerCase(mu::Array::get(j).getStr()))
                return -1;

            if (toLowerCase(mu::Array::get(i).getStr()) == toLowerCase(mu::Array::get(j).getStr()))
                return 0;

            return 1;
        }

        if (mu::Array::get(i) < mu::Array::get(j))
            return -1;

        if (mu::Array::get(i) == mu::Array::get(j))
            return 0;

        return 1; // Sort NAN to the end
    }


    /////////////////////////////////////////////////
    /// \brief This private member function is an
    /// override for the sorter object.
    ///
    /// \param line int
    /// \param col int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Cluster::isValue(int line, int col)
    {
        return mu::Array::get(line).isValid();
    }


    /////////////////////////////////////////////////
    /// \brief This private member function reorders
    /// the elements in the cluster based upon the
    /// passed index vector.
    ///
    /// \param vIndex std::vector<int>
    /// \param original const VectorIndex&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::reorderElements(std::vector<int> vIndex, const VectorIndex& original)
    {
        mu::Array vSortVector(*this);

        // Copy the contents directly from the
        // prepared in the new order
        for (size_t i = 0; i < original.size(); i++)
        {
            mu::Array::get(original[i]) = vSortVector[vIndex[i]];
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function inserts the data
    /// of all cluster items memory into the pointer
    /// passed to the function. This function is used
    /// for cached memory accesses.
    ///
    /// \param vTarget mu::Variable*
    /// \param _vLine const VectorIndex&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::insertDataInArray(mu::Variable* vTarget, const VectorIndex& _vLine)
    {
        if (vTarget == nullptr)
            return;

        vTarget->clear();

        // Try to resize the array as copy-efficient as
        // possible
        if (_vLine.size() > 1 && !size())
            vTarget->resize(1, mu::Value());
        else if (_vLine.size() == 1 && _vLine.front() < (int)size() && mu::Array::get(_vLine.front()).isArray())
            *vTarget = mu::Array::get(_vLine.front()).getArray();
        else
        {
            vTarget->resize(_vLine.size(), mu::Value());

            // Insert the elements in the passed array
            #pragma omp parallel for if (_vLine.size() > 100000)
            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (_vLine[i] < (int)size() && _vLine[i] >= 0)
                {
                    (*vTarget)[i] = mu::Array::get(_vLine[i]);
                }
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function assigns values as
    /// data for the all cluster items in memory. The
    /// type of the cluster items is adapted
    /// on-the-fly.
    ///
    /// \param a const mu::Array&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::setValueArray(const mu::Array& a)
    {
        if (a.getCommonType() == mu::TYPE_VOID)
        {
            clear();
            m_commonType = mu::TYPE_VOID;
            return;
        }

        mu::Array::operator=(a);
        dereference();
    }


    /////////////////////////////////////////////////
    /// \brief This member function assigns
    /// calculation results as data for the cluster
    /// items in memory, which are referenced by the
    /// passed indices. The type of the cluster items
    /// is adapted on-the-fly.
    ///
    /// \param _idx Indices
    /// \param data const mu::Array&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::assignResults(Indices _idx, const mu::Array& data)
    {
        if (data.getCommonType() == mu::TYPE_VOID)
        {
            clear();
            m_commonType = mu::TYPE_VOID;
            return;
        }

        // If the indices indicate a complete override
        // do that here and return
        if (_idx.row.isOpenEnd() && _idx.row.front() == 0)
        {
            setValueArray(data);
            m_commonType = mu::TYPE_VOID;
            return;
        }

        // Assign the results depending on the type of the
        // passed indices
        assignVectorResults(_idx, data);
    }


    /////////////////////////////////////////////////
    /// \brief This public member function provides
    /// access to the sorting algorithm for the
    /// cluster object.
    ///
    /// \param row const VectorIndex&
    /// \param sSortingExpression const std::string&
    /// \return std::vector<int>
    ///
    /////////////////////////////////////////////////
    std::vector<int> Cluster::sortElements(const VectorIndex& row, const std::string& sSortingExpression)
    {
        if (!size())
            return std::vector<int>();

        bool bReturnIndex = false;
        bSortCaseInsensitive = false;
        int nSign = 1;
        std::vector<int> vIndex;

        // Look for command line parameters
        if (findParameter(sSortingExpression, "desc"))
            nSign = -1;

        if (findParameter(sSortingExpression, "ignorecase"))
            bSortCaseInsensitive = true;

        if (findParameter(sSortingExpression, "index"))
            bReturnIndex = true;

        // Prepare the indices
        vIndex = row.getVector();

        // Sort everything
        if (!qSort(&vIndex[0], vIndex.size(), 0, 0, vIndex.size()-1, nSign))
            throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, "cluster{} " + sSortingExpression, SyntaxError::invalid_position);

        // If the sorting index is requested,
        // then only sort the first column and return
        if (!bReturnIndex)
            reorderElements(vIndex, row);
        else
        {
            // If the index was requested, increment every index by one
            for (size_t i = 0; i < vIndex.size(); i++)
                vIndex[i]++;
        }

        if (!bReturnIndex)
            return std::vector<int>();

        return vIndex;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function erases
    /// elements referenced by the passed VectorIndex.
    ///
    /// \param vLines const VectorIndex&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::deleteItems(const VectorIndex& vLines)
    {
        // Delete the cluster items first and
        // set the pointer to a nullpointer
        for (size_t i = 0; i < vLines.size(); i++)
        {
            if (vLines[i] < 0 || vLines[i] >= (int)size())
                continue;

            mu::Array::get(vLines[i]) = mu::Value();
        }

        auto iter = begin();

        // Remove all void entries from the array
        while (iter != end())
        {
            if (iter->getType() == mu::TYPE_VOID)
                iter = erase(iter);
            else
                ++iter;
        }

        m_commonType = mu::TYPE_VOID;
    }





    //
    // class CLUSTERMANAGER
    //
    //

    /////////////////////////////////////////////////
    /// \brief This member function detects, whether
    /// any cluster is used in the current expression.
    ///
    /// \param sCmdLine const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ClusterManager::containsClusters(const std::string& sCmdLine) const
    {
        if (sCmdLine.find('{') == std::string::npos)
            return false;

        size_t nQuotes = sCmdLine.front() == '"';
        size_t nVar2StrParserEnd = 0;

        // Search through the expression -> We do not need to examine the first character
        for (size_t i = 1; i < sCmdLine.length(); i++)
        {
            // Consider quotation marks
            if (sCmdLine[i] == '"' && sCmdLine[i-1] != '\\')
                nQuotes++;

            if (nQuotes % 2)
                continue;

            // Jump over the var2str parser operator
            if (sCmdLine[i-1] == '#')
            {
                while (sCmdLine[i] == '~')
                {
                    i++;
                    nVar2StrParserEnd = i;
                }
            }

            // Is this a candidate for a cluster
            if (sCmdLine[i] == '{'
                && (isalnum(sCmdLine[i-1])
                    || sCmdLine[i-1] == '_'
                    || sCmdLine[i-1] == '~'
                    || sCmdLine[i] == '['
                    || sCmdLine[i] == ']'))
            {
                size_t nStartPos = i-1;

                // Try to find the starting position
                while (nStartPos > nVar2StrParserEnd
                       && (isalnum(sCmdLine[nStartPos-1])
                           || sCmdLine[nStartPos-1] == '_'
                           || sCmdLine[nStartPos-1] == '~'
                           || sCmdLine[nStartPos-1] == '['
                           || sCmdLine[nStartPos-1] == ']'))
                {
                    nStartPos--;
                }

                // Try to find the candidate in the internal map
                if (isCluster(sCmdLine.substr(nStartPos, i - nStartPos)))
                    return true;
            }
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns true, if
    /// the passed cluster identifier can be found in
    /// the internal map.
    ///
    /// \param sCluster StringView
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ClusterManager::isCluster(StringView sCluster) const
    {
        mu::Variable* cluster = NumeReKernel::getInstance()->getParser().ReadVar(sCluster.subview(0, sCluster.find('{')).to_string());

        if (cluster && cluster->getCommonType() == mu::TYPE_CLUSTER)
            return true;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns true, if
    /// the passed cluster identifier can be found in
    /// the internal map.
    ///
    /// \param sCluster const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ClusterManager::isCluster(const std::string& sCluster) const
    {
        mu::Variable* cluster = NumeReKernel::getInstance()->getParser().ReadVar(sCluster.substr(0, sCluster.find('{')));

        if (cluster && cluster->getCommonType() == mu::TYPE_CLUSTER)
            return true;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns a
    /// reference to the cluster indicated by the
    /// passed cluster identifier.
    ///
    /// \param sCluster StringView
    /// \return mu::Variable&
    ///
    /////////////////////////////////////////////////
    mu::Variable& ClusterManager::getCluster(StringView sCluster)
    {
        mu::Variable* cluster = NumeReKernel::getInstance()->getParser().ReadVar(sCluster.subview(0, sCluster.find('{')).to_string());

        if (cluster && cluster->getCommonType() == mu::TYPE_CLUSTER)
            return *cluster;

        throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster.to_string(), sCluster.to_string());
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns a
    /// reference to the cluster indicated by the
    /// passed cluster identifier.
    ///
    /// \param sCluster const std::string&
    /// \return mu::Variable&
    ///
    /////////////////////////////////////////////////
    mu::Variable& ClusterManager::getCluster(const std::string& sCluster)
    {
        mu::Variable* cluster = NumeReKernel::getInstance()->getParser().ReadVar(sCluster.substr(0, sCluster.find('{')));

        if (cluster && cluster->getCommonType() == mu::TYPE_CLUSTER)
            return *cluster;

        throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster, sCluster);
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns a const
    /// reference to the cluster indicated by the
    /// passed cluster identifier. Used in context
    /// when this object is passed as const
    /// reference.
    ///
    /// \param sCluster const std::string&
    /// \return const mu::Variable&
    ///
    /////////////////////////////////////////////////
    const mu::Variable& ClusterManager::getCluster(const std::string& sCluster) const
    {
        mu::Variable* cluster = NumeReKernel::getInstance()->getParser().ReadVar(sCluster.substr(0, sCluster.find('{')));

        if (cluster && cluster->getCommonType() == mu::TYPE_CLUSTER)
            return *cluster;

        throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster, sCluster);
    }


    /////////////////////////////////////////////////
    /// \brief This member function creates a new
    /// cluster from the passed cluster identifier
    /// and returns a reference to this new object.
    ///
    /// \param sCluster const std::string&
    /// \return mu::Variable&
    ///
    /////////////////////////////////////////////////
    mu::Variable& ClusterManager::newCluster(const std::string& sCluster)
    {
        return *NumeReKernel::getInstance()->getParser().CreateVar(sCluster.substr(0, sCluster.find('{')), mu::TYPE_CLUSTER);
    }


    /////////////////////////////////////////////////
    /// \brief This member function removes the
    /// cluster from memory, which corresponds to the
    /// passed cluster identifier.
    ///
    /// \param sCluster const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ClusterManager::removeCluster(const std::string& sCluster)
    {
        NumeReKernel::getInstance()->getParser().RemoveVar(sCluster.substr(0, sCluster.find('{')));
    }


    /////////////////////////////////////////////////
    /// \brief This member function creates a
    /// temporary cluster with a unique name and
    /// returns this name to the calling function.
    ///
    /// \param suffix const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string ClusterManager::createTemporaryCluster(const std::string& suffix)
    {
        std::string sTemporaryClusterName = "_~~TC_" + toString(NumeReKernel::getInstance()->getParser().GetVar().size()) + "_" + suffix;
        NumeReKernel::getInstance()->getParser().CreateVar(sTemporaryClusterName, mu::TYPE_CLUSTER);

        return sTemporaryClusterName + "{}";
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns all
    /// temporary clusters from the internal map.
    /// Temporary clusters are indicated by their
    /// name.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ClusterManager::removeTemporaryClusters()
    {
        const mu::varmap_type& vars = NumeReKernel::getInstance()->getParser().GetVar();
        std::vector<std::string> clusters;

        for (const auto& iter : vars)
        {
            if (iter.second->getCommonType() == mu::TYPE_CLUSTER && iter.first.starts_with("_~~TC_"))
                clusters.push_back(iter.first);
        }

        for (const std::string& cluster: clusters)
        {
            NumeReKernel::getInstance()->getParser().RemoveVar(cluster);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Clear all clusters currently in memory.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ClusterManager::clearAllClusters()
    {
        const mu::varmap_type& vars = NumeReKernel::getInstance()->getParser().GetVar();
        std::vector<std::string> clusters;

        for (const auto& iter : vars)
        {
            if (iter.second->getCommonType() == mu::TYPE_CLUSTER && iter.first != "ans")
                clusters.push_back(iter.first);
        }

        for (const std::string& cluster: clusters)
        {
            NumeReKernel::getInstance()->getParser().RemoveVar(cluster);
        }
    }
}

