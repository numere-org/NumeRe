/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include <memory>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>

#include "memory.hpp"
#include "tablecolumnimpl.hpp"
#include "../../kernel.hpp"
#include "../io/file.hpp"
#include "../ui/error.hpp"
#include "../settings.hpp"
#include "../utils/tools.hpp"
#include "../version.h"
#include "../maths/resampler.h"
#include "../maths/statslogic.hpp"
#include "../maths/matdatastructures.hpp"

#define MAX_TABLE_SIZE 1e8
#define MAX_TABLE_COLS 1e4
#define DEFAULT_COL_TYPE ValueColumn


using namespace std;



/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
Memory::Memory()
{
    nCalcLines = -1;
    bSaveMutex = false;
    m_meta.save();
}


/////////////////////////////////////////////////
/// \brief Specialized constructor to allocate a
/// defined table size.
///
/// \param _nCols size_t
///
/////////////////////////////////////////////////
Memory::Memory(size_t _nCols) : Memory()
{
    Allocate(_nCols);
}


/////////////////////////////////////////////////
/// \brief Memory class destructor, which will
/// free the allocated memory.
/////////////////////////////////////////////////
Memory::~Memory()
{
    clear();
}


/////////////////////////////////////////////////
/// \brief This member function is the Memory
/// class allocator. It will handle all memory
/// allocations.
///
/// \param _nNCols size_t
/// \param shrink bool
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::Allocate(size_t _nNCols, bool shrink)
{
    if (_nNCols > MAX_TABLE_COLS)
        throw SyntaxError(SyntaxError::TOO_LARGE_CACHE, "", SyntaxError::invalid_position);

    // We simply resize the number of columns. Note, that
    // this only affects the column count. The column themselves
    // are not automatically allocated
    memArray.resize(std::max(_nNCols, memArray.size()));

    if (shrink)
    {
        // Iterate through the columns
        for (TblColPtr& col : memArray)
        {
            // If a column exist, call the shrink method
            if (col)
                col->shrink();
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// column headlines, if they are empty.
///
/// \return void
///
/////////////////////////////////////////////////
void Memory::createTableHeaders()
{
    for (size_t j = 0; j < memArray.size(); j++)
    {
        if (!memArray[j])
            memArray[j].reset(new DEFAULT_COL_TYPE);

        if (!memArray[j]->m_sHeadLine.length())
            memArray[j]->m_sHeadLine = TableColumn::getDefaultColumnHead(j);
    }
}


/////////////////////////////////////////////////
/// \brief This member function frees the
/// internally used memory block completely.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::clear()
{
    memArray.clear();

    nCalcLines = -1;
    m_meta.modify();
    bSaveMutex = false;

    return true;
}


/////////////////////////////////////////////////
/// \brief Assignment operator.
///
/// \param other const Memory&
/// \return Memory&
///
/////////////////////////////////////////////////
Memory& Memory::operator=(const Memory& other)
{
    clear();

    memArray.resize(other.memArray.size());

    #pragma omp parallel for
    for (size_t i = 0; i < other.memArray.size(); i++)
    {
        if (!other.memArray[i])
            continue;

        switch (other.memArray[i]->m_type)
        {
            case TableColumn::TYPE_VALUE:
                memArray[i].reset(new ValueColumn);
                break;
            case TableColumn::TYPE_DATETIME:
                memArray[i].reset(new DateTimeColumn);
                break;
            case TableColumn::TYPE_STRING:
                memArray[i].reset(new StringColumn);
                break;

            // These labels are only for getting warnings
            // if new column types are added
            case TableColumn::TYPE_NONE:
            case TableColumn::VALUELIKE:
            case TableColumn::STRINGLIKE:
            case TableColumn::TYPE_MIXED:
                break;
        }

        memArray[i]->assign(other.memArray[i].get());
    }

    m_meta = other.m_meta;
    nCalcLines = other.nCalcLines;
    m_meta.modify();

    return *this;
}


/////////////////////////////////////////////////
/// \brief This member function will handle all
/// memory grow operations by doubling the base
/// size, which shall be incremented, as long as
/// it is smaller than the requested size.
///
/// \param _nLines size_t
/// \param _nCols size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::resizeMemory(size_t _nLines, size_t _nCols)
{
    if (!Allocate(_nCols))
        return false;

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of columns, which are currently
/// available in this table.
///
/// \param _bFull bool true, if the reserved
/// number of columns is requested, false if only
/// the non-empty ones are requested
/// \return int
///
/////////////////////////////////////////////////
int Memory::getCols(bool _bFull) const
{
    return memArray.size();
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of lines, which are currently
/// available in this table.
///
/// \param _bFull bool true, if the reserved
/// number of lines is requested, false if only
/// the non-empty ones are requested
/// \return int
///
/////////////////////////////////////////////////
int Memory::getLines(bool _bFull) const
{
    if (memArray.size())
    {
        if (nCalcLines != -1)
            return nCalcLines;

        size_t nReturn = 0;

        for (const TblColPtr& col : memArray)
        {
            if (col && col->size() > nReturn)
                nReturn = col->size();
        }

        // Cache the number of lines until invalidation
        // for faster access
        nCalcLines = nReturn;

        return nReturn;
    }

    return 0;
}


/////////////////////////////////////////////////
/// \brief Returns the number of elements in the
/// selected column (but might contain invalid
/// values).
///
/// \param col size_t
/// \return int
///
/////////////////////////////////////////////////
int Memory::getElemsInColumn(size_t col) const
{
    if (memArray.size() > col && memArray[col])
        return memArray[col]->size();

    return 0;
}


/////////////////////////////////////////////////
/// \brief Returns the overall used number of
/// bytes for this table.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t Memory::getSize() const
{
    size_t bytes = 0;

    for (const TblColPtr& col : memArray)
    {
        if (col)
            bytes += col->getBytes();
    }

    return bytes + m_meta.comment.length();
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// element stored at the selected position.
///
/// \param _nLine size_t
/// \param _nCol size_t
/// \return double
///
/////////////////////////////////////////////////
mu::value_type Memory::readMem(size_t _nLine, size_t _nCol) const
{
    if (memArray.size() > _nCol && memArray[_nCol])
        return memArray[_nCol]->getValue(_nLine);

    return NAN;
}


/////////////////////////////////////////////////
/// \brief This static helper function calculates
/// the average value respecting NaNs.
///
/// \param values const std::vector<mu::value_type>&
/// \return mu::value_type
///
/////////////////////////////////////////////////
static mu::value_type nanAvg(const std::vector<mu::value_type>& values)
{
    mu::value_type sum = 0.0;
    double c = 0.0;

    for (mu::value_type val : values)
    {
        if (!mu::isnan(val))
        {
            sum += val;
            c++;
        }
    }

    if (c)
        return sum / c;

    return sum;
}


/////////////////////////////////////////////////
/// \brief This member function returns a
/// (bilinearily) interpolated element at the
/// selected \c double positions.
///
/// \param _dLine double
/// \param _dCol double
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::readMemInterpolated(double _dLine, double _dCol) const
{
    if (isnan(_dLine) || isnan(_dCol))
        return NAN;

    // Find the base index
    int nBaseLine = intCast(_dLine) + (_dLine < 0 ? -1 : 0);
    int nBaseCol = intCast(_dCol) + (_dCol < 0 ? -1 : 0);

    // Get the decimal part of the double indices
    double x = _dLine - nBaseLine;
    double y = _dCol - nBaseCol;

    // Find the surrounding four entries
    mu::value_type f00 = readMem(nBaseLine, nBaseCol);
    mu::value_type f10 = readMem(nBaseLine+1, nBaseCol);
    mu::value_type f01 = readMem(nBaseLine, nBaseCol+1);
    mu::value_type f11 = readMem(nBaseLine+1, nBaseCol+1);

    // If all are NAN, return NAN
    if (mu::isnan(f00) && mu::isnan(f01) && mu::isnan(f10) && mu::isnan(f11))
        return NAN;

    // Get the average respecting NaNs
    mu::value_type dNanAvg = nanAvg({f00, f01, f10, f11});

    // Otherwise set NAN to zero
    f00 = mu::isnan(f00) ? dNanAvg : f00;
    f10 = mu::isnan(f10) ? dNanAvg : f10;
    f01 = mu::isnan(f01) ? dNanAvg : f01;
    f11 = mu::isnan(f11) ? dNanAvg : f11;

    //     f(0,0) (1-x) (1-y) + f(1,0) x (1-y) + f(0,1) (1-x) y + f(1,1) x y
    return f00*(1.0-x)*(1.0-y)    + f10*x*(1.0-y)    + f01*(1.0-x)*y    + f11*x*y;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// elements stored at the selected positions.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> Memory::readMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    std::vector<mu::value_type> vReturn;

    if ((_vLine.size() > 1 && _vCol.size() > 1) || !memArray.size())
        vReturn.push_back(NAN);
    else
    {
        vReturn.resize(_vLine.size()*_vCol.size(), NAN);

        //#pragma omp parallel for
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (_vLine[i] < 0)
                    continue;

                if (_vLine[i] >= elems)
                {
                    if (_vLine.isExpanded() && _vLine.isOrdered())
                        break;

                    continue;
                }

                vReturn[j + i * _vCol.size()] = memArray[_vCol[j]]->getValue(_vLine[i]);
            }
        }
    }

    return vReturn;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// elements stored at the selected positions as
/// a Matrix.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return Matrix
///
/////////////////////////////////////////////////
Matrix Memory::readMemAsMatrix(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return Matrix(1, 1);

    Matrix mat(_vLine.size(), _vCol.size());

    for (size_t j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (size_t i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0)
                continue;

            if (_vLine[i] >= elems)
            {
                if (_vLine.isExpanded() && _vLine.isOrdered())
                    break;

                continue;
            }

            mat(i, j) = memArray[_vCol[j]]->getValue(_vLine[i]);
        }
    }

    return mat;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// elements stored at the selected positions.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return ValueVector
///
/////////////////////////////////////////////////
ValueVector Memory::readMixedMem(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    ValueVector vReturn;

    if ((_vLine.size() > 1 && _vCol.size() > 1) || !memArray.size())
        vReturn.push_back("");
    else
    {
#warning TODO (numere#1#06/08/22): Added escaped quotation marks to actually enable reading empty entries
        vReturn.resize(_vLine.size()*_vCol.size(), "\"\"");

        //#pragma omp parallel for
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (_vLine[i] < 0)
                    continue;

                if (_vLine[i] >= elems)
                {
                    if (_vLine.isExpanded() && _vLine.isOrdered())
                        break;

                    continue;
                }

                vReturn[j + i * _vCol.size()] = memArray[_vCol[j]]->getValueAsParserString(_vLine[i]);
            }
        }
    }

    return vReturn;
}


/////////////////////////////////////////////////
/// \brief Returns the "common" type of the
/// selected columns.
///
/// \param _vCol const VectorIndex&
/// \return TableColumn::ColumnType
///
/////////////////////////////////////////////////
TableColumn::ColumnType Memory::getType(const VectorIndex& _vCol) const
{
    TableColumn::ColumnType type = TableColumn::TYPE_NONE;

    for (size_t i = 0; i < _vCol.size(); i++)
    {
        if (_vCol[i] >= 0 && (int)memArray.size() > _vCol[i] && memArray[_vCol[i]])
        {
            if (type == TableColumn::TYPE_NONE)
                type = memArray[_vCol[i]]->m_type;
            else if (type != memArray[_vCol[i]]->m_type
                     && (type > TableColumn::STRINGLIKE || memArray[_vCol[i]]->m_type > TableColumn::STRINGLIKE))
                return TableColumn::TYPE_MIXED;
        }
    }

    return type;
}


/////////////////////////////////////////////////
/// \brief This member function extracts a range
/// of this table and returns it as a new Memory
/// instance.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return Memory*
///
/// \remark The caller gets ownership of the
/// returned Memory instance.
///
/////////////////////////////////////////////////
Memory* Memory::extractRange(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    Memory* _memCopy = new Memory();

    _vLine.setOpenEndIndex(getLines(false)-1);
    _vCol.setOpenEndIndex(getCols(false)-1);

    _memCopy->Allocate(_vCol.size());

    if (_vCol.size() * _vLine.size() > 10000)
    {
        #pragma omp parallel for
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] >= 0 && _vCol[j] < (int)memArray.size() && memArray[_vCol[j]])
                _memCopy->memArray[j].reset(memArray[_vCol[j]]->copy(_vLine));
        }
    }
    else
    {
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] >= 0 && _vCol[j] < (int)memArray.size() && memArray[_vCol[j]])
                _memCopy->memArray[j].reset(memArray[_vCol[j]]->copy(_vLine));
        }
    }

    _memCopy->m_meta = m_meta;
    return _memCopy;
}


