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

#include <regex>

#include "memory.hpp"
#include "tablecolumnimpl.hpp"
#include "../../kernel.hpp"
#include "../io/file.hpp"
#include "../ui/error.hpp"
#include "../settings.hpp"
#include "../utils/tools.hpp"
#include "../maths/resampler.h"
#include "../maths/statslogic.hpp"
#include "../maths/matdatastructures.hpp"
#include "../maths/units.hpp"
#include "../procedure/mangler.hpp"

#ifdef __GNUWIN64__
#define MAX_TABLE_COLS (INT_MAX-1)/2
#else
#define MAX_TABLE_COLS 1e4
#endif
#define DEFAULT_COL_TYPE ValueColumn


using namespace std;

/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
Table::Table()
{
    bSaveMutex = false;
    m_meta.save();
}


/////////////////////////////////////////////////
/// \brief Specialized constructor to allocate a
/// defined table size.
///
/// \param _nCols size_t
/// \param _nSheets size_t
///
/////////////////////////////////////////////////
Table::Table(size_t _nCols, size_t _nSheets) : Table()
{
    Allocate(_nCols, _nSheets);
}


/////////////////////////////////////////////////
/// \brief Memory class destructor, which will
/// free the allocated memory.
/////////////////////////////////////////////////
Table::~Table()
{
    clear();
}


