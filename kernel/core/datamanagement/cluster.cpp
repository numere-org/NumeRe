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

        // Clear the contents first
        clear();

        nGlobalType = cluster.nGlobalType;

        // Fill the cluster with copies from the passed cluster
        for (size_t i = 0; i < cluster.vClusterArray.size(); i++)
        {
            if (cluster.vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                vClusterArray.push_back(new ClusterDoubleItem(cluster.vClusterArray[i]->getDouble()));
            else
                vClusterArray.push_back(new ClusterStringItem(cluster.vClusterArray[i]->getInternalString()));
        }
    }


    /////////////////////////////////////////////////
    /// \brief Private double vector assignment
    /// function.
    ///
    /// \param vVals const std::vector<std::complex<double>>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::assign(const std::vector<std::complex<double>>& vVals)
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


    /////////////////////////////////////////////////
    /// \brief Private string vector assignment
    /// function.
    ///
    /// \param vStrings const std::vector<std::string>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::assign(const std::vector<std::string>& vStrings)
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
        nGlobalType = ClusterItem::ITEMTYPE_INVALID;

        if (data.size() == 1)
            _idx.row.setOpenEndIndex(std::max((int64_t)_idx.row.front(), (int64_t)size() - 1));

        // Assign the single results
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (data.size() > 1 && data.size() <= i)
                return;

            // Expand the current cluster on-the-fly
            while (_idx.row[i] >= (int)vClusterArray.size())
                push_back(new ClusterDoubleItem(NAN));

            if (data.get(i).isNumerical())
            {
                // Assign the value and expand singletons
                if (vClusterArray[_idx.row[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE)
                {
                    // Re-create the current item as double
                    delete vClusterArray[_idx.row[i]];
                    vClusterArray[_idx.row[i]] = new ClusterDoubleItem(data.get(i).getNum().asCF64());
                }
                else
                    vClusterArray[_idx.row[i]]->setDouble(data.get(i).getNum().asCF64());
            }
            else if (data.get(i).isString())
            {
                // Assign the value and expand singletons
                if (vClusterArray[_idx.row[i]]->getType() != ClusterItem::ITEMTYPE_STRING)
                {
                    // Re-create the current item as double
                    delete vClusterArray[_idx.row[i]];
                    vClusterArray[_idx.row[i]] = new ClusterStringItem(data.get(i).getStr());
                }
                else
                    vClusterArray[_idx.row[i]]->setString(data.get(i).getStr());
            }

        }
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
        if (isString() || isMixed())
        {
            if (bSortCaseInsensitive)
            {
                if (toLowerCase(vClusterArray[i]->getParserString()) < toLowerCase(vClusterArray[j]->getParserString()))
                    return -1;

                if (toLowerCase(vClusterArray[i]->getParserString()) == toLowerCase(vClusterArray[j]->getParserString()))
                    return 0;
            }
            else
            {
                if (vClusterArray[i]->getParserString() < vClusterArray[j]->getParserString())
                    return -1;

                if (vClusterArray[i]->getParserString() == vClusterArray[j]->getParserString())
                    return 0;
            }

            return 1;
        }
        else if (isDouble())
        {
            if (vClusterArray[i]->getDouble().real() < vClusterArray[j]->getDouble().real())
                return -1;

            if (vClusterArray[i]->getDouble() == vClusterArray[j]->getDouble())
                return 0;

            return 1;
        }

        return 0;
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
        if (vClusterArray[line]->getType() == ClusterItem::ITEMTYPE_DOUBLE && !std::isnan(vClusterArray[line]->getDouble().real()))
            return true;

        if (vClusterArray[line]->getType() == ClusterItem::ITEMTYPE_STRING && vClusterArray[line]->getParserString() != "\"\"")
            return true;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This private member function reorders
    /// the elements in the cluster based upon the
    /// passed index vector.
    ///
    /// \param vIndex std::vector<int>
    /// \param i1 int
    /// \param i2 int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::reorderElements(std::vector<int> vIndex, int i1, int i2)
    {
        std::vector<ClusterItem*> vSortVector = vClusterArray;

        // Copy the contents directly from the
        // prepared in the new order
        for (int i = 0; i <= i2-i1; i++)
        {
            vClusterArray[i+i1] = vSortVector[vIndex[i]];
        }
    }


    /////////////////////////////////////////////////
    /// \brief Reduces the size of this cluster to
    /// the specified number of elements. Does not
    /// create anything, if the cluster is already
    /// smaller.
    ///
    /// \param s size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::reduceSize(size_t s)
    {
        if (vClusterArray.size() <= s)
            return;

        for (size_t i = s; i < vClusterArray.size(); i++)
            delete vClusterArray[i];

        vClusterArray.resize(s);
    }


    /////////////////////////////////////////////////
    /// \brief This member function appends an
    /// arbitrary cluster item at the back of the
    /// internal cluster array buffer.
    ///
    /// \param item ClusterItem*
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::push_back(ClusterItem* item)
    {
        if (item)
            vClusterArray.push_back(item);

        if (nGlobalType == ClusterItem::ITEMTYPE_INVALID)
            return;

        if (item->getType() != nGlobalType)
            nGlobalType = ClusterItem::ITEMTYPE_MIXED;
    }


    /////////////////////////////////////////////////
    /// \brief This member function constructs a new
    /// double cluster item at the back of the
    /// internal cluster array buffer.
    ///
    /// \param val const std::complex<double>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::push_back(const std::complex<double>& val)
    {
        vClusterArray.push_back(new ClusterDoubleItem(val));

        if (nGlobalType == ClusterItem::ITEMTYPE_INVALID)
            return;

        if (nGlobalType == ClusterItem::ITEMTYPE_STRING)
            nGlobalType = ClusterItem::ITEMTYPE_MIXED;
    }


    /////////////////////////////////////////////////
    /// \brief This member function constructs a new
    /// string cluster item at the back of the
    /// internal cluster array buffer.
    ///
    /// \param strval const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::push_back(const std::string& strval)
    {
        vClusterArray.push_back(new ClusterStringItem(strval));

        if (nGlobalType == ClusterItem::ITEMTYPE_INVALID)
            return;

        if (nGlobalType == ClusterItem::ITEMTYPE_DOUBLE)
            nGlobalType = ClusterItem::ITEMTYPE_MIXED;
    }


    /////////////////////////////////////////////////
    /// \brief This member function removes the last
    /// item in the internal memory buffer and frees
    /// the associated memory.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::pop_back()
    {
        if (vClusterArray.size())
        {
            delete vClusterArray.back();
            vClusterArray.pop_back();

            nGlobalType = ClusterItem::ITEMTYPE_INVALID;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the size
    /// of the internal memory buffer as items.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Cluster::size() const
    {
        return vClusterArray.size();
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the size
    /// of the associated memory as bytes.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Cluster::getBytes() const
    {
        size_t nBytes = 0;

        // Go through the internal cluster array and calculate
        // the bytes needed for each single cluster item
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                nBytes += sizeof(std::complex<double>);
            else if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_STRING)
                nBytes += sizeof(char) * (vClusterArray[i]->getParserString().capacity()-2);
        }

        return nBytes;
    }


    /////////////////////////////////////////////////
    /// \brief This member function clears the
    /// internal memory buffer and frees the
    /// associated memory.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::clear()
    {
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            delete vClusterArray[i];
        }

        vClusterArray.clear();
        nGlobalType = ClusterItem::ITEMTYPE_INVALID;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns, whether
    /// the data in the cluster have mixed type.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function returns, whether
    /// the data in the cluster have only double as
    /// type.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function returns, whether
    /// the data in the cluster have only string as
    /// type.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function returns the type
    /// of the i-th cluster item in the internal
    /// memory buffer.
    ///
    /// \param i size_t
    /// \return unsigned short
    ///
    /////////////////////////////////////////////////
    unsigned short Cluster::getType(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getType();

        return ClusterItem::ITEMTYPE_INVALID;
    }


    mu::Value Cluster::getValue(size_t i) const
    {
        if (getType(i) == ClusterItem::ITEMTYPE_DOUBLE)
            return vClusterArray[i]->getDouble();

        if (getType(i) == ClusterItem::ITEMTYPE_STRING)
            return vClusterArray[i]->getInternalString();

        return mu::Value();
    }


    void Cluster::setValue(size_t i, const mu::Value& v)
    {
        if (v.getType() == mu::TYPE_STRING)
            setString(i, v.getStr());
        else if (v.getType() == mu::TYPE_NUMERICAL)
            setDouble(i, v.getNum().asCF64());
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the data
    /// of the i-th cluster item in memory as a value.
    ///
    /// \param i size_t
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::getDouble(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getDouble();

        return NAN;
    }


    /////////////////////////////////////////////////
    /// \brief This member function assigns a value
    /// as data for the i-th cluster item in memory.
    /// The type of the i-th cluster item is adapted
    /// on-the-fly.
    ///
    /// \param i size_t
    /// \param value const std::complex<double>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::setDouble(size_t i, const std::complex<double>& value)
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


    /////////////////////////////////////////////////
    /// \brief This member function returns the data
    /// of all cluster items memory as a value vector.
    ///
    /// \return std::vector<std::complex<double>>
    ///
    /////////////////////////////////////////////////
    std::vector<std::complex<double>> Cluster::getDoubleArray() const
    {
        std::vector<std::complex<double>> vArray;

        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            vArray.push_back(vClusterArray[i]->getDouble());
        }

        return vArray;
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
        if (_vLine.size() > 1 && !vClusterArray.size())
            vTarget->resize(1, mu::Value());
        else
        {
            vTarget->resize(_vLine.size(), mu::Value());

            // Insert the elements in the passed array
            if (_vLine.size() > 100000)
            {
                #pragma omp parallel for
                for (size_t i = 0; i < _vLine.size(); i++)
                {
                    if (_vLine[i] < (int)vClusterArray.size() && _vLine[i] >= 0)
                    {
                        if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                            (*vTarget)[i] = vClusterArray[_vLine[i]]->getDouble();
                        else if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_STRING)
                            (*vTarget)[i] = vClusterArray[_vLine[i]]->getInternalString();
                        else
                            (*vTarget)[i] = mu::Value();
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < _vLine.size(); i++)
                {
                    if (_vLine[i] < (int)vClusterArray.size() && _vLine[i] >= 0)
                    {
                        if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                            (*vTarget)[i] = vClusterArray[_vLine[i]]->getDouble();
                        else if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_STRING)
                            (*vTarget)[i] = vClusterArray[_vLine[i]]->getInternalString();
                        else
                            (*vTarget)[i] = mu::Value();
                    }
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
        // Free the internal memory first
        clear();

        if (a.getCommonType() == mu::TYPE_VOID)
            return;

        // Create empty space
        vClusterArray.resize(a.size(), nullptr);
        nGlobalType = ClusterItem::ITEMTYPE_INVALID;

        switch (a.getCommonType())
        {
            case mu::TYPE_MIXED:
                nGlobalType = ClusterItem::ITEMTYPE_MIXED;
                break;
            case mu::TYPE_NUMERICAL:
                nGlobalType = ClusterItem::ITEMTYPE_DOUBLE;
                break;
            case mu::TYPE_STRING:
                nGlobalType = ClusterItem::ITEMTYPE_STRING;
                break;
        }

        // Write data to the free space
        if (a.size() > 100000)
        {
            #pragma omp parallel for
            for (size_t i = 0; i < a.size(); i++)
            {
                if (a[i].isNumerical())
                    vClusterArray[i] = new ClusterDoubleItem(a[i].getNum().asCF64());
                else if (a[i].isString())
                    vClusterArray[i] = new ClusterStringItem(a[i].getStr());
                else
                    vClusterArray[i] = new ClusterDoubleItem(NAN);
            }
        }
        else
        {
            for (size_t i = 0; i < a.size(); i++)
            {
                if (a[i].isNumerical())
                    vClusterArray[i] = new ClusterDoubleItem(a[i].getNum().asCF64());
                else if (a[i].isString())
                    vClusterArray[i] = new ClusterStringItem(a[i].getStr());
                else
                    vClusterArray[i] = new ClusterDoubleItem(NAN);
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function assigns values as
    /// data for the all cluster items in memory. The
    /// type of the cluster items is adapted
    /// on-the-fly.
    ///
    /// \param vVals const std::vector<std::complex<double>>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::setDoubleArray(const std::vector<std::complex<double>>& vVals)
    {
        // Free the internal memory first
        clear();

        // Create empty space
        vClusterArray.resize(vVals.size(), nullptr);
        nGlobalType = ClusterItem::ITEMTYPE_DOUBLE;

        // Write data to the free space
        if (vVals.size() > 100000)
        {
            #pragma omp parallel for
            for (size_t i = 0; i < vVals.size(); i++)
            {
                vClusterArray[i] = new ClusterDoubleItem(vVals[i]);
            }
        }
        else
        {
            for (size_t i = 0; i < vVals.size(); i++)
            {
                vClusterArray[i] = new ClusterDoubleItem(vVals[i]);
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function assigns values as
    /// data for the all cluster items in memory. The
    /// type of the cluster items is adapted
    /// on-the-fly.
    ///
    /// \param nNum int
    /// \param data std::complex<double>*
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::setDoubleArray(int nNum, std::complex<double>* data)
    {
        // Free the internal memory first
        clear();

        // Create empty space
        vClusterArray.resize(nNum, nullptr);
        nGlobalType = ClusterItem::ITEMTYPE_DOUBLE;

        // Write data to the free space
        if (nNum > 100000)
        {
            #pragma omp parallel for
            for (int i = 0; i < nNum; i++)
            {
                vClusterArray[i] = new ClusterDoubleItem(data[i]);
            }
        }
        else
        {
            for (int i = 0; i < nNum; i++)
            {
                vClusterArray[i] = new ClusterDoubleItem(data[i]);
            }
        }
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
            return;
        }

        // If the indices indicate a complete override
        // do that here and return
        if (_idx.row.isOpenEnd() && _idx.row.front() == 0)
        {
            setValueArray(data);
            return;
        }

        // Assign the results depending on the type of the
        // passed indices
        assignVectorResults(_idx, data);
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the data
    /// of the i-th cluster item in memory as a
    /// string.
    ///
    /// \param i size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::getString(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getString();

        return "\"\"";
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the data
    /// of the i-th cluster item in memory as a
    /// string.
    ///
    /// \param i size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::getInternalString(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getInternalString();

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the data
    /// of the i-th cluster item in memory as a
    /// parser string.
    ///
    /// \param i size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::getParserString(size_t i) const
    {
        if (vClusterArray.size() > i)
            return vClusterArray[i]->getParserString();

        return "\"\"";
    }


    /////////////////////////////////////////////////
    /// \brief This member function assigns a string
    /// as data for the i-th cluster item in memory.
    /// The type of the i-th cluster item is adapted
    /// on-the-fly.
    ///
    /// \param i size_t
    /// \param strval const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::setString(size_t i, const std::string& strval)
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


    /////////////////////////////////////////////////
    /// \brief This member function returns the data
    /// of all cluster items memory as a value vector.
    ///
    /// \return std::vector<std::string>
    ///
    /////////////////////////////////////////////////
    std::vector<std::string> Cluster::getStringArray() const
    {
        std::vector<std::string> vArray;

        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            vArray.push_back(vClusterArray[i]->getParserString());
        }

        return vArray;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns the data
    /// of all cluster items memory as a value vector.
    ///
    /// \return std::vector<std::string>
    ///
    /////////////////////////////////////////////////
    std::vector<std::string> Cluster::getInternalStringArray() const
    {
        std::vector<std::string> vArray;

        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            vArray.push_back(vClusterArray[i]->getInternalString());
        }

        return vArray;
    }


    /////////////////////////////////////////////////
    /// \brief This member function assigns values as
    /// data for the all cluster items in memory. The
    /// type of the cluster items is adapted
    /// on-the-fly.
    ///
    /// \param sVals const std::vector<std::string>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Cluster::setStringArray(const std::vector<std::string>& sVals)
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

        reduceSize(sVals.size());
        nGlobalType = ClusterItem::ITEMTYPE_STRING;
    }


    /////////////////////////////////////////////////
    /// \brief Converts all contents of this cluster
    /// to a vector of strings. Intended to be used
    /// for data transfer.
    ///
    /// \return std::vector<std::string>
    ///
    /////////////////////////////////////////////////
    std::vector<std::string> Cluster::to_string() const
    {
        std::vector<std::string> vString(vClusterArray.size());

        // Append the contained data depending on its type
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                vString[i] = toCmdString(vClusterArray[i]->getDouble());
            else
                vString[i] = vClusterArray[i]->getParserString();
        }

        return vString;
    }


    /////////////////////////////////////////////////
    /// \brief This member function constructs a
    /// plain vector from the data in memory, which
    /// can be inserted in the commandline as a
    /// replacement for the call to the cluster.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::serialize() const
    {
        // Return nan, if no data is available
        if (!vClusterArray.size())
            return "nan";

        std::string sSerialization;

        // Append the contained data depending on its type
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            sSerialization += vClusterArray[i]->getString() + ",";
        }

        // Replace the last comma with a closing brace
        sSerialization.pop_back();

        return sSerialization;
    }


    /////////////////////////////////////////////////
    /// \brief This member function constructs a
    /// plain vector from the data in memory, which
    /// can be inserted in the commandline as a
    /// replacement for the call to the cluster.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::getVectorRepresentation() const
    {
        // Return nan, if no data is available
        if (!vClusterArray.size())
            return "nan";

        std::string sVector = "{";

        std::vector<std::string> vString = to_string();

        // Append the contained data depending on its type
        for (const auto& component : vString)
        {
            sVector += component + ",";
        }

        // Replace the last comma with a closing brace
        sVector.back() = '}';

        return sVector;
    }


    /////////////////////////////////////////////////
    /// \brief This member function constructs a
    /// short version of a plain vector from the data
    /// in memory, which is used to display a preview
    /// of the contained data in the variable viewers.
    ///
    /// \param maxStringLength size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::getShortVectorRepresentation(size_t maxStringLength) const
    {
        // Return an empty brace pair, if no data is
        // available
        if (!vClusterArray.size())
            return "{}";

        std::string sVector = "{";

        // Append the contained data depending on its type but
        // restrict the number to maximal five values (use the first
        // and the last ones) and insert an ellipsis in the middle
        for (size_t i = 0; i < vClusterArray.size(); i++)
        {
            if (vClusterArray[i]->getType() == ClusterItem::ITEMTYPE_DOUBLE)
                sVector += toString(vClusterArray[i]->getDouble(), 5) + ", ";
            else if (maxStringLength < std::string::npos)
                sVector += ellipsize(vClusterArray[i]->getString(), maxStringLength/4) + ", ";
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


    /////////////////////////////////////////////////
    /// \brief This public member function provides
    /// access to the sorting algorithm for the
    /// cluster object.
    ///
    /// \param i1 long longint
    /// \param i2 long longint
    /// \param sSortingExpression const std::string&
    /// \return std::vector<int>
    ///
    /////////////////////////////////////////////////
    std::vector<int> Cluster::sortElements(long long int i1, long long int i2, const std::string& sSortingExpression)
    {
        if (!vClusterArray.size())
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
            return std::vector<int>();

        return vIndex;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function erases
    /// elements located from the index i1 to i2.
    ///
    /// \param i1 long longint
    /// \param i2 long longint
    /// \return void
    ///
    /////////////////////////////////////////////////
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
            if (vLines[i] < 0 || vLines[i] >= (int)vClusterArray.size())
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

    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// standard deviation of the data in memory.
    /// Cluster items, which do not have the type
    /// "value" are ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::std(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        // Calculate the average of the referenced items
        std::complex<double> dAvg = avg(_vLine);
        std::complex<double> dStd = 0.0;
        size_t nInvalid = 0;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(std::abs(vClusterArray[_vLine[i]]->getDouble())))
                nInvalid++;
            else
                dStd += (dAvg - vClusterArray[_vLine[i]]->getDouble()) * conj(dAvg - vClusterArray[_vLine[i]]->getDouble());
        }

        if (nInvalid >= _vLine.size() - 1)
            return NAN;

        return std::sqrt(dStd / ((_vLine.size()) - 1 - (double)nInvalid));
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// average of the data in memory. Cluster items,
    /// which do not have the type "value" are
    /// ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::avg(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        std::complex<double> dAvg = 0.0;
        size_t nInvalid = 0;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(std::abs(vClusterArray[_vLine[i]]->getDouble())))
                nInvalid++;
            else
                dAvg += vClusterArray[_vLine[i]]->getDouble();
        }

        if (nInvalid >= _vLine.size())
            return NAN;

        return dAvg / (_vLine.size() - (double)nInvalid);
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// maximal value of the data in memory. Cluster
    /// items, which do not have the type "value" are
    /// ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::max(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isString())
            return NAN;

        double dMax = NAN;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(vClusterArray[_vLine[i]]->getDouble().real()))
                continue;

            if (std::isnan(dMax))
                dMax = vClusterArray[_vLine[i]]->getDouble().real();

            if (dMax < vClusterArray[_vLine[i]]->getDouble().real())
                dMax = vClusterArray[_vLine[i]]->getDouble().real();
        }

        return dMax;
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// maximal string value of the data in memory.
    /// Cluster items of all types are used and
    /// converted on-the-fly.
    ///
    /// \param _vLine const VectorIndex&
    /// \return string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::strmax(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isDouble())
            return "";

        std::string sMax = "";

        // Apply the operation on all items and convert
        // their values on-the-fly
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (!sMax.length())
                sMax = vClusterArray[_vLine[i]]->getParserString();

            if (sMax < vClusterArray[_vLine[i]]->getParserString())
                sMax = vClusterArray[_vLine[i]]->getParserString();
        }

        return sMax;
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// minimal value of the data in memory. Cluster
    /// items, which do not have the type "value" are
    /// ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::min(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isString())
            return NAN;

        double dMin = NAN;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(vClusterArray[_vLine[i]]->getDouble().real()))
                continue;

            if (std::isnan(dMin))
                dMin = vClusterArray[_vLine[i]]->getDouble().real();

            if (dMin > vClusterArray[_vLine[i]]->getDouble().real())
                dMin = vClusterArray[_vLine[i]]->getDouble().real();
        }

        return dMin;
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// minimal string value of the data in memory.
    /// Cluster items of all types are used and
    /// converted on-the-fly.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::strmin(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isDouble())
            return "";

        std::string sMin = "";

        // Apply the operation on all items and convert
        // their values on-the-fly
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (!sMin.length())
                sMin = vClusterArray[_vLine[i]]->getParserString();

            if (sMin > vClusterArray[_vLine[i]]->getParserString())
                sMin = vClusterArray[_vLine[i]]->getParserString();
        }

        return sMin;
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// product of the data in memory. Cluster items,
    /// which do not have the type "value" are
    /// ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::prd(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        std::complex<double> dPrd = 1.0;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (std::isnan(std::abs(vClusterArray[_vLine[i]]->getDouble())))
                continue;

            dPrd *= vClusterArray[_vLine[i]]->getDouble();
        }

        return dPrd;
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// sum of the data in memory. Cluster items,
    /// which do not have the type "value" are
    /// ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::sum(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() && isString())
            return NAN;

        std::complex<double> dSum = 0.0;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(std::abs(vClusterArray[_vLine[i]]->getDouble())))
                continue;

            dSum += vClusterArray[_vLine[i]]->getDouble();
        }

        return dSum;
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// string concatenation of the data in memory.
    /// Cluster items of all types are used and
    /// converted on-the-fly.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Cluster::strsum(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size() || isDouble())
            return "";

        std::string sSum = "";

        // Apply the operation on all items and convert
        // their values on-the-fly
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            sSum += vClusterArray[_vLine[i]]->getParserString();
        }

        return sSum;
    }


    /////////////////////////////////////////////////
    /// \brief This member function counts the number
    /// of valid cluster items in memory. Cluster
    /// items of any type are counted, if they do
    /// have a valid value (depending on their type).
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::num(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0;

        int nInvalid = 0;

        // Apply the operation and ignore invalid values
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE && std::isnan(vClusterArray[_vLine[i]]->getDouble().real()))
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_STRING && vClusterArray[_vLine[i]]->getParserString() == "\"\"")
                nInvalid++;
        }

        return _vLine.size() - (double)nInvalid;
    }


    /////////////////////////////////////////////////
    /// \brief This member function applies an "AND"
    /// to the data in memory. Cluster items, which
    /// do not have the type "value" are ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::and_func(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0.0;

        double dRetVal = NAN;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (std::isnan(dRetVal))
                dRetVal = 1.0;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(vClusterArray[_vLine[i]]->getDouble().real()) || vClusterArray[_vLine[i]]->getDouble() == 0.0)
                return 0.0;
        }

        if (std::isnan(dRetVal))
            return 0.0;

        return 1.0;
    }


    /////////////////////////////////////////////////
    /// \brief This member function applies an "OR"
    /// to the data in memory. Cluster items, which
    /// do not have the type "value" are ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::or_func(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0.0;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE
                && (std::isnan(vClusterArray[_vLine[i]]->getDouble().real()) || vClusterArray[_vLine[i]]->getDouble() != 0.0))
                return 1.0;
        }

        return 0.0;
    }


    /////////////////////////////////////////////////
    /// \brief This member function applies an
    /// "exclusive OR" to the data in memory. Cluster
    /// items, which do not have the type "value" are
    /// ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::xor_func(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0.0;

        bool isTrue = false;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() == ClusterItem::ITEMTYPE_DOUBLE
                && (std::isnan(vClusterArray[_vLine[i]]->getDouble().real()) || vClusterArray[_vLine[i]]->getDouble() != 0.0))
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


    /////////////////////////////////////////////////
    /// \brief This member function counts the number
    /// of valid cluster items in memory. Cluster
    /// items of any type are counted, if the vector
    /// index points to a valid location.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::cnt(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return 0;

        int nInvalid = 0;

        // Apply the operation and ignore invalid locations
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                nInvalid++;
        }
        return _vLine.size() - (double)nInvalid;
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// euclidic vector norm of the data in memory.
    /// Cluster items, which do not have the type
    /// "value" are ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::norm(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        std::complex<double> dNorm = 0.0;

        // Apply the operation and ignore invalid or non-double items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(std::abs(vClusterArray[_vLine[i]]->getDouble())))
                continue;

            dNorm += vClusterArray[_vLine[i]]->getDouble() * conj(vClusterArray[_vLine[i]]->getDouble());
        }

        return std::sqrt(dNorm);
    }


    /////////////////////////////////////////////////
    /// \brief This member function compares the
    /// values in memory with the referenced value
    /// and returns indices or values depending on
    /// the passed type. Cluster items, which do not
    /// have the type "value" are ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \param dRef std::complex<double>
    /// \param _nType int
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::cmp(const VectorIndex& _vLine, std::complex<double> dRef, int _nType)
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

        std::complex<double> dKeep = dRef;
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
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                continue;

            if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE
                || std::isnan(std::abs(vClusterArray[_vLine[i]]->getDouble())))
                continue;

            if (vClusterArray[_vLine[i]]->getDouble() == dRef)
            {
                if (nType & RETURN_VALUE)
                    return vClusterArray[_vLine[i]]->getDouble();

                return i+1;
            }
            else if (nType & RETURN_GE && vClusterArray[_vLine[i]]->getDouble().real() > dRef.real())
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return vClusterArray[_vLine[i]]->getDouble().real();

                    return i+1;
                }

                if (nKeep == -1 || vClusterArray[_vLine[i]]->getDouble().real() < dKeep.real())
                {
                    dKeep = vClusterArray[_vLine[i]]->getDouble().real();
                    nKeep = i;
                }
                else
                    continue;
            }
            else if (nType & RETURN_LE && vClusterArray[_vLine[i]]->getDouble().real() < dRef.real())
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return vClusterArray[_vLine[i]]->getDouble().real();

                    return i+1;
                }

                if (nKeep == -1 || vClusterArray[_vLine[i]]->getDouble().real() > dKeep.real())
                {
                    dKeep = vClusterArray[_vLine[i]]->getDouble().real();
                    nKeep = i;
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


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// median value of the data in memory. Cluster
    /// items, which do not have the type "value" are
    /// ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::med(const VectorIndex& _vLine)
    {
        if (!vClusterArray.size())
            return NAN;

        double dMed = 0.0;
        size_t nInvalid = 0;
        size_t nCount = 0;
        double* dData = 0;

        // Calculate the number of valid items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(vClusterArray[_vLine[i]]->getDouble().real()))
                nInvalid++;
        }

        if (nInvalid >= _vLine.size())
            return NAN;

        // Create a memory buffer of the corresponding size
        dData = new double[_vLine.size() - nInvalid];

        // copy the data to the buffer
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size() || vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(vClusterArray[_vLine[i]]->getDouble().real()))
                continue;

            dData[nCount] = vClusterArray[_vLine[i]]->getDouble().real();
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


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// p-th percentile of the data in memory.
    /// Cluster items, which do not have the type
    /// "value" are ignored.
    ///
    /// \param _vLine const VectorIndex&
    /// \param dPct std::complex<double>
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> Cluster::pct(const VectorIndex& _vLine, std::complex<double> dPct)
    {
        if (!vClusterArray.size())
            return NAN;

        size_t nInvalid = 0;
        size_t nCount = 0;
        double* dData = 0;

        if (dPct.real() >= 1 || dPct.real() <= 0)
            return NAN;

        // Calculate the number of valid items
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size())
                nInvalid++;
            else if (vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(vClusterArray[_vLine[i]]->getDouble().real()))
                nInvalid++;
        }

        if (nInvalid >= _vLine.size())
            return NAN;

        // Create a memory buffer of the corresponding size
        dData = new double[_vLine.size() - nInvalid];

        // copy the data to the buffer
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= (int)vClusterArray.size() || vClusterArray[_vLine[i]]->getType() != ClusterItem::ITEMTYPE_DOUBLE || std::isnan(vClusterArray[_vLine[i]]->getDouble().real()))
                continue;

            dData[nCount] = vClusterArray[_vLine[i]]->getDouble().real();
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
        dPct = gsl_stats_quantile_from_sorted_data(dData, 1, nCount, dPct.real());

        delete[] dData;

        return dPct;
    }


    //
    // class CLUSTERMANAGER
    //
    //


    /////////////////////////////////////////////////
    /// \brief This member function creates a valid
    /// cluster identifier name, which can be used to
    /// create or append a new cluster.
    ///
    /// \param sCluster const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string ClusterManager::validateClusterName(const std::string& sCluster)
    {
        std::string sClusterName = sCluster.substr(0, sCluster.find('{'));
        const static std::string sVALIDCHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_~";

        if ((sClusterName[0] >= '0' && sClusterName[0] <= '9') || sClusterName[0] == '~' || sClusterName.find_first_not_of(sVALIDCHARACTERS) != std::string::npos)
            throw SyntaxError(SyntaxError::INVALID_CLUSTER_NAME, "", SyntaxError::invalid_position, sClusterName);

        return sClusterName;
    }


    /////////////////////////////////////////////////
    /// \brief This private member function returns
    /// an iterator to the referenced cluster or
    /// std::map::end().
    ///
    /// \param view StringView
    /// \return std::map<std::string, Cluster>::const_iterator
    ///
    /////////////////////////////////////////////////
    std::map<std::string, Cluster>::const_iterator ClusterManager::mapStringViewFind(StringView view) const
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

    /////////////////////////////////////////////////
    /// \brief This private member function returns
    /// an iterator to the referenced cluster or
    /// std::map::end().
    ///
    /// \param view StringView
    /// \return std::map<std::string, Cluster>::iterator
    ///
    /////////////////////////////////////////////////
    std::map<std::string, Cluster>::iterator ClusterManager::mapStringViewFind(StringView view)
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
                if (mClusterMap.find(sCmdLine.substr(nStartPos, i - nStartPos)) != mClusterMap.end())
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
        if (mapStringViewFind(sCluster.subview(0, sCluster.find('{'))) != mClusterMap.end())
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
        if (mClusterMap.find(sCluster.substr(0, sCluster.find('{'))) != mClusterMap.end())
            return true;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns a
    /// reference to the cluster indicated by the
    /// passed cluster identifier.
    ///
    /// \param sCluster StringView
    /// \return Cluster&
    ///
    /////////////////////////////////////////////////
    Cluster& ClusterManager::getCluster(StringView sCluster)
    {
        auto iter = mapStringViewFind(sCluster.subview(0, sCluster.find('{')));

        if (iter == mClusterMap.end())
            throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster.to_string(), sCluster.to_string());

        return iter->second;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns a
    /// reference to the cluster indicated by the
    /// passed cluster identifier.
    ///
    /// \param sCluster const std::string&
    /// \return Cluster&
    ///
    /////////////////////////////////////////////////
    Cluster& ClusterManager::getCluster(const std::string& sCluster)
    {
        auto iter = mClusterMap.find(sCluster.substr(0, sCluster.find('{')));

        if (iter == mClusterMap.end())
            throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster, sCluster);

        return iter->second;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns a const
    /// reference to the cluster indicated by the
    /// passed cluster identifier. Used in context
    /// when this object is passed as const
    /// reference.
    ///
    /// \param sCluster const std::string&
    /// \return const Cluster&
    ///
    /////////////////////////////////////////////////
    const Cluster& ClusterManager::getCluster(const std::string& sCluster) const
    {
        auto iter = mClusterMap.find(sCluster.substr(0, sCluster.find('{')));

        if (iter == mClusterMap.end())
            throw SyntaxError(SyntaxError::CLUSTER_DOESNT_EXIST, sCluster, sCluster);

        return iter->second;
    }


    /////////////////////////////////////////////////
    /// \brief This member function creates a new
    /// cluster from the passed cluster identifier
    /// and returns a reference to this new object.
    ///
    /// \param sCluster const std::string&
    /// \return Cluster&
    ///
    /////////////////////////////////////////////////
    Cluster& ClusterManager::newCluster(const std::string& sCluster)
    {
        std::string sValidName = validateClusterName(sCluster);
        mClusterMap[sValidName] = Cluster();

        return mClusterMap[sValidName];
    }


    /////////////////////////////////////////////////
    /// \brief This member function appends the
    /// passed cluster to the internal cluster map
    /// using the passed string as the identifier.
    ///
    /// \param sCluster const std::string&
    /// \param cluster const Cluster&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ClusterManager::appendCluster(const std::string& sCluster, const Cluster& cluster)
    {
        mClusterMap[validateClusterName(sCluster)] = cluster;
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
        auto iter = mClusterMap.find(sCluster);

        if (iter != mClusterMap.end())
            mClusterMap.erase(iter);
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
        std::string sTemporaryClusterName = "_~~TC_" + toString(mClusterMap.size()) + "_" + suffix;
        mClusterMap[sTemporaryClusterName] = Cluster();

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
        auto iter = mClusterMap.begin();

        while (iter != mClusterMap.end())
        {
            if (iter->first.starts_with("_~~TC_"))
                iter = mClusterMap.erase(iter);
            else
                ++iter;
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
        mClusterMap.clear();
    }


    /////////////////////////////////////////////////
    /// \brief This member function updates the
    /// dimension variable reserved for cluster
    /// accesses with the size of the current
    /// accessed cluster.
    ///
    /// \param sCluster StringView
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ClusterManager::updateClusterSizeVariables(StringView sCluster)
    {
        if (isCluster(sCluster))
            dClusterElementsCount = mu::Value(getCluster(sCluster.subview(0, sCluster.find('{'))).size());
        else
            dClusterElementsCount = mu::Value(0.0);

        return true;
    }
}