/////////////////////////////////////////////////
/// \brief This member function will copy the
/// selected elements into the passed vector
/// instance. This member function avoids copies
/// of the vector instance by directly writing to
/// the target instance.
///
/// \param vTarget vector<mu::value_type>*
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return void
///
/////////////////////////////////////////////////
void Memory::copyElementsInto(vector<mu::value_type>* vTarget, const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if ((_vLine.size() > 1 && _vCol.size() > 1) || !memArray.size())
        vTarget->assign(1, NAN);
    else
    {
        vTarget->assign(_vLine.size()*_vCol.size(), NAN);

        //#pragma omp parallel for
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (_vLine[i] < 0)
                    continue;

                if (_vLine[i] >= elems)
                {
                    if (_vLine.isExpanded() && _vLine.isOrdered())
                        break;

                    continue;
                }

                (*vTarget)[j + i * _vCol.size()] = memArray[_vCol[j]]->getValue(_vLine[i]);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Returns true, if the element at the
/// selected positions is valid. Only checks
/// internally, if the value is not a NaN value.
///
/// \param _nLine size_t
/// \param _nCol size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::isValidElement(size_t _nLine, size_t _nCol) const
{
    if (_nCol < memArray.size() && _nCol >= 0 && memArray[_nCol])
        return !mu::isnan(memArray[_nCol]->getValue(_nLine));

    return false;
}


/////////////////////////////////////////////////
/// \brief Returns true, if at least a single
/// valid value is available in this table.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::isValid() const
{
    return getLines();
}


/////////////////////////////////////////////////
/// \brief This member function shrinks the table
/// memory to the smallest possible dimensions
/// reachable in powers of two.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::shrink()
{
    if (!memArray.size())
        return true;

    // Shrink each column
    for (TblColPtr& col : memArray)
    {
        if (col)
            col->shrink();

        if (col && !col->size())
            col.reset(nullptr);
    }

    nCalcLines = -1;

    // Remove obsolete columns
    for (int i = memArray.size()-1; i >= 0; i--)
    {
        if (memArray[i])
        {
            memArray.resize(i+1);
            return true;
        }
    }

    // if this place is reached, delete everything
    memArray.clear();
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function tries to convert
/// all string columns to value columns, if it
/// is possible.
///
/// \return void
///
/////////////////////////////////////////////////
void Memory::convert()
{
    #pragma omp parallel for
    for (size_t i = 0; i < memArray.size(); i++)
    {
        if (memArray[i] && memArray[i]->m_type == TableColumn::TYPE_STRING)
        {
            TableColumn* col = memArray[i]->convert();

            // Only valid conversions return a non-zero
            // pointer
            if (col && col != memArray[i].get())
                memArray[i].reset(col);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function tries to convert
/// the selected columns to the target column
/// type, if it is possible.
///
/// \param _vCol const VectorIndex&
/// \param _sType const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::convertColumns(const VectorIndex& _vCol, const std::string& _sType)
{
    if (_sType != "value" && _sType != "string" && _sType != "datetime")
        return false;

    TableColumn::ColumnType _type = TableColumn::TYPE_NONE;

    if (_sType == "value")
        _type = TableColumn::TYPE_VALUE;
    else if (_sType == "string")
        _type = TableColumn::TYPE_STRING;
    else if (_sType == "datetime")
        _type = TableColumn::TYPE_DATETIME;

    _vCol.setOpenEndIndex(memArray.size()-1);

    bool success = true;

    for (size_t i = 0; i < _vCol.size(); i++)
    {
        if (_vCol[i] < 0 || _vCol[i] >= (int)memArray.size())
            continue;

        if (memArray[_vCol[i]] && memArray[_vCol[i]]->m_type != _type)
        {
            TableColumn* col = memArray[_vCol[i]]->convert(_type);

            // Only valid conversions return a non-zero
            // pointer
            if (col && col != memArray[_vCol[i]].get())
                memArray[_vCol[i]].reset(col);
            else
                success = false;
        }
    }

    return success;
}


/////////////////////////////////////////////////
/// \brief Returns, whether the contents of the
/// current table are already saved into either
/// a usual file or into the cache file.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::getSaveStatus() const
{
    return m_meta.isSaved;
}


/////////////////////////////////////////////////
/// \brief Returns the table column headline for
/// the selected column. Will return a default
/// headline, if the column is empty or does not
/// exist.
///
/// \param _i size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string Memory::getHeadLineElement(size_t _i) const
{
    if (_i >= memArray.size() || !memArray[_i])
        return TableColumn::getDefaultColumnHead(_i+1);
    else
        return memArray[_i]->m_sHeadLine;
}


/////////////////////////////////////////////////
/// \brief Returns the table column headlines for
/// the selected columns. Will return default
/// headlines for empty or non-existing columns.
///
/// \param _vCol const VectorIndex&
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Memory::getHeadLineElement(const VectorIndex& _vCol) const
{
    vector<string> vHeadLines;

    for (unsigned int i = 0; i < _vCol.size(); i++)
    {
        if (_vCol[i] < 0)
            continue;

        vHeadLines.push_back(getHeadLineElement(_vCol[i]));
    }

    return vHeadLines;
}


/////////////////////////////////////////////////
/// \brief Writes a new table column headline to
/// the selected column.
///
/// \param _i size_t
/// \param _sHead const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::setHeadLineElement(size_t _i, const std::string& _sHead)
{
    if (_i >= memArray.size())
    {
        if (!resizeMemory(1, _i + 1))
            return false;
    }

    if (!memArray[_i])
        memArray[_i].reset(new DEFAULT_COL_TYPE);

    memArray[_i]->m_sHeadLine = _sHead;
    m_meta.modify();

    return true;
}


/////////////////////////////////////////////////
/// \brief Update the comment associated with
/// this table.
///
/// \param comment const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeComment(const std::string& comment)
{
    m_meta.comment = comment;
    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief Update the internal meta data with the
/// passed one.
///
/// \param meta const NumeRe::TableMetaData&
/// \return void
///
/////////////////////////////////////////////////
void Memory::setMetaData(const NumeRe::TableMetaData& meta)
{
    m_meta = meta;
}


/////////////////////////////////////////////////
/// \brief Mark this table as modified.
///
/// \return void
///
/////////////////////////////////////////////////
void Memory::markModified()
{
    m_meta.modify();
    nCalcLines = -1;
}


/////////////////////////////////////////////////
/// \brief Returns the number of empty cells at
/// the end of the selected columns.
///
/// \param _i size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t Memory::getAppendedZeroes(size_t _i) const
{
    if (_i < memArray.size() && memArray[_i])
        return getLines() - memArray[_i]->size();

    return getLines();
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// number of lines needed for the table column
/// headline of the selected column.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t Memory::getHeadlineCount() const
{
    size_t nHeadlineCount = 1;

    // Get the dimensions of the complete headline (i.e. including possible linebreaks)
    for (const TblColPtr& col : memArray)
    {
        // No linebreak? Continue
        if (!col || col->m_sHeadLine.find('\n') == std::string::npos)
            continue;

        size_t nLinebreak = 0;

        // Count all linebreaks
        for (size_t n = 0; n < col->m_sHeadLine.length() - 2; n++)
        {
            if (col->m_sHeadLine[n] == '\n')
                nLinebreak++;
        }

        // Save the maximal number
        if (nLinebreak + 1 > nHeadlineCount)
            nHeadlineCount = nLinebreak + 1;
    }

    return nHeadlineCount;
}


/////////////////////////////////////////////////
/// \brief Return the comment associated with
/// this table.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Memory::getComment() const
{
    return m_meta.comment;
}


/////////////////////////////////////////////////
/// \brief Return the internal meta data
/// structure.
///
/// \return NumeRe::TableMetaData
///
/////////////////////////////////////////////////
NumeRe::TableMetaData Memory::getMetaData() const
{
    return m_meta;
}


/////////////////////////////////////////////////
/// \brief This member function writes the passed
/// value to the selected position. The table is
/// automatically enlarged, if necessary.
///
/// \param _nLine int
/// \param _nCol int
/// \param _dData const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeData(int _nLine, int _nCol, const mu::value_type& _dData)
{
    if (!memArray.size() && mu::isnan(_dData))
        return;

    if ((int)memArray.size() <= _nCol)
        resizeMemory(_nLine+1, _nCol+1);

    convert_if_empty(memArray[_nCol], _nCol, TableColumn::TYPE_VALUE);
    memArray[_nCol]->setValue(_nLine, _dData);

    if (nCalcLines != -1 && (mu::isnan(_dData) || _nLine >= nCalcLines))
        nCalcLines = -1;

    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function provides an
/// unsafe but direct way of writing data to the
/// table. It will not check for the existence of
/// the needed amount of columns.
///
/// \param _nLine int
/// \param _nCol int
/// \param _dData const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeDataDirect(int _nLine, int _nCol, const mu::value_type& _dData)
{
    convert_if_empty(memArray[_nCol], _nCol, TableColumn::TYPE_VALUE);
    memArray[_nCol]->setValue(_nLine, _dData);
}


/////////////////////////////////////////////////
/// \brief This member function provides an even
/// more unsafe but direct way of writing data to
/// the table. It will neither check for
/// existence of the internal pointer nor for the
/// existence of the needed amount of columns.
/// Use this only, if real pre-allocation is
/// possible.
///
/// \param _nLine int
/// \param _nCol int
/// \param _dData const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeDataDirectUnsafe(int _nLine, int _nCol, const mu::value_type& _dData)
{
    memArray[_nCol]->setValue(_nLine, _dData);
}


/////////////////////////////////////////////////
/// \brief Writes string data to the internal
/// table.
///
/// \param _nLine int
/// \param _nCol int
/// \param sValue const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeData(int _nLine, int _nCol, const std::string& sValue)
{
    if (!memArray.size() && !sValue.length())
        return;

    if ((int)memArray.size() <= _nCol)
        resizeMemory(_nLine+1, _nCol+1);

    convert_if_empty(memArray[_nCol], _nCol, TableColumn::TYPE_STRING);
    memArray[_nCol]->setValue(_nLine, sValue);

    if (!sValue.length() || _nLine >= nCalcLines)
        nCalcLines = -1;

    // --> Setze den Zeitstempel auf "jetzt", wenn der Memory eben noch gespeichert war <--
    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function writes a whole
/// array of data to the selected table range.
/// The table is automatically enlarged, if
/// necessary.
///
/// \param _idx Indices&
/// \param _dData mu::value_type*
/// \param _nNum unsigned int
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeData(Indices& _idx, mu::value_type* _dData, unsigned int _nNum)
{
    int nDirection = LINES;

    if (_nNum == 1)
    {
        writeSingletonData(_idx, _dData[0]);
        return;
    }

    bool rewriteColumn = false;

    if (_idx.row.front() == 0 && _idx.row.isOpenEnd())
        rewriteColumn = true;

    _idx.row.setOpenEndIndex(_idx.row.front() + _nNum - 1);
    _idx.col.setOpenEndIndex(_idx.col.front() + _nNum - 1);

    if (_idx.row.size() > 1)
        nDirection = COLS;
    else if (_idx.col.size() > 1)
        nDirection = LINES;

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j < _idx.col.size(); j++)
        {
            if (!i && _idx.col[j] >= (int)memArray.size())
                resizeMemory(i, _idx.col[j]+1);

            if (!i)
                convert_if_empty(memArray[_idx.col[j]], _idx.col[j], TableColumn::TYPE_VALUE);

            if (nDirection == COLS)
            {
                if (!i && rewriteColumn)
                    convert_for_overwrite(memArray[_idx.col[j]], _idx.col[j], TableColumn::TYPE_VALUE);

                if (_nNum > i)
                {
                    memArray[_idx.col[j]]->setValue(_idx.row[i], _dData[i]);

                    if (nCalcLines != -1 && (nCalcLines <= _idx.row[i] || mu::isnan(_dData[i])))
                        nCalcLines = -1;
                }
            }
            else
            {
                if (_nNum > j)
                {
                    memArray[_idx.col[j]]->setValue(_idx.row[i], _dData[j]);

                    if (nCalcLines != -1 && (nCalcLines <= _idx.row[i] || mu::isnan(_dData[j])))
                        nCalcLines = -1;
                }
            }
        }
    }

    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function writes multiple
/// copies of a single value to a range in the
/// table. The table is automatically enlarged,
/// if necessary.
///
/// \param _idx Indices&
/// \param _dData const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeSingletonData(Indices& _idx, const mu::value_type& _dData)
{
    bool rewriteColumn = false;

    if (_idx.row.front() == 0 && _idx.row.isOpenEnd())
        rewriteColumn = true;

    _idx.row.setOpenEndIndex(std::max(_idx.row.front(), getLines(false)) - 1);
    _idx.col.setOpenEndIndex(std::max(_idx.col.front(), getCols(false)) - 1);

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j < _idx.col.size(); j++)
        {
            if (!i && rewriteColumn && (int)memArray.size() > _idx.col[j])
                convert_for_overwrite(memArray[_idx.col[j]], _idx.col[j], TableColumn::TYPE_VALUE);

            writeData(_idx.row[i], _idx.col[j], _dData);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function writes a whole
/// array of values to the selected table range.
/// The table is automatically enlarged, if
/// necessary.
///
/// \param _idx Indices&
/// \param _values const ValueVector&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeData(Indices& _idx, const ValueVector& _values)
{
    int nDirection = LINES;

    if (_values.size() == 1)
    {
        writeSingletonData(_idx, _values.front());
        return;
    }

    bool rewriteColumn = false;

    if (_idx.row.front() == 0 && _idx.row.isOpenEnd())
        rewriteColumn = true;

    _idx.row.setOpenEndIndex(_idx.row.front() + _values.size() - 1);
    _idx.col.setOpenEndIndex(_idx.col.front() + _values.size() - 1);

    if (_idx.row.size() > 1)
        nDirection = COLS;
    else if (_idx.col.size() > 1)
        nDirection = LINES;

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j < _idx.col.size(); j++)
        {
            if (nDirection == COLS)
            {
                if (!i && rewriteColumn && (int)memArray.size() > _idx.col[j])
                    convert_for_overwrite(memArray[_idx.col[j]], _idx.col[j], TableColumn::TYPE_STRING);

                if (_values.size() > i)
                    writeData(_idx.row[i], _idx.col[j], _values[i]);
            }
            else
            {
                if (_values.size() > j)
                    writeData(_idx.row[i], _idx.col[j], _values[j]);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function writes multiple
/// copies of a single string to a range in the
/// table. The table is automatically enlarged,
/// if necessary.
///
/// \param _idx Indices&
/// \param _sValue const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Memory::writeSingletonData(Indices& _idx, const std::string& _sValue)
{
    bool rewriteColumn = false;

    if (_idx.row.front() == 0 && _idx.row.isOpenEnd())
        rewriteColumn = true;

    _idx.row.setOpenEndIndex(std::max(_idx.row.front(), getLines(false)) - 1);
    _idx.col.setOpenEndIndex(std::max(_idx.col.front(), getCols(false)) - 1);

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j < _idx.col.size(); j++)
        {
            if (!i && rewriteColumn && (int)memArray.size() > _idx.col[j])
                convert_for_overwrite(memArray[_idx.col[j]], _idx.col[j], TableColumn::TYPE_STRING);

            writeData(_idx.row[i], _idx.col[j], _sValue);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function changes the saved
/// state to the passed value.
///
/// \param _bIsSaved bool
/// \return void
///
/////////////////////////////////////////////////
void Memory::setSaveStatus(bool _bIsSaved)
{
    if (_bIsSaved)
        m_meta.save();
    else
        m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function returns the time-
/// point, where the table was saved last time.
///
/// \return long long int
///
/////////////////////////////////////////////////
long long int Memory::getLastSaved() const
{
    return m_meta.lastSavedTime;
}


/////////////////////////////////////////////////
/// \brief This member function is the interface
/// function for the Sorter class. It will pre-
/// evaluate the passed parameters and redirect
/// the control to the corresponding sorting
/// function.
///
/// \param i1 int
/// \param i2 int
/// \param j1 int
/// \param j2 int
/// \param sSortingExpression const std::string&
/// \return vector<int>
///
/////////////////////////////////////////////////
vector<int> Memory::sortElements(int i1, int i2, int j1, int j2, const std::string& sSortingExpression)
{
    if (!memArray.size())
        return vector<int>();

    bool bError = false;
    bool bReturnIndex = false;
    bSortCaseInsensitive = findParameter(sSortingExpression, "ignorecase");
    int nSign = 1;

    i1 = std::max(0, i1);
    j1 = std::max(0, j1);

    vector<int> vIndex;

    // Determine the sorting direction
    if (findParameter(sSortingExpression, "desc"))
        nSign = -1;

    if (i2 == -1)
        i2 = i1;

    if (j2 == -1)
        j2 = j1;

    // Prepare the sorting index
    for (int i = i1; i <= i2; i++)
        vIndex.push_back(i);

    // Evaluate, whether an index shall be returned
    // (instead of actual reordering the columns)
    if (findParameter(sSortingExpression, "index"))
        bReturnIndex = true;

    // Is a column group selected or do we actually
    // sort everything?
    if (!findParameter(sSortingExpression, "cols", '=') && !findParameter(sSortingExpression, "c", '='))
    {
        // Make a copy of the global index for the private threads
        std::vector<int> vPrivateIndex = vIndex;

        // Sort everything independently (we use vIndex from
        // the outside, we therefore must declare it as firstprivate)
        #pragma omp parallel for firstprivate(vPrivateIndex)
        for (int i = j1; i <= j2; i++)
        {
            // Change for OpenMP
            if (i > j1 && bReturnIndex)
                continue;

            // Sort the current column
            if (!qSort(&vPrivateIndex[0], i2 - i1 + 1, i, 0, i2 - i1, nSign))
                throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);

            // Abort after the first column, if
            // an index shall be returned
            // Continue is a change for OpenMP
            if (bReturnIndex)
            {
                vIndex = vPrivateIndex;
                continue;
            }

            // Actually reorder the column
            reorderColumn(vPrivateIndex, i1, i2, i);

            // Reset the sorting index
            for (int j = i1; j <= i2; j++)
                vPrivateIndex[j-i1] = j;
        }
    }
    else
    {
        // Sort groups of columns (including
        // hierarchical sorting)
        string sCols = "";

        // Find the column group definition
        if (findParameter(sSortingExpression, "cols", '='))
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "cols", '=') + 4);
        else
            sCols = getArgAtPos(sSortingExpression, findParameter(sSortingExpression, "c", '=') + 1);

        // As long as the column group definition
        // has a length
        while (sCols.length())
        {
            // Get a new column keys instance
            ColumnKeys* keys = evaluateKeyList(sCols, j2 - j1 + 1);

            // Ensure that we obtained an actual
            // instance
            if (!keys)
                throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE,  sSortingExpression, SyntaxError::invalid_position);

            if (keys->nKey[1] == -1)
                keys->nKey[1] = keys->nKey[0] + 1;

            // Go through the group definition
            for (int j = keys->nKey[0]; j < keys->nKey[1]; j++)
            {
                // Sort the current key list level
                // independently
                if (!qSort(&vIndex[0], i2 - i1 + 1, j + j1, 0, i2 - i1, nSign))
                {
                    delete keys;
                    throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
                }

                // Subkey list: sort the subordinate group
                // depending on the higher-level key group
                if (keys->subkeys && keys->subkeys->subkeys)
                {
                    if (!sortSubList(&vIndex[0], i2 - i1 + 1, keys, i1, i2, j1, nSign, getCols(false)))
                    {
                        delete keys;
                        throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, sSortingExpression, SyntaxError::invalid_position);
                    }
                }

                // Break, if the index shall be returned
                if (bReturnIndex)
                    break;

                // Actually reorder the current column
                reorderColumn(vIndex, i1, i2, j + j1);

                // Obtain the subkey list
                ColumnKeys* subKeyList = keys->subkeys;

                // As long as a subkey list is available
                while (subKeyList)
                {
                    if (subKeyList->nKey[1] == -1)
                        subKeyList->nKey[1] = subKeyList->nKey[0] + 1;

                    // Reorder the subordinate key list
                    for (int _j = subKeyList->nKey[0]; _j < subKeyList->nKey[1]; _j++)
                        reorderColumn(vIndex, i1, i2, _j + j1);

                    // Find the next subordinate list
                    subKeyList = subKeyList->subkeys;
                }

                // Reset the sorting index for the next column
                for (int _j = i1; _j <= i2; _j++)
                    vIndex[_j-i1] = _j;
            }

            // Free the occupied memory
            delete keys;

            if (bReturnIndex)
                break;
        }
    }

    // Number of lines might have changed
    nCalcLines = -1;

    // Increment each index value, if the index
    // vector shall be returned
    if (bReturnIndex)
    {
        for (int i = 0; i <= i2 - i1; i++)
            vIndex[i]++;
    }

    m_meta.modify();

    if (bError || !bReturnIndex)
        return vector<int>();

    return vIndex;
}


/////////////////////////////////////////////////
/// \brief This member function simply reorders
/// the contents of the selected column using the
/// passed index vector.
///
/// \param vIndex const VectorIndex&
/// \param i1 int
/// \param i2 int
/// \param j1 int
/// \return void
///
/////////////////////////////////////////////////
void Memory::reorderColumn(const VectorIndex& vIndex, int i1, int i2, int j1)
{
    if ((int)memArray.size() > j1 && memArray[j1])
    {
        TblColPtr col(memArray[j1]->copy(vIndex));
        memArray[j1]->insert(VectorIndex(i1, i2), col.get());
        memArray[j1]->shrink();
    }
}


/////////////////////////////////////////////////
/// \brief Override for the virtual Sorter class
/// member function. Returns 0, if both elements
/// are equal, -1 if element i is smaller than
/// element j and 1 otherwise.
///
/// \param i int
/// \param j int
/// \param col int
/// \return int
///
/////////////////////////////////////////////////
int Memory::compare(int i, int j, int col)
{
    if (col < (int)memArray.size() && memArray[col])
        return memArray[col]->compare(i, j, bSortCaseInsensitive);

    return 0;
}


/////////////////////////////////////////////////
/// \brief Override for the virtual Sorter class
/// member function. Returns true, if the
/// selected element is a valid value.
///
/// \param line int
/// \param col int
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::isValue(int line, int col)
{
    if (col < (int)memArray.size() && memArray[col])
        return memArray[col]->isValid(line);

    return false;
}


/////////////////////////////////////////////////
/// \brief Create a copy-efficient table object
/// from the data contents.
///
/// \param _sTable const string&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table Memory::extractTable(const string& _sTable, const VectorIndex& lines, const VectorIndex& cols)
{
    lines.setOpenEndIndex(getLines(false)-1);
    cols.setOpenEndIndex(getCols(false)-1);

    NumeRe::Table table(lines.size(), cols.size());

    table.setName(_sTable);
    table.setMetaData(m_meta);

    #pragma omp parallel for
    for (size_t j = 0; j < cols.size(); j++)
    {
        if (cols[j] < (int)memArray.size() && memArray[cols[j]])
            table.setColumn(j, memArray[cols[j]]->copy(lines));
    }

    return table;
}


/////////////////////////////////////////////////
/// \brief Import data from a copy-efficient
/// table object. Completely replaces the
/// contents, which were in the internal storage
/// before.
///
/// \param _table NumeRe::Table
/// \return void
///
/////////////////////////////////////////////////
void Memory::importTable(NumeRe::Table _table, const VectorIndex& lines, const VectorIndex& cols)
{
    // We construct separate objects because they might be overwritten
    deleteBulk(VectorIndex(lines), VectorIndex(cols));

    lines.setOpenEndIndex(lines.front() + _table.getLines()-1);
    cols.setOpenEndIndex(cols.front() + _table.getCols()-1);

    resizeMemory(lines.max()+1, cols.max()+1);
    m_meta = _table.getMetaData();

    #pragma omp parallel for
    for (size_t j = 0; j < _table.getCols(); j++)
    {
        if (j >= cols.size())
            continue;

        TableColumn* tabCol = _table.getColumn(j);

        if (!tabCol)
            continue;

        if (!memArray[cols[j]])
        {
            if (tabCol->m_type == TableColumn::TYPE_VALUE)
                memArray[cols[j]].reset(new ValueColumn);
            else if (tabCol->m_type == TableColumn::TYPE_DATETIME)
                memArray[cols[j]].reset(new DateTimeColumn);
            else if (tabCol->m_type == TableColumn::TYPE_STRING)
                memArray[cols[j]].reset(new StringColumn);
        }

        memArray[cols[j]]->insert(lines, tabCol);
        memArray[cols[j]]->m_sHeadLine = tabCol->m_sHeadLine;
    }

    // Try to convert string- to valuecolumns
    convert();
    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function is used for
/// saving the contents of this memory page into
/// a file. The type of the file is selected by
/// the name of the file.
///
/// \param _sFileName string
/// \param sTableName const string&
/// \param nPrecision unsigned short
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::save(string _sFileName, const string& sTableName, unsigned short nPrecision)
{
    // Get an instance of the desired file type
    NumeRe::GenericFile* file = NumeRe::getFileByType(_sFileName);

    // Ensure that a file was created
    if (!file)
        throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, _sFileName, SyntaxError::invalid_position, _sFileName);

    int lines = getLines(false);
    int cols = getCols(false);

    // Set the dimensions and the generic information
    // in the file
    file->setDimensions(lines, cols);
    file->setData(&memArray, lines, cols);
    file->setTableName(sTableName);
    file->setTextfilePrecision(nPrecision);

    // If the file type is a NumeRe data file, then
    // we can also set the comment associated with
    // this memory page
    if (file->getExtension() == "ndat")
        static_cast<NumeRe::NumeReDataFile*>(file)->setComment(m_meta.comment);

    // Try to write the data to the file. This might
    // either result in writing errors or the write
    // function is not defined for this file type
    try
    {
        if (!file->write())
            throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, _sFileName, SyntaxError::invalid_position, _sFileName);
    }
    catch (...)
    {
        delete file;
        throw;
    }

    // Delete the created file instance
    delete file;

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function deletes a single
/// entry from the memory table.
///
/// \param _nLine int
/// \param _nCol int
/// \return void
///
/////////////////////////////////////////////////
void Memory::deleteEntry(int _nLine, int _nCol)
{
    if ((int)memArray.size() > _nCol && memArray[_nCol])
    {
        if (memArray[_nCol]->isValid(_nLine))
        {
            // Delete the element
            memArray[_nCol]->deleteElements(VectorIndex(_nLine));
            m_meta.modify();

            // Evaluate, whether we can remove
            // the column from memory
            if (!_nLine && !memArray[_nCol]->size())
                memArray[_nCol].reset(nullptr);

            nCalcLines = -1;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function deletes a whole
/// range of entries from the memory table.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return void
///
/////////////////////////////////////////////////
void Memory::deleteBulk(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    if (!memArray.size())
        return;

    _vLine.setOpenEndIndex(getLines()-1);
    _vCol.setOpenEndIndex(getCols()-1);

    bool bHasFirstLine = _vLine.min() == 0;

    // Delete the selected entries
    #pragma omp parallel for
    for (size_t j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] >= 0 && _vCol[j] < (int)memArray.size() && memArray[_vCol[j]])
            memArray[_vCol[j]]->deleteElements(_vLine);
    }

    m_meta.modify();

    // Remove all invalid elements and columns
    if (bHasFirstLine)
        shrink();

    nCalcLines = -1;
}


/////////////////////////////////////////////////
/// \brief Driver code for simplifying the
/// calculation of various stats using OpenMP, if
/// possible.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param operation std::vector<StatsLogic>&
/// \return void
///
/////////////////////////////////////////////////
void Memory::calculateStats(const VectorIndex& _vLine, const VectorIndex& _vCol, std::vector<StatsLogic>& operation) const
{
    constexpr size_t MINTHREADCOUNT = 16;
    constexpr size_t MINELEMENTPERCOL = 1000;

    // Only apply multiprocessing, if there are really a lot of
    // elements to process
    if (operation.size() >= MINTHREADCOUNT && _vLine.size() >= MINELEMENTPERCOL)
    {
        #pragma omp parallel for
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (_vLine[i] < 0)
                    continue;

                if (_vLine[i] >= elems)
                {
                    if (_vLine.isExpanded() && _vLine.isOrdered())
                        break;

                    continue;
                }

                operation[j](readMem(_vLine[i], _vCol[j]));
            }
        }
    }
    else
    {
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (_vLine[i] < 0)
                    continue;

                if (_vLine[i] >= elems)
                {
                    if (_vLine.isExpanded() && _vLine.isOrdered())
                        break;

                    continue;
                }

                operation[j](readMem(_vLine[i], _vCol[j]));
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Implementation for the STD multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::std(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    mu::value_type dAvg = avg(_vLine, _vCol);
    mu::value_type dStd = 0.0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADDSQSUB, 0.0, dAvg));
    calculateStats(_vLine, _vCol, vLogic);

    for (const auto& val : vLogic)
        dStd += val.m_val;

    return sqrt(dStd / (num(_vLine, _vCol) - 1.0));
}


/////////////////////////////////////////////////
/// \brief Implementation for the AVG multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::avg(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    return sum(_vLine, _vCol) / num(_vLine, _vCol);
}


/////////////////////////////////////////////////
/// \brief Implementation for the MAX multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::max(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    double dMax = NAN;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_MAX, NAN));
    calculateStats(_vLine, _vCol, vLogic);

    for (const auto& val : vLogic)
    {
        if (isnan(dMax) || dMax < val.m_val.real())
            dMax = val.m_val.real();
    }

    return dMax;
}


/////////////////////////////////////////////////
/// \brief Implementation for the MIN multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::min(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    double dMin = NAN;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_MIN, NAN));
    calculateStats(_vLine, _vCol, vLogic);

    for (const auto& val : vLogic)
    {
        if (isnan(dMin) || dMin > val.m_val.real())
            dMin = val.m_val.real();
    }

    return dMin;
}


/////////////////////////////////////////////////
/// \brief Implementation for the PRD multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::prd(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    mu::value_type dPrd = 1.0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_MULT, 1.0));
    calculateStats(_vLine, _vCol, vLogic);

    for (const auto& val : vLogic)
    {
        dPrd *= val.m_val;
    }

    return dPrd;
}


/////////////////////////////////////////////////
/// \brief Implementation for the SUM multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::sum(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    mu::value_type dSum = 0.0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADD));
    calculateStats(_vLine, _vCol, vLogic);

    for (const auto& val : vLogic)
    {
        dSum += val.m_val;
    }

    return dSum;
}


/////////////////////////////////////////////////
/// \brief Implementation for the NUM multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::num(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return 0;

    int nInvalid = 0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    for (unsigned int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
        {
            nInvalid += _vLine.size();
            continue;
        }

        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= elems || mu::isnan(readMem(_vLine[i], _vCol[j])))
                nInvalid++;
        }
    }

    return (_vLine.size() * _vCol.size()) - nInvalid;
}