/////////////////////////////////////////////////
/// \brief This member function is the Memory
/// class allocator. It will handle all memory
/// allocations.
///
/// \param nCols size_t
/// \param nSheets size_t
/// \param shrink bool
/// \return bool
///
/////////////////////////////////////////////////
bool Table::Allocate(size_t nCols, size_t nSheets, bool shrink)
{
    if (nCols*nSheets > MAX_TABLE_COLS)
        throw SyntaxError(SyntaxError::TOO_LARGE_CACHE, "", SyntaxError::invalid_position);

    m_data.reserve(nSheets);

    for (size_t k = 0; k < nSheets; k++)
    {
        m_data.getSheet(k).reserve(nCols);
    }

    if (shrink)
        this->shrink();

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// column headlines, if they are empty.
///
/// \return void
///
/////////////////////////////////////////////////
void Table::createTableHeaders()
{
    for (size_t k = 0; k < m_data.sheets(); k++)
    {
        TableSheet& sheet = m_data.getSheet(k);

        for (size_t j = 0; j < sheet.cols(); j++)
        {
            if (!sheet.m_columns[j])
                sheet.m_columns[j].reset(new DEFAULT_COL_TYPE);

            if (!sheet.m_columns[j]->m_sHeadLine.length())
                sheet.m_columns[j]->m_sHeadLine = TableColumn::getDefaultColumnHead(j);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function frees the
/// internally used memory block completely.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Table::clear()
{
    m_data.clear();

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
Table& Table::operator=(const Table& other)
{
    clear();

    m_data = other.m_data;
    m_meta = other.m_meta;
    m_meta.modify();

    return *this;
}


/////////////////////////////////////////////////
/// \brief This member function will handle all
/// memory grow operations by doubling the base
/// size, which shall be incremented, as long as
/// it is smaller than the requested size.
///
/// \param _nCols size_t
/// \param _nSheets size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::reserve(size_t _nCols, size_t _nSheets)
{
    return Allocate(_nCols, _nSheets);
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of columns, which are currently
/// available in this table.
///
/// \param sheet size_t
/// \return int
///
/////////////////////////////////////////////////
int Table::getCols(size_t sheet) const
{
    if (sheet >= m_data.sheets())
        return m_data.cols();

    return m_data.getSheet(sheet).cols();
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of rows, which are currently
/// available in this table.
///
/// \param sheet size_t
/// \return int
///
/////////////////////////////////////////////////
int Table::getRows(size_t sheet) const
{
    if (sheet >= m_data.sheets())
        return m_data.rows();

    return m_data.getSheet(sheet).rows();
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// number of available layers ("sheets"), which
/// are currently available in this table.
///
/// \return int
///
/////////////////////////////////////////////////
int Table::getSheets() const
{
    return m_data.sheets();
}


/////////////////////////////////////////////////
/// \brief Returns the number of elements in the
/// selected column (but might contain invalid
/// values).
///
/// \param col size_t
/// \param sheet size_t
/// \return int
///
/////////////////////////////////////////////////
int Table::getElemsInColumn(size_t col, size_t sheet) const
{
    if (m_data.sheets() > sheet)
        return m_data.getSheet(sheet).getElemsInColumn(col);

    return 0;
}


/////////////////////////////////////////////////
/// \brief Returns the number of filled elements
/// in the selected column without the trailing
/// but with the internal invalid values.
///
/// \param col size_t
/// \param sheet size_t
/// \return int
///
/////////////////////////////////////////////////
int Table::getFilledElemsInColumn(size_t col, size_t sheet) const
{
   if (m_data.sheets() > sheet)
        return m_data.getSheet(sheet).getFilledElemsInColumn(col);

    return 0;
}


/////////////////////////////////////////////////
/// \brief Returns the overall used number of
/// bytes for this table.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t Table::getSize() const
{
    return m_data.getSize() + m_meta.comment.length();
}


/////////////////////////////////////////////////
/// \brief Formats the dimensions of this table
/// to be printed on screen.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Table::printDims() const
{
    if (m_data.sheets() > 1)
        return toString(m_data.rows()) + " x " + toString(m_data.cols()) + " x " + toString(m_data.sheets());

    return toString(m_data.rows()) + " x " + toString(m_data.cols());
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// element stored at the selected position.
///
/// \param _nRow size_t
/// \param _nCol size_t
/// \param _nSheet size_t
/// \return mu::Value
///
/////////////////////////////////////////////////
mu::Value Table::read(size_t _nRow, size_t _nCol, size_t _nSheet) const
{
    return m_data.read(_nRow, _nCol, _nSheet);
}


/////////////////////////////////////////////////
/// \brief This static helper function calculates
/// the average value respecting NaNs.
///
/// \param values const std::vector<std::complex<double>>&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
static std::complex<double> nanAvg(const std::vector<std::complex<double>>& values)
{
    std::complex<double> sum = 0.0;
    double c = 0.0;

    for (const std::complex<double>& val : values)
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
/// \param sheet size_t
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::readMemInterpolated(double _dLine, double _dCol, size_t sheet) const
{
    if (std::isnan(_dLine) || std::isnan(_dCol) || sheet >= m_data.sheets())
        return NAN;

    // Find the base index
    int nBaseRow = intCast(_dLine) + (_dLine < 0 ? -1 : 0);
    int nBaseCol = intCast(_dCol) + (_dCol < 0 ? -1 : 0);

    // Get the decimal part of the double indices
    double x = _dLine - nBaseRow;
    double y = _dCol - nBaseCol;

    auto readCmplx = [&](int nRow, int nCol, size_t sheet)
        {return nCol < 0 || !m_data.getSheet(sheet).isValidColumn(nCol) ? NAN : m_data.getSheet(sheet).m_columns[nCol]->getValue(nRow);};

    // Find the surrounding four entries
    std::complex<double> f00 = readCmplx(nBaseRow, nBaseCol, sheet);
    std::complex<double> f10 = readCmplx(nBaseRow+1, nBaseCol, sheet);
    std::complex<double> f01 = readCmplx(nBaseRow, nBaseCol+1, sheet);
    std::complex<double> f11 = readCmplx(nBaseRow+1, nBaseCol+1, sheet);

    // If all are NAN, return NAN
    if (mu::isnan(f00) && mu::isnan(f01) && mu::isnan(f10) && mu::isnan(f11))
        return NAN;

    // Get the average respecting NaNs
    std::complex<double> dNanAvg = nanAvg({f00, f01, f10, f11});

    // Otherwise set NAN to zero
    if (mu::isnan(f00))
        f00 = dNanAvg;

    if (mu::isnan(f10))
        f10 = dNanAvg;

    if (mu::isnan(f01))
        f01 = dNanAvg;

    if (mu::isnan(f11))
        f11 = dNanAvg;

    //     f(0,0) (1-x) (1-y) + f(1,0) x (1-y) + f(0,1) (1-x) y + f(1,1) x y
    return f00*(1.0-x)*(1.0-y) + f10*x*(1.0-y) + f01*(1.0-x)*y + f11*x*y;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// elements stored at the selected positions.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array Table::read(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    return m_data.read(TableIndices(_vRow, _vCol, _vSheet));
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// elements stored at the selected positions as
/// a Matrix.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \return Matrix
///
/////////////////////////////////////////////////
Matrix Table::readMemAsMatrix(const VectorIndex& _vRow, const VectorIndex& _vCol) const
{
    if (m_data.empty())
        return Matrix(1, 1);

    Matrix mat(_vRow.size(), _vCol.size());
    const TableSheet& sheet = m_data.getSheet(0);

    #pragma omp parallel for
    for (size_t j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0 || !sheet.isValidColumn(_vCol[j]))
            continue;

        // Get the complete column as a whole because it seems
        // to be much faster (VTABLE issues? Cache locality?)
        std::vector<std::complex<double>> vEntries = sheet.m_columns[_vCol[j]]->getValue(_vRow);

        for (size_t i = 0; i < vEntries.size(); i++)
        {
            mat(i, j) = vEntries[i];
        }

    }

    return mat;
}


/////////////////////////////////////////////////
/// \brief Returns the "common" type of the
/// selected columns.
///
/// \param _vCol const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \return TableColumn::ColumnType
///
/////////////////////////////////////////////////
TableColumn::ColumnType Table::getType(const VectorIndex& _vCol, const VectorIndex& _vSheets) const
{
    return m_data.getType(_vCol, _vSheets);
}


/////////////////////////////////////////////////
/// \brief Returns true, if all selected columns
/// are either empty or do not contain
/// string-like data.
///
/// \param _vCol const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::isValueLike(const VectorIndex& _vCol, const VectorIndex& _vSheets) const
{
    _vCol.setOpenEndIndex(getCols()-1);
    _vSheets.setOpenEndIndex(getSheets()-1);

    for (size_t k = 0; k < _vSheets.size(); k++)
    {
        const TableSheet& sheet = m_data.getSheet(k);

        for (size_t i = 0; i < _vCol.size(); i++)
        {
            if (_vCol[i] >= 0 && sheet.isValidColumn(_vCol[i]))
            {
                if (sheet.m_columns[_vCol[i]]->m_type >= TableColumn::STRINGLIKE)
                    return false;
            }
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Returns a key-value list containing
/// the categories and their respective index.
///
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array Table::getCategoryList(const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    mu::Array vRet;

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        if (_vSheet[k] < 0 || _vSheet[k] >= (int)m_data.sheets())
            continue;

        const TableSheet& sheet = m_data.getSheet(_vSheet[k]);

        for (size_t i = 0; i < _vCol.size(); i++)
        {
            if (_vCol[i] >= 0 && sheet.isValidColumn(_vCol[i]))
            {
                if (sheet.m_columns[_vCol[i]]->m_type == TableColumn::TYPE_CATEGORICAL)
                {
                    const std::vector<std::string>& vCategories = static_cast<CategoricalColumn*>(sheet.m_columns[_vCol[i]].get())->getCategories();

                    for (size_t c = 0; c < vCategories.size(); c++)
                    {
                        vRet.push_back(mu::Category{mu::Numerical(c+1), vCategories[c]});
                    }
                }
            }
        }
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief This member function extracts a range
/// of this table and returns it as a new Memory
/// instance.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \return Memory*
///
/// \remark The caller gets ownership of the
/// returned Memory instance.
///
/////////////////////////////////////////////////
Table* Table::extractRange(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheets) const
{
    Table* _memCopy = new Table();

    _vRow.setOpenEndIndex(getRows()-1);
    _vCol.setOpenEndIndex(getCols()-1);
    _vSheets.setOpenEndIndex(getSheets()-1);

    _memCopy->m_data = *m_data.extract(TableIndices(_vRow, _vCol, _vSheets));
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
/// \param vTarget mu::Variable*
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \return void
///
/////////////////////////////////////////////////
void Table::copyElementsInto(mu::Variable* vTarget, const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheets) const
{
    m_data.read(TableIndices(_vRow, _vCol, _vSheets), vTarget);
}


/////////////////////////////////////////////////
/// \brief Returns true, if the element at the
/// selected positions is valid. Only checks
/// internally, if the value is not a NaN value.
///
/// \param _nRow size_t
/// \param _nCol size_t
/// \param _nSheet size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::isValidElement(size_t _nRow, size_t _nCol, size_t _nSheet) const
{
    return m_data.read(_nRow, _nCol, _nSheet).isValid();
}


/////////////////////////////////////////////////
/// \brief Returns true, if at least a single
/// valid value is available in this table.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Table::isValid() const
{
    return !m_data.empty();
}


/////////////////////////////////////////////////
/// \brief This member function shrinks the table
/// memory to the smallest possible dimensions
/// reachable in powers of two.
///
/// \param _vCol const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::shrink(const VectorIndex& _vCol, const VectorIndex& _vSheets)
{
    if (!m_data.sheets())
        return true;

    _vCol.setOpenEndIndex(getCols()-1);
    _vSheets.setOpenEndIndex(getSheets()-1);

    for (size_t k = 0; k < _vSheets.size(); k++)
    {
        if (_vSheets[k] < 0 || (size_t)_vSheets[k] >= m_data.sheets())
            continue;

        m_data.getSheet(_vSheets[k]).shrink(_vCol);
    }

    // Remove obsolete sheets
    for (int i = m_data.sheets()-1; i >= 0; i--)
    {
        if (m_data.getSheet(i).rows())
        {
            m_data.resize(i+1);
            return true;
        }
    }

    // if this place is reached, delete everything
    m_data.clear();
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
void Table::convert()
{
    #pragma omp parallel for
    for (size_t k = 0; k < m_data.sheets(); k++)
    {
        TableSheet& sheet = m_data.getSheet(k);

        for (size_t i = 0; i < sheet.cols(); i++)
        {
            if (sheet.isValidColumn(i) && sheet.m_columns[i]->m_type == TableColumn::TYPE_STRING)
            {
                TableColumn* col = sheet.m_columns[i]->convert();

                // Only valid conversions return a non-zero
                // pointer
                if (col && col != sheet.m_columns[i].get())
                    sheet.m_columns[i].reset(col);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function tries to convert
/// the selected columns to the target column
/// type, if it is possible.
///
/// \param _vCol const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \param _sType const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::convertColumns(const VectorIndex& _vCol, const VectorIndex& _vSheets, const std::string& _sType)
{
    TableColumn::ColumnType _type = TableColumn::stringToType(_sType);

    if (_type == TableColumn::TYPE_NONE)
        return false;

    bool success = true;
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheets.setOpenEndIndex(m_data.sheets()-1);

    for (size_t k = 0; k < _vSheets.size(); k++)
    {
        if (_vSheets[k] < 0 || (size_t)_vSheets[k] >= m_data.sheets())
            continue;

        TableSheet& sheet = m_data.getSheet(_vSheets[k]);

        for (size_t i = 0; i < _vCol.size(); i++)
        {
            if (_vCol[i] < 0 || !sheet.isValidColumn(_vCol[i]))
                continue;

            if (sheet.m_columns[_vCol[i]]->m_type != _type && sheet.m_columns[_vCol[i]]->m_type != TableColumn::TYPE_NONE)
            {
                TableColumn* col = sheet.m_columns[_vCol[i]]->convert(_type);

                // Only valid conversions return a non-zero
                // pointer
                if (col && col != sheet.m_columns[_vCol[i]].get())
                    sheet.m_columns[_vCol[i]].reset(col);
                else
                    success = _type == TableColumn::TYPE_MIXED; // Auto conversions do not flag errors
            }
        }
    }

    // If successful: mark the whole table as modified
    if (success)
        m_meta.modify();

    return success;
}


/////////////////////////////////////////////////
/// \brief Updates the categories of a
/// categorical column and switches the column
/// type if necessary.
///
/// \param _vCol const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \param vCategories const std::vector<std::string>&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::setCategories(const VectorIndex& _vCol, const VectorIndex& _vSheets, const std::vector<std::string>& vCategories)
{
    bool success = true;
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheets.setOpenEndIndex(m_data.sheets()-1);

    for (size_t k = 0; k < _vSheets.size(); k++)
    {
        if (_vSheets[k] < 0 || (size_t)_vSheets[k] >= m_data.sheets())
            continue;

        TableSheet& sheet = m_data.getSheet(_vSheets[k]);

        for (size_t i = 0; i < _vCol.size(); i++)
        {
            if (_vCol[i] < 0 || !sheet.isValidColumn(_vCol[i]))
                continue;

            if (sheet.m_columns[_vCol[i]]->m_type != TableColumn::TYPE_CATEGORICAL)
            {
                TableColumn* col = sheet.m_columns[_vCol[i]]->convert(TableColumn::TYPE_CATEGORICAL);

                // Only valid conversions return a non-zero
                // pointer
                if (col && col != sheet.m_columns[_vCol[i]].get())
                {
                    sheet.m_columns[_vCol[i]].reset(col);
                    static_cast<CategoricalColumn*>(col)->setCategories(vCategories);
                }
                else
                    success = false;
            }
            else
                static_cast<CategoricalColumn*>(sheet.m_columns[_vCol[i]].get())->setCategories(vCategories);
        }
    }

    if (success)
        m_meta.modify();

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
bool Table::getSaveStatus() const
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
/// \param sheet size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string Table::getHeadLineElement(size_t _i, size_t sheet) const
{
    if (sheet >= m_data.sheets() || !m_data.getSheet(sheet).isValidColumn(_i))
        return TableColumn::getDefaultColumnHead(_i+1);

    return m_data.getSheet(sheet).m_columns[_i]->m_sHeadLine;
}


/////////////////////////////////////////////////
/// \brief Returns the table column headlines for
/// the selected columns. Will return default
/// headlines for empty or non-existing columns.
///
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Table::getHeadLineElement(const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    vector<string> vHeadLines;

    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        for (size_t i = 0; i < _vCol.size(); i++)
        {
            vHeadLines.push_back(getHeadLineElement(_vCol[i], _vSheet[k]));
        }
    }

    return vHeadLines;
}


/////////////////////////////////////////////////
/// \brief Returns the unit of the selected
/// column.
///
/// \param nCol int
/// \param sheet size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string Table::getUnit(int nCol, size_t sheet) const
{
    if (sheet >= m_data.sheets() || !m_data.getSheet(sheet).isValidColumn(nCol))
        return "";

    return m_data.getSheet(sheet).m_columns[nCol]->m_sUnit;
}


/////////////////////////////////////////////////
/// \brief Returns the values in SI units, if a
/// conversion is known.
///
/// \param nCol size_t
/// \param nSheet size_t
/// \param mode UnitConversionMode
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> Table::asSiUnits(size_t nCol, size_t nSheet, UnitConversionMode mode) const
{
    if (nSheet < m_data.sheets()
        && m_data.getSheet(nSheet).isValidColumn(nCol)
        && TableColumn::isValueType(m_data.getSheet(nSheet).m_columns[nCol]->m_type))
    {
        const TblColPtr& col = m_data.getSheet(nSheet).m_columns[nCol];
        std::vector<std::complex<double>> vConverted = col->getValue(VectorIndex(0, VectorIndex::OPEN_END));
        UnitConversion convert = getUnitConversion(col->m_sUnit, mode);

        for (size_t i = 0; i < vConverted.size(); i++)
        {
            vConverted[i] = convert(mu::Value(vConverted[i])).front().getNum().asCF64();
        }

        return vConverted;
    }

    return std::vector<std::complex<double>>(1, NAN);
}


/////////////////////////////////////////////////
/// \brief Show the conversion from the selected
/// column's unit to the corresponding SI units.
///
/// \param nCol size_t
/// \param nSheet size_t
/// \param mode UnitConversionMode
/// \return std::string
///
/////////////////////////////////////////////////
std::string Table::showUnitConversion(size_t nCol, size_t nSheet, UnitConversionMode mode) const
{
    if (nSheet < m_data.sheets()
        && m_data.getSheet(nSheet).isValidColumn(nCol)
        && TableColumn::isValueType(m_data.getSheet(nSheet).m_columns[nCol]->m_type))
        return printUnitConversion(m_data.getSheet(nSheet).m_columns[nCol]->m_sUnit, mode);

    return "";
}


/////////////////////////////////////////////////
/// \brief Writes a new table column headline to
/// the selected column.
///
/// \param _i size_t
/// \param sheet size_t
/// \param _sHead const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::setHeadLineElement(size_t _i, size_t sheet, const std::string& _sHead)
{
    if (sheet >= m_data.sheets())
        m_data.reserve(sheet+1);

    m_data.getSheet(sheet).writeHeading(_sHead, _i);
    m_meta.modify();

    return true;
}


/////////////////////////////////////////////////
/// \brief Set the unit of the selected column.
///
/// \param nCol int
/// \param sheet size_t
/// \param sUnit const std::string&
/// \return void
///
/////////////////////////////////////////////////
bool Table::setUnit(int nCol, size_t sheet, const std::string& sUnit)
{
    if (sheet >= m_data.sheets())
        m_data.reserve(sheet+1);

    TableSheet& currSheet = m_data.getSheet(sheet);
    currSheet.reserve(nCol+1);

    if (!currSheet.m_columns[nCol])
    {
        currSheet.m_columns[nCol].reset(new DEFAULT_COL_TYPE);
        currSheet.m_columns[nCol]->m_sHeadLine = TableColumn::getDefaultColumnHead(nCol+1);
    }

    currSheet.m_columns[nCol]->m_sUnit = sUnit;
    m_meta.modify();

    return true;

}


/////////////////////////////////////////////////
/// \brief Converts the selected columns to SI
/// units, if a conversion is known and returns
/// the new units.
///
/// \param _vCols const VectorIndex&
/// \param _vSheets const VectorIndex&
/// \param mode UnitConversionMode
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> Table::toSiUnits(const VectorIndex& _vCols, const VectorIndex& _vSheets, UnitConversionMode mode)
{
    std::vector<std::string> vUnits;
    _vCols.setOpenEndIndex(m_data.cols()-1);
    _vSheets.setOpenEndIndex(m_data.sheets()-1);

    for (size_t k = 0; k < _vSheets.size(); k++)
    {
        if (_vSheets[k] < 0 || (size_t)_vSheets[k] >= m_data.sheets())
            continue;

        TableSheet& sheet = m_data.getSheet(_vSheets[k]);

        for (size_t i = 0; i < _vCols.size(); i++)
        {
            if (sheet.isValidColumn(_vCols[i]) && TableColumn::isValueType(sheet.m_columns[_vCols[i]]->m_type))
            {
                sheet.m_columns[_vCols[i]]->setValue(VectorIndex(0, VectorIndex::OPEN_END), asSiUnits(_vCols[i], _vSheets[k], mode));
                std::string sUnit = getUnitConversion(sheet.m_columns[_vCols[i]]->m_sUnit, mode).formatUnit(mode);
                sheet.m_columns[_vCols[i]]->m_sUnit = sUnit;
                vUnits.push_back(sUnit);
            }
            else
                vUnits.push_back("");
        }
    }

    return vUnits;
}


/////////////////////////////////////////////////
/// \brief Update the comment associated with
/// this table.
///
/// \param comment const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Table::writeComment(const std::string& comment)
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
void Table::setMetaData(const NumeRe::TableMetaData& meta)
{
    m_meta = meta;
}


/////////////////////////////////////////////////
/// \brief Mark this table as modified.
///
/// \return void
///
/////////////////////////////////////////////////
void Table::markModified()
{
    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// number of lines needed for the table column
/// headline of the selected column.
///
/// \param sheet size_t
/// \return size_t
///
/////////////////////////////////////////////////
size_t Table::getHeadlineCount(size_t sheet) const
{
    if (sheet >= m_data.sheets())
        return 0;

    size_t nHeadlineCount = 1;

    // Get the dimensions of the complete headline (i.e. including possible linebreaks)
    for (const TblColPtr& col : m_data.getSheet(sheet).m_columns)
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
std::string Table::getComment() const
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
NumeRe::TableMetaData Table::getMetaData() const
{
    return m_meta;
}


/////////////////////////////////////////////////
/// \brief This member function writes the passed
/// value to the selected position. The table is
/// automatically enlarged, if necessary.
///
/// \param _nRow int
/// \param _nCol int
/// \param _nSheet int
/// \param _value const mu::Value&
/// \return void
///
/////////////////////////////////////////////////
void Table::write(int _nRow, int _nCol, int _nSheet, const mu::Value& _value)
{
    if (_nSheet < 0 || _nCol < 0 || (_nRow < 0 && _nRow != VectorIndex::STRING))
        return;

    m_data.reserve(_nSheet+1);
    m_data.getSheet(_nSheet).write(_value, _nRow, _nCol);

    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function provides an
/// unsafe but direct way of writing data to the
/// table. It will not check for the existence of
/// the needed amount of columns.
///
/// \param _nRow int
/// \param _nCol int
/// \param _nSheet int
/// \param _dData const std::complex<double>&
/// \return void
///
/////////////////////////////////////////////////
void Table::writeDirect(int _nRow, int _nCol, int _nSheet, const std::complex<double>& _dData)
{
    TblColPtr& col = m_data.m_sheets[_nSheet]->m_columns[_nCol];
    promote_if_needed(col, _nCol, TableColumn::TYPE_VALUE_CF64);
    col->setValue(_nRow, _dData);
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
/// \param _nRow int
/// \param _nCol int
/// \param _nShet int
/// \param _dData const std::complex<double>&
/// \return void
///
/////////////////////////////////////////////////
void Table::writeUnsafe(int _nRow, int _nCol, int _nSheet, const std::complex<double>& _dData)
{
    m_data.m_sheets[_nSheet]->m_columns[_nCol]->setValue(_nRow, _dData);
}


/////////////////////////////////////////////////
/// \brief This member function writes a whole
/// array of values to the selected table range.
/// The table is automatically enlarged, if
/// necessary.
///
/// \param _idx Indices&
/// \param _values const mu::Array&
/// \return void
///
/////////////////////////////////////////////////
void Table::write(Indices& _idx, const mu::Array& _values)
{
    m_data.write(_values, TableIndices(_idx.row, _idx.col, _idx.layer));
    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief This member function changes the saved
/// state to the passed value.
///
/// \param _bIsSaved bool
/// \return void
///
/////////////////////////////////////////////////
void Table::setSaveStatus(bool _bIsSaved)
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
long long int Table::getLastSaved() const
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
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \param sSortingExpression const std::string&
/// \return std::vector<int>
///
/////////////////////////////////////////////////
std::vector<int> Table::sortElements(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet, const std::string& sSortingExpression)
{
    if (m_data.empty())
        return vector<int>();

    bool bReturnIndex = false;

    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<int> vIndex;

    // Prepare the sorting index
    vIndex = _vRow.getVector();

    // Evaluate, whether an index shall be returned
    // (instead of actual reordering the columns)
    if (findParameter(sSortingExpression, "index"))
        bReturnIndex = true;

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        if (_vSheet[k] < 0 || (size_t)_vSheet[k] >= m_data.sheets())
            continue;

        TableSheet& sheet = m_data.getSheet(_vSheet[k]);
        vIndex = sheet.sortElements(_vRow, _vCol, sSortingExpression);

        if (bReturnIndex)
            break;
    }

    m_meta.modify();

    if (!bReturnIndex)
        return std::vector<int>();

    return vIndex;
}


/////////////////////////////////////////////////
/// \brief Create a copy-efficient table object
/// from the data contents.
///
/// \param _sTable const std::string&
/// \param rows const VectorIndex&
/// \param cols const VectorIndex&
/// \param sheets const VectorIndex&
/// \return NumeRe::TableView
///
/////////////////////////////////////////////////
NumeRe::TableView Table::extractView(const std::string& _sTable, const VectorIndex& rows, const VectorIndex& cols, const VectorIndex& sheets)
{
    rows.setOpenEndIndex(m_data.rows()-1);
    cols.setOpenEndIndex(m_data.cols()-1);
    sheets.setOpenEndIndex(m_data.sheets()-1);

    NumeRe::TableView table(m_data.extract(TableIndices(rows, cols, sheets)));

    table.setName(_sTable);
    table.setMetaData(m_meta);

    return table;
}


/////////////////////////////////////////////////
/// \brief Import data from a copy-efficient
/// table object. Completely replaces the
/// contents, which were in the internal storage
/// before.
///
/// \param _table NumeRe::Table
/// \param rows const VectorIndex&
/// \param cols const VectorIndex&
/// \param sheets const VectorIndex&
/// \return void
///
/////////////////////////////////////////////////
void Table::importView(NumeRe::TableView _table, const VectorIndex& rows, const VectorIndex& cols, const VectorIndex& sheets)
{
    m_meta = _table.getMetaData();

    // We construct separate objects because they might be overwritten
    insertCopiedView(_table, rows, cols, sheets, false);
}


/////////////////////////////////////////////////
/// \brief Insert data from a copied table and
/// possibly transpose it during insertion. Will
/// trigger multiple column type conversions,
/// especially, if the table gets transposed.
/// Will also only transpose the columns without
/// considering the column headlines.
///
/// \param _table NumeRe::Table
/// \param rows const VectorIndex&
/// \param cols const VectorIndex&
/// \param sheets const VectorIndex&
/// \param transpose bool
/// \return void
///
/////////////////////////////////////////////////
void Table::insertCopiedView(NumeRe::TableView _table, const VectorIndex& rows, const VectorIndex& cols, const VectorIndex& sheets, bool transpose)
{
    // We construct separate objects because they might be overwritten
    deleteBulk(VectorIndex(rows), VectorIndex(cols), VectorIndex(sheets));

    rows.setOpenEndIndex(rows.front() + (transpose ? _table.getCols()-1 : _table.getRows()-1));
    cols.setOpenEndIndex(cols.front() + (transpose ? _table.getRows()-1 : _table.getCols()-1));
    sheets.setOpenEndIndex(sheets.front() + _table.getSheets()-1);

    reserve(cols.max()+1, sheets.max()+1);

    for (size_t k = 0; k < sheets.size(); k++)
    {
        if (sheets[k] < 0 || (size_t)sheets[k] >= m_data.sheets())
            continue;

        TableSheet& sheet = m_data.getSheet(sheets[k]);

        // Shall we transpose the table?
        if (!transpose)
        {
            // Insert without transposition
            #pragma omp parallel for
            for (size_t j = 0; j < _table.getCols(); j++)
            {
                if (j >= cols.size())
                    continue;

                TableColumn* tabCol = _table.getColumn(j, k);

                if (!tabCol)
                    continue;

                // Do we have to create a new column?
                if (!sheet.m_columns[cols[j]])
                {
                    if (TableColumn::isValueType(tabCol->m_type))
                        sheet.m_columns[cols[j]].reset(createValueTypeColumn(tabCol->m_type));
                    else if (tabCol->m_type == TableColumn::TYPE_DATETIME)
                        sheet.m_columns[cols[j]].reset(new DateTimeColumn);
                    else if (tabCol->m_type == TableColumn::TYPE_DURATION)
                        sheet.m_columns[cols[j]].reset(new DurationColumn);
                    else if (tabCol->m_type == TableColumn::TYPE_STRING)
                        sheet.m_columns[cols[j]].reset(new StringColumn);
                    else if (tabCol->m_type == TableColumn::TYPE_LOGICAL)
                        sheet.m_columns[cols[j]].reset(new LogicalColumn);
                    else if (tabCol->m_type == TableColumn::TYPE_CATEGORICAL)
                        sheet.m_columns[cols[j]].reset(new CategoricalColumn);
                    else
                    {
                        NumeReKernel::issueWarning("In Memory::insertCopiedTable(): TableColumn::ColumnType not implemented.");
                        continue;
                    }

                    sheet.m_columns[cols[j]]->assignMetaData(tabCol);
                }
                else if (tabCol->m_type != sheet.m_columns[cols[j]]->m_type)
                {
                    // Convert the column if the type does not fit
                    sheet.m_columns[cols[j]].reset(sheet.m_columns[cols[j]]->convert(TableColumn::TYPE_STRING));
                    sheet.m_columns[cols[j]]->insert(rows, tabCol->convert(TableColumn::TYPE_STRING));
                    continue;
                }

                // Common type: simply insert the data
                sheet.m_columns[cols[j]]->insert(rows, tabCol);
            }
        }
        else
        {
            // Transpose the table
            #pragma omp parallel for
            for (size_t j = 0; j < _table.getRows(); j++)
            {
                if (j >= cols.size())
                    continue;

                // If we have to create a column, we'll create a string column,
                // otherwise we'll convert the current one to a string column
                if (!sheet.m_columns[cols[j]])
                {
                    sheet.m_columns[cols[j]].reset(new StringColumn);
                    sheet.m_columns[cols[j]]->m_sHeadLine = TableColumn::getDefaultColumnHead(cols[j]);
                }
                else
                    sheet.m_columns[cols[j]].reset(sheet.m_columns[cols[j]]->convert(TableColumn::TYPE_STRING));

                // There's no easier way to store the result because the
                // table does not return rows as vectors
                for (size_t i = 0; i < _table.getCols(); i++)
                {
                    if (i >= rows.size())
                        break;

                    sheet.m_columns[cols[j]]->setValue(rows[i], _table.getValueAsInternalString(j, i, k));
                }
            }
        }
    }

    // Try to convert string- to valuecolumns
    convert();
    m_meta.modify();
}


/////////////////////////////////////////////////
/// \brief Insert a block of elements starting
/// from the indicated topleft position, moving
/// that downwards.
///
/// \param atRow size_t
/// \param atCol size_t
/// \param atSheet size_t
/// \param rows size_t
/// \param cols size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::insertBlock(size_t atRow, size_t atCol, size_t atSheet, size_t rows, size_t cols)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    if (!sheet.isValidColumn(atCol))
        return false;

    // Catch whole rows and cols
    if (atRow == 0 && rows >= sheet.rows())
        return insertCols(atCol, atSheet, cols);
    else if (atCol == 0 && cols >= sheet.cols())
        return insertRows(atRow, atSheet, rows);

    for (size_t c = atCol; c < std::min(atCol+cols, sheet.cols()); c++)
    {
        if (sheet.m_columns[c])
            sheet.m_columns[c]->insertElements(atRow, rows);
    }

    m_meta.modify();
    sheet.m_calculatedRows = 0;
    return true;
}


/////////////////////////////////////////////////
/// \brief Insert a set of columns in front of
/// the desired column.
///
/// \param atCol size_t
/// \param atSheet size_t
/// \param num size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::insertCols(size_t atCol, size_t atSheet, size_t num)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    if (atCol >= sheet.cols())
        return false;

    TableColumnArray arr(num);
    sheet.m_columns.insert(sheet.m_columns.begin()+atCol, std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
    m_meta.modify();
    return true;
}


/////////////////////////////////////////////////
/// \brief Insert a set of rows in front of the
/// desired row.
///
/// \param atRow size_t
/// \param atSheet size_t
/// \param num size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::insertRows(size_t atRow, size_t atSheet, size_t num)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    if (atRow >= sheet.rows())
        return false;

    for (TblColPtr& col : sheet.getCols())
    {
        if (col)
            col->insertElements(atRow, num);
    }

    m_meta.modify();
    sheet.m_calculatedRows = 0;
    return true;
}


/////////////////////////////////////////////////
/// \brief Insert a set of sheets in front of the
/// desired sheet.
///
/// \param atSheet size_t
/// \param num size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::insertSheets(size_t atSheet, size_t num)
{
    if (atSheet >= m_data.sheets())
        return false;

    for (size_t s = atSheet; s < atSheet+num; s++)
    {
        m_data.m_sheets.insert(m_data.m_sheets.begin()+s, TblSheetPtr(new TableSheet("Sheet_" + toString(s+1))));
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Remove a block of elements starting
/// from the indicated topleft position, moving
/// everything below the block upwards.
///
/// \param atRow size_t
/// \param atCol size_t
/// \param atSheet size_t
/// \param rows size_t
/// \param cols size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::removeBlock(size_t atRow, size_t atCol, size_t atSheet, size_t rows, size_t cols)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    if (atRow >= sheet.rows() || atCol >= sheet.cols())
        return false;

    // Catch whole rows and cols
    if (atRow == 0 && rows >= sheet.rows())
        return removeCols(VectorIndex(atCol, atCol+cols-1), atSheet);
    else if (atCol == 0 && cols >= sheet.cols())
        return removeRows(VectorIndex(atRow, atRow+rows-1), atSheet);

    for (size_t c = atCol; c < std::min(atCol+cols, sheet.cols()); c++)
    {
        if (sheet.m_columns[c])
            sheet.m_columns[c]->removeElements(atRow, rows);
    }

    m_meta.modify();
    sheet.m_calculatedRows = 0;
    return true;
}


/////////////////////////////////////////////////
/// \brief Remove a set of indicated columns,
/// moving everything behind to the left.
///
/// \param _vCols const VectorIndex&
/// \param atSheet size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::removeCols(const VectorIndex& _vCols, size_t atSheet)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    if (_vCols.min() >= (int)sheet.cols())
        return false;

    _vCols.setOpenEndIndex(sheet.cols()-1);

    if (_vCols.isExpanded())
        sheet.m_columns.erase(sheet.m_columns.begin()+_vCols.min(), sheet.m_columns.begin()+_vCols.max()+1);
    else if (_vCols.size() == 1)
        sheet.m_columns.erase(sheet.m_columns.begin()+_vCols.front());
    else
    {
        std::vector<int> vVals = _vCols.getVector();
        std::sort(vVals.begin(), vVals.end());

        for (int i = vVals.size()-1; i >= 0; i--)
        {
            if (vVals[i] < (int)sheet.cols())
                sheet.m_columns.erase(sheet.m_columns.begin()+vVals[i]);
        }
    }

    sheet.m_calculatedRows = 0;
    m_meta.modify();
    return true;
}


/////////////////////////////////////////////////
/// \brief Remove a set of indicated rows, moving
/// everything upwards.
///
/// \param _vRows const VectorIndex&
/// \param atSheet size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::removeRows(const VectorIndex& _vRows, size_t atSheet)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    if (_vRows.min() >= (int)sheet.rows())
        return false;

    _vRows.setOpenEndIndex(sheet.rows()-1);

    if (_vRows.isExpanded())
    {
        for (TblColPtr& col : sheet.getCols())
        {
            if (col)
                col->removeElements(_vRows.min(), _vRows.size());
        }
    }
    else if (_vRows.size() == 1)
    {
        for (TblColPtr& col : sheet.getCols())
        {
            if (col)
                col->removeElements(_vRows.front(), 1);
        }
    }
    else
    {
        std::vector<int> vVals = _vRows.getVector();
        std::sort(vVals.begin(), vVals.end());

        for (int i = vVals.size()-1; i >= 0; i--)
        {
            for (TblColPtr& col : sheet.getCols())
            {
                if (col)
                    col->removeElements(vVals[i], 1);
            }
        }
    }

    m_meta.modify();
    sheet.m_calculatedRows = 0;
    return true;
}


/////////////////////////////////////////////////
/// \brief Remove a set of indicated sheets,
/// moving everthing forwards.
///
/// \param _vSheets const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::removeSheets(const VectorIndex& _vSheets)
{
    if (_vSheets.min() >= (int)m_data.sheets())
        return false;

    _vSheets.setOpenEndIndex(m_data.sheets()-1);

    if (_vSheets.isExpanded())
        m_data.m_sheets.erase(m_data.m_sheets.begin()+_vSheets.min(), m_data.m_sheets.begin()+_vSheets.max()+1);
    else if (_vSheets.size() == 1)
        m_data.m_sheets.erase(m_data.m_sheets.begin()+_vSheets.front());
    else
    {
        std::vector<int> vVals = _vSheets.getVector();
        std::sort(vVals.begin(), vVals.end());

        for (int i = vVals.size()-1; i >= 0; i--)
        {
            if (vVals[i] < (int)m_data.sheets())
                m_data.m_sheets.erase(m_data.m_sheets.begin()+vVals[i]);
        }
    }

    m_meta.modify();
    return true;
}


/////////////////////////////////////////////////
/// \brief Reorder a set of columns.
///
/// \param _vCols const VectorIndex&
/// \param atSheet size_t
/// \param _vNewOrder const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::reorderCols(const VectorIndex& _vCols, size_t atSheet, const VectorIndex& _vNewOrder)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    // The new order must not be open ended
    if (_vNewOrder.isOpenEnd())
        return false;

    _vCols.setOpenEndIndex(sheet.cols()-1);

    // Ensure that the indices reflect reasonable combinations
    if (_vNewOrder.size() != _vCols.size()
        || _vCols.size() > sheet.cols()
        || _vNewOrder.max() >= (int)_vNewOrder.size()
        || !_vCols.isUnique()
        || !_vNewOrder.isPermutation())
        return false;

    TableColumnArray buffer;
    buffer.resize(_vCols.size());

    // Move the columns to the buffer
    for (size_t i = 0; i < _vCols.size(); i++)
    {
        if (sheet.isValidColumn(_vCols[i]))
            buffer[i].reset(sheet.m_columns[_vCols[i]].release());
    }

    // Move the columns back to the original array but in the new order
    for (size_t i = 0; i < _vNewOrder.size(); i++)
    {
        if (_vNewOrder[i] != VectorIndex::INVALID && _vCols[i] != VectorIndex::INVALID)
            sheet.m_columns[_vCols[i]].reset(buffer[_vNewOrder[i]].release());
    }

    m_meta.modify();
    return true;
}


/////////////////////////////////////////////////
/// \brief Reorder a set of rows.
///
/// \param _vCols const VectorIndex&
/// \param atSheet size_t
/// \param _vNewOrder const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::reorderRows(const VectorIndex& _vRows, size_t atSheet, const VectorIndex& _vNewOrder)
{
    if (atSheet >= m_data.sheets())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);

    // The new order must not be open ended
    if (_vNewOrder.isOpenEnd())
        return false;

    _vRows.setOpenEndIndex(sheet.rows()-1);

    // Ensure that the indices reflect reasonable combinations
    if (_vNewOrder.size() != _vRows.size()
        || _vRows.size() > sheet.rows()
        || _vNewOrder.max() >= (long long int)_vNewOrder.size()
        || !_vRows.isUnique()
        || !_vNewOrder.isPermutation())
        return false;

    VectorIndex vPlain = _vRows.get(_vNewOrder);

    // Reorder the cells in each column
    for (auto& col : sheet.getCols())
    {
        if (col)
        {
            TblColPtr cpy(col->copy(vPlain));
            col->insert(_vRows, cpy.get());
            col->shrink();
        }
    }

    m_meta.modify();
    return true;
}


/////////////////////////////////////////////////
/// \brief Reorder a set of sheets.
///
/// \param _vSheets const VectorIndex&
/// \param _vNewOrder const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::reorderSheets(const VectorIndex& _vSheets, const VectorIndex& _vNewOrder)
{
    // The new order must not be open ended
    if (_vNewOrder.isOpenEnd())
        return false;

    _vSheets.setOpenEndIndex(m_data.sheets()-1);

    // Ensure that the indices reflect reasonable combinations
    if (_vNewOrder.size() != _vSheets.size()
        || _vSheets.size() > m_data.sheets()
        || _vNewOrder.max() >= (int)_vNewOrder.size()
        || !_vSheets.isUnique()
        || !_vNewOrder.isPermutation())
        return false;

    TableSheetArray buffer;
    buffer.resize(_vSheets.size());

    // Move the columns to the buffer
    for (size_t i = 0; i < _vSheets.size(); i++)
    {
        if (_vSheets[i] >= 0 && (size_t)_vSheets[i] < m_data.sheets())
            buffer[i].reset(m_data.m_sheets[_vSheets[i]].release());
    }

    // Move the columns back to the original array but in the new order
    for (size_t i = 0; i < _vNewOrder.size(); i++)
    {
        if (_vNewOrder[i] != VectorIndex::INVALID && _vSheets[i] != VectorIndex::INVALID)
            m_data.m_sheets[_vSheets[i]].reset(buffer[_vNewOrder[i]].release());
    }

    m_meta.modify();
    return true;

}


/////////////////////////////////////////////////
/// \brief Replaces the old with the new values
/// in the selected columns.
///
/// \param _vCols const VectorIndex&
/// \param _oldVals const mu::Array&
/// \param _newVals const mu::Array&
/// \param enableRegEx bool
/// \return bool
///
/////////////////////////////////////////////////
bool Table::replaceVals(const VectorIndex& _vCols, size_t atSheet, const mu::Array& _oldVals, const mu::Array& _newVals, bool enableRegEx)
{
    if (atSheet >= m_data.sheets())
        return false;

    if ((_oldVals.size() != _newVals.size() && _newVals.size() > 1) || !_oldVals.size())
        return false;

    TableSheet& sheet = m_data.getSheet(atSheet);
    _vCols.setOpenEndIndex(sheet.cols()-1);
    bool success = false;
    std::regex regEx;

    // Search in all columns
    for (size_t j = 0; j < _vCols.size(); j++)
    {
        if (sheet.isValidColumn(_vCols[j]))
        {
            // Get a reference to the current column
            TblColPtr& col = sheet.m_columns[_vCols[j]];

            // Categorical columns only compare their category values
            if (col->m_type == TableColumn::TYPE_CATEGORICAL)
            {
                std::vector<std::string>& vCategories = static_cast<CategoricalColumn*>(col.get())->getCategories();

                for (size_t n = 0; n < _oldVals.size(); n++)
                {
                    const mu::Value& oldVal = _oldVals.get(n);

                    if (!oldVal.isString())
                        continue;

                    if (enableRegEx)
                        regEx = oldVal.getStr();

                    for (size_t i = 0; i < vCategories.size(); i++)
                    {
                        if ((enableRegEx && std::regex_match(vCategories[i], regEx))
                            || (!enableRegEx && oldVal.getStr() == vCategories[i]))
                        {
                            vCategories[i] = _newVals.get(n).printVal();
                            success = true;
                            break;
                        }
                    }
                }

                continue;
            }

            TableColumn::ColumnType targetType = TableColumn::TYPE_NONE;
            std::map<size_t, size_t> idxMap;

            // Find all matches and get the promoted type
            for (size_t n = 0; n < _oldVals.size(); n++)
            {
                const mu::Value& oldVal = _oldVals.get(n);

                if (oldVal.isString() && enableRegEx)
                    regEx = oldVal.getStr();

                for (size_t i = 0; i < col->size(); i++)
                {
                    mu::Value v = col->get(i);

                    if ((enableRegEx && oldVal.isString() && v.isString() && std::regex_match(v.getStr(), regEx))
                        || (!enableRegEx && oldVal == v))
                    {
                        targetType = to_promoted_type(targetType, to_column_type(_newVals.get(n)));
                        idxMap[i] = n;
                        success = true;
                    }
                }
            }

            // Did we find something?
            if (idxMap.size())
            {
                // Ensure that a type is available. Otherwise fall back to
                // strings
                if (targetType == TableColumn::TYPE_NONE)
                    targetType = TableColumn::TYPE_STRING;

                if (col->m_type != targetType)
                {
                    targetType = to_promoted_type(col->m_type, targetType);

                    if (targetType == TableColumn::TYPE_NONE)
                        targetType = TableColumn::TYPE_STRING;

                    convert_if_needed(col, _vCols[j], targetType, true);
                }

                for (const auto& iter : idxMap)
                {
                    col->set(iter.first, _newVals.get(iter.second));
                }
            }
        }
    }

    return success;
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
/// \param sExt std::string
/// \return bool
///
/////////////////////////////////////////////////
bool Table::save(string _sFileName, const string& sTableName, unsigned short nPrecision, std::string sExt)
{
    // Get an instance of the desired file type
    NumeRe::GenericFile* file = NumeRe::getFileByType(_sFileName, sExt);

    // Ensure that a file was created
    if (!file)
        throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, _sFileName, SyntaxError::invalid_position, _sFileName);

    int64_t rows = m_data.rows();
    int64_t cols = m_data.cols();
    int64_t sheets = m_data.sheets();

    // Set the dimensions and the generic information
    // in the file
    file->setDimensions(rows, cols, sheets);
    file->setData(&m_data, rows, cols, sheets);
    file->setTableName(Mangler::demangleName(sTableName));
    file->setTextfilePrecision(nPrecision);

    // If the file type is a NumeRe data file, then
    // we can also set the comment associated with
    // this memory page
    if (file->getExtension() == "ndat" || toLowerCase(sExt) == "ndat")
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
/// \brief This member function deletes a whole
/// range of entries from the memory table.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return void
///
/////////////////////////////////////////////////
void Table::deleteBulk(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet)
{
    if (m_data.empty())
        return;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    bool bHasFirstLine = _vRow.min() == 0;

    m_data.erase(TableIndices(_vRow, _vCol, _vSheet));
    m_meta.modify();

    // Remove all invalid elements and columns
    if (bHasFirstLine)
        shrink(_vCol, _vSheet);
}


/////////////////////////////////////////////////
/// \brief Driver code for simplifying the
/// calculation of various stats using OpenMP, if
/// possible.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \param operation std::vector<StatsLogic>&
/// \return void
///
/////////////////////////////////////////////////
void Table::calculateStats(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet, std::vector<StatsLogic>& operation) const
{
    constexpr size_t MINTHREADCOUNT = 16;
    constexpr size_t MINELEMENTPERCOL = 1000;

    // Only apply multiprocessing, if there are really a lot of
    // elements to process
    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        if (_vSheet[k] < 0 || (size_t)_vSheet[k] > m_data.sheets())
            continue;

        size_t s = _vSheet[k];

        #pragma omp parallel for if (operation.size() >= MINTHREADCOUNT && _vRow.size() >= MINELEMENTPERCOL)
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j], s);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0)
                    continue;

                if (_vRow[i] >= elems)
                {
                    if (_vRow.isExpanded() && _vRow.isOrdered())
                        break;

                    continue;
                }

                try
                {
                    operation[j](read(_vRow[i], _vCol[j], s));
                }
                catch (...)
                {
                    //
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Return min and max value of the table
/// to be presented as part of the variable
/// viewer.
///
/// \return std::pair<double, double>
///
/////////////////////////////////////////////////
std::pair<double, double> Table::minmax() const
{
    if (!m_meta.statsCurrent)
    {
        m_meta.minVal = min(VectorIndex(0, VectorIndex::OPEN_END),
                            VectorIndex(0, VectorIndex::OPEN_END),
                            VectorIndex(0, VectorIndex::OPEN_END)).real();
        m_meta.maxVal = max(VectorIndex(0, VectorIndex::OPEN_END),
                            VectorIndex(0, VectorIndex::OPEN_END),
                            VectorIndex(0, VectorIndex::OPEN_END)).real();
        m_meta.statsCurrent = true;
    }

    return std::make_pair(m_meta.minVal, m_meta.maxVal);
}


/////////////////////////////////////////////////
/// \brief Implementation for the STD multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::std(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    std::complex<double> dAvg = avg(_vRow, _vCol, _vSheet);
    std::complex<double> dStd = 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADDSQSUB, 0.0, dAvg));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
        dStd += val.m_val.as_cmplx();

    return sqrt(dStd / (num(_vRow, _vCol, _vSheet) - 1.0));
}


/////////////////////////////////////////////////
/// \brief Implementation for the AVG multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::avg(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    return sum(_vRow, _vCol, _vSheet) / num(_vRow, _vCol, _vSheet);
}


/////////////////////////////////////////////////
/// \brief Implementation for the MAX multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::max(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    mu::Value dMax = NAN;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_MAX, NAN));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
    {
        if (dMax != dMax || dMax < val.m_val)
            dMax = val.m_val;
    }

    return dMax.as_cmplx();
}


/////////////////////////////////////////////////
/// \brief Implementation for the MIN multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::min(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    mu::Value dMin = NAN;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_MIN, NAN));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
    {
        if (dMin != dMin || dMin > val.m_val)
            dMin = val.m_val;
    }

    return dMin.as_cmplx();
}


/////////////////////////////////////////////////
/// \brief Implementation for the PRD multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::prd(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    std::complex<double> dPrd = 1.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_MULT, 1.0));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
    {
        dPrd *= val.m_val.as_cmplx();
    }

    return dPrd;
}


/////////////////////////////////////////////////
/// \brief Implementation for the SUM multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::sum(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    std::complex<double> dSum = 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADD));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
    {
        dSum += val.m_val.as_cmplx();
    }

    return dSum;
}


/////////////////////////////////////////////////
/// \brief Implementation for the NUM multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::num(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return 0;

    int nInvalid = 0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        int s = _vSheet[k];

        if (s < 0 || (size_t)s >= m_data.sheets())
            continue;

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j], s);

            if (!elems)
            {
                nInvalid += _vRow.size();
                continue;
            }

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0 || _vRow[i] >= elems || !isValidElement(_vRow[i], _vCol[j], s))
                    nInvalid++;
            }
        }
    }

    return (_vRow.size() * _vCol.size() * _vSheet.size()) - nInvalid;
}


/////////////////////////////////////////////////
/// \brief Implementation for the AND multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::and_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    double dRetVal = NAN;

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        if (_vSheet[k] < 0 || (size_t)_vSheet[k] > m_data.sheets())
            continue;

        const TableSheet& sheet = m_data.getSheet(_vSheet[k]);

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = sheet.getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0)
                    continue;

                if (_vRow[i] >= elems)
                {
                    if (_vRow.isExpanded() && _vRow.isOrdered())
                        break;

                    continue;
                }

                if (isnan(dRetVal))
                    dRetVal = 1.0;

                if (!sheet.m_columns[j]->asBool(i))
                    return 0.0;
            }
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
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::or_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        if (_vSheet[k] < 0 || (size_t)_vSheet[k] > m_data.sheets())
            continue;

        const TableSheet& sheet = m_data.getSheet(_vSheet[k]);

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = sheet.getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0)
                    continue;

                if (_vRow[i] >= elems)
                {
                    if (_vRow.isExpanded() && _vRow.isOrdered())
                        break;

                    continue;
                }

                if (sheet.m_columns[j]->asBool(i))
                    return 1.0;
            }
        }
    }

    return 0.0;
}


