/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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


#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP

#include <string>
#include <vector>
#include <cmath>

using namespace std;

long long int intCast(double);


/////////////////////////////////////////////////
/// \brief This class abstracts all the index
/// logics, i.e. the logical differences between
/// single indices and indices described by a
/// vector.
/////////////////////////////////////////////////
class VectorIndex
{
    private:
        vector<long long int> vStorage;
        bool expand;

        /////////////////////////////////////////////////
        /// \brief This private memnber function
        /// calculates the index corresponding to the
        /// selected position n. Otherwise simply return
        /// the n-th vector index.
        ///
        /// \param n size_t
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        inline long long int getIndex(size_t n) const
        {
            // If single indices are used, they will be expanded
            // into virtual vectors
            if (expand)
            {
                // If the second index has not been defined,
                // we'll expand the index using no end
                if (vStorage.back() == OPEN_END)
                {
                    if (vStorage.size() == 2)
                        return vStorage.front() + n;
                    else if (vStorage.size() > n+1)
                        return vStorage[n];
                    else
                        return vStorage[vStorage.size()-1] + n - (vStorage.size()-1);
                }

                // Consider the case that the order of the indices
                // could be inverted
                if (vStorage.front() <= vStorage.back() && n + vStorage.front() <= vStorage.back())
                    return vStorage.front() + n;
                else if (vStorage.front() > vStorage.back() && vStorage.front() - n >= vStorage.back()) // >= because vStorage.back() was not decremented first
                    return vStorage.front() - n;
            }
            else if (n < vStorage.size())
                return vStorage[n];

            return INVALID;
        }

    public:
        enum
        {
            INVALID = -1,
            OPEN_END = -2,
            STRING = -3
        };

        /////////////////////////////////////////////////
        /// \brief Default constructor.
        ///
        ///
        /////////////////////////////////////////////////
        VectorIndex()
        {
            vStorage.assign({INVALID, INVALID});
            expand = true;
        }

        /////////////////////////////////////////////////
        /// \brief Copy constructor.
        ///
        /// \param vindex const VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const VectorIndex& vindex)
        {
            vStorage = vindex.vStorage;
            expand = vindex.expand;
        }

        /////////////////////////////////////////////////
        /// \brief Constructor from an array of doubles.
        /// The third argument is used only to avoid
        /// misinterpretation from the compiler.
        ///
        /// \param indices const double*
        /// \param nResults int
        /// \param unused int
        ///
        /////////////////////////////////////////////////
        VectorIndex(const double* indices, int nResults, int unused)
        {
            // Store the indices and convert them to integers
            // using the intCast() function
            for (int i = 0; i < nResults; i++)
            {
                if (!isnan(indices[i]) && !isinf(indices[i]))
                    vStorage.push_back(intCast(indices[i]) - 1);
            }

            expand = false;
        }

        /////////////////////////////////////////////////
        /// \brief Constructor for single indices.
        ///
        /// \param nStart long longint
        /// \param nEnd long long int nEnd
        ///
        /////////////////////////////////////////////////
        VectorIndex(long long int nStart, long long int nEnd = INVALID)
        {
            vStorage.assign({nStart, nEnd});
            expand = true;
        }

