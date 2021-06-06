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
#include "../ui/error.hpp"

namespace NumeRe
{
    //
    // class CLUSTER
    //
    //

    // Private cluster copy assignment function
    void Cluster::assign(const Cluster& cluster)
    {
        bSortCaseInsensitive = false;

        // Clear the contents first
        clear();

        nGlobalType = cluster.nGlobalType;

        // Fill the cluster with copies from the passed cluster
        for (size_t i = 0; i < cluster.vClusterArray.size(); i++)
        {
            if (cluster.vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                vClusterArray.push_back(new ClusterDoubleItem(cluster.vClusterArray[i]->getDouble()));
            else
                vClusterArray.push_back(new ClusterStringItem(cluster.vClusterArray[i]->getString()));
        }
    }

    // Private double vector assign ment function
    void Cluster::assign(const vector<double>& vVals)
    {
        bSortCaseInsensitive = false;

        // clear the contents first
        clear();

        nGlobalType = ClusterItem::ITEMTYPE_DOUBLE;

        // Fill the cluster with new double items
        for (size_t i = 0; i < vVals.size(); i++)
        {
            vClusterArray.push_back(new ClusterDoubleItem(vVals[i]));
        }
    }

    // Private string vector assignment function
    void Cluster::assign(const vector<string>& vStrings)
    {
        bSortCaseInsensitive = false;

        // Clear the contents first
        clear();

        nGlobalType = ClusterItem::ITEMTYPE_STRING;

        // Fill the cluster with new string items
        for (size_t i = 0; i < vStrings.size(); i++)
        {
            vClusterArray.push_back(new ClusterStringItem(vStrings[i]));
        }
    }

    // Private result assignment function for doubles
    // using vectors as indices
    void Cluster::assignVectorResults(Indices _idx, int nNum, double* data)
    {
        if (nGlobalType != ClusterItem::ITEMTYPE_DOUBLE)
            nGlobalType = ClusterItem::ITEMTYPE_INVALID;

        // Assign the single results
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (nNum > 1 && (size_t)nNum <= i)
                return;

            // Expand the current cluster on-the-fly
            while (_idx.row[i] >= vClusterArray.size())
                push_back(new ClusterDoubleItem(NAN));

            // Assign the value and expand singletons
            if (vClusterArray[_idx.row[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE)
            {
                // Re-create the current item as double
                delete vClusterArray[_idx.row[i]];
                vClusterArray[_idx.row[i]] = new ClusterDoubleItem(nNum == 1 ? data[0] : data[i]);
            }
            else
                vClusterArray[_idx.row[i]]->setDouble(nNum == 1 ? data[0] : data[i]);
        }
    }

    // This private member function is an override for the
    // sorter object
    int Cluster::compare(int i, int j, int col)
    {
        if (isString() || isMixed())
        {
            if (bSortCaseInsensitive)
            {
                if (toLowerCase(vClusterArray[i]->getString()) < toLowerCase(vClusterArray[j]->getString()))
                    return -1;

                if (toLowerCase(vClusterArray[i]->getString()) == toLowerCase(vClusterArray[j]->getString()))
                    return 0;
            }
            else
            {
                if (vClusterArray[i]->getString() < vClusterArray[j]->getString())
                    return -1;

                if (vClusterArray[i]->getString() == vClusterArray[j]->getString())
                    return 0;
            }

            return 1;
        }
        else if (isDouble())
        {
            if (vClusterArray[i]->getDouble() < vClusterArray[j]->getDouble())
                return -1;

            if (vClusterArray[i]->getDouble() == vClusterArray[j]->getDouble())
                return 0;

            return 1;
        }

        return 0;
    }

    // This private member function is an override for the
    // sorter object
    bool Cluster::isValue(int line, int col)
    {
        if (vClusterArray[line]->getType() == ClusterItem::ITEMTYPE_DOUBLE && !isnan(vClusterArray[line]->getDouble()))
            return true;

        if (vClusterArray[line]->getType() == ClusterItem::ITEMTYPE_STRING && vClusterArray[line]->getString() != "\"\"")
            return true;

        return false;
    }

    // This private member function reorders the elements in the
    // cluster based upon the passed index vector
    void Cluster::reorderElements(vector<int> vIndex, int i1, int i2)
    {
        vector<ClusterItem*> vSortVector = vClusterArray;

        // Copy the contents directly from the
        // prepared in the new order
        for (int i = 0; i <= i2-i1; i++)
        {
            vClusterArray[i+i1] = vSortVector[vIndex[i]];
        }

    }

    // This member function appends an arbitrary cluster
    // item at the back of the internal clsuter array buffer
    void Cluster::push_back(ClusterItem* item)
    {
        if (item)
            vClusterArray.push_back(item);

        if (nGlobalType == ClusterItem::ITEMTYPE_INVALID)
            return;

        if (item->getType() != nGlobalType)
            nGlobalType = ClusterItem::ITEMTYPE_MIXED;
    }

    // This member function constructs a new double cluster
    // item at the back of the internal clsuter array buffer
    void Cluster::push_back(double val)
    {
        vClusterArray.push_back(new ClusterDoubleItem(val));

        if (nGlobalType == ClusterItem::ITEMTYPE_INVALID)
            return;

        if (nGlobalType == ClusterItem::ITEMTYPE_STRING)
            nGlobalType = ClusterItem::ITEMTYPE_MIXED;
    }

    // This member function constructs a new string cluster
    // item at the back of the internal clsuter array buffer
    void Cluster::push_back(const string& strval)
    {
        vClusterArray.push_back(new ClusterStringItem(strval));

        if (nGlobalType == ClusterItem::ITEMTYPE_INVALID)
            return;

        if (nGlobalType == ClusterItem::ITEMTYPE_DOUBLE)
            nGlobalType = ClusterItem::ITEMTYPE_MIXED;
    }

    // This member function removes the last item in the internal
    // memory buffer and frees the associated mempry
    void Cluster::pop_back()
    {
        if (vClusterArray.size())
        {
            delete vClusterArray.back();
            vClusterArray.pop_back();

            nGlobalType = ClusterItem::ITEMTYPE_INVALID;
        }
    }

    // This member function returns the size of the internal
    // memory buffer as items
    size_t Cluster::size() const
    {
        return vClusterArray.size();
    }

    // This member function returns the size of the associated
    // memory as bytes
    size_t Cluster::getBytes() const
    {
        size_t nBytes = 0;

        // Go through the internal cluster array and calculate
        // the bytes needed for each single cluster item
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                nBytes += sizeof(double);
            else if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_STRING)
                nBytes += sizeof(char) * (vClusterArray[i]->getString().length()-2);
        }

        return nBytes;
    }