/////////////////////////////////////////////////
/// \brief Implementation for the XOR multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::xor_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    bool isTrue = false;

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        if (_vSheet[k] < 0 || (size_t)_vSheet[k] > m_data.sheets())
            continue;

        const TableSheet& sheet = m_data.getSheet(_vSheet[k]);

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = sheet.getElemsInColumn(_vCol[j]);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0)
                    continue;

                if (_vRow[i] >= elems)
                {
                    if (_vRow.isExpanded() && _vRow.isOrdered())
                        break;

                    continue;
                }

                if (sheet.m_columns[j]->asBool(i))
                {
                    if (!isTrue)
                        isTrue = true;
                    else
                        return 0.0;
                }
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
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::cnt(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return 0;

    int nInvalid = 0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        int s = _vSheet[k];

        if (s < 0 || (size_t)s >= m_data.sheets())
            continue;

        std::vector<std::complex<double>> vDimLen;

        // Calculate the size in the corresponding direction first
        if (_vCol.size() == 1 && _vRow.size() > 1)
            vDimLen = size(_vCol, VectorIndex(0, VectorIndex::OPEN_END), s, AppDir::COLS);
        else if (_vCol.size() > 1 && _vRow.size() == 1)
            vDimLen = size(_vRow, VectorIndex(0, VectorIndex::OPEN_END), s, AppDir::ROWS);

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            if (_vCol.size() > 1
                && _vRow.size() == 1
                && vDimLen.front().real() <= _vCol[j])
            {
                nInvalid++;
                continue;
            }

            int elems = getElemsInColumn(_vCol[j], s);

            if (!elems)
            {
                // If this goes for columns individual, then count it as empty
                // otherwise it counts for cnt()
                if (_vCol.size() == 1 && _vRow.size() > 1)
                    nInvalid += _vRow.size();

                continue;
            }

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0 || _vRow[i] >= elems)
                    nInvalid++;
            }
        }
    }

    return (_vRow.size() * _vCol.size() * _vSheet.size()) - nInvalid;
}