/////////////////////////////////////////////////
/// \brief Implementation for the AND multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::and_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return 0.0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    double dRetVal = NAN;

    for (unsigned int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0)
                continue;

            if (_vLine[i] >= elems)
            {
                if (_vLine.isExpanded() && _vLine.isOrdered())
                    break;

                continue;
            }

            if (isnan(dRetVal))
                dRetVal = 1.0;

            if (!memArray[j] || !memArray[j]->asBool(i))
                return 0.0;
        }
    }

    if (isnan(dRetVal))
        return 0.0;

    return 1.0;
}


/////////////////////////////////////////////////
/// \brief Implementation for the OR multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::or_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return 0.0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    for (unsigned int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0)
                continue;

            if (_vLine[i] >= elems)
            {
                if (_vLine.isExpanded() && _vLine.isOrdered())
                    break;

                continue;
            }

            if (memArray[j] && memArray[j]->asBool(i))
                return 1.0;
        }
    }

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief Implementation for the XOR multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::xor_func(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return 0.0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    bool isTrue = false;

    for (unsigned int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0)
                continue;

            if (_vLine[i] >= elems)
            {
                if (_vLine.isExpanded() && _vLine.isOrdered())
                    break;

                continue;
            }

            if (memArray[j] && memArray[j]->asBool(i))
            {
                if (!isTrue)
                    isTrue = true;
                else
                    return 0.0;
            }
        }
    }

    if (isTrue)
        return 1.0;

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief Implementation for the CNT multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::cnt(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return 0;

    int nInvalid = 0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    for (unsigned int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0 || _vLine[i] >= elems)
                nInvalid++;
        }
    }

    return (_vLine.size() * _vCol.size()) - nInvalid;
}


