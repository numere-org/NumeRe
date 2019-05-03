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

class VectorIndex
{
    private:
        vector<long long int> vStorage;
        bool expand;

        inline long long int getIndex(size_t n) const
        {
            if (expand)
            {
                if (vStorage.back() == INVALID && !n)
                    return vStorage.front();

                if (vStorage.back() == OPEN_END)
                {
                    if (vStorage.size() == 2)
                        return vStorage.front() + n;
                    else if (vStorage.size() > n+1)
                        return vStorage[n];
                    else
                        return vStorage[vStorage.size()-1] + n - (vStorage.size()-1);
                }

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

        VectorIndex()
        {
            vStorage.assign({INVALID, INVALID});
            expand = true;
        }

        VectorIndex(const VectorIndex& vindex)
        {
            vStorage = vindex.vStorage;
            expand = vindex.expand;
        }

        VectorIndex(const double* indices, int nResults, int unused)
        {
            for (int i = 0; i < nResults; i++)
            {
                if (!isnan(indices[i]) && !isinf(indices[i]))
                    vStorage.push_back(intCast(indices[i]) - 1);
            }

            expand = false;
        }

        VectorIndex(long long int nStart, long long int nEnd = INVALID)
        {
            vStorage.assign({nStart, nEnd});
            expand = true;
        }

        VectorIndex(const vector<long long int>& vIndex)
        {
            vStorage = vIndex;
            expand = false;
        }

        VectorIndex& operator=(const VectorIndex& vindex)
        {
            vStorage = vindex.vStorage;
            expand = vindex.expand;
            return *this;
        }

        VectorIndex& operator=(const vector<long long int>& vIndex)
        {
            vStorage = vIndex;
            expand = false;
            return *this;
        }

        VectorIndex subidx(size_t pos, size_t nLen = string::npos) const
        {
            if (pos >= size())
                return VectorIndex();

            if (nLen >= size())
                nLen = size() - pos;

            if (expand)
                return VectorIndex(getIndex(pos), getIndex(pos+nLen));

            return VectorIndex(vector<long long int>(vStorage.begin()+pos, vStorage.begin()+pos+nLen));
        }

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

        inline long long int operator[](size_t n) const
        {
            return getIndex(n);
        }

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

        size_t numberOfNodes() const
        {
            if (!isValid())
                return 0;
            else if (vStorage.size() == 2 && vStorage.back() == INVALID)
                return 1;
            else
                return vStorage.size();
        }

        bool isOrdered() const
        {
            if (isValid())
                return vStorage.front() <= vStorage.back() || vStorage.back() == INVALID || vStorage.back() == OPEN_END;

            return false;
        }

        bool isExpanded() const
        {
            return expand;
        }

        void setIndex(size_t nthIndex, long long int nVal)
        {
            if (nthIndex < vStorage.size())
            {
                vStorage[nthIndex] = nVal;
            }
            else
            {
                while (vStorage.size() <= nthIndex)
                    vStorage.push_back(INVALID);

                vStorage[nthIndex] = nVal;

                if (vStorage.size() > 2 && vStorage.back() != OPEN_END)
                    expand = false;
            }
        }

        vector<long long int> getVector() const
        {
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

        inline bool isValid() const
        {
            return vStorage.front() != INVALID;
        }

        inline bool isOpenEnd() const
        {
            return vStorage.back() == OPEN_END;
        }

        inline bool isString() const
        {
            return vStorage.front() == STRING || vStorage.back() == STRING;
        }

        long long int& front()
        {
            return vStorage.front();
        }

        long long int& back()
        {
            return vStorage.back();
        }

        const long long int& front() const
        {
            return vStorage.front();
        }

        const long long int& back() const
        {
            return vStorage.back();
        }

        long long int last() const
        {
            if (expand && vStorage.back() == INVALID)
                return vStorage.front();

            return vStorage.back();
        }

        void setRange(long long int nMin, long long int nMax)
        {
            if (nMin > nMax)
            {
                long long int nTemp = nMin;
                nMin = nMax;
                nMax = nTemp;
            }

            for (size_t i = 0; i < vStorage.size(); i++)
            {
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