    // This member function clears the internal memory buffer
    // and frees the associated memory
    void Cluster::clear()
    {
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            delete vClusterArray[i];
        }

        vClusterArray.clear();
        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    // This member function returns, whether the data in the
    // cluster have mixed type
    bool Cluster::isMixed() const
    {
        // Only do something, if the array has a length
        if (vClusterArray.size())
        {
            if (nGlobalType != ClusterItem::ITEMTYPE_INVALID)
                return nGlobalType == ClusterItem::ITEMTYPE_MIXED;

            // Store the first type
            int nFirstType = vClusterArray[0]->getType();

            for (size_t i = 1; i < vClusterArray.size(); i++)
            {
                // Is there any item with a different type?
                if (vClusterArray[i]->getType() != nFirstType)
                {
                    nGlobalType = ClusterItem::ITEMTYPE_MIXED;
                    return true;
                }
            }

            nGlobalType = nFirstType;
            return false;
        }

        return false;
    }

    // This member function returns, whether the data in the
    // cluster have only double as type
    bool Cluster::isDouble() const
    {
        // Only do something, if the array has a length
        if (vClusterArray.size())
        {
            if (nGlobalType != ClusterItem::ITEMTYPE_INVALID)
                return nGlobalType == ClusterItem::ITEMTYPE_DOUBLE;

            for (size_t i = 0; i < vClusterArray.size(); i++)
            {
                // Is there any item, which is NOT a double?
                if (vClusterArray[i]->getType() != ClusterItem::ITEMTYPE_DOUBLE)
                {
                    if (i)
                        nGlobalType = ClusterItem::ITEMTYPE_MIXED;

                    return false;
                }
            }

            nGlobalType = ClusterItem::ITEMTYPE_DOUBLE;
            return true;
        }

        return false;
    }

    // This member function returns, whether the data in the
    // cluster have only string as type
    bool Cluster::isString() const
    {
        // Only do something, if the array has a length
        if (vClusterArray.size())
        {
            if (nGlobalType != ClusterItem::ITEMTYPE_INVALID)
                return nGlobalType == ClusterItem::ITEMTYPE_STRING;

            for (size_t i = 0; i < vClusterArray.size(); i++)
            {
                // Is there any item, which is NOT a string?
                if (vClusterArray[i]->getType() != ClusterItem::ITEMTYPE_STRING)
                {
                    if (i)
                        nGlobalType = ClusterItem::ITEMTYPE_MIXED;

                    return false;
                }
            }

            nGlobalType = ClusterItem::ITEMTYPE_STRING;
            return true;
        }

        return false;
    }