/////////////////////////////////////////////////
/// \brief Implementation for the NORM multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::norm(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    mu::value_type dNorm = 0.0;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADDSQ));
    calculateStats(_vLine, _vCol, vLogic);

    for (const auto& val : vLogic)
    {
        dNorm += val.m_val;
    }

    return sqrt(dNorm);
}


/////////////////////////////////////////////////
/// \brief Implementation for the CMP multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param dRef mu::value_type
/// \param _nType int
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::cmp(const VectorIndex& _vLine, const VectorIndex& _vCol, mu::value_type dRef, int _nType) const
{
    if (!memArray.size())
        return NAN;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    enum
    {
        RETURN_VALUE = 1,
        RETURN_LE = 2,
        RETURN_GE = 4,
        RETURN_FIRST = 8
    };

    int nType = 0;

    double dKeep = dRef.real();
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

    for (long long int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (long long int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0)
                continue;

            if (_vLine[i] >= elems)
            {
                if (_vLine.isExpanded() && _vLine.isOrdered())
                    break;

                continue;
            }

            mu::value_type val = readMem(_vLine[i], _vCol[j]);

            if (mu::isnan(val))
                continue;

            if (val == dRef)
            {
                if (nType & RETURN_VALUE)
                    return val;

                if (_vLine[0] == _vLine[_vLine.size() - 1])
                    return _vCol[j] + 1;

                return _vLine[i] + 1;
            }
            else if (nType & RETURN_GE && val.real() > dRef.real())
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return val.real();

                    if (_vLine[0] == _vLine[_vLine.size() - 1])
                        return _vCol[j] + 1;

                    return _vLine[i] + 1;
                }

                if (nKeep == -1 || val.real() < dKeep)
                {
                    dKeep = val.real();
                    if (_vLine[0] == _vLine[_vLine.size() - 1])
                        nKeep = _vCol[j];
                    else
                        nKeep = _vLine[i];
                }
                else
                    continue;
            }
            else if (nType & RETURN_LE && val.real() < dRef.real())
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return val.real();

                    if (_vLine[0] == _vLine[_vLine.size() - 1])
                        return _vCol[j] + 1;

                    return _vLine[i] + 1;
                }

                if (nKeep == -1 || val.real() > dKeep)
                {
                    dKeep = val.real();
                    if (_vLine[0] == _vLine[_vLine.size() - 1])
                        nKeep = _vCol[j];
                    else
                        nKeep = _vLine[i];
                }
                else
                    continue;
            }
        }
    }

    if (nKeep == -1)
        return NAN;
    else if (nType & RETURN_VALUE)
        return dKeep;
    else
        return nKeep + 1;
}


