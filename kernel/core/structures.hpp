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
#include <set>
#include <cmath>
#include <algorithm>
#include "interval.hpp"
#include "utils/stringview.hpp"

long long int intCast(const std::complex<double>&);
std::string toString(int);
std::string toString(long long int);


/////////////////////////////////////////////////
/// \brief This structure combines a vector of
/// declared variables including their values and
/// respective sizes with a set of variable type
/// counts, which can be used to separate the
/// single vector in multiple vectors containing
/// only a single type of variables.
/////////////////////////////////////////////////
struct NumeReVariables
{
    std::vector<std::string> vNumVars;
    std::vector<std::string> vStrVars;
    std::vector<std::string> vTables;
    std::vector<std::string> vClusters;
    std::vector<std::string> vObjects;
    std::vector<std::string> vArguments;
    std::vector<std::string> vGlobals;
};


/////////////////////////////////////////////////
/// \brief This class abstracts all the index
/// logics, i.e. the logical differences between
/// single indices and indices described by a
/// vector.
/////////////////////////////////////////////////
class VectorIndex
{
    private:
        mutable std::vector<int> vStorage;
        bool expand;

        /////////////////////////////////////////////////
        /// \brief This private memnber function
        /// calculates the index corresponding to the
        /// selected position n. Otherwise simply return
        /// the n-th vector index.
        ///
        /// \param n size_t
        /// \return int
        ///
        /////////////////////////////////////////////////
        inline int getIndex(size_t n) const
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
                if (vStorage.front() <= vStorage.back() && (int)n + vStorage.front() <= vStorage.back())
                    return vStorage.front() + n;
                else if (vStorage.front() > vStorage.back() && vStorage.front() - (int)n >= vStorage.back()) // >= because vStorage.back() was not decremented first
                    return vStorage.front() - n;
            }
            else if (n < vStorage.size())
                return vStorage[n];

            return INVALID;
        }

    public:
        /////////////////////////////////////////////////
        /// \brief Definition of a forward iterator for
        /// the VectorIndex class
        /////////////////////////////////////////////////
        class ForwardIterator
        {
            public:
                // Definition of necessary iterator tags
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = int;
                using pointer           = int*;
                using reference         = int&;

            private:
                const VectorIndex* m_data;
                size_t m_curPos;

                /////////////////////////////////////////////////
                /// \brief Determine, if the iterator is beyond
                /// the end.
                ///
                /// \return bool
                ///
                /////////////////////////////////////////////////
                bool isEnd() const
                {
                    return m_curPos >= m_data->size();
                }

            public:
                ForwardIterator(const VectorIndex* data = nullptr, size_t p = 0) : m_data(data), m_curPos(p) {}
                ForwardIterator(const ForwardIterator& other) : m_data(other.m_data), m_curPos(other.m_curPos) {}

                /////////////////////////////////////////////////
                /// \brief Assignment operator
                ///
                /// \param other const ForwardIterator&
                /// \return ForwardIterator&
                ///
                /////////////////////////////////////////////////
                ForwardIterator& operator=(const ForwardIterator& other)
                {
                    m_data = other.m_data;
                    m_curPos = other.m_curPos;
                    return *this;
                }

                /////////////////////////////////////////////////
                /// \brief Equality operator
                ///
                /// \param other const ForwardIterator&
                /// \return bool
                ///
                /////////////////////////////////////////////////
                bool operator==(const ForwardIterator& other) const
                {
                    return m_data == other.m_data
                        && (m_curPos == other.m_curPos || (isEnd() && other.isEnd()));
                }

                /////////////////////////////////////////////////
                /// \brief Inequality operator
                ///
                /// \param other const ForwardIterator&
                /// \return bool
                ///
                /////////////////////////////////////////////////
                bool operator!=(const ForwardIterator& other) const
                {
                    return !operator==(other);
                }

                /////////////////////////////////////////////////
                /// \brief Pre-increment operator
                ///
                /// \return ForwardIterator&
                ///
                /////////////////////////////////////////////////
                ForwardIterator& operator++()
                {
                    m_curPos++;
                    return *this;
                }

                /////////////////////////////////////////////////
                /// \brief Post-increment operator
                ///
                /// \param int
                /// \return ForwardIterator
                ///
                /////////////////////////////////////////////////
                ForwardIterator operator++(int)
                {
                    ForwardIterator t(*this);
                    m_curPos++;
                    return t;
                }

                /////////////////////////////////////////////////
                /// \brief Dereferencing operator
                ///
                /// \return int
                ///
                /////////////////////////////////////////////////
                int operator*() const
                {
                    if (m_data && m_curPos < m_data->size())
                        return (*m_data)[m_curPos];

                    throw std::out_of_range("VectorIndex::ForwardIterator is invalid or beyond range");
                }
        };

        /////////////////////////////////////////////////
        /// \brief Special values for vector indices.
        /////////////////////////////////////////////////
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
        /// \param vIndex const VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const VectorIndex& vIndex) : vStorage(vIndex.vStorage), expand(vIndex.expand)
        {
        }

        /////////////////////////////////////////////////
        /// \brief VectorIndex move constructor.
        ///
        /// \param vIndex VectorIndex&&
        ///
        /////////////////////////////////////////////////
        VectorIndex(VectorIndex&& vIndex) : expand(std::move(vIndex.expand))
        {
			std::swap(vStorage, vIndex.vStorage);
        }

        /////////////////////////////////////////////////
        /// \brief Constructor from an array of doubles.
        /// The third argument is used only to avoid
        /// misinterpretation from the compiler.
        ///
        /// \param indices const mu::Array&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const mu::Array& indices)
        {
            vStorage.resize(indices.size(), INVALID);

            // Store the indices and convert them to integers
            // using the intCast() function
            for (size_t i = 0; i < indices.size(); i++)
            {
                if (!std::isnan(indices.get(i).getNum().asF64()) && !std::isinf(indices.get(i).getNum().asF64()))
                    vStorage[i] = indices.get(i).getNum().asI64() - 1;
            }

            expand = false;
        }

        /////////////////////////////////////////////////
        /// \brief Constructor for single indices.
        ///
        /// \param nStart int The first reachable index
        /// \param nEnd int The last reachable index
        ///
        /////////////////////////////////////////////////
        VectorIndex(int nStart, int nEnd = INVALID)
        {
            vStorage.assign({nStart, nEnd});
            expand = true;
        }

        /////////////////////////////////////////////////
        /// \brief Constructor from a STL vector.
        ///
        /// \param vIndex const std::vector<int>&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const std::vector<int>& vIndex)
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
        /// \brief Constructor from a size_t STL vector.
        ///
        /// \param vIndex const std::vector<size_t>&
        ///
        /////////////////////////////////////////////////
        VectorIndex(const std::vector<size_t>& vIndex)
        {
            if (vIndex.size())
            {
                vStorage.resize(vIndex.size());

                for (size_t i = 0; i < vIndex.size(); i++)
                {
                    vStorage[i] = (int)vIndex[i]-1;
                }

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
        /// \brief Assignment operator overload for the
        /// same type.
        ///
        /// \param indices const mu::Array&
        /// \return VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex& operator=(const mu::Array& indices)
        {
            vStorage.resize(indices.size(), INVALID);

            // Store the indices and convert them to integers
            // using the intCast() function
            for (size_t i = 0; i < indices.size(); i++)
            {
                if (!std::isnan(indices[i].getNum().asF64()) && !std::isinf(indices[i].getNum().asF64()))
                    vStorage[i] = indices[i].getNum().asI64() - 1;
            }

            expand = false;
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload for STL
        /// vectors.
        ///
        /// \param vIndex const std::vector<int>&
        /// \return VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex& operator=(const std::vector<int>& vIndex)
        {
            if (vIndex.size())
            {
                vStorage = vIndex;
                expand = false;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload for STL
        /// vectors containing size_t.
        ///
        /// \param vIndex const std::vector<size_t>&
        /// \return VectorIndex&
        ///
        /////////////////////////////////////////////////
        VectorIndex& operator=(const std::vector<size_t>& vIndex)
        {
            if (vIndex.size())
            {
                vStorage.resize(vIndex.size());

                for (size_t i = 0; i < vIndex.size(); i++)
                {
                    vStorage[i] = (int)vIndex[i]-1;
                }

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
        /// \note A length of 0 or 1 will both create a
        /// VectorIndex with only one valid entry (the
        /// first).
        ///
        /// \param pos size_t
        /// \param nLen size_t
        /// \return VectorIndex
        ///
        /////////////////////////////////////////////////
        VectorIndex subidx(size_t pos, size_t nLen = std::string::npos) const
        {
            // Consider some strange border cases
            if (pos >= size())
                return VectorIndex();

            if (nLen > size() - pos)
                nLen = size() - pos;

            // Return a single index
            if (nLen <= 1)
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
            return VectorIndex(std::vector<int>(vStorage.begin()+pos, vStorage.begin()+pos+nLen));
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
                int nMin = min();
                int nMax = max();

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
        /// \return int
        ///
        /////////////////////////////////////////////////
        inline int operator[](size_t n) const
        {
            return getIndex(n);
        }

        /////////////////////////////////////////////////
        /// \brief Get a portion of this index possible
        /// with another order.
        ///
        /// \param other const VectorIndex&
        /// \return VectorIndex
        ///
        /////////////////////////////////////////////////
        VectorIndex get(const VectorIndex& other) const
        {
            VectorIndex idx;

            if (!other.isOpenEnd())
                idx.vStorage.resize(other.size());
            else
                idx.vStorage.resize(size());

            idx.expand = false;

            for (size_t i = 0; i < (other.isOpenEnd() ? size() : other.size()); i++)
            {
                idx.vStorage[i] = getIndex(other[i]);
            }

            return idx;
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
            else if (vStorage.back() == INVALID && expand)
                return 1;
            else if (vStorage.back() == OPEN_END)
                return -1;
            else if (vStorage.size() == 2 && expand)
                return abs(vStorage.back() - vStorage.front()) + 1;
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
            else if (vStorage.size() == 2 && ((vStorage.back() == INVALID) xor (vStorage.front() == INVALID)))
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
        /// \brief Determine, whether the list of indices
        /// are unique (relevant for sorting).
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isUnique() const
        {
            if (!isValid())
                return false;

            // Expanded are always unique
            if (expand)
                return true;

            std::set<int> test;

            // Test by inserting into a set
            for (int i : vStorage)
            {
                if (!test.insert(i).second)
                    return false;
            }

            return true;
        }

        /////////////////////////////////////////////////
        /// \brief Ensure that the contained indices
        /// represent a permutation of the corresponding
        /// linear array.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isPermutation() const
        {
            if (!isValid())
                return false;

            // Expanded are always a permutation
            if (expand)
                return true;

            VectorIndex vPlain(0, this->max());

            // Ensure that the indices reflect reasonable combinations
            return std::is_permutation(vPlain.begin(), vPlain.end(), this->begin(), this->end());
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
        /// \param nVal int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setIndex(size_t nthIndex, int nVal)
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
        /// \param nVal int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void push_back(int nVal)
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
        /// \param vVector const std::vector<int>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void append(const std::vector<int>& vVector)
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
        /// \param vVector const std::vector<int>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void prepend(const std::vector<int>& vVector)
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
        /// \brief Apply an additional offset to all
        /// index values within this vector.
        ///
        /// \param offset int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void apply_offset(int offset)
        {
            if (expand)
            {
                vStorage.front() = vStorage.front() > INVALID ? vStorage.front()+offset : vStorage.front();
                vStorage.back() = vStorage.back() > INVALID ? vStorage.back()+offset : vStorage.back();
            }
            else
            {
                for (size_t i = 0; i < vStorage.size(); i++)
                {
                    if (vStorage[i] > INVALID)
                        vStorage[i] += offset;
                }
            }
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a STL
        /// vector, which will resemble the indices
        /// stored internally. This includes that the
        /// single indices are expanded in the returned
        /// vector.
        ///
        /// \return std::vector<int>
        ///
        /////////////////////////////////////////////////
        std::vector<int> getVector() const
        {
            // Expand the single indices stored internally
            // if needed
            if (expand)
            {
                std::vector<int> vReturn;

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
        /// \return int
        ///
        /////////////////////////////////////////////////
        int max() const
        {
            if (isOpenEnd())
                return OPEN_END;
            else if (expand)
                return std::max(vStorage.front(), vStorage.back());

            return *std::max_element(vStorage.begin(), vStorage.end());
        }

        /////////////////////////////////////////////////
        /// \brief This member function calculates the
        /// minimal index value obtained from the values
        /// stored internally.
        ///
        /// \return int
        ///
        /////////////////////////////////////////////////
        int min() const
        {
            if (expand && isOpenEnd())
                return vStorage.front();
            else if (expand)
                return std::min(vStorage.front(), vStorage.back());

            int nMin = vStorage.front();

            for (size_t i = 1; i < vStorage.size(); i++)
            {
                if (nMin < 0 || (vStorage[i] > 0 && vStorage[i] < nMin))
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
            return vStorage.front() != INVALID || vStorage.back() != INVALID;
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
        /// whether the managed range contains the full
        /// range of the target data set.
        ///
        /// \param max_range int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isFullRange(int max_range) const
        {
            return expand && vStorage.front() == 0 && (vStorage.back() == OPEN_END || vStorage.back() >= max_range);
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
        /// \return int&
        ///
        /////////////////////////////////////////////////
        int& front()
        {
            return vStorage.front();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a
        /// reference to the final index value stored
        /// internally.
        ///
        /// \return int&
        ///
        /////////////////////////////////////////////////
        int& back()
        {
            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// reference to the first index value stored
        /// internally.
        ///
        /// \return const int&
        ///
        /////////////////////////////////////////////////
        const int& front() const
        {
            return vStorage.front();
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a const
        /// reference to the final index value stored
        /// internally.
        ///
        /// \return const int&
        ///
        /////////////////////////////////////////////////
        const int& back() const
        {
            return vStorage.back();
        }

        /////////////////////////////////////////////////
        /// \brief Return an iterator to the start of the
        /// sequence.
        ///
        /// \return ForwardIterator
        ///
        /////////////////////////////////////////////////
        ForwardIterator begin() const
        {
            return ForwardIterator(this);
        }

        /////////////////////////////////////////////////
        /// \brief Return an iterator to the end of the
        /// sequence.
        ///
        /// \return ForwardIterator
        ///
        /////////////////////////////////////////////////
        ForwardIterator end() const
        {
            return ForwardIterator(this, size());
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the last
        /// index value, which can be reached by the
        /// values stored internally (this is most
        /// probably different from the final value).
        ///
        /// \return int
        ///
        /////////////////////////////////////////////////
        int last() const
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
        /// \param nMin int
        /// \param nMax int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setRange(int nMin, int nMax)
        {
            // Change the order of the minimal and
            // maximal value, if needed
            if (nMin > nMax)
            {
                int nTemp = nMin;
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

        /////////////////////////////////////////////////
        /// \brief Check, whether all values of the
        /// indices are in a range, which can be handled
        /// by all algorithms.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool checkRange() const
        {
            // Expanded ones need only two indices checked
            if (expand)
                return (vStorage.front() == STRING || vStorage.front() >= INVALID)
                    && vStorage.back() >= OPEN_END;

            // All others need explicit checks
            for (size_t i = 0; i+1 < vStorage.size(); i++)
            {
                if (vStorage[i] < INVALID && i+1 < vStorage.size())
                    return false;
            }

            // The last one can be an open end one
            return vStorage.back() >= OPEN_END;
        }

        /////////////////////////////////////////////////
        /// \brief This member function can be used to
        /// replace the open end state with a defined
        /// index value although the VectorIndex instance
        /// was passed as const ref.
        ///
        /// \param nLast int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setOpenEndIndex(int nLast) const
        {
            if (vStorage.back() == OPEN_END)
                vStorage.back() = nLast;
        }

        /////////////////////////////////////////////////
        /// \brief This member function converts the
        /// vector indexes contents into a human-readable
        /// string representation.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string to_string() const
        {
            if (expand)
            {
                std::string sVect;

                if (vStorage.front() == INVALID)
                    sVect += "INVALID";
                else if (vStorage.front() == STRING)
                    sVect += "#";
                else
                    sVect += toString(vStorage.front()+1);

                // Do not present the second index if
                // it corresponds to an invalid value
                if (vStorage.back() == OPEN_END)
                    sVect += ":inf";
                else if (vStorage.back() == STRING)
                    sVect += ":#";
                else if (vStorage.back() != INVALID)
                    sVect += ":"+toString(vStorage.back()+1);

                return sVect;
            }
            else
            {
                std::string sVect = "{";

                for (size_t i = 0; i < size(); i++)
                {
                    // Jump over the middle section,
                    // if the vector length is larger
                    // than 5
                    if (size() >= 5u && i == 2)
                    {
                        sVect += "...,";
                        i = size() - 2;
                    }

                    if (getIndex(i) == INVALID)
                        sVect += "INVALID";
                    else if (getIndex(i) == OPEN_END)
                        sVect += "inf";
                    else if (getIndex(i) == STRING)
                        sVect += "#";
                    else
                        sVect += toString(getIndex(i)+1);

                    if (i + 1 < size())
                        sVect += ",";
                }

                return sVect + "}";
            }
        }
};


/////////////////////////////////////////////////
/// \brief This class extends the std::vector for
/// endlessness.
///
/// This class template automatically creates
/// empty elements, if the index operator access
/// elements beyond its size.
/////////////////////////////////////////////////
template<class T>
class EndlessVector : public std::vector<T>
{
    private:
        T m_fallback;
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor
        /////////////////////////////////////////////////
        EndlessVector() : std::vector<T>(), m_fallback() {}

        /////////////////////////////////////////////////
        /// \brief Copy constructor from same.
        ///
        /// \param vec const EndlessVector&
        ///
        /////////////////////////////////////////////////
        EndlessVector(const EndlessVector& vec) : std::vector<T>(vec), m_fallback() {}

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
            std::vector<T>::operator=(vec);
            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Access operator overload. Will return
        /// default constructed instances of the template
        /// type T, if the position n points to a
        /// position beyond the internal array.
        ///
        /// \param n size_t
        /// \return T&
        ///
        /////////////////////////////////////////////////
        T& operator[](size_t n)
        {
            if (n < std::vector<T>::size())
                return std::vector<T>::operator[](n);

            return m_fallback;
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
    std::string sCompiledAccessEquation;

    Indices() { }
    Indices(const Indices& _idx) : row(_idx.row), col(_idx.col), sCompiledAccessEquation(_idx.sCompiledAccessEquation)
    {
    }
    Indices(Indices&& _idx)
    {
        std::swap(_idx.row, row);
        std::swap(_idx.col, col);
        std::swap(_idx.sCompiledAccessEquation, sCompiledAccessEquation);
    }
    Indices& operator=(const Indices& _idx)
    {
        row = _idx.row;
        col = _idx.col;
        sCompiledAccessEquation = _idx.sCompiledAccessEquation;
        return *this;
    }
};


/////////////////////////////////////////////////
/// \brief Structure for the findCommand
/// function.
/////////////////////////////////////////////////
struct Match
{
    std::string sString;
    size_t nPos;
};


/////////////////////////////////////////////////
/// \brief This represents a point in 2D space.
/////////////////////////////////////////////////
struct Point
{
    double x;
    double y;

    Point(double _x, double _y) : x(_x), y(_y) {}

    void rotate90(int n, const Point& origin = Point(0, 0))
    {
        x -= origin.x;
        y -= origin.y;

        double x1 = x;
        double y1 = y;

        // Remove multiples of 360 degrees
        n = n % 4;

        // Move negative angles to the corresponding
        // positive value
        if (n < 0)
            n += 4;

        // We only have to consider the values 1-3
        switch (n)
        {
            case 1:
                x1 = -y;
                y1 = x;
                break;
            case 2:
                x1 = -x;
                y1 = -y;
                break;
            case 3:
                x1 = y;
                y1 = -x;
                break;
        }

        x = x1 + origin.x;
        y = y1 + origin.y;
    }

    void rotate(double dAlpha, const Point& origin = Point(0, 0))
    {
        // Use the 90 degree rotation optimisation
        if (abs(dAlpha / M_PI_2 - rint(dAlpha / M_PI_2)) < 1e-8)
        {
            rotate90(rint(dAlpha / M_PI_2), origin);
            return;
        }

        double sina = sin(dAlpha);
        double cosa = cos(dAlpha);

        double x1 = (x - origin.x) * cosa - (y - origin.y) * sina + origin.x;
        double y1 = (x - origin.x) * sina + (y - origin.y) * cosa + origin.y;

        x = x1;
        y = y1;
    }

    Point operator+(const Point& a) const
    {
        return Point(x + a.x, y + a.y);
    }

    Point operator-(const Point& a) const
    {
        return Point(x - a.x, y - a.y);
    }

    Point operator*(double a) const
    {
        return Point(x*a, y*a);
    }

    Point operator/(double a) const
    {
        return Point(x/a, y/a);
    }
};


/////////////////////////////////////////////////
/// \brief Structure as wrapper for the return
/// value of procedures (which may be numerical
/// or string values or a mixture of both).
/////////////////////////////////////////////////
struct Returnvalue
{
    std::vector<mu::Array> valArray;
    std::string sReturnedTable;
    bool delayDelete;
    Indices sourceIdx;

    Returnvalue() : delayDelete(false) {}

    // clear method
    void clear()
    {
        valArray.clear();
        sReturnedTable.clear();
        delayDelete = false;
    }
};


/////////////////////////////////////////////////
/// \brief Structure for the four standard
/// variables.
/////////////////////////////////////////////////
struct DefaultVariables
{
    std::string sName[4] = {"x", "y", "z", "t"};
    mu::Variable vValue[4][4];
};


/////////////////////////////////////////////////
/// \brief Structure for the sorting
/// functionality: used for the recursive
/// definition of the index columns for sorting.
/////////////////////////////////////////////////
struct ColumnKeys
{
    VectorIndex cols;
    // Contains a recursive pointer
    ColumnKeys* subkeys;

    // Default constructor
    ColumnKeys() : cols(), subkeys(nullptr) {}
    // Destructor recursively deletes the stored pointers
    ~ColumnKeys()
        {
            if (subkeys)
                delete subkeys;
        }
};


/////////////////////////////////////////////////
/// \brief This structure contains the
/// information of a two-dimensional boundary.
/////////////////////////////////////////////////
struct Boundary
{
    long long int n;
    long long int m;
    size_t rows;
    size_t cols;

    Boundary(long long int i, long long int j, size_t _row, size_t _col) : n(i), m(j), rows(_row), cols(_col) {}

    long long int rf()
    {
        return n;
    }

    long long int re()
    {
        return n+rows;
    }

    long long int cf()
    {
        return m;
    }

    long long int ce()
    {
        return m+cols;
    }
};

#endif
