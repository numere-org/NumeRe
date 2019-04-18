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

using namespace std;

namespace NumeRe
{
    // This is an abstract cluster item. It is used as root class
    // of any cluster items and only contains the type of the item
    // and virtual functions as interfaces to the child classes
    class ClusterItem
    {
        private:
            unsigned short nType;

        public:
            enum ClusterItemType
            {
                ITEMTYPE_INVALID,
                ITEMTYPE_DOUBLE,
                ITEMTYPE_STRING
            };

            ClusterItem(unsigned short type) : nType(type) {}
            virtual ~ClusterItem() {}

            unsigned short getType()
            {
                return nType;
            }

            virtual double getDouble()
            {
                return NAN;
            }
            virtual void setDouble(double val) {}

            virtual string getString()
            {
                return "\"\"";
            }
            virtual void setString(const string& strval) {}
    };


    // This is a cluster item, which contains a double. It
    // features conversions to and from strings on-the-fly
    class ClusterDoubleItem : public ClusterItem
    {
        private:
            double dData;

        public:
            ClusterDoubleItem(double value) : ClusterItem(ClusterItem::ITEMTYPE_DOUBLE), dData(value) {}
            virtual ~ClusterDoubleItem() override {}

            virtual double getDouble() override
            {
                return dData;
            }
            virtual void setDouble(double val) override
            {
                dData = val;
            }
            virtual string getString() override
            {
                if (isnan(dData))
                    return "\"nan\"";

                return "\"" + toString(dData, 7) + "\"";
            }
            virtual void setString(const string& strval) override
            {
                if (strval.front() == '"' && strval.back() == '"')
                    dData = atof(strval.substr(1, strval.length()-2).c_str());
                else
                    dData = atof(strval.c_str());
            }
    };


    // This is a cluster item, which contains a string. It
    // features conversions to and from doubles on-the-fly
    class ClusterStringItem : public ClusterItem
    {
        private:
            string sData;

        public:
            ClusterStringItem(const string& strval) : ClusterItem(ClusterItem::ITEMTYPE_STRING) {setString(strval);}
            virtual ~ClusterStringItem() override {}

            virtual double getDouble() override
            {
                return atof(sData.c_str());
            }
            virtual void setDouble(double val) override
            {
                sData = toString(val, 7);
            }
            virtual string getString() override
            {
                return "\"" + sData + "\"";
            }
            virtual void setString(const string& strval) override
            {
                if (strval.front() == '"' && strval.back() == '"')
                    sData = strval.substr(1, strval.length()-2);
                else
                    sData = strval;
            }
    };


    // This class represents a whole cluster. The single items
    // are stored as pointers to the abstract cluster item. This
    // object can be constructed from many different base items
    // and has more or less all memory-like functions.
    class Cluster
    {
        private:
            vector<ClusterItem*> vClusterArray;

            void assign(const Cluster& cluster);
            void assign(const vector<double>& vVals);
            void assign(const vector<string>& vStrings);
            void assignVectorResults(Indices _idx, int nNum, double* data);
            void assignIndexResults(Indices _idx, int nNum, double* data);

        public:
            Cluster() {}
            Cluster(const Cluster& cluster)
            {
                assign(cluster);
            }
            Cluster(const vector<double>& vVals)
            {
                assign(vVals);
            }
            Cluster(const vector<string>& vStrings)
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
            Cluster& operator=(const vector<double>& vVals)
            {
                assign(vVals);
                return *this;
            }
            Cluster& operator=(const vector<string>& vStrings)
            {
                assign(vStrings);
                return *this;
            }

            void push_back(ClusterItem* item);
            void push_back(double val);
            void push_back(const string& strval);
            void pop_back();

            size_t size() const;
            size_t getBytes() const;
            void clear();

            bool isMixed() const;
            bool isDouble() const;
            bool isString() const;

            unsigned short getType(size_t i) const;

            double getDouble(size_t i) const;
            void setDouble(size_t i, double value);
            vector<double> getDoubleArray() const;
            void insertDataInArray(vector<double>* vTarget, vector<long long int> vLine);
            void setDoubleArray(const vector<double>& vVals);
            void setDoubleArray(int nNum, double* data);
            void assignResults(Indices _idx, int nNum, double* data);

            string getString(size_t i) const;
            void setString(size_t i, const string& strval);
            vector<string> getStringArray() const;
            void setStringArray(const vector<string>& sVals);

            string getVectorRepresentation() const;
            string getShortVectorRepresentation() const;

            double std(const vector<long long int>& _vLine);
            double avg(const vector<long long int>& _vLine);
            double max(const vector<long long int>& _vLine);
            string strmax(const vector<long long int>& _vLine);
            double min(const vector<long long int>& _vLine);
            string strmin(const vector<long long int>& _vLine);
            double prd(const vector<long long int>& _vLine);
            double sum(const vector<long long int>& _vLine);
            string strsum(const vector<long long int>& _vLine);
            double num(const vector<long long int>& _vLine);
            double and_func(const vector<long long int>& _vLine);
            double or_func(const vector<long long int>& _vLine);
            double xor_func(const vector<long long int>& _vLine);
            double cnt(const vector<long long int>& _vLine);
            double norm(const vector<long long int>& _vLine);
            double cmp(const vector<long long int>& _vLine, double dRef, int nType);
            double med(const vector<long long int>& _vLine);
            double pct(const vector<long long int>& _vLine, double dPct);

    };


    // This class is the management class for the different
    // clusters, which are currently available in memory
    class ClusterManager
    {
        private:
            map<string, Cluster> mClusterMap;

            string validateClusterName(const string& sCluster);

        public:
            ClusterManager() {dClusterElementsCount = 0.0;}
            ~ClusterManager() {}

            double dClusterElementsCount;

            bool containsClusters(const string& sCmdLine) const;
            bool isCluster(const string& sCluster) const;
            Cluster& getCluster(const string& sCluster);
            const Cluster& getCluster(const string& sCluster) const;
            Cluster& newCluster(const string& sCluster);
            void appendCluster(const string& sCluster, const Cluster& cluster);
            void removeCluster(const string& sCluster);
            string createTemporaryCluster();
            void removeTemporaryClusters();
            bool updateClusterSizeVariables(const string& sCluster);

            const map<string, Cluster>& getClusterMap() const
            {
                return mClusterMap;
            }
    };

}


#endif // CLUSTER_HPP