/////////////////////////////////////////////////
/// \brief Implementation for the MED multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::med(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    if (!memArray.size())
        return NAN;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    vector<double> vData;

    vData.reserve(_vLine.size()*_vCol.size());

    for (unsigned int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0)
                continue;

            if (_vLine[i] >= elems)
            {
                if (_vLine.isExpanded() && _vLine.isOrdered())
                    break;

                continue;
            }

            mu::value_type val = readMem(_vLine[i], _vCol[j]);

            if (!mu::isnan(val))
                vData.push_back(val.real());
        }
    }

    if (!vData.size())
        return NAN;

    size_t nCount = qSortDouble(&vData[0], vData.size());

    if (!nCount)
        return NAN;

    return gsl_stats_median_from_sorted_data(&vData[0], 1, nCount);
}


/////////////////////////////////////////////////
/// \brief Implementation for the PCT multi
/// argument function.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param dPct mu::value_type
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type Memory::pct(const VectorIndex& _vLine, const VectorIndex& _vCol, mu::value_type dPct) const
{
    if (!memArray.size())
        return NAN;

    int lines = getLines(false);
    int cols = getCols(false);

    _vLine.setOpenEndIndex(lines-1);
    _vCol.setOpenEndIndex(cols-1);

    vector<double> vData;

    vData.reserve(_vLine.size()*_vCol.size());

    if (dPct.real() >= 1 || dPct.real() <= 0)
        return NAN;

    for (unsigned int j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j]);

        if (!elems)
            continue;

        for (unsigned int i = 0; i < _vLine.size(); i++)
        {
            if (_vLine[i] < 0)
                continue;

            if (_vLine[i] >= elems)
            {
                if (_vLine.isExpanded() && _vLine.isOrdered())
                    break;

                continue;
            }

            mu::value_type val = readMem(_vLine[i], _vCol[j]);

            if (!mu::isnan(val))
                vData.push_back(val.real());
        }
    }

    if (!vData.size())
        return NAN;


    size_t nCount = qSortDouble(&vData[0], vData.size());

    if (!nCount)
        return NAN;

    return gsl_stats_quantile_from_sorted_data(&vData[0], 1, nCount, dPct.real());
}


/////////////////////////////////////////////////
/// \brief Implementation of the SIZE multi
/// argument function.
///
/// \param _vIndex const VectorIndex&
/// \param dir int Bitcomposition of AppDir values
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> Memory::size(const VectorIndex& _vIndex, int dir) const
{
    if (!memArray.size())
        return std::vector<mu::value_type>(2, 0.0);

    int lines = getLines(false);
    int cols = getCols(false);

    _vIndex.setOpenEndIndex(dir & LINES ? lines-1 : cols-1);
    int nGridOffset = 2*((dir & GRID) != 0);

    // Handle simple things first
    if (dir == ALL)
        return std::vector<mu::value_type>({lines, cols});
    else if (dir == GRID)
        return std::vector<mu::value_type>({getElemsInColumn(0), getElemsInColumn(1)});
    else if (dir & LINES)
    {
        // Compute the sizes of the table rows
        std::vector<mu::value_type> vSizes;

        for (size_t i = 0; i < _vIndex.size(); i++)
        {
            if (_vIndex[i] < 0 || _vIndex[i] >= lines)
                continue;

            for (int j = memArray.size()-1; j >= 0; j--)
            {
                if (memArray[j] && memArray[j]->isValid(_vIndex[i]))
                {
                    vSizes.push_back(j+1 - nGridOffset);
                    break;
                }
            }
        }

        if (!vSizes.size())
            vSizes.push_back(NAN);

        return vSizes;
    }
    else if (dir & COLS)
    {
        // Compute the sizes of the table columns
        std::vector<mu::value_type> vSizes;

        for (size_t j = 0; j < _vIndex.size(); j++)
        {
            if (_vIndex[j] < nGridOffset || _vIndex[j] >= cols)
                continue;

            vSizes.push_back(getElemsInColumn(_vIndex[j]));
        }

        if (!vSizes.size())
            vSizes.push_back(NAN);

        return vSizes;
    }

    return std::vector<mu::value_type>(2, 0.0);
}


