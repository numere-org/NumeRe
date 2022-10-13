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
    /// \brief This is an abstract cluster item. It
    /// is used as root class of any cluster items
    /// and only contains the type of the item and
    /// virtual functions as interfaces to the child
    /// classes.
    /////////////////////////////////////////////////
    class ClusterItem
    {
        private:
            unsigned short nType;

        public:
            /////////////////////////////////////////////////
            /// \brief Defines the available types of
            /// clusters.
            /////////////////////////////////////////////////
            enum ClusterItemType
            {
                ITEMTYPE_INVALID = -1,
                ITEMTYPE_MIXED,
                ITEMTYPE_DOUBLE,
                ITEMTYPE_STRING
            };

            ClusterItem(unsigned short type) : nType(type) {}
            virtual ~ClusterItem() {}

            /////////////////////////////////////////////////
            /// \brief Returns the ClusterItemType.
            ///
            /// \return unsigned short
            ///
            /////////////////////////////////////////////////
            unsigned short getType() const
            {
                return nType;
            }

            /////////////////////////////////////////////////
            /// \brief Base implementation. Returns always
            /// NaN.
            ///
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type getDouble()
            {
                return NAN;
            }

            /////////////////////////////////////////////////
            /// \brief Base implementation. Does nothing.
            ///
            /// \param val const mu::value_type&
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setDouble(const mu::value_type& val) {}

            /////////////////////////////////////////////////
            /// \brief Base implementation. Always returns an
            /// empty string.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getString()
            {
                return "\"\"";
            }

            /////////////////////////////////////////////////
            /// \brief Base implementation. Returns an empty
            /// string.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getInternalString()
            {
                return "";
            }

            /////////////////////////////////////////////////
            /// \brief Base implementation. Returns a string
            /// with quotation marks.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getParserString()
            {
                return "\"" + getInternalString() + "\"";
            }

            /////////////////////////////////////////////////
            /// \brief Base implementation. Does nothing.
            ///
            /// \param strval const std::string&
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setString(const std::string& strval) {}
    };



    /////////////////////////////////////////////////
    /// \brief This is a cluster item, which contains
    /// a double. It features conversions to and from
    /// strings on-the-fly.
    /////////////////////////////////////////////////
    class ClusterDoubleItem : public ClusterItem
    {
        private:
            mu::value_type dData;

        public:
            ClusterDoubleItem(const mu::value_type& value) : ClusterItem(ClusterItem::ITEMTYPE_DOUBLE), dData(value) {}
            virtual ~ClusterDoubleItem() override {}

            /////////////////////////////////////////////////
            /// \brief Returns the internal value.
            ///
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type getDouble() override
            {
                return dData;
            }

            /////////////////////////////////////////////////
            /// \brief Overwrites the internal value.
            ///
            /// \param val const mu::value_type&
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setDouble(const mu::value_type& val) override
            {
                dData = val;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the internal value converted
            /// to a string.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getString() override
            {
                if (std::isnan(std::abs(dData)))
                    return toExternalString("nan");

                return toExternalString(toString(dData, 7));
            }

            /////////////////////////////////////////////////
            /// \brief Returns the internal value converted
            /// to a string.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getInternalString() override
            {
                if (std::isnan(std::abs(dData)))
                    return "nan";

                return toString(dData, 7);
            }

            /////////////////////////////////////////////////
            /// \brief Returns the internal value converted
            /// to a string.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getParserString() override
            {
                return getInternalString();
            }

            /////////////////////////////////////////////////
            /// \brief Overwrites the internal value with the
            /// passed string, which will converted to a
            /// value first.
            ///
            /// \param strval const std::string&
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setString(const std::string& strval) override
            {
                if (isConvertible(strval, CONVTYPE_VALUE))
                    dData = StrToCmplx(toInternalString(strval));
                else
                    dData = NAN;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This is a cluster item, which contains
    /// a string. It features conversions to and from
    /// doubles on-the-fly.
    /////////////////////////////////////////////////
    class ClusterStringItem : public ClusterItem
    {
        private:
            std::string sData;

        public:
            ClusterStringItem(const std::string& strval) : ClusterItem(ClusterItem::ITEMTYPE_STRING) {setString(strval);}
            virtual ~ClusterStringItem() override {}

            /////////////////////////////////////////////////
            /// \brief Returns the internal string converted
            /// to a value.
            ///
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type getDouble() override
            {
                if (isConvertible(sData, CONVTYPE_VALUE))
                    return StrToCmplx(sData);

                return NAN;
            }

            /////////////////////////////////////////////////
            /// \brief Overwrites the internal string with
            /// the passed value, which will be converted to
            /// a string first.
            ///
            /// \param val const mu::value_type&
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setDouble(const mu::value_type& val) override
            {
                sData = toString(val, 7);
            }

            /////////////////////////////////////////////////
            /// \brief Returns the internal string.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getString() override
            {
                return toExternalString(sData);
            }

            /////////////////////////////////////////////////
            /// \brief Returns the internal string.
            ///
            /// \return virtual std::string
            ///
            /////////////////////////////////////////////////
            virtual std::string getInternalString() override
            {
                return sData;
            }

            /////////////////////////////////////////////////
            /// \brief Overwrites the internal string.
            ///
            /// \param strval const std::string&
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setString(const std::string& strval) override
            {
                sData = strval;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class represents a whole cluster.
    /// The single items are stored as pointers to
    /// the abstract cluster item. This object can be
    /// constructed from many different base items
    /// and has more or less all memory-like
    /// functions.
    /////////////////////////////////////////////////
    class Cluster : public Sorter
    {
        private:
            std::vector<ClusterItem*> vClusterArray;
            bool bSortCaseInsensitive;
            mutable int nGlobalType;

            void assign(const Cluster& cluster);
            void assign(const std::vector<mu::value_type>& vVals);
            void assign(const std::vector<std::string>& vStrings);
            void assignVectorResults(Indices _idx, int nNum, mu::value_type* data);
            virtual int compare(int i, int j, int col) override;
            virtual bool isValue(int line, int col) override;
            void reorderElements(std::vector<int> vIndex, int i1, int i2);
            void reduceSize(size_t z);

        public:
            Cluster()
            {
                bSortCaseInsensitive = false;
                nGlobalType = ClusterItem::ITEMTYPE_INVALID;
            }
            Cluster(const Cluster& cluster)
            {
                assign(cluster);
            }
            Cluster(const std::vector<mu::value_type>& vVals)
            {
                assign(vVals);
            }
            Cluster(const std::vector<std::string>& vStrings)
            {
                assign(vStrings);
            }

            ~Cluster()
            {
                clear();
            }

            Cluster& operator=(const Cluster& cluster)
            {
                assign(cluster);
                return *this;
            }
            Cluster& operator=(const std::vector<mu::value_type>& vVals)
            {
                assign(vVals);
                return *this;
            }
            Cluster& operator=(const std::vector<std::string>& vStrings)
            {
                assign(vStrings);
                return *this;
            }

            void push_back(ClusterItem* item);
            void push_back(const mu::value_type& val);
            void push_back(const std::string& strval);
            void pop_back();

            size_t size() const;
            size_t getBytes() const;
            void clear();

            bool isMixed() const;
            bool isDouble() const;
            bool isString() const;

            unsigned short getType(size_t i) const;

            mu::value_type getDouble(size_t i) const;
            void setDouble(size_t i, const mu::value_type& value);
            std::vector<mu::value_type> getDoubleArray() const;
            void insertDataInArray(std::vector<mu::value_type>* vTarget, const VectorIndex& _vLine);
            void setDoubleArray(const std::vector<mu::value_type>& vVals);
            void setDoubleArray(int nNum, mu::value_type* data);
            void assignResults(Indices _idx, int nNum, mu::value_type* data);

            std::string getString(size_t i) const;
            std::string getParserString(size_t i) const;
            void setString(size_t i, const std::string& strval);
            std::vector<std::string> getStringArray() const;
            std::vector<std::string> getInternalStringArray() const;
            void setStringArray(const std::vector<std::string>& sVals);

            std::vector<std::string> to_string() const;
            std::string getVectorRepresentation() const;
            std::string getShortVectorRepresentation() const;

            std::vector<int> sortElements(long long int i1, long long int i2, const std::string& sSortingExpression);
            void deleteItems(long long int i1, long long int i2);
            void deleteItems(const VectorIndex& vLines);

            mu::value_type std(const VectorIndex& _vLine);
            mu::value_type avg(const VectorIndex& _vLine);
            mu::value_type max(const VectorIndex& _vLine);
            std::string strmax(const VectorIndex& _vLine);
            mu::value_type min(const VectorIndex& _vLine);
            std::string strmin(const VectorIndex& _vLine);
            mu::value_type prd(const VectorIndex& _vLine);
            mu::value_type sum(const VectorIndex& _vLine);
            std::string strsum(const VectorIndex& _vLine);
            mu::value_type num(const VectorIndex& _vLine);
            mu::value_type and_func(const VectorIndex& _vLine);
            mu::value_type or_func(const VectorIndex& _vLine);
            mu::value_type xor_func(const VectorIndex& _vLine);
            mu::value_type cnt(const VectorIndex& _vLine);
            mu::value_type norm(const VectorIndex& _vLine);
            mu::value_type cmp(const VectorIndex& _vLine, mu::value_type dRef, int _nType);
            mu::value_type med(const VectorIndex& _vLine);
            mu::value_type pct(const VectorIndex& _vLine, mu::value_type dPct);

    };


    /////////////////////////////////////////////////
    /// \brief This class is the management class for
    /// the different clusters, which are currently
    /// available in memory.
    /////////////////////////////////////////////////
    class ClusterManager
    {
        private:
            std::map<std::string, Cluster> mClusterMap;

            std::string validateClusterName(const std::string& sCluster);
            std::map<std::string, Cluster>::iterator mapStringViewFind(StringView view);
            std::map<std::string, Cluster>::const_iterator mapStringViewFind(StringView view) const;

        public:
            ClusterManager() {dClusterElementsCount = 0.0;}
            ~ClusterManager() {}

            mu::value_type dClusterElementsCount;

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