    // This member function returns the type of the i-th
    // cluster item in the internal memory buffer
    unsigned short Cluster::getType(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getType();

        return ClusterItem::ITEMTYPE_INVALID;
    }

    // This member function returns the data of the i-th
    // cluster item in memory as a double
    double Cluster::getDouble(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getDouble();

        return NAN;
    }

    // This member function assigns a double as data for
    // the i-th cluster item in memory. The type of the
    // i-th cluster item is adapted on-the-fly
    void Cluster::setDouble(size_t i, double value)
    {
        // Create new items if needed
        while (vClusterArray.size() <= i)
            push_back(new ClusterDoubleItem(NAN));

        // Assign the data
        if (vClusterArray[i]->getType() != ClusterItem::ITEMTYPE_DOUBLE)
        {
            // Re-create the item as double item
            delete vClusterArray[i];
            vClusterArray[i] = new ClusterDoubleItem(value);
        }
        else
            vClusterArray[i]->setDouble(value);

        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    // This member function returns the data of all cluster
    // items memory as a double vector
    vector<double> Cluster::getDoubleArray() const
    {
        vector<double> vArray;

        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            vArray.push_back(vClusterArray[i]->getDouble());
        }

        return vArray;
    }

    // This member function inserts the data of all cluster
    // items memory into the pointer passed to the function.
    // This function is used for cached memory accesses
    void Cluster::insertDataInArray(vector<double>* vTarget, const VectorIndex& _vLine)
    {
        if (vTarget == nullptr)
            return;

        // Clear the passed array
        vTarget->clear();

        // Try to resize the array as copy-efficient as
        // possible
        if (_vLine.size() > 1 && !vClusterArray.size())
            vTarget->resize(1, NAN);
        else
        {
            vTarget->resize(_vLine.size(), NAN);

            // Insert the elements in the passed array
            for (unsigned int i = 0; i < _vLine.size(); i++)
            {
                if (_vLine[i] >= vClusterArray.size() || _vLine[i] < 0)
                    (*vTarget)[i] = NAN;
                else
                    (*vTarget)[i] = vClusterArray[_vLine[i]]->getDouble();

            }
        }
    }