/////////////////////////////////////////////////
/// \brief Implementation of the MINPOS multi
/// argument function.
///
/// \param _vIndex const VectorIndex&
/// \param dir int
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> Memory::minpos(const VectorIndex& _vIndex, int dir) const
{
    if (!memArray.size())
        return std::vector<mu::value_type>(1, NAN);

    int lines = getLines(false);
    int cols = getCols(false);

    _vIndex.setOpenEndIndex(dir & COLS ? cols-1 : lines-1);
    int nGridOffset = 2*((dir & GRID) != 0);

    // If a grid is required, get the grid dimensions
    // of this table
    if (nGridOffset)
    {
        std::vector<mu::value_type> vSize = size(VectorIndex(), GRID);
        lines = vSize.front().real();
        cols = vSize.back().real()+nGridOffset; // compensate the offset
    }

    // A special case for the columns. We will compute the
    // results for ALL and GRID using the results for LINES
    if (dir & COLS)
    {
        std::vector<mu::value_type> vPos;

        for (size_t j = 0; j < _vIndex.size(); j++)
        {
            if (_vIndex[j] < nGridOffset || _vIndex[j] >= cols)
                continue;

            vPos.push_back(cmp(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(_vIndex[j]), min(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(_vIndex[j])), 0));
        }

        if (!vPos.size())
            vPos.push_back(NAN);

        return vPos;
    }

    std::vector<mu::value_type> vPos;
    double dMin = NAN;
    size_t pos = 0;

    // Compute the results for LINES and find as
    // well the global minimal value, which will be used
    // for GRID and ALL
    for (size_t i = 0; i < _vIndex.size(); i++)
    {
        if (_vIndex[i] < 0 || _vIndex[i] >= lines)
            continue;

        vPos.push_back(cmp(VectorIndex(_vIndex[i]), VectorIndex(nGridOffset, VectorIndex::OPEN_END), min(VectorIndex(_vIndex[i]), VectorIndex(nGridOffset, VectorIndex::OPEN_END)), 0));

        if (isnan(dMin) || dMin > readMem(_vIndex[i], intCast(vPos.back())-1).real())
        {
            dMin = readMem(_vIndex[i], intCast(vPos.back())-1).real();
            pos = i;
        }
    }

    if (!vPos.size())
        return std::vector<mu::value_type>(1, NAN);

    // Use the global minimal value for ALL and GRID
    if (dir == ALL || dir == GRID)
        return std::vector<mu::value_type>({_vIndex[pos]+1, vPos[pos]});

    return vPos;
}


/////////////////////////////////////////////////
/// \brief Implementation of the MAXPOS multi
/// argument function.
///
/// \param _vIndex const VectorIndex&
/// \param dir int
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> Memory::maxpos(const VectorIndex& _vIndex, int dir) const
{
    if (!memArray.size())
        return std::vector<mu::value_type>(1, NAN);

    int lines = getLines(false);
    int cols = getCols(false);

    _vIndex.setOpenEndIndex(dir & COLS ? cols-1 : lines-1);
    int nGridOffset = 2*((dir & GRID) != 0);

    // If a grid is required, get the grid dimensions
    // of this table
    if (nGridOffset)
    {
        std::vector<mu::value_type> vSize = size(VectorIndex(), GRID);
        lines = vSize.front().real();
        cols = vSize.back().real()+nGridOffset; // compensate the offset
    }

    // A special case for the columns. We will compute the
    // results for ALL and GRID using the results for LINES
    if (dir & COLS)
    {
        std::vector<mu::value_type> vPos;

        for (size_t j = 0; j < _vIndex.size(); j++)
        {
            if (_vIndex[j] < nGridOffset || _vIndex[j] >= cols)
                continue;

            vPos.push_back(cmp(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(_vIndex[j]), max(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(_vIndex[j])), 0));
        }

        if (!vPos.size())
            vPos.push_back(NAN);

        return vPos;
    }

    std::vector<mu::value_type> vPos;
    double dMax = NAN;
    size_t pos;

    // Compute the results for LINES and find as
    // well the global maximal value, which will be used
    // for GRID and ALL
    for (size_t i = 0; i < _vIndex.size(); i++)
    {
        if (_vIndex[i] < 0 || _vIndex[i] >= lines)
            continue;

        vPos.push_back(cmp(VectorIndex(_vIndex[i]), VectorIndex(nGridOffset, VectorIndex::OPEN_END), max(VectorIndex(_vIndex[i]), VectorIndex(nGridOffset, VectorIndex::OPEN_END)), 0));

        if (isnan(dMax) || dMax < readMem(_vIndex[i], intCast(vPos.back())-1).real())
        {
            dMax = readMem(_vIndex[i], intCast(vPos.back())-1).real();
            pos = i;
        }
    }

    if (!vPos.size())
        return std::vector<mu::value_type>(1, NAN);

    // Use the global maximal value for ALL and GRID
    if (dir == ALL || dir == GRID)
        return std::vector<mu::value_type>({_vIndex[pos]+1, vPos[pos]});

    return vPos;
}


/////////////////////////////////////////////////
/// \brief This method is the retouching main
/// method. It will redirect the control into the
/// specialized member functions.
///
/// \param _vLine VectorIndex
/// \param _vCol VectorIndex
/// \param Direction AppDir
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::retouch(VectorIndex _vLine, VectorIndex _vCol, AppDir Direction)
{
    bool bUseAppendedZeroes = false;

    if (!memArray.size())
        return false;

    if (!_vLine.isValid() || !_vCol.isValid())
        return false;

    // Evaluate the indices
    if (_vLine.isOpenEnd())
        bUseAppendedZeroes = true;

    _vLine.setRange(0, getLines()-1);
    _vCol.setRange(0, getCols()-1);

    if ((Direction == ALL || Direction == GRID) && _vLine.size() < 4)
        Direction = LINES;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // Pre-evaluate the axis values in the GRID case
    if (Direction == GRID)
    {
        if (bUseAppendedZeroes)
        {
            if (!retouch(_vLine, VectorIndex(_vCol[0]), COLS) || !retouch(_vLine, VectorIndex(_vCol[1]), COLS))
                return false;
        }
        else
        {
            if (!retouch(_vLine, _vCol.subidx(0, 2), COLS))
                return false;
        }

        _vCol = _vCol.subidx(2);
    }

    // Redirect the control to the specialized member
    // functions
    if (Direction == ALL || Direction == GRID)
    {
        _vLine.linearize();
        _vCol.linearize();

        return retouch2D(_vLine, _vCol);
    }
    else
        return retouch1D(_vLine, _vCol, Direction);
}


/////////////////////////////////////////////////
/// \brief This member function retouches single
/// dimension data (along columns or rows).
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param Direction AppDir
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::retouch1D(const VectorIndex& _vLine, const VectorIndex& _vCol, AppDir Direction)
{
    bool markModified = false;

    if (Direction == LINES)
    {
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            for (size_t j = 0; j < _vCol.size(); j++)
            {
                if (mu::isnan(readMem(_vLine[i], _vCol[j])))
                {
                    for (size_t _j = j; _j < _vCol.size(); _j++)
                    {
                        if (!mu::isnan(readMem(_vLine[i], _vCol[_j])))
                        {
                            if (j)
                            {
                                for (size_t __j = j; __j < _j; __j++)
                                {
                                    writeData(_vLine[i],
                                              _vCol[__j],
                                              (readMem(_vLine[i], _vCol[_j]) - readMem(_vLine[i], _vCol[j-1])) / (double)(_j - j) * (double)(__j - j + 1) + readMem(_vLine[i], _vCol[j-1]));
                                }

                                markModified = true;
                                break;
                            }
                            else if (_j+1 < _vCol.size())
                            {
                                for (size_t __j = j; __j < _j; __j++)
                                {
                                    writeData(_vLine[i], _vCol[__j], readMem(_vLine[i], _vCol[_j]));
                                }

                                markModified = true;
                                break;
                            }
                        }

                        if (j && _j+1 == _vCol.size() && mu::isnan(readMem(_vLine[i], _vCol[_j])))
                        {
                            for (size_t __j = j; __j < _vCol.size(); __j++)
                            {
                                writeData(_vLine[i], _vCol[__j], readMem(_vLine[i], _vCol[j-1]));
                            }

                            markModified = true;
                        }
                    }
                }
            }
        }
    }
    else if (Direction == COLS)
    {
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            for (size_t i = 0; i < _vLine.size(); i++)
            {
                if (mu::isnan(readMem(_vLine[i], _vCol[j])))
                {
                    for (size_t _i = i; _i < _vLine.size(); _i++)
                    {
                        if (!mu::isnan(readMem(_vLine[_i], _vCol[j])))
                        {
                            if (i)
                            {
                                for (size_t __i = i; __i < _i; __i++)
                                {
                                    writeData(_vLine[__i],
                                              _vCol[j],
                                              (readMem(_vLine[_i], _vCol[j]) - readMem(_vLine[i-1], _vCol[j])) / (double)(_i - i) * (double)(__i - i + 1) + readMem(_vLine[i-1], _vCol[j]));
                                }

                                markModified = true;
                                break;
                            }
                            else if (_i+1 < _vLine.size())
                            {
                                for (size_t __i = i; __i < _i; __i++)
                                {
                                    writeData(_vLine[__i], _vCol[j], readMem(_vLine[_i], _vCol[j]));
                                }

                                markModified = true;
                                break;
                            }
                        }

                        if (i  && _i+1 == _vLine.size() && mu::isnan(readMem(_vLine[_i], _vCol[j])))
                        {
                            for (size_t __i = i; __i < _vLine.size(); __i++)
                            {
                                writeData(_vLine[__i], _vCol[j], readMem(_vLine[i-1], _vCol[j]));
                            }

                            markModified = true;
                        }
                    }
                }
            }
        }
    }

    if (markModified)
        m_meta.modify();

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function retouches two
/// dimensional data (using a specialized filter
/// class instance).
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::retouch2D(const VectorIndex& _vLine, const VectorIndex& _vCol)
{
    bool bMarkModified = false;

    for (long long int i = _vLine.front(); i <= _vLine.last(); i++)
    {
        for (long long int j = _vCol.front(); j <= _vCol.last(); j++)
        {
            if (mu::isnan(readMem(i, j)))
            {
                Boundary _boundary = findValidBoundary(_vLine, _vCol, i, j);
                NumeRe::RetouchRegion _region(_boundary.rows-1,
                                              _boundary.cols-1,
                                              med(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(_boundary.cf(), _boundary.ce())));

                long long int l,r,t,b;

                // Find the correct boundary to be used instead of the
                // one outside of the range (if one of the indices is on
                // any of the four boundaries
                l = _boundary.cf() < _vCol.front() ? _boundary.ce() : _boundary.cf();
                r = _boundary.ce() > _vCol.last() ? _boundary.cf() : _boundary.ce();
                t = _boundary.rf() < _vLine.front() ? _boundary.re() : _boundary.rf();
                b = _boundary.re() > _vLine.last() ? _boundary.rf() : _boundary.re();

                _region.setBoundaries(readMem(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(l)),
                                      readMem(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(r)),
                                      readMem(VectorIndex(t), VectorIndex(_boundary.cf(), _boundary.ce())),
                                      readMem(VectorIndex(b), VectorIndex(_boundary.cf(), _boundary.ce())));

                for (long long int _n = _boundary.rf()+1; _n < _boundary.re(); _n++)
                {
                    for (long long int _m = _boundary.cf()+1; _m < _boundary.ce(); _m++)
                    {
                        writeData(_n, _m,
                                  _region.retouch(_n - _boundary.rf() - 1,
                                                  _m - _boundary.cf() - 1,
                                                  readMem(_n, _m),
                                                  med(VectorIndex(_n-1, _n+1), VectorIndex(_m-1, _m+1))));
                    }
                }

                bMarkModified = true;
            }
        }
    }

    if (bMarkModified)
        m_meta.modify();

    return true;
}


