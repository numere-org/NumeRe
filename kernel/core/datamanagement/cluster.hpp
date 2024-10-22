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

#ifndef CLUSTER_HPP
#define CLUSTER_HPP

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include "../utils/tools.hpp"
#include "sorter.hpp"

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class represents a whole cluster.
    /// The single items are stored as pointers to
    /// the abstract cluster item. This object can be
    /// constructed from many different base items
    /// and has more or less all memory-like
    /// functions.
    /////////////////////////////////////////////////
    class Cluster : public mu::Array, public Sorter
    {
        private:
            bool bSortCaseInsensitive;

            void assign(const Cluster& cluster);
            void assignVectorResults(Indices _idx, const mu::Array& data);
            virtual int compare(int i, int j, int col) override;
            virtual bool isValue(int line, int col) override;
            void reorderElements(std::vector<int> vIndex, const VectorIndex& original);

        public:
            Cluster() : mu::Array()
            {
                bSortCaseInsensitive = false;
            }
            Cluster(const Cluster& cluster) : mu::Array()
            {
                assign(cluster);
            }

            Cluster& operator=(const Cluster& cluster)
            {
                assign(cluster);
                return *this;
            }

            void set(size_t i, const mu::Value& v);
            void insertDataInArray(mu::Variable* vTarget, const VectorIndex& _vLine);
            void setValueArray(const mu::Array& a);
            void assignResults(Indices _idx, const mu::Array& data);

            std::string getShortVectorRepresentation(size_t maxStringLength) const;

            std::vector<int> sortElements(const VectorIndex& row, const std::string& sSortingExpression);
            void deleteItems(const VectorIndex& vLines);
    };


    /////////////////////////////////////////////////
    /// \brief This class is the management class for
    /// the different clusters, which are currently
    /// available in memory.
    /////////////////////////////////////////////////
    class ClusterManager
    {
        protected:
            std::map<std::string, Cluster> mClusterMap;

            std::string validateClusterName(const std::string& sCluster);
            std::map<std::string, Cluster>::iterator mapStringViewFind(StringView view);
            std::map<std::string, Cluster>::const_iterator mapStringViewFind(StringView view) const;

        public:
            ClusterManager() {dClusterElementsCount = mu::Value(0.0);}
            ~ClusterManager() {}

            mu::Variable dClusterElementsCount;

            bool containsClusters(const std::string& sCmdLine) const;
            bool isCluster(StringView sCluster) const;
            bool isCluster(const std::string& sCluster) const;
            Cluster& getCluster(StringView sCluster);
            Cluster& getCluster(const std::string& sCluster);
            const Cluster& getCluster(const std::string& sCluster) const;
            Cluster& newCluster(const std::string& sCluster);
            void appendCluster(const std::string& sCluster, const Cluster& cluster);
            void removeCluster(const std::string& sCluster);
            std::string createTemporaryCluster(const std::string& suffix = "");
            void removeTemporaryClusters();
            void clearAllClusters();
            bool updateClusterSizeVariables(StringView sCluster);

            const std::map<std::string, Cluster>& getClusterMap() const
            {
                return mClusterMap;
            }
    };

}


#endif // CLUSTER_HPP