/////////////////////////////////////////////////
/// \brief Implementation for the NORM multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::norm(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    std::complex<double> dNorm = 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADDSQ));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
    {
        dNorm += val.m_val.as_cmplx();
    }

    return sqrt(dNorm);
}


/////////////////////////////////////////////////
/// \brief Implementation for the CMP multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param sheet size_t
/// \param dRef std::complex<double>
/// \param _nType int
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::cmp(const VectorIndex& _vRow, const VectorIndex& _vCol, size_t sheet, std::complex<double> dRef, int _nType) const
{
    if (m_data.empty() || sheet >= m_data.sheets())
        return NAN;

    _vRow.setOpenEndIndex(m_data.getSheet(sheet).rows()-1);
    _vCol.setOpenEndIndex(m_data.getSheet(sheet).cols()-1);

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

    for (size_t j = 0; j < _vCol.size(); j++)
    {
        if (_vCol[j] < 0)
            continue;

        int elems = getElemsInColumn(_vCol[j], sheet);

        if (!elems)
            continue;

        for (size_t i = 0; i < _vRow.size(); i++)
        {
            if (_vRow[i] < 0)
                continue;

            if (_vRow[i] >= elems)
            {
                if (_vRow.isExpanded() && _vRow.isOrdered())
                    break;

                continue;
            }

            std::complex<double> val = read(_vRow[i], _vCol[j], sheet).getNum().asCF64();

            if (mu::isnan(val))
                continue;

            if (val == dRef)
            {
                if (nType & RETURN_VALUE)
                    return val;

                if (_vRow[0] == _vRow[_vRow.size() - 1])
                    return j+1;

                return i+1;
            }
            else if (nType & RETURN_GE && val.real() > dRef.real())
            {
                if (nType & RETURN_FIRST)
                {
                    if (nType & RETURN_VALUE)
                        return val.real();

                    if (_vRow[0] == _vRow[_vRow.size() - 1])
                        return j+1;

                    return i+1;
                }

                if (nKeep == -1 || val.real() < dKeep)
                {
                    dKeep = val.real();
                    if (_vRow[0] == _vRow[_vRow.size() - 1])
                        nKeep = j;
                    else
                        nKeep = i;
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

                    if (_vRow[0] == _vRow[_vRow.size() - 1])
                        return j+1;

                    return i+1;
                }

                if (nKeep == -1 || val.real() > dKeep)
                {
                    dKeep = val.real();
                    if (_vRow[0] == _vRow[_vRow.size() - 1])
                        nKeep = j;
                    else
                        nKeep = i;
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
        return nKeep+1;
}


/////////////////////////////////////////////////
/// \brief Implementation for the MED multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::med(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    vector<double> vData;

    vData.reserve(_vRow.size()*_vCol.size()*_vSheet.size());

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        int s = _vSheet[k];

        if (s < 0 || (size_t)s >= m_data.sheets())
            continue;

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j], s);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0)
                    continue;

                if (_vRow[i] >= elems)
                {
                    if (_vRow.isExpanded() && _vRow.isOrdered())
                        break;

                    continue;
                }

                std::complex<double> val = read(_vRow[i], _vCol[j], s).getNum().asCF64();

                if (!mu::isnan(val))
                    vData.push_back(val.real());
            }
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
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \param dPct std::complex<double>
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::pct(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet, std::complex<double> dPct) const
{
    if (m_data.empty())
        return NAN;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    if (dPct.real() >= 1 || dPct.real() <= 0)
        return NAN;

    vector<double> vData;

    vData.reserve(_vRow.size()*_vCol.size()*_vSheet.size());

    for (size_t k = 0; k < _vSheet.size(); k++)
    {
        int s = _vSheet[k];

        if (s < 0 || (size_t)s >= m_data.sheets())
            continue;

        for (size_t j = 0; j < _vCol.size(); j++)
        {
            if (_vCol[j] < 0)
                continue;

            int elems = getElemsInColumn(_vCol[j], s);

            if (!elems)
                continue;

            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (_vRow[i] < 0)
                    continue;

                if (_vRow[i] >= elems)
                {
                    if (_vRow.isExpanded() && _vRow.isOrdered())
                        break;

                    continue;
                }

                std::complex<double> val = read(_vRow[i], _vCol[j], s).getNum().asCF64();

                if (!mu::isnan(val))
                    vData.push_back(val.real());
            }
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
/// \brief Implementation for the EXC multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::exc(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    std::complex<double> dAvg = avg(_vRow, _vCol, _vSheet);
    std::complex<double> dExc = 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADDSQSQSUB, 0.0, dAvg));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
        dExc += val.m_val.as_cmplx();

    return dExc / (num(_vRow, _vCol, _vSheet) * intPower(std(_vRow, _vCol, _vSheet), 4)) - 3.0;
}


/////////////////////////////////////////////////
/// \brief Implementation for the SKEW multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::skew(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    if (m_data.empty())
        return NAN;

    std::complex<double> dAvg = avg(_vRow, _vCol, _vSheet);
    std::complex<double> dSkew = 0.0;

    _vRow.setOpenEndIndex(m_data.rows()-1);
    _vCol.setOpenEndIndex(m_data.cols()-1);
    _vSheet.setOpenEndIndex(m_data.sheets()-1);

    std::vector<StatsLogic> vLogic(_vCol.size(), StatsLogic(StatsLogic::OPERATION_ADDCBSUB, 0.0, dAvg));
    calculateStats(_vRow, _vCol, _vSheet, vLogic);

    for (const auto& val : vLogic)
        dSkew += val.m_val.as_cmplx();

    return dSkew / (num(_vRow, _vCol, _vSheet) * intPower(std(_vRow, _vCol, _vSheet), 3));
}


/////////////////////////////////////////////////
/// \brief Implementation for the STDERR multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::stderr_func(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    return std(_vRow, _vCol, _vSheet) / std::sqrt(num(_vRow, _vCol, _vSheet));
}


/////////////////////////////////////////////////
/// \brief Implementation for the RMS multi
/// argument function.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> Table::rms(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    return norm(_vRow, _vCol, _vSheet) / std::sqrt(num(_vRow, _vCol, _vSheet));
}


static std::vector<size_t> size_impl(const TableSheet& currSheet, VectorIndex _everyIdx, VectorIndex _cellsIdx, int dir)
{
    std::vector<size_t> vSizes;

    int rows = currSheet.rows();
    int cols = currSheet.cols();

    int nGridOffset = 2*((dir & Table::GRID) != 0);
    _everyIdx.setOpenEndIndex(dir & Table::ROWS ? rows-1 : cols-1);
    _cellsIdx.setOpenEndIndex(dir & Table::ROWS ? cols-1-nGridOffset : rows-1);

    // Handle simple things first
    if (dir == Table::ALL)
        return std::vector<size_t>({rows, cols});
    else if (dir == Table::GRID)
        return std::vector<size_t>({currSheet.getFilledElemsInColumn(0), currSheet.getFilledElemsInColumn(1)});
    else if (dir & Table::ROWS)
    {
        // Compute the sizes of the table rows
        for (size_t i = 0; i < _everyIdx.size(); i++)
        {
            if (_everyIdx[i] < 0 || _everyIdx[i] >= rows)
                continue;

            for (int j = _cellsIdx.size()-1; j >= 0; j--)
            {
                if (currSheet.m_columns[_cellsIdx[j]+nGridOffset] && currSheet.m_columns[_cellsIdx[j]+nGridOffset]->isValid(_everyIdx[i]))
                {
                    vSizes.push_back(j+1 - nGridOffset);
                    break;
                }
            }
        }

        if (!vSizes.size())
            vSizes.push_back(0);
    }
    else if (dir & Table::COLS)
    {
        // Compute the sizes of the table columns
        for (size_t j = 0; j < _everyIdx.size(); j++)
        {
            if (_everyIdx[j]+nGridOffset < 0 || _everyIdx[j]+nGridOffset >= cols)
                continue;

            if (!currSheet.m_columns[_everyIdx[j]+nGridOffset])
            {
                vSizes.push_back(0);
                continue;
            }

            for (int i = _cellsIdx.size()-1; i >= 0; i--)
            {
                if (currSheet.m_columns[_everyIdx[j]+nGridOffset]->isValid(_cellsIdx[i]))
                {
                    vSizes.push_back(i+1);
                    break;
                }
            }
        }

        if (!vSizes.size())
            vSizes.push_back(0);
    }

    return vSizes;
}