    // This member function assigns a doubles as data for
    // the all cluster items in memory. The type of the
    // cluster items is adapted on-the-fly
    void Cluster::setDoubleArray(const vector<double>& vVals)
    {
        // Create new cluster items, if needed
        while (vClusterArray.size() < vVals.size())
            push_back(new ClusterDoubleItem(NAN));

        for (size_t i = 0; i < vVals.size(); i++)
        {
            // Assign the value
            if (vClusterArray[i]->getType() != ClusterItem::ITEMTYPE_DOUBLE)
            {
                // Re-create the current item as double
                delete vClusterArray[i];
                vClusterArray[i] = new ClusterDoubleItem(vVals[i]);
            }
            else
                vClusterArray[i]->setDouble(vVals[i]);
        }

        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    // This member function assigns a doubles as data for
    // the all cluster items in memory. The type of the
    // cluster items is adapted on-the-fly
    void Cluster::setDoubleArray(int nNum, double* data)
    {
        // Create new cluster items, if needed
        while (vClusterArray.size() < (size_t)nNum)
            push_back(new ClusterDoubleItem(NAN));

        for (int i = 0; i < nNum; i++)
        {
            // Assign the value
            if (vClusterArray[i]->getType() != ClusterItem::ITEMTYPE_DOUBLE)
            {
                // Re-create the current item as double
                delete vClusterArray[i];
                vClusterArray[i] = new ClusterDoubleItem(data[i]);
            }
            else
                vClusterArray[i]->setDouble(data[i]);
        }

        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    // This member function assigns a calculation results
    // as data for the cluster items in memory, which are
    // referenced by the passed indices. The type of the
    // cluster items is adapted on-the-fly
    void Cluster::assignResults(Indices _idx, int nNum, double* data)
    {
        // If the indices indicate a complete override
        // do that here and return
        if (_idx.row.isOpenEnd() && _idx.row.front() == 0)
        {
            if (vClusterArray.size() > (size_t)nNum)
                clear();

            setDoubleArray(nNum, data);
            return;
        }

        // Assign the results depending on the type of the
        // passed indices
        assignVectorResults(_idx, nNum, data);
    }

    // This member function returns the data of the i-th
    // cluster item in memory as a string
    string Cluster::getString(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getString();

        return "";
    }

    // This member function assigns a string as data for
    // the i-th cluster item in memory. The type of the
    // i-th cluster item is adapted on-the-fly
    void Cluster::setString(size_t i, const string& strval)
    {
        // Create new cluster items, if needed
        while (vClusterArray.size() <= i)
            push_back(new ClusterStringItem(""));

        // Assign the value
        if (vClusterArray[i]->getType() != ClusterItem::ITEMTYPE_STRING)
        {
            // Re-create the current item as a string
            delete vClusterArray[i];
            vClusterArray[i] = new ClusterStringItem(strval);
        }
        else
            vClusterArray[i]->setString(strval);

        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    // This member function returns the data of all cluster
    // items memory as a double vector
    vector<string> Cluster::getStringArray() const
    {
        vector<string> vArray;

        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            vArray.push_back(vClusterArray[i]->getString());
        }

        return vArray;
    }

    // This member function assigns a doubles as data for
    // the all cluster items in memory. The type of the
    // cluster items is adapted on-the-fly
    void Cluster::setStringArray(const vector<string>& sVals)
    {
        // Create new cluster items, if needed
        while (vClusterArray.size() < sVals.size())
            push_back(new ClusterStringItem(""));

        for (size_t i = 0; i < sVals.size(); i++)
        {
            // Assign the value
            if (vClusterArray[i]->getType() != ClusterItem::ITEMTYPE_STRING)
            {
                // Re-create the current item as a string
                delete vClusterArray[i];
                vClusterArray[i] = new ClusterStringItem(sVals[i]);
            }
            else
                vClusterArray[i]->setString(sVals[i]);
        }

        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    // This member function constructs a plain vector
    // from the data in memory, which can be inserted
    // in the commandline as a replacement for the
    // call to the cluster
    string Cluster::getVectorRepresentation() const
    {
        // Return nan, if no data is available
        if (!vClusterArray.size())
            return "nan";

        string sVector = "{";

        // Append the contained data depending on its type
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                sVector += toCmdString(vClusterArray[i]->getDouble()) + ",";
            else
                sVector += vClusterArray[i]->getString() + ",";
        }

        // Replace the last comma with a closing brace
        sVector.back() = '}';

        return sVector;
    }

    // This member function constructs a short version of
    // a plain vector from the data in memory, which is used
    // to display a preview of the contained data in the
    // variable viewers
    string Cluster::getShortVectorRepresentation() const
    {
        // Return an empty brace pair, if no data is
        // available
        if (!vClusterArray.size())
            return "{}";

        string sVector = "{";

        // Append the contained data depending on its type but
        // restrict the number to maximal five values (use the first
        // and the last ones) and insert an ellipsis in the middle
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                sVector += toString(vClusterArray[i]->getDouble(), 5) + ", ";
            else
                sVector += vClusterArray[i]->getString() + ", ";

            // Insert the ellipsis in the middle
            if (i == 1 && vClusterArray.size() > 5)
            {
                sVector += "..., ";
                i = vClusterArray.size()-3;
            }
        }

        sVector.pop_back();
        sVector.back() = '}';

        return sVector;
    }

    // This public member function provides access to the sorting
    // algorithm for the cluster object
    vector<int> Cluster::sortElements(long long int i1, long long int i2, const string& sSortingExpression)
    {
        if (!vClusterArray.size())
            return vector<int>();

        bool bReturnIndex = false;
        bSortCaseInsensitive = false;
        int nSign = 1;
        vector<int> vIndex;

        // Look for command line parameters
        if (findParameter(sSortingExpression, "desc"))
            nSign = -1;

        if (findParameter(sSortingExpression, "ignorecase"))
            bSortCaseInsensitive = true;

        if (findParameter(sSortingExpression, "index"))
            bReturnIndex = true;

        // Prepare the indices
        if (i2 == -1)
            i2 = i1;

        // Create the sorting index
        for (int i = i1; i <= i2; i++)
            vIndex.push_back(i);

        // Sort everything
        if (!qSort(&vIndex[0], i2-i1+1, 0, 0, i2-i1, nSign))
        {
            throw SyntaxError(SyntaxError::CANNOT_SORT_DATA, "cluster{} " + sSortingExpression, SyntaxError::invalid_position);
        }

        // If the sorting index is requested,
        // then only sort the first column and return
        if (!bReturnIndex)
        {
            reorderElements(vIndex, i1, i2);
        }
        else
        {
            // If the index was requested, increment every index by one
            for (int i = 0; i <= i2-i1; i++)
                vIndex[i]++;
        }

        if (!bReturnIndex)
            return vector<int>();

        return vIndex;
    }

    // This public member function erases elements located from
    // the index i1 to i2
    void Cluster::deleteItems(long long int i1, long long int i2)
    {
        if (i2 >= vClusterArray.size())
            i2 = vClusterArray.size()-1;

        // If everything shall be erased, use the
        // "clear()" function
        if (!i1 && i2+1 == vClusterArray.size())
        {
            clear();
            return;
        }

        // Delete the cluster items first and
        // set the pointer to a nullpointer
        for (long long int i = i1; i < i2; i++)
        {
            delete vClusterArray[i];
            vClusterArray[i] = nullptr;
        }

        auto iter = vClusterArray.begin();

        // Remove all nullpointers from the array
        while (iter != vClusterArray.end())
        {
            if (!(*iter))
                iter = vClusterArray.erase(iter);
            else
                ++iter;
        }

        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    // This public member function erases elements located from
    // the index i1 to i2
    void Cluster::deleteItems(const VectorIndex& vLines)
    {
        // Delete the cluster items first and
        // set the pointer to a nullpointer
        for (size_t i = 0; i < vLines.size(); i++)
        {
            if (vLines[i] < 0 || vLines[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[vLines[i]])
                delete vClusterArray[vLines[i]];

            vClusterArray[vLines[i]] = nullptr;
        }

        auto iter = vClusterArray.begin();

        // Remove all nullpointers from the array
        while (iter != vClusterArray.end())
        {
            if (!(*iter))
                iter = vClusterArray.erase(iter);
            else
                ++iter;
        }

        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }

    //
    // Statistic functions section
    //
    // This member function calculates the standard deviation
    // of the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::std(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        // Calculate the average of the referenced items
        double dAvg = avg(_vLine);
        double dStd = 0.0;
        unsigned int nInvalid = 0;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                nInvalid++;
            else
                dStd += (dAvg - vClusterArray[_vLine[i]]->getDouble()) * (dAvg - vClusterArray[_vLine[i]]->getDouble());
        }

        if (nInvalid >= _vLine.size() - 1)
            return NAN;

        return sqrt(dStd / ((_vLine.size()) - 1 - nInvalid));
    }

    // This member function calculates the average of
    // the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::avg(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        double dAvg = 0.0;
        unsigned int nInvalid = 0;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                nInvalid++;
            else
                dAvg += vClusterArray[_vLine[i]]->getDouble();
        }