        /////////////////////////////////////////////////
        /// \brief Constructor from a STL vector.
        ///
        /// \param vIndex const vector<long long int>&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const vector<long long int>& vIndex)
        {
            if (vIndex.size())
            {
                vStorage = vIndex;
                expand = false;
            }
            else
            {
                vStorage.assign({INVALID, INVALID});
                expand = true;
            }
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload for the
        /// same type.
        ///
        /// \param vIndex const VectorIndex&
        /// \return VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex& operator=(const VectorIndex& vIndex)
        {
            vStorage = vIndex.vStorage;
            expand = vIndex.expand;
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload for STL
        /// vectors.
        ///
        /// \param vIndex const vector<long long int>&
        /// \return VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex& operator=(const vector<long long int>& vIndex)
        {
            if (vIndex.size())
            {
                vStorage = vIndex;
                expand = false;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a subset
        /// of the internal stored index just like the
        /// std::string::substr() member function.
        ///
        /// Single indices are not expanded therefore
        /// the storage space used for the newly created
        /// index won't be larger than the current one.
        /// However, already expanded indices will result
        /// in an expanded subindex vector.
        ///
        /// \param pos size_t
        /// \param nLen size_t
        /// \return VectorIndex
        ///
        /////////////////////////////////////////////////
        VectorIndex subidx(size_t pos, size_t nLen = string::npos) const
        {
            // Consider some strange border cases
            if (pos >= size())
                return VectorIndex();

            if (nLen > size() - pos)
                nLen = size() - pos;

            // Return a single index
            if (!nLen)
                return VectorIndex(getIndex(pos));

            // Calculate the starting and ending indices for
            // single indices. The last index is decremented,
            // because the first and the last index already
            // count as two indices.
            if (expand)
                return VectorIndex(getIndex(pos), getIndex(pos+nLen-1));

            // Return a copy of the internal storage, if it is already
            // expanded. The last index is not decremented, because
            // terminating iterator always points after the last
            // element in the list
            return VectorIndex(vector<long long int>(vStorage.begin()+pos, vStorage.begin()+pos+nLen));
        }

        /////////////////////////////////////////////////
        /// \brief This member function linearizes the
        /// contents of a vector-described index set. The
        /// vectorial information is lost after using this
        /// function and the index will be described by
        /// single indices constructed from the minimal
        /// and maximal index values.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void linearize()
        {
            if (!expand)
            {
                long long int nMin = min();
                long long int nMax = max();

                vStorage.clear();
                vStorage.assign({nMin, nMax});
                expand = true;
            }
        }

        /////////////////////////////////////////////////
        /// \brief Overload for the access operator.
        /// Redirects the control to the private
        /// getIndex() member function.
        ///
        /// \param n size_t
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        inline long long int operator[](size_t n) const
        {
            return getIndex(n);
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the size
        /// of the indices stored in this class.
        ///
        /// It will return either the size of the stored
        /// vectorial indices or the calculated size for
        /// expanded single indices.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t size() const
        {
            if (vStorage.size() == 2 && !isValid())
                return 0;
            else if (vStorage.back() == INVALID)
                return 1;
            else if (vStorage.back() == OPEN_END)
                return -1;
            else if (vStorage.size() == 2 && expand)
                return abs(vStorage.back() - vStorage.front() + 1);
            else
                return vStorage.size();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the
        /// number of nodes describing the index set,
        /// which is stored internally.
        ///
        /// This is used in cases, where the indices will
        /// be interpreted slightly different than in the
        /// usual cases.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t numberOfNodes() const
        {
            if (!isValid())
                return 0;
            else if (vStorage.size() == 2 && vStorage.back() == INVALID)
                return 1;
            else
                return vStorage.size();
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the single indices are in the correct
        /// order.
        ///
        /// This function won't return correct results for
        /// already expanded vectors.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isOrdered() const
        {
            if (isValid())
                return vStorage.front() <= vStorage.back() || vStorage.back() == INVALID || vStorage.back() == OPEN_END;

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the indices are calculated or actual
        /// vectorial indices.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isExpanded() const
        {
            return expand;
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// set the index at a special position. This
        /// will expand the internal storage and switch
        /// from single indices to vectorial ones
        /// automatically.
        ///
        /// \param nthIndex size_t
        /// \param nVal long long int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setIndex(size_t nthIndex, long long int nVal)
        {
            // If the size is large enough, simply store
            // the passed index. Otherwise expand the
            // index using invalid values and store it
            // afterwards
            if (nthIndex < vStorage.size())
            {
                vStorage[nthIndex] = nVal;
            }
            else
            {
                // increase the size by using invalid values
                while (vStorage.size() <= nthIndex)
                    vStorage.push_back(INVALID);

                vStorage[nthIndex] = nVal;

                // If the last value is not an open end
                // value and the size of the internal storage
                // is larger than two, deactivate the expanding
                if (vStorage.size() > 2 && vStorage.back() != OPEN_END)
                    expand = false;
            }
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// index value at the end of the index vector.
        /// The internal storage is expanded first, if
        /// necessary.
        ///
        /// \param nVal long long int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void push_back(long long int nVal)
        {
            if (expand)
            {
                vStorage = getVector();
                expand = false;
            }

            vStorage.push_back(nVal);
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// vector to the end of the index vector. The
        /// internal storage is expanded first, if
        /// necessary.
        ///
        /// \param vVector const vector<long long int>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void append(const vector<long long int>& vVector)
        {
            if (expand)
            {
                vStorage = getVector();
                expand = false;
            }

            vStorage.insert(vStorage.end(), vVector.begin(), vVector.end());
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// VectorIndex to the end of the index vector.
        /// The internal storage is expanded first, if
        /// necessary. This function is an overload for
        /// convenience.
        ///
        /// \param vIndex const VectorIndex&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void append(const VectorIndex& vIndex)
        {
            if (vIndex.expand)
                append(vIndex.getVector());
            else
                append(vIndex.vStorage);
        }

        /////////////////////////////////////////////////
        /// \brief This function will prepend the passed
        /// vector before the beginning of the index
        /// vector. The internal storage is expanded
        /// first, if necessary.
        ///
        /// \param vVector const vector<long long int>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void prepend(const vector<long long int>& vVector)
        {
            if (expand)
            {
                vStorage = getVector();
                expand = false;
            }

            vStorage.insert(vStorage.begin(), vVector.begin(), vVector.end());
        }

        /////////////////////////////////////////////////
        /// \brief This function will append the passed
        /// VectorIndex before the beginning of the index
        /// vector. The internal storage is expanded
        /// first, if necessary. This function is an
        /// overload for convenience.
        ///
        /// \param vIndex const VectorIndex&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void prepend(const VectorIndex& vIndex)
        {
            if (vIndex.expand)
                prepend(vIndex.getVector());
            else
                prepend(vIndex.vStorage);
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a STL
        /// vector, which will resemble the indices
        /// stored internally. This includes that the
        /// single indices are expanded in the returned
        /// vector.
        ///
        /// \return vector<long long int>
        ///
        /////////////////////////////////////////////////
        vector<long long int> getVector() const
        {
            // Expand the single indices stored internally
            // if needed
            if (expand)
            {
                vector<long long int> vReturn;

                for (size_t i = 0; i < size(); i++)
                {
                    vReturn.push_back(getIndex(i));
                }

                return vReturn;
            }

            return vStorage;
        }

        /////////////////////////////////////////////////
        /// \brief This function calculates the maximal
        /// index value obtained from the values stored
        /// internally.
        ///
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        long long int max() const
        {
            if (expand)
                return ::max(vStorage.front(), vStorage.back());

            long long int nMax = vStorage.front();

            for (size_t i = 1; i < vStorage.size(); i++)
            {
                if (vStorage[i] > nMax)
                    nMax = vStorage[i];
            }

            return nMax;
        }

        /////////////////////////////////////////////////
        /// \brief This member function calculates the
        /// minimal index value obtained from the values
        /// stored internally.
        ///
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        long long int min() const
        {
            if (expand)
                return ::min(vStorage.front(), vStorage.back());

            long long int nMin = vStorage.front();

            for (size_t i = 1; i < vStorage.size(); i++)
            {
                if (vStorage[i] > nMin)
                    nMin = vStorage[i];
            }

            return nMin;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the internal index set is valid.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isValid() const
        {
            return vStorage.front() != INVALID;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the internal index set has an open
        /// end.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isOpenEnd() const
        {
            return vStorage.back() == OPEN_END;
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines,
        /// whether the internal index set referres to
        /// the table headlines.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isString() const
        {
            return vStorage.front() == STRING || vStorage.back() == STRING;
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a
        /// reference to the first index value stored
        /// internally.
        ///
        /// \return long long int&
        ///
        /////////////////////////////////////////////////
        long long int& front()
        {
            return vStorage.front();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a
        /// reference to the final index value stored
        /// internally.
        ///
        /// \return long long int&
        ///
        /////////////////////////////////////////////////
        long long int& back()
        {
            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// reference to the first index value stored
        /// internally.
        ///
        /// \return const long long int&
        ///
        /////////////////////////////////////////////////
        const long long int& front() const
        {
            return vStorage.front();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// reference to the final index value stored
        /// internally.
        ///
        /// \return const long long int&
        ///
        /////////////////////////////////////////////////
        const long long int& back() const
        {
            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the last
        /// index value, which can be reached by the
        /// values stored internally (this is most
        /// probably different from the final value).
        ///
        /// \return long long int
        ///
        /////////////////////////////////////////////////
        long long int last() const
        {
            if (expand && vStorage.back() == INVALID)
                return vStorage.front();

            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// force the indices stored internally to be in
        /// a defined interval. If the values are already
        /// in a smaller interval, nothing happens.
        ///
        /// \param nMin long long int
        /// \param nMax long long int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setRange(long long int nMin, long long int nMax)
        {
            // Change the order of the minimal and
            // maximal value, if needed
            if (nMin > nMax)
            {
                long long int nTemp = nMin;
                nMin = nMax;
                nMax = nTemp;
            }

            // Compare all values to the defined
            // interval
            for (size_t i = 0; i < vStorage.size(); i++)
            {
                // Special cases: if the current value
                // is open end, use the maximal value
                // passed to this function
                if (vStorage[i] == OPEN_END)
                {
                    vStorage[i] = nMax;
                    continue;
                }
                else if (vStorage[i] == INVALID)
                    continue;

                if (vStorage[i] < nMin)
                    vStorage[i] = nMin;

                if (vStorage[i] > nMax)
                    vStorage[i] = nMax;
            }
        }

};


/////////////////////////////////////////////////
/// \brief This class extends the std::vector for endlessness.
///
/// This class template automatically creates empty elements,
/// if the index operator access elements beyond its size.
/////////////////////////////////////////////////
template<class T>
class EndlessVector : public vector<T>
{
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor
        /////////////////////////////////////////////////
        EndlessVector() : vector<T>() {}

        /////////////////////////////////////////////////
        /// \brief Copy constructor from same.
        ///
        /// \param vec const EndlessVector&
        ///
        /////////////////////////////////////////////////
        EndlessVector(const EndlessVector& vec) : vector<T>(vec) {}

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload from
        /// same.
        ///
        /// \param vec const EndlessVector&
        /// \return EndlessVector&
        ///
        /////////////////////////////////////////////////
        EndlessVector& operator=(const EndlessVector& vec)
        {
            vector<T>::operator=(vec);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Access operator overload. Will return
        /// default constructed instances of the template
        /// type T, if the position n points to a
        /// position beyond the internal array.
        ///
        /// \param n size_t
        /// \return T
        ///
        /////////////////////////////////////////////////
        T operator[](size_t n)
        {
            if (n < vector<T>::size())
                return vector<T>::operator[](n);

            return T();
        }
};


/////////////////////////////////////////////////
/// \brief This structure is central for managing
/// the indices of a table or cluster read or
/// write data access. The contained indices are
/// of the VectorIndex type. The structure also
/// contains a precompiled data access equation,
/// which is used to speed up the data access in
/// loops.
/////////////////////////////////////////////////
struct Indices
{
    VectorIndex row;
    VectorIndex col;
    string sCompiledAccessEquation;
};

// Structure for the findCommand function
struct Match
{
    string sString;
    unsigned int nPos;
};

// Structure for the horizontal and vertical lines in plots
struct Line
{
    string sDesc;
    string sStyle;
    double dPos;

    Line() : sDesc(""), sStyle("k;2"), dPos(0.0) {}
};

// Structure for the axes in plots
struct Axis
{
    string sLabel;
    string sStyle;
    double dMin;
    double dMax;
};

// Structure for using time axes
struct TimeAxis
{
    string sTimeFormat;
    bool use;

    TimeAxis() : sTimeFormat(""), use(false) {}

    void activate(const string& sFormat = "")
    {
        use = true;
        sTimeFormat = sFormat;

        if (!sTimeFormat.length())
            return;

        if (sTimeFormat.find("YYYY") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("YYYY"), 4, "%Y");

        if (sTimeFormat.find("YY") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("YY"), 2, "%y");

        if (sTimeFormat.find("MM") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("MM"), 2, "%m");

        if (sTimeFormat.find("DD") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("DD"), 2, "%d");

        if (sTimeFormat.find("HH") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("HH"), 2, "%H");

        if (sTimeFormat.find("hh") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("hh"), 2, "%H");

        if (sTimeFormat.find("mm") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("mm"), 2, "%M");

        if (sTimeFormat.find("ss") != string::npos)
            sTimeFormat.replace(sTimeFormat.find("ss"), 2, "%S");
    }

    void deactivate()
    {
        use = false;
        sTimeFormat.clear();
    }
};

// Structure as wrapper for the return value of procedures (which may be numerical or string values or a mixture of both)
struct Returnvalue
{
    vector<double> vNumVal;
    vector<string> vStringVal;

    // clear method
    void clear()
    {
        vNumVal.clear();
        vStringVal.clear();
    }

    // boolean checkers
    bool isString() const
    {
        return vStringVal.size();
    }
    bool isNumeric() const
    {
        return vNumVal.size() && !vStringVal.size();
    }
};

// Structure for the retouch functionality
struct RetouchRegion
{
    vector<vector<double> > vDataArray;
    double dMedian;

    RetouchRegion(size_t nSize, double _dMedian = NAN)
    {
        vDataArray.resize(nSize);

        for (unsigned int i = 0; i < nSize; i++)
        {
            vDataArray[i].resize(nSize, NAN);
        }

        dMedian = _dMedian;
    }

    void retouch()
    {
        int nOrder = vDataArray.size() - 2;

        for (unsigned int i = 0; i < vDataArray.size(); i++)
        {
            for (unsigned int j = 0; j < vDataArray.size(); j++)
            {
                if (isnan(vDataArray[i][j]))
                {
                    double dAverage = vDataArray[0][j]
                                      + (vDataArray[nOrder + 1][j] - vDataArray[0][j]) / (double)(nOrder + 1) * (double)i
                                      + vDataArray[i][0]
                                      + (vDataArray[i][nOrder + 1] - vDataArray[i][0]) / (double)(nOrder + 1) * (double)j;

                    dAverage *= 2.0;

                    if (i >= j)
                    {
                        dAverage += vDataArray[0][(i - j)]
                                    + (vDataArray[nOrder + 1 - (i - j)][nOrder + 1] - vDataArray[0][(i - j)]) / (double)(nOrder - (i - j) + 1) * (double)i;
                    }
                    else
                    {
                        dAverage += vDataArray[0][(j - i)]
                                    + (vDataArray[nOrder + 1][nOrder + 1 - (j - i)] - vDataArray[(j - i)][0]) / (double)(nOrder - (j - i) + 1) * (double)i;
                    }
                    if (i + j <= (unsigned)nOrder + 1)
                    {
                        dAverage += vDataArray[i + j][0]
                                    + (vDataArray[0][i + j] - vDataArray[i + j][0]) / (double)(i + j) * (double)j;
                    }
                    else
                    {
                        dAverage += vDataArray[nOrder + 1][(i + j - nOrder - 1)]
                                    + (vDataArray[(i + j - nOrder - 1)][nOrder + 1] - vDataArray[nOrder + 1][(i + j - nOrder - 1)]) / (double)(2 * nOrder + 2 - (i + j)) * (double)(j - (i + j - nOrder - 1));
                    }

                    dAverage /= 6.0;

                    if (isnan(dMedian))
                        vDataArray[i][j] = dAverage;
                    else
                    {
                        vDataArray[i][j] =
                                        0.5 * (1.0 - 0.5 * hypot(i - (nOrder) / 2.0, j - (nOrder) / 2.0) / (M_SQRT2 * (nOrder / 2.0)))
                                        * dMedian
                                        + 0.5 * (1.0 + 0.5 * hypot(i - (nOrder) / 2.0, j - (nOrder) / 2.0) / (M_SQRT2 * (nOrder / 2.0))) * dAverage;
                    }
                }
            }
        }
    }
};

// Structure for the four standard variables
struct Integration_Vars
{
    string sName[4] = {"x", "y", "z", "t"};
    double vValue[4][4];
};

// Structure for the sorting functionality: used for the recursive definition of
// the index columns for sorting
struct ColumnKeys
{
    int nKey[2];
    // Contains a recursive pointer
    ColumnKeys* subkeys;

    // Default constructor
    ColumnKeys() : nKey{-1,-1}, subkeys(nullptr) {}
    // Destructor recursively deletes the stored pointers
    ~ColumnKeys()
        {
            if (subkeys)
                delete subkeys;
        }
};

#endif