/////////////////////////////////////////////////
/// \brief Implementation of the SIZE multi
/// argument function.
///
/// \param _everyIdx const VectorIndex&
/// \param _everyIdx const VectorIndex&
/// \param _slicesIdx const VectorIndex&
/// \param dir int Bitcomposition of AppDir values
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> Table::size(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, const VectorIndex& _slicesIdx, int dir) const
{
    if (m_data.empty())
        return std::vector<std::complex<double>>(3, 0.0);

    std::vector<size_t> vSizes;

    for (size_t k = 0; k < _slicesIdx.size(); k++)
    {
        int s = _slicesIdx[k];

        if (s < 0 || s >= m_data.sheets())
            continue;

        const TableSheet& currSheet = m_data.getSheet(s);

        std::vector<size_t> sheetSizes = size_impl(currSheet, _everyIdx, _cellsIdx, dir);

        for (size_t i = 0; i < sheetSizes.size(); i++)
        {
            if (i >= vSizes.size())
                vSizes.push_back(sheetSizes[i]);
            else
                vSizes[i] = std::max(vSizes[i], sheetSizes[i]);
        }
    }

    if (!vSizes.size())
        return std::vector<std::complex<double>>(3, 0.0);

    std::vector<std::complex<double>> sizeResult(vSizes.size());

    for (size_t i = 0; i < vSizes.size(); i++)
    {
        sizeResult[i] = vSizes[i];
    }

    if ((dir == ALL || dir == GRID) && _slicesIdx.size() == m_data.sheets())
        sizeResult.push_back(m_data.sheets());

    return sizeResult;
}


/////////////////////////////////////////////////
/// \brief Implementation of the MINPOS multi
/// argument function.
///
/// \param _everyIdx const VectorIndex&
/// \param _cellsIdx const VectorIndex&
/// \param sheet size_t
/// \param dir int
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> Table::minpos(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, size_t sheet, int dir) const
{
    if (m_data.empty() || sheet >= m_data.sheets() || !m_data.getSheet(sheet).rows())
        return std::vector<std::complex<double>>(1, NAN);

    int rows = m_data.getSheet(sheet).rows();
    int cols = m_data.getSheet(sheet).cols();

    int nGridOffset = 2*((dir & GRID) != 0);
    _everyIdx.setOpenEndIndex(dir & COLS ? cols-1 : rows-1);
    _cellsIdx.setOpenEndIndex(dir & COLS ? rows-1 : cols-1-nGridOffset);

    // If a grid is required, get the grid dimensions
    // of this table
    if (nGridOffset)
    {
        std::vector<std::complex<double>> vSize = size(VectorIndex(), VectorIndex(), sheet, GRID);
        rows = vSize.front().real();
        cols = vSize.back().real()+nGridOffset; // compensate the offset
    }

    // A special case for the columns. We will compute the
    // results for ALL and GRID using the results for LINES
    if (dir & COLS)
    {
        std::vector<std::complex<double>> vPos;

        for (size_t j = 0; j < _everyIdx.size(); j++)
        {
            if (_everyIdx[j]+nGridOffset < 0 || _everyIdx[j]+nGridOffset >= cols)
                continue;

            vPos.push_back(cmp(_cellsIdx, VectorIndex(_everyIdx[j]+nGridOffset), sheet,
                               min(_cellsIdx, VectorIndex(_everyIdx[j]+nGridOffset), VectorIndex(sheet)), 0));
        }

        if (!vPos.size())
            vPos.push_back(NAN);

        return vPos;
    }

    std::vector<std::complex<double>> vPos;
    double dMin = NAN;
    size_t pos = 0;
    VectorIndex _cellsTemp = _cellsIdx;
    _cellsTemp.apply_offset(nGridOffset);

    // Compute the results for LINES and find as
    // well the global minimal value, which will be used
    // for GRID and ALL
    for (size_t i = 0; i < _everyIdx.size(); i++)
    {
        if (_everyIdx[i] < 0 || _everyIdx[i] >= rows)
            continue;

        vPos.push_back(cmp(VectorIndex(_everyIdx[i]), _cellsTemp, sheet,
                           min(VectorIndex(_everyIdx[i]), _cellsTemp, VectorIndex(sheet)), 0));

        if (isnan(dMin) || dMin > read(_everyIdx[i], intCast(vPos.back())-1, sheet).getNum().asF64())
        {
            dMin = read(_everyIdx[i], intCast(vPos.back())-1, sheet).getNum().asF64();
            pos = i;
        }
    }

    if (!vPos.size())
        return std::vector<std::complex<double>>(1, NAN);

    // Use the global minimal value for ALL and GRID
    if (dir == ALL || dir == GRID)
        return std::vector<std::complex<double>>({_everyIdx[pos]+1, vPos[pos]});

    return vPos;
}


/////////////////////////////////////////////////
/// \brief Implementation of the MAXPOS multi
/// argument function.
///
/// \param _everyIdx const VectorIndex&
/// \param _cellsIdx const VectorIndex&
/// \param sheet size_t
/// \param dir int
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> Table::maxpos(const VectorIndex& _everyIdx, const VectorIndex& _cellsIdx, size_t sheet, int dir) const
{
    if (m_data.empty() || sheet >= m_data.sheets() || !m_data.getSheet(sheet).rows())
        return std::vector<std::complex<double>>(1, NAN);

    int rows = m_data.getSheet(sheet).rows();
    int cols = m_data.getSheet(sheet).cols();

    int nGridOffset = 2*((dir & GRID) != 0);
    _everyIdx.setOpenEndIndex(dir & COLS ? cols-1 : rows-1);
    _cellsIdx.setOpenEndIndex(dir & COLS ? rows-1 : cols-1-nGridOffset);

    // If a grid is required, get the grid dimensions
    // of this table
    if (nGridOffset)
    {
        std::vector<std::complex<double>> vSize = size(VectorIndex(), VectorIndex(), sheet, GRID);
        rows = vSize.front().real();
        cols = vSize.back().real()+nGridOffset; // compensate the offset
    }

    // A special case for the columns. We will compute the
    // results for ALL and GRID using the results for LINES
    if (dir & COLS)
    {
        std::vector<std::complex<double>> vPos;

        for (size_t j = 0; j < _everyIdx.size(); j++)
        {
            if (_everyIdx[j]+nGridOffset < 0 || _everyIdx[j]+nGridOffset >= cols)
                continue;

            vPos.push_back(cmp(_cellsIdx, VectorIndex(_everyIdx[j]+nGridOffset), sheet,
                               max(_cellsIdx, VectorIndex(_everyIdx[j]+nGridOffset), VectorIndex(sheet)), 0));
        }

        if (!vPos.size())
            vPos.push_back(NAN);

        return vPos;
    }

    std::vector<std::complex<double>> vPos;
    double dMax = NAN;
    size_t pos = 0;
    VectorIndex _cellsTemp = _cellsIdx;
    _cellsTemp.apply_offset(nGridOffset);

    // Compute the results for LINES and find as
    // well the global maximal value, which will be used
    // for GRID and ALL
    for (size_t i = 0; i < _everyIdx.size(); i++)
    {
        if (_everyIdx[i] < 0 || _everyIdx[i] >= rows)
            continue;

        vPos.push_back(cmp(VectorIndex(_everyIdx[i]), _cellsTemp, sheet,
                           max(VectorIndex(_everyIdx[i]), _cellsTemp, VectorIndex(sheet)), 0));

        if (isnan(dMax) || dMax < read(_everyIdx[i], intCast(vPos.back())-1, sheet).getNum().asF64())
        {
            dMax = read(_everyIdx[i], intCast(vPos.back())-1, sheet).getNum().asF64();
            pos = i;
        }
    }

    if (!vPos.size())
        return std::vector<std::complex<double>>(1, NAN);

    // Use the global maximal value for ALL and GRID
    if (dir == ALL || dir == GRID)
        return std::vector<std::complex<double>>({_everyIdx[pos]+1, vPos[pos]});

    return vPos;
}


/////////////////////////////////////////////////
/// \brief Static helper function to ensure that
/// two doubles are actually close enough to be
/// considered equal.
///
/// \param d1 double
/// \param d2 double
/// \return bool
///
/////////////////////////////////////////////////
static bool closeEnough(double d1, double d2)
{
    return abs(d1 - d2) < 1e-16 * max(1.0, min(abs(d1), abs(d2)));
}


/////////////////////////////////////////////////
/// \brief Static helper function to ensure that
/// two complex values are actually close enough
/// to be considered equal.
///
/// \param v1 const std::complex<double>&
/// \param v2 const std::complex<double>&
/// \return bool
///
/////////////////////////////////////////////////
static bool closeEnough(const std::complex<double>& v1, const std::complex<double>& v2)
{
    return closeEnough(v1.real(), v2.real()) && closeEnough(v1.imag(), v2.imag());
}


/////////////////////////////////////////////////
/// \brief Finds the columns IDs, whose headlines
/// match to the passed strings. Can return
/// multiple column IDs per string.
///
/// \param vColNames const std::vector<std::string>&
/// \param inSheet size_t
/// \param enableRegEx bool
/// \param autoCreate bool
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
std::vector<size_t> Table::findCols(const std::vector<std::string>& vColNames, size_t inSheet, bool enableRegEx, bool autoCreate)
{
    std::vector<size_t> vColIndices;

    if (inSheet >= m_data.sheets() || !m_data.getSheet(inSheet).rows())
        return vColIndices;

    TableSheet& sheet = m_data.getSheet(inSheet);

    for (const auto& sName : vColNames)
    {
        bool found = false;
        std::regex regEx;

        for (size_t i = 0; i < sheet.cols(); i++)
        {
            if (enableRegEx)
            {
                if (!i)
                    regEx = sName;

                if (sheet.m_columns[i] && std::regex_match(sheet.m_columns[i]->m_sHeadLine, regEx))
                    vColIndices.push_back(i+1);
            }
            else
            {
                if (sheet.m_columns[i] && sheet.m_columns[i]->m_sHeadLine == sName)
                {
                    vColIndices.push_back(i+1);
                    found = true;
                }
            }
        }

        if (!found && !enableRegEx && autoCreate)
        {
            int pos = sheet.cols();
            sheet.reserve(pos+1);
            sheet.m_columns[pos].reset(new LogicalColumn);
            sheet.m_columns[pos]->m_sHeadLine = sName;
            g_logger.info("Created new column " + toString(pos+1) + " for '" + sName + "'");
            vColIndices.push_back(pos+1);
            m_meta.modify();
        }
    }

    return vColIndices;
}


/////////////////////////////////////////////////
/// \brief Finds the sheet IDs, whose names
/// match to the passed strings. Can return
/// multiple sheet IDs per string.
///
/// \param vSheetNames const std::vector<std::string>&
/// \param enableRegEx bool
/// \param autoCreate bool
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
std::vector<size_t> Table::findSheets(const std::vector<std::string>& vSheetNames, bool enableRegEx, bool autoCreate)
{
    std::vector<size_t> vSheetIndices;

    if (m_data.empty())
        return vSheetIndices;

    for (const auto& sName : vSheetNames)
    {
        bool found = false;
        std::regex regEx;

        for (size_t i = 0; i < m_data.sheets(); i++)
        {
            if (enableRegEx)
            {
                if (!i)
                    regEx = sName;

                if (std::regex_match(m_data.m_sheets[i]->m_sheetName, regEx))
                    vSheetIndices.push_back(i+1);
            }
            else
            {
                if (m_data.m_sheets[i]->m_sheetName == sName)
                {
                    vSheetIndices.push_back(i+1);
                    found = true;
                }
            }
        }

        if (!found && !enableRegEx && autoCreate)
        {
            int pos = m_data.sheets();
            m_data.addSheet(sName, 0);
            g_logger.info("Created new sheet " + toString(pos+1) + " for '" + sName + "'");
            vSheetIndices.push_back(pos+1);
            m_meta.modify();
        }
    }

    return vSheetIndices;
}


/////////////////////////////////////////////////
/// \brief Counts all values in the selected
/// columns, which match to the passed values
/// (either numerically or string values) and
/// returns the corresponding sums.
///
/// \param _vCols const VectorIndex&
/// \param inSheet size_t
/// \param vValues const mu::Array&
/// \param enableRegEx bool
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
std::vector<size_t> Table::countIfEqual(const VectorIndex& _vCols, size_t inSheet, const mu::Array& vValues, bool enableRegEx) const
{
    std::vector<size_t> vCounted;

    if (inSheet >= m_data.sheets())
        return vCounted;

    const TableSheet& sheet = m_data.getSheet(inSheet);

    for (size_t j = 0; j < _vCols.size(); j++)
    {
        if (!sheet.isValidColumn(_vCols[j]))
            continue;

        for (size_t n = 0; n < vValues.size(); n++)
        {
            const mu::Value& val = vValues.get(n);
            size_t count = 0;
            std::regex regEx;
            bool isNum = val.isNumerical();

            if (!isNum && enableRegEx)
                regEx = val.getStr();

            for (size_t i = 0; i < sheet.m_columns[_vCols[j]]->size(); i++)
            {
                if (isNum)
                {
                    if (closeEnough(sheet.m_columns[_vCols[j]]->getValue(i), val.getNum().asCF64()))
                        count++;
                }
                else
                {
                    if ((enableRegEx && std::regex_match(sheet.m_columns[_vCols[j]]->getValueAsInternalString(i), regEx))
                        || (!enableRegEx && sheet.m_columns[_vCols[j]]->getValueAsInternalString(i) == val.getStr()))
                        count++;
                }
            }

            vCounted.push_back(count);
        }
    }

    return vCounted;
}


/////////////////////////////////////////////////
/// \brief Determines the positions of all
/// elements, which correspond to the passed
/// values (either numerically or string values),
/// and returns them as an index.
///
/// \param col size_t
/// \param inSheet size_t
/// \param vValues const mu::Array&
/// \param enableRegEx bool
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array Table::getIndex(size_t col, size_t inSheet, const mu::Array& vValues, bool enableRegEx) const
{
    mu::Array indices;

    if (inSheet >= m_data.sheets() || !m_data.getSheet(inSheet).isValidColumn(col))
    {
        indices.push_back(NAN);
        return indices;
    }

    const TableSheet& sheet = m_data.getSheet(inSheet);

    for (size_t n = 0; n < vValues.size(); n++)
    {
        const mu::Value& val = vValues.get(n);
        mu::Array idxSet;
        std::regex regEx;
        bool isNum = val.isNumerical();

        if (!isNum && enableRegEx)
            regEx = val.getStr();

        for (size_t i = 0; i < sheet.m_columns[col]->size(); i++)
        {
            if (isNum)
            {
                if (closeEnough(sheet.m_columns[col]->getValue(i), val.getNum().asCF64()))
                    idxSet.push_back(i+1);
            }
            else
            {
                if ((enableRegEx && std::regex_match(sheet.m_columns[col]->getValueAsInternalString(i), regEx))
                    || (!enableRegEx && sheet.m_columns[col]->getValueAsInternalString(i) == val.getStr()))
                    idxSet.push_back(i+1);
            }
        }

        if (vValues.size() > 1)
        {
            if (idxSet.size())
                indices.push_back(idxSet);
            else
                indices.push_back(mu::Value());
        }
        else
            indices = idxSet;
    }

    if (!indices.size())
        indices.push_back(NAN);

    return indices;
}