        if (nInvalid >= _vLine.size())
            return NAN;

        return dAvg / (_vLine.size() - nInvalid);
    }

    // This member function calculates the maximal value
    // of the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::max(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isString())
            return NAN;

        double dMax = NAN;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            if (isnan(dMax))
                dMax = vClusterArray[_vLine[i]]->getDouble();

            if (dMax < vClusterArray[_vLine[i]]->getDouble())
                dMax = vClusterArray[_vLine[i]]->getDouble();
        }

        return dMax;
    }

    // This member function calculates the maximal string value
    // of the data in memory. Cluster items of all types are used
    // and converted on-the-fly
    string Cluster::strmax(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isDouble())
            return "";

        string sMax = "";

        // Apply the operation on all items and convert
        // their values on-the-fly
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (!sMax.length())
                sMax = vClusterArray[_vLine[i]]->getString();

            if (sMax < vClusterArray[_vLine[i]]->getString())
                sMax = vClusterArray[_vLine[i]]->getString();
        }

        return sMax;
    }

    // This member function calculates the minimal value
    // of the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::min(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isString())
            return NAN;

        double dMin = NAN;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            if (isnan(dMin))
                dMin = vClusterArray[_vLine[i]]->getDouble();

            if (dMin > vClusterArray[_vLine[i]]->getDouble())
                dMin = vClusterArray[_vLine[i]]->getDouble();
        }

        return dMin;
    }

    // This member function calculates the minimal string value
    // of the data in memory. Cluster items of all types are used
    // and converted on-the-fly
    string Cluster::strmin(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isDouble())
            return "";

        string sMin = "";

        // Apply the operation on all items and convert
        // their values on-the-fly
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (!sMin.length())
                sMin = vClusterArray[_vLine[i]]->getString();

            if (sMin > vClusterArray[_vLine[i]]->getString())
                sMin = vClusterArray[_vLine[i]]->getString();
        }

        return sMin;
    }

    // This member function calculates the product
    // of the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::prd(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        double dPrd = 1.0;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            dPrd *= vClusterArray[_vLine[i]]->getDouble();
        }

        return dPrd;
    }

    // This member function calculates the sum of the
    // data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::sum(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() && isString())
            return NAN;

        double dSum = 0.0;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            dSum += vClusterArray[_vLine[i]]->getDouble();
        }

        return dSum;
    }

    // This member function calculates the string concatenation
    // of the data in memory. Cluster items of all types are used
    // and converted on-the-fly
    string Cluster::strsum(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isDouble())
            return "";

        string sSum = "";

        // Apply the operation on all items and convert
        // their values on-the-fly
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            sSum += vClusterArray[_vLine[i]]->getString();
        }

        return sSum;
    }

    // This member function counts the number of valid cluster
    // items in memory. Cluster items of any type are counted, if
    // they do have a valid value (depending on their type)
    double Cluster::num(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0;

        int nInvalid = 0;

        // Apply the operation and ignore invalid values
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE && isnan(vClusterArray[_vLine[i]]->getDouble()))
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_STRING && vClusterArray[_vLine[i]]->getString() == "\"\"")
                nInvalid++;
        }

        return _vLine.size() - nInvalid;
    }

    // This member function applies an "AND" to
    // the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::and_func(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0.0;

        double dRetVal = NAN;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (isnan(dRetVal))
                dRetVal = 1.0;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()) || vClusterArray[_vLine[i]]->getDouble() == 0)
                return 0.0;
        }

        if (isnan(dRetVal))
            return 0.0;

        return 1.0;
    }

    // This member function applies an "OR" to
    // the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::or_func(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0.0;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE
                && (isnan(vClusterArray[_vLine[i]]->getDouble()) || vClusterArray[_vLine[i]]->getDouble() != 0))
                return 1.0;
        }

        return 0.0;
    }

    // This member function applies an "exclusive OR" to
    // the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::xor_func(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0.0;

        bool isTrue = false;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE
                && (isnan(vClusterArray[_vLine[i]]->getDouble()) || vClusterArray[_vLine[i]]->getDouble() != 0))
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return 0.0;
            }
        }

        if (isTrue)
            return 1.0;

        return 0.0;
    }

    // This member function counts the number of valid cluster
    // items in memory. Cluster items of any type are counted, if
    // the vector index points to a valid location
    double Cluster::cnt(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0;

        int nInvalid = 0;

        // Apply the operation and ignore invalid locations
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                nInvalid++;
        }
        return _vLine.size()  - nInvalid;
    }

    // This member function calculates the euclidic vector norm
    // of the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::norm(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        double dNorm = 0.0;

        // Apply the operation and ignore invalid or non-double items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            dNorm += vClusterArray[_vLine[i]]->getDouble() * vClusterArray[_vLine[i]]->getDouble();
        }

        return sqrt(dNorm);
    }

    // This member function compares the values in memory with
    // the referenced value and returns indices or values depending
    // on the passed type. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::cmp(const VectorIndex& _vLine, double dRef, int _nType)
    {
        if (!vClusterArray.size())
            return NAN;

        enum
        {
            RETURN_VALUE = 1,
            RETURN_LE = 2,
            RETURN_GE = 4,
            RETURN_FIRST = 8
        };

        int nType = 0;

        double dKeep = dRef;
        int nKeep = -1;

        if (_nType > 0)
            nType = RETURN_GE;
        else if (_nType < 0)
            nType = RETURN_LE;

        switch (intCast(fabs(_nType)))
        {
            case 2:
                nType |= RETURN_VALUE;
                break;
            case 3:
                nType |= RETURN_FIRST;
                break;
            case 4:
                nType |= RETURN_FIRST | RETURN_VALUE;
                break;
        }

        // Apply the operation and ignore invalid or non-double items
        for (long long int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            if (vClusterArray[_vLine[i]]->getDouble() == dRef)
            {
                if (nType & RETURN_VALUE)
                    return vClusterArray[_vLine[i]]->getDouble();

                return _vLine[i] + 1;
            }
            else if (nType & RETURN_GE && vClusterArray[_vLine[i]]->getDouble() > dRef)
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return vClusterArray[_vLine[i]]->getDouble();

                    return _vLine[i]+1;
                }

                if (nKeep == -1 || vClusterArray[_vLine[i]]->getDouble() < dKeep)
                {
                    dKeep = vClusterArray[_vLine[i]]->getDouble();
                    nKeep = _vLine[i];
                }
                else
                    continue;
            }
            else if (nType & RETURN_LE && vClusterArray[_vLine[i]]->getDouble() < dRef)
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return vClusterArray[_vLine[i]]->getDouble();

                    return _vLine[i]+1;
                }

                if (nKeep == -1 || vClusterArray[_vLine[i]]->getDouble() > dKeep)
                {
                    dKeep = vClusterArray[_vLine[i]]->getDouble();
                    nKeep = _vLine[i];
                }
                else
                    continue;
            }
        }

        if (nKeep == -1)
            return NAN;
        else if (nType & RETURN_VALUE)
            return dKeep;

        return nKeep+1;
    }

    // This member function calculates the median value
    // of the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::med(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        double dMed = 0.0;
        unsigned int nInvalid = 0;
        unsigned int nCount = 0;
        double* dData = 0;

        // Calculate the number of valid items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                nInvalid++;
        }

        if (nInvalid >= _vLine.size())
            return NAN;

        // Create a memory buffer of the corresponding size
        dData = new double[_vLine.size() - nInvalid];

        // copy the data to the buffer
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size() || vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            dData[nCount] = vClusterArray[_vLine[i]]->getDouble();
            nCount++;

            if (nCount == _vLine.size() - nInvalid)
                break;
        }

        // Sort the data
        nCount = qSortDouble(dData, nCount);

        if (!nCount)
        {
            delete[] dData;
            return NAN;
        }

        // Calculate the median value
        dMed = gsl_stats_median_from_sorted_data(dData, 1, nCount);

        delete[] dData;

        return dMed;
    }

    // This member function calculates the p-th percentile
    // of the data in memory. Cluster items, which do not
    // have the type "double" are ignored
    double Cluster::pct(const VectorIndex& _vLine, double dPct)
    {
        if (!vClusterArray.size())
            return NAN;

        unsigned int nInvalid = 0;
        unsigned int nCount = 0;
        double* dData = 0;

        if (dPct >= 1 || dPct <= 0)
            return NAN;

        // Calculate the number of valid items
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                nInvalid++;
        }

        if (nInvalid >= _vLine.size())
            return NAN;

        // Create a memory buffer of the corresponding size
        dData = new double[_vLine.size() - nInvalid];

        // copy the data to the buffer
        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= vClusterArray.size() || vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || isnan(vClusterArray[_vLine[i]]->getDouble()))
                continue;

            dData[nCount] = vClusterArray[_vLine[i]]->getDouble();
            nCount++;

            if (nCount == _vLine.size() - nInvalid)
                break;
        }

        // Sort the data
        nCount = qSortDouble(dData, nCount);

        if (!nCount)
        {
            delete[] dData;
            return NAN;
        }

        // Calculate the p-th percentile
        dPct = gsl_stats_quantile_from_sorted_data(dData, 1, nCount, dPct);

        delete[] dData;

        return dPct;
    }


    //
    // class CLUSTERMANAGER
    //
    //

    // This member function creates a valid cluster identifier
    // name, which can be used to create or append a new cluster
    string ClusterManager::validateClusterName(const string& sCluster)
    {
        string sClusterName = sCluster.substr(0, sCluster.find('{'));
        const static string sVALIDCHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_~";

        if ((sClusterName[0] >= '0' && sClusterName[0] <= '9') || sClusterName[0] == '~' || sClusterName.find_first_not_of(sVALIDCHARACTERS) != string::npos)
            throw SyntaxError(SyntaxError::INVALID_CLUSTER_NAME, "", SyntaxError::invalid_position, sClusterName);

        return sClusterName;
    }

    map<string, Cluster>::const_iterator ClusterManager::mapStringViewFind(StringView view) const
    {
        for (auto iter = mClusterMap.begin(); iter != mClusterMap.end(); ++iter)
        {
            if (view == iter->first)
                return iter;
            else if (view < iter->first)
                return mClusterMap.end();
        }

        return mClusterMap.end();
    }

    map<string, Cluster>::iterator ClusterManager::mapStringViewFind(StringView view)
    {
        for (auto iter = mClusterMap.begin(); iter != mClusterMap.end(); ++iter)
        {
            if (view == iter->first)
                return iter;
            else if (view < iter->first)
                return mClusterMap.end();
        }

        return mClusterMap.end();
    }

    // This member function detects, whether a cluster is used
    // in the current expression
    bool ClusterManager::containsClusters(const string& sCmdLine) const
    {
        size_t nQuotes = 0;

        if (sCmdLine.find('{') == string::npos)
            return false;

        // Search through the expression
        for (size_t i = 0; i < sCmdLine.length(); i++)
        {
            // Consider quotation marks
            if (sCmdLine[i] == '"' && (!i || sCmdLine[i-1] != '\\'))
                nQuotes++;

            if (!(nQuotes % 2))
            {
                // If the current character might probably be an
                // identifier for a table, search for the next
                // nonmatching character and try to find the obtained
                // string in the internal map
                if (isalpha(sCmdLine[i]) || sCmdLine[i] == '_' || sCmdLine[i] == '~')
                {
                    size_t nStartPos = i;

                    do
                    {
                        i++;
                    }
                    while (isalnum(sCmdLine[i]) || sCmdLine[i] == '_' || sCmdLine[i] == '~' || sCmdLine[i] == '[' || sCmdLine[i] == ']');

                    if (sCmdLine[i] == '{')
                    {
                        if (mClusterMap.find(sCmdLine.substr(nStartPos, i - nStartPos)) != mClusterMap.end())
                            return true;
                    }
                }
            }
        }

        return false;
    }

    // This member function returns true, if the passed cluster
    // identifier can be found in the internal map
    bool ClusterManager::isCluster(StringView sCluster) const
    {
        if (mapStringViewFind(sCluster.subview(0, sCluster.find('{'))) != mClusterMap.end())
            return true;

        return false;
    }

    // This member function returns true, if the passed cluster
    // identifier can be found in the internal map
    bool ClusterManager::isCluster(const string& sCluster) const
    {
        if (mClusterMap.find(sCluster.substr(0, sCluster.find('{'))) != mClusterMap.end())
            return true;

        return false;
    }

    // This member function returns a reference to the cluster indicated
    // by the passed cluster identifier
    Cluster& ClusterManager::getCluster(StringView sCluster)
    {
        auto iter = mapStringViewFind(sCluster);

        if (iter == mClusterMap.end())
            throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster.to_string(), sCluster.to_string());

        return iter->second;
    }

    // This member function returns a reference to the cluster indicated
    // by the passed cluster identifier
    Cluster& ClusterManager::getCluster(const string& sCluster)
    {
        auto iter = mClusterMap.find(sCluster);

        if (iter == mClusterMap.end())
            throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster, sCluster);

        return iter->second;
    }

    // This member function returns a const reference to the cluster
    // indicated by the passed cluster identifier. Used in context
    // when this object is passed as const reference
    const Cluster& ClusterManager::getCluster(const string& sCluster) const
    {
        auto iter = mClusterMap.find(sCluster);

        if (iter == mClusterMap.end())
            throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster, sCluster);

        return iter->second;
    }

    // This member function creates a new cluster from the passed
    // cluster identifier and returns a reference to this new
    // object
    Cluster& ClusterManager::newCluster(const string& sCluster)
    {
        string sValidName = validateClusterName(sCluster);
        mClusterMap[sValidName] = Cluster();

        return mClusterMap[sValidName];
    }

    // This member function appends the passed cluster to the
    // internal cluster map using the passed string as the identifier
    void ClusterManager::appendCluster(const string& sCluster, const Cluster& cluster)
    {
        mClusterMap[validateClusterName(sCluster)] = cluster;
    }

    // This member function removes the cluster from memory,
    // which corresponds to the passed cluster identifier
    void ClusterManager::removeCluster(const string& sCluster)
    {
        auto iter = mClusterMap.find(sCluster);

        if (iter != mClusterMap.end())
            mClusterMap.erase(iter);
    }

    // This member function creates a temporary cluster
    // with a unique name and returns this name to the
    // calling function
    string ClusterManager::createTemporaryCluster()
    {
        string sTemporaryClusterName = "_~~TEMPCLUSTER_" + toString(mClusterMap.size()) + "_";
        mClusterMap[sTemporaryClusterName] = Cluster();

        return sTemporaryClusterName + "{}";
    }

    // This member function returns all temporary
    // clusters from the internal map. Temporary
    // clusters are indicated by their name
    void ClusterManager::removeTemporaryClusters()
    {
        auto iter = mClusterMap.begin();

        while (iter != mClusterMap.end())
        {
            if (iter->first.substr(0, 15) == "_~~TEMPCLUSTER_")
            {
                iter = mClusterMap.erase(iter);
            }
            else
                ++iter;
        }
    }

    // Clear all clusters currently in memory
    void ClusterManager::clearAllClusters()
    {
        mClusterMap.clear();
    }

    // This member function updates the dimension variable
    // reserved for cluster accesses with the size of the
    // current accessed cluster
    bool ClusterManager::updateClusterSizeVariables(StringView sCluster)
    {
        if (isCluster(sCluster))
            dClusterElementsCount = getCluster(sCluster.subview(0, sCluster.find('{'))).size();
        else
            dClusterElementsCount = 0.0;

        return true;
    }
}