/////////////////////////////////////////////////
/// \brief This method is a wrapper for
/// detecting, whether a row or column does only
/// contain valid values (no NaNs).
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::onlyValidValues(const VectorIndex& _vLine, const VectorIndex& _vCol) const
{
    return num(_vLine, _vCol) == cnt(_vLine, _vCol);
}


/////////////////////////////////////////////////
/// \brief This member function finds the
/// smallest possible boundary around a set of
/// invalid values to be used as boundary values
/// for retouching the values.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param i int
/// \param j int
/// \return RetouchBoundary
///
/////////////////////////////////////////////////
Boundary Memory::findValidBoundary(const VectorIndex& _vLine, const VectorIndex& _vCol, int i, int j) const
{
    Boundary _boundary(i-1, j-1, 2, 2);

    bool reEvaluateBoundaries = true;

    while (reEvaluateBoundaries)
    {
        reEvaluateBoundaries = false;

        if (!onlyValidValues(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(_boundary.cf())) && _boundary.cf() > _vCol.front())
        {
            _boundary.m--;
            _boundary.cols++;
            reEvaluateBoundaries = true;
        }

        if (!onlyValidValues(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(_boundary.ce())) && _boundary.ce() < _vCol.last())
        {
            _boundary.cols++;
            reEvaluateBoundaries = true;
        }

        if (!onlyValidValues(VectorIndex(_boundary.rf()), VectorIndex(_boundary.cf(), _boundary.ce())) && _boundary.rf() > _vLine.front())
        {
            _boundary.n--;
            _boundary.rows++;
            reEvaluateBoundaries = true;
        }

        if (!onlyValidValues(VectorIndex(_boundary.re()), VectorIndex(_boundary.cf(), _boundary.ce())) && _boundary.re() < _vLine.last())
        {
            _boundary.rows++;
            reEvaluateBoundaries = true;
        }
    }

    return _boundary;
}


/////////////////////////////////////////////////
/// \brief This private member function realizes
/// the application of a smoothing window to 1D
/// data sets.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param i size_t
/// \param j size_t
/// \param _filter NumeRe::Filter*
/// \param smoothLines bool
/// \return void
///
/////////////////////////////////////////////////
void Memory::smoothingWindow1D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter, bool smoothLines)
{
    auto sizes = _filter->getWindowSize();

    mu::value_type sum = 0.0;
    NumeRe::FilterBuffer& filterBuffer = _filter->getBuffer();

    // Apply the filter to the data
    for (size_t n = 0; n < sizes.first; n++)
    {
        if (!_filter->isConvolution())
            writeData(_vLine[i+n*(!smoothLines)], _vCol[j+n*smoothLines], _filter->apply(n, 0, readMem(_vLine[i+n*(!smoothLines)], _vCol[j+n*smoothLines])));
        else
            sum += _filter->apply(n, 0, readMem(_vLine[i+n*(!smoothLines)], _vCol[j+n*smoothLines]));
    }

    // If the filter is a convolution, store the new value here
    if (_filter->isConvolution())
        filterBuffer.push(sum);

    // If enough elements are stored in the buffer
    // remove the first one
    if (filterBuffer.size() > sizes.first/2)
    {
        // Writes the element to the first position of the window
        writeData(_vLine[i], _vCol[j], filterBuffer.front());
        filterBuffer.pop();
    }

    // Is this the last point? Then extract all remaining points from the
    // buffer
    if (smoothLines && _vCol.size()-sizes.first-1 == j)
    {
        while (!filterBuffer.empty())
        {
            j++;
            writeData(_vLine[i], _vCol[j], filterBuffer.front());
            filterBuffer.pop();
        }
    }
    else if (!smoothLines && _vLine.size()-sizes.first-1 == i)
    {
        while (!filterBuffer.empty())
        {
            i++;
            writeData(_vLine[i], _vCol[j], filterBuffer.front());
            filterBuffer.pop();
        }
    }
}