/////////////////////////////////////////////////
/// \brief Calculates the simples form of a ANOVA
/// F test using Type 1 Sum of Squares (relevant for 2+ anova)
///
/// \param colCategories const VectorIndex&
/// \param colValues size_t
/// \param _vIndex const VectorIndex&
/// \param significance double
/// \return AnovaResult
///
/////////////////////////////////////////////////
std::vector<AnovaResult> Table::getAnova(const VectorIndex& colCategories, size_t colValues, const VectorIndex& _vIndex, size_t inSheet, double significance) const
{
    if (significance >= 1.0
        || significance <= 0.0
        || inSheet >= m_data.sheets())
    {
        AnovaResult res;
        res.m_FRatio = NAN;
        return std::vector<AnovaResult> {res};
    }

    const TableSheet& sheet = m_data.getSheet(inSheet);

    size_t col_size = sheet.getElemsInColumn(colValues);

    for (size_t i = 0; i < colCategories.size(); i++)
    {
        if (!sheet.isValidColumn(colCategories[i])
            || sheet.m_columns[colCategories[i]]->m_type != TableColumn::TYPE_CATEGORICAL
            || (size_t)sheet.getElemsInColumn(colCategories[i]) != col_size)
        {
            AnovaResult res;
            res.m_FRatio = NAN;
            return std::vector<AnovaResult> {res};
        }
    }

    colCategories.setOpenEndIndex(sheet.cols()-1);
    _vIndex.setOpenEndIndex(col_size-1);

    // We need to eliminate invalid values from the index vector first
    std::vector<int> vExpanded = _vIndex.getVector();
    auto iter = vExpanded.begin();

    while (iter != vExpanded.end())
    {
        // Any invalids in the numerical values colum n?
        if (!sheet.m_columns[colValues]->get(*iter).isValid())
        {
            iter = vExpanded.erase(iter);
            continue;
        }

        bool erased = false;

        // Any invalids in the categorical columns?
        for (size_t i = 0; i < colCategories.size(); i++)
        {
            if (!sheet.m_columns[colCategories[i]]->get(*iter).isValid())
            {
                iter = vExpanded.erase(iter);
                erased = true;
                break;
            }
        }

        if (!erased)
            iter++;
    }

    // Create a new index vector
    VectorIndex vPrepared(vExpanded);

    Table _mem(colCategories.size()+1);
    TableSheet& targetSheet = _mem.m_data.getSheet(0);
    targetSheet.m_columns[0].reset(sheet.m_columns[colValues]->copy(vPrepared));

    for (size_t i = 0; i < colCategories.size(); i++)
    {
        targetSheet.m_columns[i+1].reset(sheet.m_columns[colCategories[i]]->copy(vPrepared));
    }

    std::vector<std::vector<std::string>> factors;

    for (size_t i = 0; i < colCategories.size(); i++)
    {
        factors.push_back(static_cast<CategoricalColumn*>(targetSheet.m_columns[i+1].get())->getCategories());
    }

    AnovaCalculationStructure ft = AnovaCalculationStructure();
    ft.buildTree(factors, &_mem, significance);
    ft.calculateResults();
    return ft.getResults();
}


/////////////////////////////////////////////////
/// \brief check if value type vector is already in a vector of vectors
///
/// \param newVector const std::vector<std::complex<double>>&
/// \param centroids const std::vector<std::vector<std::complex<double>>>&
/// \return bool
///
/////////////////////////////////////////////////
static bool isUnique(const std::vector<std::complex<double>>& newVector, const std::vector<std::vector<std::complex<double>>>& centroids)
{
    return std::none_of(centroids.begin(), centroids.end(),
                        [&newVector](const std::vector<std::complex<double>>& centroid)
                        { return newVector == centroid; });
}


/////////////////////////////////////////////////
/// \brief Calculate L2 Norm of 2 given value type vectors
///
/// \param vec1 const std::vector<std::complex<double>>&
/// \param vec2 const std::vector<std::complex<double>>&
/// \return double
///
/////////////////////////////////////////////////
static double calculateL2Norm(const std::vector<std::complex<double>>& vec1, const std::vector<std::complex<double>>& vec2)
{
    // for norm of omplex => z * conj(z)
    // conjugate a complex number z = a + bi -> z* = a - bi

    std::complex<double> sum = 0.0;
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        sum += (vec1[i] - vec2[i]) * (conj(vec1[i] - vec2[i]));
    }

    // no need for sqrt in our case
    return sum.real();
}


/////////////////////////////////////////////////
/// \brief get all Indices of elements with given value
///
/// \param vec const std::vector<std::complex<double>>&
/// \param value std::complex<double>
/// \return std::vector<int>
///
/////////////////////////////////////////////////
static std::vector<int> findIndicesOfValue(const std::vector<std::complex<double>>& vec, std::complex<double> value)
{
    std::vector<int> indices;
    auto it = vec.begin();

    while ((it = std::find(it, vec.end(), value)) != vec.end())
    {
        indices.push_back(std::distance(vec.begin(), it));
        ++it;  // Move past the last found element
    }

    return indices;
}


/////////////////////////////////////////////////
/// \brief parse string to KmeansInit enum
///
/// \param init_type const std::string&
/// \return Memory::KmeansInit
///
/////////////////////////////////////////////////
Table::KmeansInit Table::stringToKmeansInit(const std::string& init_type)
{
    if (init_type == "random")
        return INIT_RANDOM;
    else if (init_type == "kmeans++")
        return INIT_KMEANSPP;

    return INVALID;
}

/////////////////////////////////////////////////
/// \brief calculate kmeans
///
/// \param columns const VectorIndex&
/// \param inSheet size_t
/// \param nClusters size_t
/// \param maxIterations size_t
/// \param init_method Memory::KmeansInit
/// \return KMeansResult
///
/////////////////////////////////////////////////
KMeansResult Table::getKMeans(const VectorIndex& columns, size_t inSheet, size_t nClusters, size_t maxIterations, Table::KmeansInit init_method) const
{
    if (inSheet >= m_data.sheets())
        return KMeansResult();

    const TableSheet& sheet = m_data.getSheet(inSheet);

    for (size_t i = 0; i < columns.size(); i++)
    {
        if (!sheet.isValidColumn(columns[i])                // check if column does have data
            || sheet.getElemsInColumn(columns[0]) != sheet.getElemsInColumn(columns[i]))      // All columns should have same size
            return KMeansResult();
    }

    size_t col_size = sheet.getElemsInColumn(columns[0]);

    if (col_size < nClusters || !isValueLike(columns, VectorIndex(inSheet)))
        return KMeansResult();

    std::vector<std::complex<double>> clusters(col_size, 0);
    std::vector<std::vector<std::complex<double>>> centroids;

    // Random generator
    std::mt19937& mt = getRandGenInstance();

    // Step 1 Initialization: Calculate starting centroids
    if (init_method == INIT_KMEANSPP)
    {
        // Run KMeans ++ init
        // https://www.geeksforgeeks.org/ml-k-means-algorithm/

        // K++ Step 1: randomly select first centroid
        size_t randinit = mt()%col_size;
        std::vector<std::complex<double>> new_value = read(VectorIndex(randinit), columns, VectorIndex(inSheet)).as_cmplx_vector();
        centroids.push_back(new_value);

        for (size_t i = 1; i < nClusters; i++)
        {
            // K++ Step 2: calculate distance to nearest centroid
            std::vector<double> distance_vec;

            for (size_t i = 0; i < col_size; i++)
            {
                std::vector<std::complex<double>> value = read(VectorIndex(i), columns, VectorIndex(inSheet)).as_cmplx_vector();
                double dis = calculateL2Norm(value, centroids[0]);
                size_t idx = 0;

                for (size_t j = 1; j < centroids.size(); j++)
                {
                    double dis2 = calculateL2Norm(value, centroids[j]);

                    if (dis2 < dis)
                    {
                        dis = dis2;
                        idx = j;
                    }
                }

                distance_vec.push_back(dis);

                if (clusters[i] != std::complex<double>(idx))
                    clusters[i] = idx;
            }

            // K++ Step 3: add new centroid at point with highest distance to nearest neighbour
            size_t max_elem_idx = std::distance(distance_vec.begin(), std::max_element(distance_vec.begin(), distance_vec.end()));
            std::vector<std::complex<double>> new_value = read(VectorIndex(max_elem_idx), columns, VectorIndex(inSheet)).as_cmplx_vector();
            centroids.push_back(new_value);
        }
    }
    else if (init_method == INIT_RANDOM || init_method == INVALID)
    {
        // Run standard Init: Random Seeds
        for (size_t i = 0; i < nClusters; )
        {
            size_t randinit = mt() % col_size;
            std::vector<std::complex<double>> new_value = read(VectorIndex(randinit), columns, VectorIndex(inSheet)).as_cmplx_vector();

            if (isUnique(new_value, centroids))
            {
                centroids.push_back(new_value);
                i++;
            }
        }
    }

    long double inertia = 0;

    for (size_t iteration = 0; iteration < maxIterations; iteration++)
    {
        // Step 2 assign points to closest Cluster centroid
        size_t change = 0;
        inertia = 0;

        for (size_t i = 0; i < col_size; i++)
        {
            std::vector<std::complex<double>> value = read(VectorIndex(i), columns, VectorIndex(inSheet)).as_cmplx_vector();
            double min_distance = calculateL2Norm(value, centroids[0]);
            size_t idx = 0;

            for (size_t j = 1; j < nClusters; j++)
            {
                double curr_distance = calculateL2Norm(value, centroids[j]);

                if (curr_distance < min_distance)
                {
                    min_distance = curr_distance;
                    idx = j;
                }
            }

            idx++;

            if (clusters[i] != std::complex<double>(idx))
            {
                clusters[i] = idx;
                change++;
            }

            inertia += intPower(min_distance,2);
        }

        // stop criteria: all points remain in same cluster
        if (change == 0)
            break;

        // Step 3 calculate new clusters
        change = 0;

        for (size_t i = 0; i < nClusters; i++)
        {
            VectorIndex indices(findIndicesOfValue(clusters, std::complex<double>(i+1)));

            for (size_t elemIdx = 0; elemIdx < columns.size(); elemIdx++)
            {
                std::complex<double> new_val = avg(indices, VectorIndex(elemIdx), VectorIndex(inSheet));

                if (!closeEnough(centroids[i][elemIdx], new_val))
                {
                    centroids[i][elemIdx] = new_val;
                    change++;
                }
            }
        }

        // stop criteria: all centroids stay same
        if (change == 0)
            break;
    }

    KMeansResult res;
    res.cluster_labels = clusters;
    res.inertia = inertia;
    return res;
}

/////////////////////////////////////////////////
/// \brief Implements the cov() table method and
/// calculates the covariance of the two selected
/// columns.
///
/// \param col1 size_t
/// \param _vIndex1 const VectorIndex&
/// \param col2 size_t
/// \param _vIndex2 const VectorIndex&
/// \param inSheet size_t
/// \return double
///
/////////////////////////////////////////////////
double Table::getCovariance(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2, size_t inSheet) const
{
    if (inSheet >= m_data.sheets())
        return NAN;

    _vIndex1.setOpenEndIndex(getElemsInColumn(col1, inSheet)-1);
    _vIndex2.setOpenEndIndex(getElemsInColumn(col2, inSheet)-1);

    size_t minSize = std::min(_vIndex1.size(), _vIndex2.size());

    std::complex<double> vAvg1 = avg(_vIndex1.subidx(0, minSize), VectorIndex(col1), VectorIndex(inSheet));
    std::complex<double> vAvg2 = avg(_vIndex2.subidx(0, minSize), VectorIndex(col2), VectorIndex(inSheet));

    double vCov = 0.0;

    for (size_t i = 0; i < minSize; i++)
    {
        vCov += std::real((read(_vIndex1[i], col1, inSheet).getNum().asCF64() - vAvg1)
                            * std::conj(read(_vIndex2[i], col2, inSheet).getNum().asCF64() - vAvg2));
    }

    return vCov / (minSize-1.0);
}


/////////////////////////////////////////////////
/// \brief Implements the pcorr() table method
/// and calculates the pearson correlation
/// coefficient of the two selected columns.
///
/// \param col1 size_t
/// \param _vIndex1 const VectorIndex&
/// \param col2 size_t
/// \param _vIndex2 const VectorIndex&
/// \param inSheet size_t
/// \return double
///
/////////////////////////////////////////////////
double Table::getPearsonCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2, size_t inSheet) const
{
    if (inSheet >= m_data.sheets())
        return NAN;

    _vIndex1.setOpenEndIndex(getElemsInColumn(col1, inSheet)-1);
    _vIndex2.setOpenEndIndex(getElemsInColumn(col2, inSheet)-1);

    size_t minSize = std::min(_vIndex1.size(), _vIndex2.size());

    return getCovariance(col1, _vIndex1, col2, _vIndex2, inSheet)
        / (std(_vIndex1.subidx(0, minSize), VectorIndex(col1), VectorIndex(inSheet)).real()
            * std(_vIndex2.subidx(0, minSize), VectorIndex(col2), VectorIndex(inSheet)).real());
}


/////////////////////////////////////////////////
/// \brief Implements the scorr() table method
/// and calculates the spearman correlation
/// coefficient of the two selected columns.
///
/// \param col1 size_t
/// \param _vIndex1 const VectorIndex&
/// \param col2 size_t
/// \param _vIndex2 const VectorIndex&
/// \param inSheet size_t
/// \return double
///
/////////////////////////////////////////////////
double Table::getSpearmanCorr(size_t col1, const VectorIndex& _vIndex1, size_t col2, const VectorIndex& _vIndex2, size_t inSheet) const
{
    if (inSheet >= m_data.sheets())
        return NAN;

    _vIndex1.setOpenEndIndex(getElemsInColumn(col1, inSheet)-1);
    _vIndex2.setOpenEndIndex(getElemsInColumn(col2, inSheet)-1);

    size_t minSize = std::min(_vIndex1.size(), _vIndex2.size());

    Table _mem(2);

    _mem.m_data.m_sheets[0]->m_columns[0].reset(new F64ValueColumn(minSize));
    _mem.m_data.m_sheets[0]->m_columns[0]->setValue(VectorIndex(0, minSize), getRank(col1, _vIndex1.subidx(0, minSize), inSheet, RANK_FRACTIONAL));
    _mem.m_data.m_sheets[0]->m_columns[1].reset(new F64ValueColumn(minSize));
    _mem.m_data.m_sheets[0]->m_columns[1]->setValue(VectorIndex(0, minSize), getRank(col2, _vIndex2.subidx(0, minSize), inSheet, RANK_FRACTIONAL));

    return _mem.getPearsonCorr(0, VectorIndex(0, VectorIndex::OPEN_END), 1, VectorIndex(0, VectorIndex::OPEN_END), 0);
}


/////////////////////////////////////////////////
/// \brief Evaluate the identical ranked values
/// according the selected ranking strategy.
///
/// \param vRank std::vector<double>&
/// \param nEqualRanks size_t&
/// \param _strat Memory::RankingStrategy
/// \return void
///
/////////////////////////////////////////////////
static void evaluateRankingStrategy(std::vector<double>& vRank, size_t& nEqualRanks, Table::RankingStrategy _strat)
{
    switch (_strat)
    {
        case Table::RANK_DENSE:
            vRank.insert(vRank.end(), nEqualRanks, vRank.back());
            vRank.push_back(vRank.back()+1.0);
            break;
        case Table::RANK_COMPETETIVE:
            vRank.insert(vRank.end(), nEqualRanks, vRank.back());
            vRank.push_back(vRank.back()+(nEqualRanks+1.0));
            break;
        case Table::RANK_FRACTIONAL:
        {
            double val = vRank.back();
            vRank.pop_back();
            vRank.insert(vRank.end(), nEqualRanks+1, val+0.5*nEqualRanks);
            vRank.push_back(val+(nEqualRanks+1.0));
            break;
        }
    }

    nEqualRanks = 0;
}


