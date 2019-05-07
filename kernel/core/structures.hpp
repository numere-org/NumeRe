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

// This class abstracts all the index logics, i.e.
// the logical differences between single indices
// and indices described by a vector
class VectorIndex
{
    private:
        vector<long long int> vStorage;
        bool expand;

        // This private memnber function calculates the index
        // corresponding to the selected position n. Otherwise
        // simply return the nth vector index
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

        // Default constructor
        VectorIndex()
        {
            vStorage.assign({INVALID, INVALID});
            expand = true;
        }

        // Copy constructor
        VectorIndex(const VectorIndex& vindex)
        {
            vStorage = vindex.vStorage;
            expand = vindex.expand;
        }

        // Constructor from an array of doubles. The third
        // argument is used only to avoid misinterpretation of
        // the compiler
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

        // Constructor for single indices
        VectorIndex(long long int nStart, long long int nEnd = INVALID)
        {
            vStorage.assign({nStart, nEnd});
            expand = true;
        }

        // Constructor from a STL vector
        VectorIndex(const vector<long long int>& vIndex)
        {
            vStorage = vIndex;
            expand = false;
        }

        // Assignment operator overload for the same type
        VectorIndex& operator=(const VectorIndex& vindex)
        {
            vStorage = vindex.vStorage;
            expand = vindex.expand;
            return *this;
        }

        // Assignment operator overload for STL vectors
        VectorIndex& operator=(const vector<long long int>& vIndex)
        {
            vStorage = vIndex;
            expand = false;
            return *this;
        }

        // This member function returns a subset of the
        // internal stored index just like the substr()
        // member function. Single indices are not expanded
        // therefore the storage space used for the newly
        // created index won't be larger than the current one
        VectorIndex subidx(size_t pos, size_t nLen = string::npos) const
        {
            // Consider some strange border cases
            if (pos >= size())
                return VectorIndex();

            if (nLen >= size())
                nLen = size() - pos;

            // Return a single index
            if (!nLen)
            {
                return VectorIndex(getIndex(pos));
            }

            // Calculate the starting and ending indices for
            // single indices
            return VectorIndex(getIndex(pos), getIndex(pos+nLen-1));
        }

        // This member function linearizes the contents
        // of a vector-described index set. The vectorial
        // information is lost after using this function and
        // the index will be described by single indices
        // constructed from the minimal and maximal index
        // values
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

        // Overload for the access operator. Redirects the
        // control to the private getIndex() member function
        inline long long int operator[](size_t n) const
        {
            return getIndex(n);
        }

        // This member function returns the size of the
        // indices stored in this class. It will return
        // either the size of the stored vectorial indices
        // or the calculated size for expanded single
        // indices
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

        // This member function returns the number of
        // nodes describing the index set, which is
        // stored internally. This is used in cases,
        // where the indices will be interpreted slightly
        // different than in the usual cases
        size_t numberOfNodes() const
        {
            if (!isValid())
                return 0;
            else if (vStorage.size() == 2 && vStorage.back() == INVALID)
                return 1;
            else
                return vStorage.size();
        }

        // This member function determines, whether
        // the single indices are in the correct order
        bool isOrdered() const
        {
            if (isValid())
                return vStorage.front() <= vStorage.back() || vStorage.back() == INVALID || vStorage.back() == OPEN_END;

            return false;
        }

        // This member function determines, whether
        // the indices are calculated or actual
        // vectorial indices
        bool isExpanded() const
        {
            return expand;
        }

        // This member function can be used to set the
        // index at a special position. This will expand
        // the internal storage and switch from single
        // indices to vectorial ones automatically
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

        // This member function returns a STL vector, which
        // will resemble the indices stored internally. This
        // includes that the single indices are expanded in
        // the returned vector
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

        // This function calculates the maximal index value
        // obtained from the values stored internally
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

        // This member function calculates the minimal index
        // value obtained from the values stored internally
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

        // This member function determines, whether the
        // internal index set is valid
        inline bool isValid() const
        {
            return vStorage.front() != INVALID;
        }

        // This member function determines, whether the
        // internal index set has an open end
        inline bool isOpenEnd() const
        {
            return vStorage.back() == OPEN_END;
        }

        // This member function determines, whether the
        // internal index set referres to the table headlines
        inline bool isString() const
        {
            return vStorage.front() == STRING || vStorage.back() == STRING;
        }

        // This member function returns a reference to
        // the first index value stored internally
        long long int& front()
        {
            return vStorage.front();
        }

        // This member function returns a reference to
        // the final index value stored internally
        long long int& back()
        {
            return vStorage.back();
        }

        // This member function returns a const reference to
        // the first index value stored internally
        const long long int& front() const
        {
            return vStorage.front();
        }

        // This member function returns a const reference to
        // the final index value stored internally
        const long long int& back() const
        {
            return vStorage.back();
        }

        // This member function returns the last index value,
        // which can be reached by the values stored internally
        // (this is most probably different from the final value)
        long long int last() const
        {
            if (expand && vStorage.back() == INVALID)
                return vStorage.front();

            return vStorage.back();
        }

        // This member function can be used to force the indices
        // stored internally to be in a defined interval.
        // If the values are already in a smaller interval,
        // nothing happens.
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



// Structure for the indices
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
struct RetoqueRegion
{
    vector<vector<double> > vDataArray;
    vector<vector<bool> > vValidationArray;
    double dMedian;
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