/////////////////////////////////////////////////
/// \brief This private member function realizes
/// the application of a smoothing window to 2D
/// data sets.
///
/// \param _vLine const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param i size_t
/// \param j size_t
/// \param _filter NumeRe::Filter*
/// \return void
///
/////////////////////////////////////////////////
void Memory::smoothingWindow2D(const VectorIndex& _vLine, const VectorIndex& _vCol, size_t i, size_t j, NumeRe::Filter* _filter)
{
    auto sizes = _filter->getWindowSize();
    NumeRe::FilterBuffer2D& filterBuffer = _filter->get2DBuffer();

    mu::value_type sum = 0.0;

    // Apply the filter to the data
    for (size_t n = 0; n < sizes.first; n++)
    {
        for (size_t m = 0; m < sizes.second; m++)
        {
            if (!_filter->isConvolution())
                writeData(_vLine[i+n], _vCol[j+m], _filter->apply(n, m, readMem(_vLine[i+n], _vCol[j+m])));
            else
                sum += _filter->apply(n, m, readMem(_vLine[i+n], _vCol[j+m]));
        }
    }

    // If the filter is a convolution, store the new value here
    if (_filter->isConvolution())
    {
        if (j == 1)
            filterBuffer.push(std::vector<mu::value_type>());

        filterBuffer.back().push_back(sum);
    }

    // If enough elements are stored in the buffer
    // remove the first row
    if (filterBuffer.size() > sizes.first/2+1)
    {
        // Write the finished row
        for (size_t k = 0; k < filterBuffer.front().size(); k++)
            writeData(_vLine[i-1], _vCol[k+sizes.second/2+1], filterBuffer.front()[k]);

        filterBuffer.pop();
    }

    // Is this the last point? Then extract all remaining points from the
    // buffer
    if (_vLine.size()-sizes.first-1 == i && _vCol.size()-sizes.second-1 == j)
    {
        while (!filterBuffer.empty())
        {

            for (size_t k = 0; k < filterBuffer.front().size(); k++)
                writeData(_vLine[i], _vCol[k+sizes.second/2+1], filterBuffer.front()[k]);

            i++;
            filterBuffer.pop();
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function smoothes the data
/// described by the passed VectorIndex indices
/// using the passed FilterSettings to construct
/// the corresponding filter.
///
/// \param _vLine VectorIndex
/// \param _vCol VectorIndex
/// \param _settings NumeRe::FilterSettings
/// \param Direction AppDir
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::smooth(VectorIndex _vLine, VectorIndex _vCol, NumeRe::FilterSettings _settings, AppDir Direction)
{
    bool bUseAppendedZeroes = false;

    // Avoid the border cases
    if (!memArray.size())
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, "smooth", SyntaxError::invalid_position);

    if (!_vLine.isValid() || !_vCol.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX, "smooth", SyntaxError::invalid_position, _vLine.to_string() + ", " + _vCol.to_string());

    // Evaluate the indices
    if (_vLine.isOpenEnd())
        bUseAppendedZeroes = true;

    // Force the index ranges
    _vLine.setRange(0, getLines()-1);
    _vCol.setRange(0, getCols()-1);

    // Change the predefined application directions, if it's needed
    if ((Direction == ALL || Direction == GRID) && _vLine.size() < 4)
        Direction = LINES;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // Check the order
    if ((_settings.row >= (size_t)getLines() && Direction == COLS) || (_settings.col >= (size_t)getCols() && Direction == LINES) || ((_settings.row >= (size_t)getLines() || _settings.col >= (size_t)getCols()) && (Direction == ALL || Direction == GRID)))
        throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, "smooth", SyntaxError::invalid_position);


    // If the application direction is equal to GRID, then the first two columns
    // should be evaluted separately, because they contain the axis values
    if (Direction == GRID)
    {
        // Will never return false
        if (bUseAppendedZeroes)
        {
            if (!smooth(_vLine, VectorIndex(_vCol[0]), _settings, COLS) || !smooth(_vLine, VectorIndex(_vCol[1]), _settings, COLS))
                return false;
        }
        else
        {
            if (!smooth(_vLine, _vCol.subidx(0, 2), _settings, COLS))
                return false;
        }

        _vCol = _vCol.subidx(2);
    }

    // The first job is to simply remove invalid values and then smooth the
    // framing points of the data section
    if (Direction == ALL || Direction == GRID)
    {
        // Retouch everything
        Memory::retouch(_vLine, _vCol, ALL);

        //Memory::smooth(_vLine, VectorIndex(_vCol.front()), _settings, COLS);
        //Memory::smooth(_vLine, VectorIndex(_vCol.last()), _settings, COLS);
        //Memory::smooth(VectorIndex(_vLine.front()), _vCol, _settings, LINES);
        //Memory::smooth(VectorIndex(_vLine.last()), _vCol, _settings, LINES);

        if (_settings.row == 1u && _settings.col != 1u)
            _settings.row = _settings.col;
        else if (_settings.row != 1u && _settings.col == 1u)
            _settings.col = _settings.row;
    }
    else
    {
        _settings.row = std::max(_settings.row, _settings.col);
        _settings.col = 1u;
    }

    if (isnan(_settings.alpha))
        _settings.alpha = 1.0;

    // Apply the actual smoothing of the data
    if (Direction == LINES)
    {
        // Create a filter from the filter settings
        std::unique_ptr<NumeRe::Filter> _filterPtr(NumeRe::createFilter(_settings));

        // Update the sizes, because they might be
        // altered by the filter constructor
        auto sizes = _filterPtr->getWindowSize();
        _settings.row = sizes.first;

        // Pad the beginning and the of the vector with multiple copies
        _vCol.prepend(vector<int>(_settings.row/2+1, _vCol.front()));
        _vCol.append(vector<int>(_settings.row/2+1, _vCol.last()));

        // Smooth the lines
        for (size_t i = 0; i < _vLine.size(); i++)
        {
            for (size_t j = 1; j < _vCol.size() - _settings.row; j++)
            {
                smoothingWindow1D(_vLine, _vCol, i, j, _filterPtr.get(), true);
            }
        }
    }
    else if (Direction == COLS)
    {
        // Create a filter from the settings
        std::unique_ptr<NumeRe::Filter> _filterPtr(NumeRe::createFilter(_settings));

        // Update the sizes, because they might be
        // altered by the filter constructor
        auto sizes = _filterPtr->getWindowSize();
        _settings.row = sizes.first;

        // Pad the beginning and end of the vector with multiple copies
        _vLine.prepend(vector<int>(_settings.row/2+1, _vLine.front()));
        _vLine.append(vector<int>(_settings.row/2+1, _vLine.last()));

        // Smooth the columns
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            for (size_t i = 1; i < _vLine.size() - _settings.row; i++)
            {
                smoothingWindow1D(_vLine, _vCol, i, j, _filterPtr.get(), false);
            }
        }
    }
    else if ((Direction == ALL || Direction == GRID) && _vLine.size() > 2 && _vCol.size() > 2)
    {
        // Create a filter from the settings
        std::unique_ptr<NumeRe::Filter> _filterPtr(NumeRe::createFilter(_settings));

        // Update the sizes, because they might be
        // altered by the filter constructor
        auto sizes = _filterPtr.get()->getWindowSize();
        _settings.row = sizes.first;
        _settings.col = sizes.second;

        // Pad the beginning and end of both vectors
        // with a mirrored copy of themselves
        std::vector<int> vMirror = _vLine.subidx(1, _settings.row/2+1).getVector();
        _vLine.prepend(vector<int>(vMirror.rbegin(), vMirror.rend()));

        vMirror = _vLine.subidx(_vLine.size() - _settings.row/2-2, _settings.row/2+1).getVector();
        _vLine.append(vector<int>(vMirror.rbegin(), vMirror.rend()));

        vMirror = _vCol.subidx(1, _settings.col/2+1).getVector();
        _vCol.prepend(vector<int>(vMirror.rbegin(), vMirror.rend()));

        vMirror = _vCol.subidx(_vCol.size() - _settings.col/2-2, _settings.row/2+1).getVector();
        _vCol.append(vector<int>(vMirror.rbegin(), vMirror.rend()));

        // Smooth the data in two dimensions, if that is reasonable
        // Go through every point
        for (size_t i = 1; i < _vLine.size() - _settings.row; i++)
        {
            for (size_t j = 1; j < _vCol.size() - _settings.col; j++)
            {
                smoothingWindow2D(_vLine, _vCol, i, j, _filterPtr.get());
            }
        }
    }

    m_meta.modify();
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function resamples the
/// data described by the passed coordinates
/// using the new samples nSamples.
///
/// \param _vLine VectorIndex
/// \param _vCol VectorIndex
/// \param samples std::pair<size_t,size_t>
/// \param Direction AppDir
/// \param sFilter std::string sFilter
/// \return bool
///
/////////////////////////////////////////////////
bool Memory::resample(VectorIndex _vLine, VectorIndex _vCol, std::pair<size_t,size_t> samples, AppDir Direction, std::string sFilter)
{
    bool bUseAppendedZeroes = false;

    int nLinesToInsert = 0;
    int nColsToInsert = 0;

    static std::vector<std::string> vFilters({"box", "tent", "bell", "bspline", "mitchell", "lanczos3", "blackman",
                                             "lanczos4", "lanczos6", "lanczos12", "kaiser", "gaussian", "catmullrom",
                                             "quadratic_interp", "quadratic_approx", "quadratic_mix"});

    if (std::find(vFilters.begin(), vFilters.end(), sFilter) == vFilters.end())
        sFilter = "lanczos3";

    // Avoid border cases
    if (!memArray.size())
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, "resample", SyntaxError::invalid_position);

    if (!samples.first || !samples.second)
        throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);

    if (!_vLine.isValid() || !_vCol.isValid())
        throw SyntaxError(SyntaxError::INVALID_INDEX, "resample", SyntaxError::invalid_position, _vLine.to_string() + ", " + _vCol.to_string());

    // Evaluate the indices
    if (_vCol.isOpenEnd())
        bUseAppendedZeroes = true;

    _vLine.setRange(0, getLines()-1);
    _vLine.linearize();
    _vCol.setRange(0, getCols()-1);

    // Change the predefined application directions, if it's needed
    if ((Direction == ALL || Direction == GRID) && _vLine.size() < 4)
        Direction = LINES;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // If the application direction is equal to GRID, then the indices should
    // match a sufficiently enough large data array
    if (Direction == GRID)
    {
        if (_vCol.size() - 2 != _vLine.size() && !bUseAppendedZeroes)
            throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
        else if ((!memArray[1] || _vCol.size() - 2 != memArray[1]->size() - _vLine.front()) && bUseAppendedZeroes)
            throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
    }

    // Prepare a pointer to the resampler object
    std::unique_ptr<Resampler> _resampler;

    // Create the actual resample object based upon the application direction.
    // Additionally determine the size of the resampling buffer, which might
    // be larger than the current data set
    if (Direction == ALL || Direction == GRID) // 2D
    {
        if (Direction == GRID)
        {
            // Apply the resampling to the first two columns first:
            // These contain the axis values
            resample(_vLine, VectorIndex(_vCol[0]), samples, COLS);
            resample(_vLine, VectorIndex(_vCol[1]), std::make_pair(samples.second, samples.first), COLS);

            // Increment the first column
            _vCol = _vCol.subidx(2);
            _vCol.linearize();

            // Determine the size of the buffer
            if (samples.first > _vLine.size())
                nLinesToInsert = samples.first - _vLine.size();

            if (samples.second > _vCol.size())
                nColsToInsert = samples.second - _vCol.size();
        }

        // Create the resample object and prepare the needed memory
        _resampler.reset(new Resampler(_vCol.size(), _vLine.size(),
                                       samples.second, samples.first,
                                       Resampler::BOUNDARY_CLAMP, 1.0, 0.0, sFilter.c_str()));
    }
    else if (Direction == COLS) // cols
    {
        _vCol.linearize();

        // Create the resample object and prepare the needed memory
        _resampler.reset(new Resampler(_vCol.size(), _vLine.size(),
                                       _vCol.size(), samples.first,
                                       Resampler::BOUNDARY_CLAMP, 1.0, 0.0, sFilter.c_str()));

        // Determine final size (only upscale)
        if (samples.first > _vLine.size())
            nLinesToInsert = samples.first - _vLine.size();
    }
    else if (Direction == LINES)// lines
    {
        // Create the resample object and prepare the needed memory
        _resampler.reset(new Resampler(_vCol.size(), _vLine.size(),
                                       samples.second, _vLine.size(),
                                       Resampler::BOUNDARY_CLAMP, 1.0, 0.0, sFilter.c_str()));

        // Determine final size (only upscale)
        if (samples.second > _vCol.size())
            nColsToInsert = samples.second - _vCol.size();
    }

    // Ensure that the resampler was created
    if (!_resampler)
        throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);

    // Create and initialize the dynamic memory: inserted rows and columns
    if (nLinesToInsert)
    {
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if ((int)memArray.size() < _vCol[j] && memArray[_vCol[j]])
                memArray[_vCol[j]]->insertElements(_vLine.last()+1, nLinesToInsert);
        }
    }

    if (nColsToInsert)
    {
        TableColumnArray arr(nColsToInsert);
        memArray.insert(memArray.begin()+_vCol.last()+1, std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
    }

    // resampler output buffer
    const double* dOutputSamples = 0;
    std::vector<double> dInputSamples(_vCol.size());
    int _ret_line = 0;
    int _final_cols = 0;

    // Determine the number of final columns. These will stay constant only in
    // the column application direction
    if (Direction == ALL || Direction == GRID || Direction == LINES)
        _final_cols = samples.second;
    else
        _final_cols = _vCol.size();

    // Resample the data table
    // Apply the resampling linewise
    for (size_t i = 0; i < _vLine.size(); i++)
    {
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            dInputSamples[j] = readMem(_vLine[i], _vCol[j]).real();
        }

        // If the resampler doesn't accept a further line
        // the buffer is probably full
        if (!_resampler->put_line(&dInputSamples[0]))
        {
            if (_resampler->status() != Resampler::STATUS_SCAN_BUFFER_FULL)
            {
                // Obviously not the case
                throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
            }
            else if (_resampler->status() == Resampler::STATUS_SCAN_BUFFER_FULL)
            {
                // Free the scan buffer of the resampler by extracting the already resampled lines
                while (true)
                {
                    dOutputSamples = _resampler->get_line();

                    // dOutputSamples will be a nullptr, if no more resampled
                    // lines are available
                    if (!dOutputSamples)
                        break;

                    for (int _fin = 0; _fin < _final_cols; _fin++)
                    {
                        writeData(_vLine.front()+_ret_line, _vCol.front()+_fin, dOutputSamples[_fin]);
                    }

                    _ret_line++;
                }

                // Try again to put the current line
                _resampler->put_line(&dInputSamples[0]);
            }
        }
    }

    // Extract the remaining resampled lines from the resampler's memory
    while (true)
    {
        dOutputSamples = _resampler->get_line();

        // dOutputSamples will be a nullptr, if no more resampled
        // lines are available
        if (!dOutputSamples)
            break;

        for (int _fin = 0; _fin < _final_cols; _fin++)
        {
            writeData(_vLine.front()+_ret_line, _vCol.front()+_fin, dOutputSamples[_fin]);
        }

        _ret_line++;
    }

    // Delete empty lines
    if (Direction != LINES && samples.first < _vLine.size())
        deleteBulk(VectorIndex(_vLine.front() + samples.first, _vLine.last()), _vCol);

    // Delete empty cols
    if (Direction != COLS && samples.second < _vCol.size())
        deleteBulk(_vLine, VectorIndex(_vCol.front() + samples.second, _vCol.last()));

    // Reset the calculated lines and columns
    nCalcLines = -1;
    m_meta.modify();

    return true;
}