/////////////////////////////////////////////////
/// \brief Rank the selected column according the
/// selected ranking strategy.
///
/// \param col size_t
/// \param _vIndex const VectorIndex&
/// \param inSheet size_t
/// \param _strat Memory::RankingStrategy
/// \return std::vector<double>
///
/////////////////////////////////////////////////
std::vector<double> Table::getRank(size_t col, const VectorIndex& _vIndex, size_t inSheet, Table::RankingStrategy _strat) const
{
    if (inSheet >= m_data.sheets() || !m_data.getSheet(inSheet).isValidColumn(col))
        return {NAN};

    _vIndex.setOpenEndIndex(getElemsInColumn(col, inSheet)-1);

    TableSheet sheet("", 1);
    sheet.m_columns.back().reset(m_data.getSheet(inSheet).m_columns[col]->copy(_vIndex));

    std::vector<int> vIndex = sheet.sortElements(VectorIndex(0, sheet.rows()-1), VectorIndex(0), "-index");
    std::vector<double> vRank(1, 1.0);
    size_t nEqualRanks = 0;

    for (size_t i = 1; i < vIndex.size(); i++)
    {
        // Indices are already 1-based and NANs are always at the end
        if (!sheet.m_columns.front()->get(vIndex[i]-1).isValid())
            vRank.push_back(NAN);
        else if (sheet.m_columns.front()->get(vIndex[i]-1) != sheet.m_columns.front()->get(vIndex[i-1]-1))
        {
            if (nEqualRanks)
                evaluateRankingStrategy(vRank, nEqualRanks, _strat);
            else
                vRank.push_back(vRank.back()+1.0);
        }
        else
            nEqualRanks++;
    }

    if (nEqualRanks)
    {
        evaluateRankingStrategy(vRank, nEqualRanks, _strat);
        vRank.pop_back();
    }

    std::vector<double> vRankReordered(vRank.size());

    for (size_t i = 0; i < vIndex.size(); i++)
    {
        vRankReordered[vIndex[i]-1] = vRank[i];
    }

    return vRankReordered;
}


/////////////////////////////////////////////////
/// \brief Calculate the standardized values of
/// the selected column.
///
/// \param col size_t
/// \param _vIndex const VectorIndex&
/// \param inSheet size_t
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
std::vector<std::complex<double>> Table::getZScore(size_t col, const VectorIndex& _vIndex, size_t inSheet) const
{
    std::vector<std::complex<double>> vZScore;

    if (inSheet >= m_data.sheets())
        return vZScore;

    _vIndex.setOpenEndIndex(getElemsInColumn(col, inSheet)-1);

    std::complex<double> avgVal = avg(_vIndex, VectorIndex(col), VectorIndex(inSheet));
    std::complex<double> stdVal = std(_vIndex, VectorIndex(col), VectorIndex(inSheet));

    for (size_t i = 0; i < _vIndex.size(); i++)
    {
        vZScore.push_back((read(_vIndex[i], col, inSheet).getNum().asCF64() - avgVal) / stdVal);
    }

    return vZScore;
}


/////////////////////////////////////////////////
/// \brief Calculate the number of elements per
/// bin in the selected column.
///
/// \param col size_t
/// \param inSheet size_t
/// \param nBins size_t
/// \return std::vector<int64_t>
///
/////////////////////////////////////////////////
std::vector<int64_t> Table::getBins(size_t col, size_t inSheet, size_t nBins) const
{
    std::vector<int64_t> vBins;

    // Ensure that we have data
    if (inSheet >= m_data.sheets() || !m_data.getSheet(inSheet).isValidColumn(col))
    {
        vBins.resize(!nBins || nBins >= m_data.getSheet(inSheet).rows() ? 1 : nBins, -1);
        return vBins;
    }

    const TableSheet& sheet = m_data.getSheet(inSheet);

    // Get the column type
    TableColumn::ColumnType type = sheet.m_columns[col]->m_type;

    // Handle different column types differently
    if (type == TableColumn::TYPE_CATEGORICAL)
    {
        // We use the categories as bins and ignore user settings
        std::vector<std::string> vCategories = static_cast<CategoricalColumn*>(sheet.m_columns[col].get())->getCategories();
        nBins = vCategories.size();
        vBins.resize(nBins, 0);

        for (size_t i = 0; i < sheet.m_columns[col]->size(); i++)
        {
            if (sheet.m_columns[col]->getValue(i).real() > 0.0)
                vBins[sheet.m_columns[col]->getValue(i).real()-1]++;
        }
    }
    else if (type == TableColumn::TYPE_LOGICAL)
    {
        // We use the logical values as bins and ignore user settings
        nBins = 2;
        vBins.resize(nBins, 0);

        for (size_t i = 0; i < sheet.m_columns[col]->size(); i++)
        {
            if (sheet.m_columns[col]->getValue(i) == 1.0)
                vBins[0]++;
            else if (sheet.m_columns[col]->getValue(i) == 0.0)
                vBins[1]++;
        }
    }
    else if (type == TableColumn::TYPE_STRING)
        vBins.resize(!nBins || nBins >= sheet.m_columns[col]->size() ? 1 : nBins, -1); // Strings are not binnable
    else
    {
        // Calculate the bins following the (simple) Sturges rule
        if (!nBins || nBins >= sheet.m_columns[col]->size() )
            nBins = (int)std::rint(1.0 + 3.3 * std::log10(num(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(col), VectorIndex(inSheet)).real()));

        // Calculate min, max and range of the data. We'll only consider real values
        vBins.resize(nBins, 0);
        double dMin = min(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(col), VectorIndex(inSheet)).real();
        double dMax = max(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(col), VectorIndex(inSheet)).real();
        double dRange = dMax - dMin;

        for (size_t i = 0; i < sheet.m_columns[col]->size(); i++)
        {
            if (!mu::isnan(sheet.m_columns[col]->getValue(i)))
                vBins[std::min(nBins-1.0, nBins * (sheet.m_columns[col]->getValue(i).real()-dMin) / dRange)]++;
        }
    }

    return vBins;
}


/////////////////////////////////////////////////
/// \brief This method is the retouching main
/// method. It will redirect the control into the
/// specialized member functions.
///
/// \param _vRow VectorIndex
/// \param _vCol VectorIndex
/// \param inSheet size_t
/// \param Direction AppDir
/// \return bool
///
/////////////////////////////////////////////////
bool Table::retouch(VectorIndex _vRow, VectorIndex _vCol, size_t inSheet, AppDir Direction)
{
    bool bUseAppendedZeroes = false;

    if (m_data.sheets() <= inSheet)
        return false;

    if (!_vRow.isValid() || !_vCol.isValid())
        return false;

    // Evaluate the indices
    if (_vRow.isOpenEnd())
        bUseAppendedZeroes = true;

    _vRow.setRange(0, m_data.getSheet(inSheet).rows()-1);
    _vCol.setRange(0, m_data.getSheet(inSheet).cols()-1);

    if ((Direction == ALL || Direction == GRID) && _vRow.size() < 4)
        Direction = ROWS;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // Pre-evaluate the axis values in the GRID case
    if (Direction == GRID)
    {
        if (bUseAppendedZeroes)
        {
            if (!retouch(_vRow, VectorIndex(_vCol[0]), inSheet, COLS)
                || !retouch(_vRow, VectorIndex(_vCol[1]), inSheet, COLS))
                return false;
        }
        else
        {
            if (!retouch(_vRow, _vCol.subidx(0, 2), inSheet, COLS))
                return false;
        }

        _vCol = _vCol.subidx(2);
    }

    // Redirect the control to the specialized member
    // functions
    if (Direction == ALL || Direction == GRID)
    {
        _vRow.linearize();
        _vCol.linearize();

        return retouch2D(_vRow, _vCol, inSheet);
    }
    else
        return retouch1D(_vRow, _vCol, inSheet, Direction);
}


/////////////////////////////////////////////////
/// \brief This member function retouches single
/// dimension data (along columns or rows).
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param inSheet size_t
/// \param Direction AppDir
/// \return bool
///
/////////////////////////////////////////////////
bool Table::retouch1D(const VectorIndex& _vRow, const VectorIndex& _vCol, size_t inSheet, AppDir Direction)
{
    bool markModified = false;

    if (Direction == ROWS)
    {
        for (size_t i = 0; i < _vRow.size(); i++)
        {
            for (size_t j = 0; j < _vCol.size(); j++)
            {
                if (mu::isnan(read(_vRow[i], _vCol[j], inSheet)))
                {
                    for (size_t _j = j; _j < _vCol.size(); _j++)
                    {
                        if (!mu::isnan(read(_vRow[i], _vCol[_j], inSheet)))
                        {
                            if (j)
                            {
                                for (size_t __j = j; __j < _j; __j++)
                                {
                                    write(_vRow[i],
                                          _vCol[__j],
                                          inSheet,
                                          (read(_vRow[i], _vCol[_j], inSheet) - read(_vRow[i], _vCol[j-1], inSheet))
                                            / mu::Value(_j - j) * mu::Value(__j - j + 1) + read(_vRow[i], _vCol[j-1], inSheet));
                                }

                                markModified = true;
                                break;
                            }
                            else if (_j+1 < _vCol.size())
                            {
                                for (size_t __j = j; __j < _j; __j++)
                                {
                                    write(_vRow[i], _vCol[__j], inSheet, read(_vRow[i], _vCol[_j], inSheet));
                                }

                                markModified = true;
                                break;
                            }
                        }

                        if (j && _j+1 == _vCol.size() && mu::isnan(read(_vRow[i], _vCol[_j], inSheet)))
                        {
                            for (size_t __j = j; __j < _vCol.size(); __j++)
                            {
                                write(_vRow[i], _vCol[__j], inSheet, read(_vRow[i], _vCol[j-1], inSheet));
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
            for (size_t i = 0; i < _vRow.size(); i++)
            {
                if (mu::isnan(read(_vRow[i], _vCol[j], inSheet)))
                {
                    for (size_t _i = i; _i < _vRow.size(); _i++)
                    {
                        if (!mu::isnan(read(_vRow[_i], _vCol[j], inSheet)))
                        {
                            if (i)
                            {
                                for (size_t __i = i; __i < _i; __i++)
                                {
                                    write(_vRow[__i],
                                          _vCol[j],
                                          inSheet,
                                          (read(_vRow[_i], _vCol[j], inSheet) - read(_vRow[i-1], _vCol[j], inSheet))
                                            / mu::Value(_i - i) * mu::Value(__i - i + 1) + read(_vRow[i-1], _vCol[j], inSheet));
                                }

                                markModified = true;
                                break;
                            }
                            else if (_i+1 < _vRow.size())
                            {
                                for (size_t __i = i; __i < _i; __i++)
                                {
                                    write(_vRow[__i], _vCol[j], inSheet, read(_vRow[_i], _vCol[j], inSheet));
                                }

                                markModified = true;
                                break;
                            }
                        }

                        if (i  && _i+1 == _vRow.size() && mu::isnan(read(_vRow[_i], _vCol[j], inSheet)))
                        {
                            for (size_t __i = i; __i < _vRow.size(); __i++)
                            {
                                write(_vRow[__i], _vCol[j], inSheet, read(_vRow[i-1], _vCol[j], inSheet));
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
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param inSheet size_t
/// \return bool
///
/////////////////////////////////////////////////
bool Table::retouch2D(const VectorIndex& _vRow, const VectorIndex& _vCol, size_t inSheet)
{
    bool bMarkModified = false;

    for (long long int i = _vRow.front(); i <= _vRow.last(); i++)
    {
        for (long long int j = _vCol.front(); j <= _vCol.last(); j++)
        {
            if (mu::isnan(read(i, j, inSheet)))
            {
                Boundary _boundary = findValidBoundary(_vRow, _vCol, inSheet, i, j);
                NumeRe::RetouchRegion _region(_boundary.rows-1,
                                              _boundary.cols-1,
                                              med(VectorIndex(_boundary.rf(), _boundary.re()),
                                                  VectorIndex(_boundary.cf(), _boundary.ce()),
                                                  inSheet));

                long long int l,r,t,b;

                // Find the correct boundary to be used instead of the
                // one outside of the range (if one of the indices is on
                // any of the four boundaries
                l = _boundary.cf() < _vCol.front() ? _boundary.ce() : _boundary.cf();
                r = _boundary.ce() > _vCol.last() ? _boundary.cf() : _boundary.ce();
                t = _boundary.rf() < _vRow.front() ? _boundary.re() : _boundary.rf();
                b = _boundary.re() > _vRow.last() ? _boundary.rf() : _boundary.re();

                _region.setBoundaries(read(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(l), inSheet).as_cmplx_vector(),
                                      read(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(r), inSheet).as_cmplx_vector(),
                                      read(VectorIndex(t), VectorIndex(_boundary.cf(), _boundary.ce()), inSheet).as_cmplx_vector(),
                                      read(VectorIndex(b), VectorIndex(_boundary.cf(), _boundary.ce()), inSheet).as_cmplx_vector());

                for (long long int _n = _boundary.rf()+1; _n < _boundary.re(); _n++)
                {
                    for (long long int _m = _boundary.cf()+1; _m < _boundary.ce(); _m++)
                    {
                        write(_n, _m, inSheet,
                                  _region.retouch(_n - _boundary.rf() - 1,
                                                  _m - _boundary.cf() - 1,
                                                  read(_n, _m, inSheet).getNum().asCF64(),
                                                  med(VectorIndex(_n-1, _n+1), VectorIndex(_m-1, _m+1), inSheet)));
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
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param _vSheet const VectorIndex&
/// \return bool
///
/////////////////////////////////////////////////
bool Table::onlyValidValues(const VectorIndex& _vRow, const VectorIndex& _vCol, const VectorIndex& _vSheet) const
{
    return num(_vRow, _vCol, _vSheet) == cnt(_vRow, _vCol, _vSheet);
}


/////////////////////////////////////////////////
/// \brief This member function finds the
/// smallest possible boundary around a set of
/// invalid values to be used as boundary values
/// for retouching the values.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param inSheet size_t
/// \param i int
/// \param j int
/// \return RetouchBoundary
///
/////////////////////////////////////////////////
Boundary Table::findValidBoundary(const VectorIndex& _vRow, const VectorIndex& _vCol, size_t inSheet, int i, int j) const
{
    Boundary _boundary(i-1, j-1, 2, 2);

    bool reEvaluateBoundaries = true;

    while (reEvaluateBoundaries)
    {
        reEvaluateBoundaries = false;

        if (!onlyValidValues(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(_boundary.cf()), inSheet)
            && _boundary.cf() > _vCol.front())
        {
            _boundary.m--;
            _boundary.cols++;
            reEvaluateBoundaries = true;
        }

        if (!onlyValidValues(VectorIndex(_boundary.rf(), _boundary.re()), VectorIndex(_boundary.ce()), inSheet)
            && _boundary.ce() < _vCol.last())
        {
            _boundary.cols++;
            reEvaluateBoundaries = true;
        }

        if (!onlyValidValues(VectorIndex(_boundary.rf()), VectorIndex(_boundary.cf(), _boundary.ce()), inSheet)
            && _boundary.rf() > _vRow.front())
        {
            _boundary.n--;
            _boundary.rows++;
            reEvaluateBoundaries = true;
        }

        if (!onlyValidValues(VectorIndex(_boundary.re()), VectorIndex(_boundary.cf(), _boundary.ce()), inSheet)
            && _boundary.re() < _vRow.last())
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
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param inSheet size_t
/// \param i size_t
/// \param j size_t
/// \param _filter NumeRe::Filter*
/// \param smoothLines bool
/// \return void
///
/////////////////////////////////////////////////
void Table::smoothingWindow1D(const VectorIndex& _vRow, const VectorIndex& _vCol, size_t inSheet, size_t i, size_t j, NumeRe::Filter* _filter, bool smoothLines)
{
    auto sizes = _filter->getWindowSize();

    std::complex<double> sum = 0.0;
    NumeRe::FilterBuffer& filterBuffer = _filter->getBuffer();

    // Apply the filter to the data
    for (size_t n = 0; n < sizes.first; n++)
    {
        if (!_filter->isConvolution())
            write(_vRow[i+n*(!smoothLines)], _vCol[j+n*smoothLines], inSheet,
                  _filter->apply(n, 0, read(_vRow[i+n*(!smoothLines)],
                                            _vCol[j+n*smoothLines],
                                            inSheet).getNum().asCF64()));
        else
            sum += _filter->apply(n, 0, read(_vRow[i+n*(!smoothLines)],
                                             _vCol[j+n*smoothLines],
                                             inSheet).getNum().asCF64());
    }

    // If the filter is a convolution, store the new value here
    if (_filter->isConvolution())
        filterBuffer.push(sum);

    // If enough elements are stored in the buffer
    // remove the first one
    if (filterBuffer.size() > sizes.first/2)
    {
        // Writes the element to the first position of the window
        write(_vRow[i], _vCol[j], inSheet, filterBuffer.front());
        filterBuffer.pop();
    }

    // Is this the last point? Then extract all remaining points from the
    // buffer
    if (smoothLines && _vCol.size()-sizes.first-1 == j)
    {
        while (!filterBuffer.empty())
        {
            j++;
            write(_vRow[i], _vCol[j], inSheet, filterBuffer.front());
            filterBuffer.pop();
        }
    }
    else if (!smoothLines && _vRow.size()-sizes.first-1 == i)
    {
        while (!filterBuffer.empty())
        {
            i++;
            write(_vRow[i], _vCol[j], inSheet, filterBuffer.front());
            filterBuffer.pop();
        }
    }
}


/////////////////////////////////////////////////
/// \brief This private member function realizes
/// the application of a smoothing window to 2D
/// data sets.
///
/// \param _vRow const VectorIndex&
/// \param _vCol const VectorIndex&
/// \param inSheet size_t
/// \param i size_t
/// \param j size_t
/// \param _filter NumeRe::Filter*
/// \return void
///
/////////////////////////////////////////////////
void Table::smoothingWindow2D(const VectorIndex& _vRow, const VectorIndex& _vCol, size_t inSheet, size_t i, size_t j, NumeRe::Filter* _filter)
{
    auto sizes = _filter->getWindowSize();
    NumeRe::FilterBuffer2D& filterBuffer = _filter->get2DBuffer();

    std::complex<double> sum = 0.0;

    // Apply the filter to the data
    for (size_t n = 0; n < sizes.first; n++)
    {
        for (size_t m = 0; m < sizes.second; m++)
        {
            if (!_filter->isConvolution())
                write(_vRow[i+n], _vCol[j+m], inSheet,
                      _filter->apply(n, m, read(_vRow[i+n],
                                                _vCol[j+m],
                                                inSheet).getNum().asCF64()));
            else
                sum += _filter->apply(n, m, read(_vRow[i+n],
                                                 _vCol[j+m],
                                                 inSheet).getNum().asCF64());
        }
    }

    // If the filter is a convolution, store the new value here
    if (_filter->isConvolution())
    {
        if (j == 1)
            filterBuffer.push(std::vector<std::complex<double>>());

        filterBuffer.back().push_back(sum);
    }

    // If enough elements are stored in the buffer
    // remove the first row
    if (filterBuffer.size() > sizes.first/2+1)
    {
        // Write the finished row
        for (size_t k = 0; k < filterBuffer.front().size(); k++)
            write(_vRow[i-1], _vCol[k+sizes.second/2+1], inSheet,
                  filterBuffer.front()[k]);

        filterBuffer.pop();
    }

    // Is this the last point? Then extract all remaining points from the
    // buffer
    if (_vRow.size()-sizes.first-1 == i && _vCol.size()-sizes.second-1 == j)
    {
        while (!filterBuffer.empty())
        {

            for (size_t k = 0; k < filterBuffer.front().size(); k++)
                write(_vRow[i], _vCol[k+sizes.second/2+1], inSheet,
                      filterBuffer.front()[k]);

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
/// \param _vRow VectorIndex
/// \param _vCol VectorIndex
/// \param inSheet size_t
/// \param _settings NumeRe::FilterSettings
/// \param Direction AppDir
/// \return bool
///
/////////////////////////////////////////////////
bool Table::smooth(VectorIndex _vRow, VectorIndex _vCol, size_t inSheet, NumeRe::FilterSettings _settings, AppDir Direction)
{
    bool bUseAppendedZeroes = false;

    // Avoid the border cases
    if (m_data.empty())
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, "smooth", SyntaxError::invalid_position);

    if (!_vRow.isValid() || !_vCol.isValid() || inSheet >= m_data.sheets())
        throw SyntaxError(SyntaxError::INVALID_INDEX, "smooth", SyntaxError::invalid_position,
                          _vRow.to_string() + ", " + _vCol.to_string() + ", " + toString(inSheet));

    // Evaluate the indices
    if (_vRow.isOpenEnd())
        bUseAppendedZeroes = true;

    size_t nColCount = m_data.getSheet(inSheet).cols();
    size_t nRowCount = m_data.getSheet(inSheet).rows();

    // Force the index ranges
    _vCol.setRange(0, nColCount-1);

    if (bUseAppendedZeroes)
    {
        std::vector<double> sizes = mu::real(size(_vCol, VectorIndex(0, VectorIndex::OPEN_END), inSheet, AppDir::COLS));
        nRowCount = *std::max_element(sizes.begin(), sizes.end());
    }

    _vRow.setRange(0, nRowCount-1);

    // Change the predefined application directions, if it's needed
    if ((Direction == ALL || Direction == GRID) && _vRow.size() < 4)
        Direction = ROWS;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // Check the order
    if ((_settings.row >= nRowCount && Direction == COLS)
        || (_settings.col >= nColCount && Direction == ROWS)
        || ((_settings.row >= nRowCount || _settings.col >= nColCount) && (Direction == ALL || Direction == GRID)))
        throw SyntaxError(SyntaxError::CANNOT_SMOOTH_CACHE, "smooth", SyntaxError::invalid_position);


    // If the application direction is equal to GRID, then the first two columns
    // should be evaluted separately, because they contain the axis values
    if (Direction == GRID)
    {
        // Will never return false
        if (bUseAppendedZeroes)
        {
            if (!smooth(_vRow, VectorIndex(_vCol[0]), inSheet, _settings, COLS)
                || !smooth(_vRow, VectorIndex(_vCol[1]), inSheet, _settings, COLS))
                return false;
        }
        else
        {
            if (!smooth(_vRow, _vCol.subidx(0, 2), inSheet, _settings, COLS))
                return false;
        }

        _vCol = _vCol.subidx(2);
    }

    // The first job is to simply remove invalid values and then smooth the
    // framing points of the data section
    if (Direction == ALL || Direction == GRID)
    {
        // Retouch everything
        Table::retouch(_vRow, _vCol, inSheet, ALL);

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
    if (Direction == ROWS)
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
        for (size_t i = 0; i < _vRow.size(); i++)
        {
            for (size_t j = 1; j < _vCol.size() - _settings.row; j++)
            {
                smoothingWindow1D(_vRow, _vCol, inSheet, i, j, _filterPtr.get(), true);
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
        _vRow.prepend(vector<int>(_settings.row/2+1, _vRow.front()));
        _vRow.append(vector<int>(_settings.row/2+1, _vRow.last()));

        // Smooth the columns
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            for (size_t i = 1; i < _vRow.size() - _settings.row; i++)
            {
                smoothingWindow1D(_vRow, _vCol, inSheet, i, j, _filterPtr.get(), false);
            }
        }
    }
    else if ((Direction == ALL || Direction == GRID) && _vRow.size() > 2 && _vCol.size() > 2)
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
        std::vector<int> vMirror = _vRow.subidx(1, _settings.row/2+1).getVector();
        _vRow.prepend(vector<int>(vMirror.rbegin(), vMirror.rend()));

        vMirror = _vRow.subidx(_vRow.size() - _settings.row/2-2, _settings.row/2+1).getVector();
        _vRow.append(vector<int>(vMirror.rbegin(), vMirror.rend()));

        vMirror = _vCol.subidx(1, _settings.col/2+1).getVector();
        _vCol.prepend(vector<int>(vMirror.rbegin(), vMirror.rend()));

        vMirror = _vCol.subidx(_vCol.size() - _settings.col/2-2, _settings.row/2+1).getVector();
        _vCol.append(vector<int>(vMirror.rbegin(), vMirror.rend()));

        // Smooth the data in two dimensions, if that is reasonable
        // Go through every point
        for (size_t i = 1; i < _vRow.size() - _settings.row; i++)
        {
            for (size_t j = 1; j < _vCol.size() - _settings.col; j++)
            {
                smoothingWindow2D(_vRow, _vCol, inSheet, i, j, _filterPtr.get());
            }
        }
    }

    m_meta.modify();
    return true;
}


static void extractResampled(Table* _this, Resampler* _resampler, int start_line, int start_col, size_t inSheet, int _primary_dim, int& _secondary_dim, bool linewise)
{
    while (true)
    {
        const double* dOutputSamples = _resampler->get_line();

        // dOutputSamples will be a nullptr, if no more resampled
        // lines are available
        if (!dOutputSamples)
            break;

        if (linewise)
        {
            for (int _fin = 0; _fin < _primary_dim; _fin++)
            {
                _this->write(start_line+_secondary_dim, start_col+_fin, inSheet,
                             dOutputSamples[_fin]);
            }
        }
        else
        {
            for (int _fin = 0; _fin < _primary_dim; _fin++)
            {
                _this->write(start_line+_fin, start_col+_secondary_dim, inSheet,
                             dOutputSamples[_fin]);
            }
        }

        _secondary_dim++;
    }
}


/////////////////////////////////////////////////
/// \brief This member function resamples the
/// data described by the passed coordinates
/// using the new samples nSamples.
///
/// \param _vRow VectorIndex
/// \param _vCol VectorIndex
/// \param inSheet size_t
/// \param samples std::pair<size_t,size_t>
/// \param Direction AppDir
/// \param sFilter std::string sFilter
/// \return bool
///
/////////////////////////////////////////////////
bool Table::resample(VectorIndex _vRow, VectorIndex _vCol, size_t inSheet, std::pair<size_t,size_t> samples, AppDir Direction, std::string sFilter)
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
    if (m_data.empty())
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, "resample", SyntaxError::invalid_position);

    if (!samples.first || !samples.second)
        throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);

    if (!_vRow.isValid() || !_vCol.isValid() || m_data.sheets() <= inSheet)
        throw SyntaxError(SyntaxError::INVALID_INDEX, "resample", SyntaxError::invalid_position,
                          _vRow.to_string() + ", " + _vCol.to_string() + ", " + toString(inSheet));

    // Evaluate the indices
    if (_vCol.isOpenEnd())
        bUseAppendedZeroes = true;

    TableSheet& sheet = m_data.getSheet(inSheet);

    _vRow.setRange(0, sheet.rows()-1);
    _vRow.linearize();
    _vCol.setRange(0, sheet.cols()-1);

    // Change the predefined application directions, if it's needed
    if ((Direction == ALL || Direction == GRID) && _vRow.size() < 4)
        Direction = ROWS;

    if ((Direction == ALL || Direction == GRID) && _vCol.size() < 4)
        Direction = COLS;

    // If the application direction is equal to GRID, then the indices should
    // match a sufficiently enough large data array
    if (Direction == GRID)
    {
        if (_vCol.size() - 2 != _vRow.size() && !bUseAppendedZeroes)
            throw SyntaxError(SyntaxError::CANNOT_RESAMPLE_CACHE, "resample", SyntaxError::invalid_position);
        else if ((!sheet.isValidColumn(_vCol[1]) || _vCol.size() - 2 != sheet.m_columns[_vCol[1]]->size() - _vRow.front())
                 && bUseAppendedZeroes)
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
            resample(_vRow, VectorIndex(_vCol[0]), inSheet, samples, COLS);
            resample(_vRow, VectorIndex(_vCol[1]), inSheet, std::make_pair(samples.second, samples.first), COLS);

            // Increment the first column
            _vCol = _vCol.subidx(2);
            _vCol.linearize();

            // Determine the size of the buffer
            if (samples.first > _vRow.size())
                nLinesToInsert = samples.first - _vRow.size();

            if (samples.second > _vCol.size())
                nColsToInsert = samples.second - _vCol.size();
        }

        // Create the resample object and prepare the needed memory
        _resampler.reset(new Resampler(_vCol.size(), _vRow.size(),
                                       samples.second, samples.first,
                                       Resampler::BOUNDARY_CLAMP, 1.0, 0.0, sFilter.c_str()));
    }
    else if (Direction == COLS) // cols
    {
        _vCol.linearize();

        // Create the resample object and prepare the needed memory
        // NOTE: It is intentional, that the line dimension is changing
        // here, because that is way faster in the end
        _resampler.reset(new Resampler(_vRow.size(), _vCol.size(),
                                       samples.first, _vCol.size(),
                                       Resampler::BOUNDARY_CLAMP, 1.0, 0.0, sFilter.c_str()));

        // Determine final size (only upscale)
        if (samples.first > _vRow.size())
            nLinesToInsert = samples.first - _vRow.size();
    }
    else if (Direction == ROWS)// lines
    {
        // Create the resample object and prepare the needed memory
        _resampler.reset(new Resampler(_vCol.size(), _vRow.size(),
                                       samples.second, _vRow.size(),
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
            if (sheet.isValidColumn(_vCol[j]))
                sheet.m_columns[_vCol[j]]->insertElements(_vRow.last()+1, nLinesToInsert);
        }
    }

    if (nColsToInsert)
    {
        TableColumnArray arr(nColsToInsert);
        sheet.m_columns.insert(sheet.m_columns.begin()+_vCol.last()+1,
                               std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
    }

    std::vector<double> dInputSamples(Direction == COLS ? _vRow.size() : _vCol.size());
    int _seconday_dim = 0;
    int _primary_dim = 0;

    // Resample the data table
    if (Direction == COLS)
    {
        // Determine the number of final primary dimension. These will stay constant only in
        // the column application direction
        _primary_dim = samples.first;

        // Apply the resampling colwise
        for (size_t j = 0; j < _vCol.size(); j++)
        {
            for (size_t i = 0; i < _vRow.size(); i++)
            {
                dInputSamples[i] = read(_vRow[i], _vCol[j], inSheet).getNum().asF64();
            }

            // If the resampler doesn't accept a further line
            // the buffer is probably full
            if (!_resampler->put_line(&dInputSamples[0]))
            {
                if (_resampler->status() != Resampler::STATUS_SCAN_BUFFER_FULL)
                    throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
                else if (_resampler->status() == Resampler::STATUS_SCAN_BUFFER_FULL)
                {
                    // Free the scan buffer of the resampler by extracting the already resampled lines
                    extractResampled(this, _resampler.get(), _vRow.front(), _vCol.front(), inSheet, _primary_dim, _seconday_dim, false);

                    // Try again to put the current line
                    _resampler->put_line(&dInputSamples[0]);
                }
            }
        }
    }
    else
    {
        // Determine the number of final primary dimension. These will stay constant only in
        // the column application direction
        _primary_dim = samples.second;

        // Apply the resampling linewise
        for (size_t i = 0; i < _vRow.size(); i++)
        {
            for (size_t j = 0; j < _vCol.size(); j++)
            {
                dInputSamples[j] = read(_vRow[i], _vCol[j], inSheet).getNum().asF64();
            }

            // If the resampler doesn't accept a further line
            // the buffer is probably full
            if (!_resampler->put_line(&dInputSamples[0]))
            {
                if (_resampler->status() != Resampler::STATUS_SCAN_BUFFER_FULL)
                    throw SyntaxError(SyntaxError::INTERNAL_RESAMPLER_ERROR, "resample", SyntaxError::invalid_position);
                else if (_resampler->status() == Resampler::STATUS_SCAN_BUFFER_FULL)
                {
                    // Free the scan buffer of the resampler by extracting the already resampled lines
                    extractResampled(this, _resampler.get(), _vRow.front(), _vCol.front(), inSheet, _primary_dim, _seconday_dim, true);

                    // Try again to put the current line
                    _resampler->put_line(&dInputSamples[0]);
                }
            }
        }
    }

    // Extract the remaining resampled lines from the resampler's memory
    extractResampled(this, _resampler.get(), _vRow.front(), _vCol.front(), inSheet, _primary_dim, _seconday_dim, Direction != COLS);

    // Delete empty lines
    if (Direction != ROWS && samples.first < _vRow.size())
        deleteBulk(VectorIndex(_vRow.front() + samples.first, _vRow.last()), _vCol, inSheet);

    // Delete empty cols
    if (Direction != COLS && samples.second < _vCol.size())
        deleteBulk(_vRow, VectorIndex(_vCol.front() + samples.second, _vCol.last()), inSheet);

    // Reset the calculated lines and columns
    sheet.m_calculatedRows = 0;
    m_meta.modify();

    return true;
}


