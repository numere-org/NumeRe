/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef TABLECOLUMNIMPL_HPP
#define TABLECOLUMNIMPL_HPP

#include "tablecolumn.hpp"
#include "../utils/tools.hpp"
#include "../ui/error.hpp"
#include "../../kernel.hpp"

// Forward declaration for ValueColumn::convert()
class StringColumn;
class CategoricalColumn;
class DateTimeColumn;
class LogicalColumn;

// Helper for all TYPE_VALUE_* conversions
TableColumn* createValueTypeColumn(TableColumn::ColumnType type, size_t nSize = 0);
TableColumn* convertNumericType(TableColumn::ColumnType type, TableColumn* current);

/////////////////////////////////////////////////
/// \brief A table column containing only
/// numerical values.
/////////////////////////////////////////////////
template <class T, TableColumn::ColumnType COLTYPE>
class GenericValueColumn : public TableColumn
{
    protected:
        std::vector<T> m_data;
        const T INVALID_VALUE = std::is_integral<T>::value ? 0 : NAN;// (T(1.1) == 1.1 ? NAN : 0);
        size_t m_numElements;

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        GenericValueColumn() : TableColumn(), m_numElements(0)
        {
            m_type = COLTYPE;
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        GenericValueColumn(size_t nElem) : GenericValueColumn()
        {
            resize(nElem);
        }

        virtual ~GenericValueColumn() {}

        /////////////////////////////////////////////////
        /// \brief Returns the selected value as a string
        /// or a default value, if it does not exist.
        ///
        /// \param elem size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        virtual std::string getValueAsString(size_t elem) const override
        {
            return getValueAsInternalString(elem) + (this->m_sUnit.length() ? " " + this->m_sUnit : "");
        }

        /////////////////////////////////////////////////
        /// \brief Returns the contents as an internal
        /// string (i.e. without quotation marks and
        /// unit).
        ///
        /// \param elem size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        virtual std::string getValueAsInternalString(size_t elem) const override
        {
            if (elem < m_data.size())
                return toString(std::complex<double>(m_data[elem]), 14);

            return "nan";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the contents as parser
        /// string (i.e. without quotation marks).
        ///
        /// \param elem size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        virtual std::string getValueAsParserString(size_t elem) const override
        {
            return getValueAsString(elem);
        }

        /////////////////////////////////////////////////
        /// \brief Returns the contents as parser
        /// string (i.e. without quotation marks).
        ///
        /// \param elem size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        virtual std::string getValueAsStringLiteral(size_t elem) const override
        {
#warning FIXME (numere#1#11/09/23): Using the explicit constructor is a hack
            if (elem < m_data.size())
                return toString(std::complex<double>(m_data[elem]), NumeReKernel::getInstance()->getSettings().getPrecision()) + (this->m_sUnit.length() ? " " + this->m_sUnit : "");

            return "nan";
        }

        /////////////////////////////////////////////////
        /// \brief Returns the selected value as a
        /// numerical type or an invalid value, if it
        /// does not exist.
        ///
        /// \param elem size_t
        /// \return std::complex<double>
        ///
        /////////////////////////////////////////////////
        virtual std::complex<double> getValue(size_t elem) const override
        {
            if (elem < m_data.size())
                return m_data[elem];

            return NAN;
        }

        virtual void setValue(size_t elem, const std::complex<double>& vValue) = 0;

        /////////////////////////////////////////////////
        /// \brief Set a single string value.
        ///
        /// \param elem size_t
        /// \param sValue const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void setValue(size_t elem, const std::string& sValue) override
        {
            if (isConvertible(sValue, CONVTYPE_VALUE))
                setValue(elem, StrToCmplx(toInternalString(sValue)));
            else
                throw SyntaxError(SyntaxError::STRING_ERROR, sValue, sValue, _lang.get("ERR_NR_3603_INCONVERTIBLE_STRING"));
        }

        /////////////////////////////////////////////////
        /// \brief Assign another TableColumn's contents
        /// to this table column.
        ///
        /// \param column const TableColumn*
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void assign(const TableColumn* column) override
        {
            if (column->m_type == m_type)
            {
                assignMetaData(column);
                m_data = static_cast<const GenericValueColumn*>(column)->m_data;
                m_numElements = static_cast<const GenericValueColumn*>(column)->m_numElements;
            }
            else
                throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
        }

        /////////////////////////////////////////////////
        /// \brief Insert the contents of the passed
        /// column at the specified positions.
        ///
        /// \param idx const VectorIndex&
        /// \param column const TableColumn*
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void insert(const VectorIndex& idx, const TableColumn* column) override
        {
            if (column->m_type == m_type)
                TableColumn::setValue(idx, static_cast<const GenericValueColumn*>(column)->TableColumn::getValue(VectorIndex(0, VectorIndex::OPEN_END)));
            else
                throw SyntaxError(SyntaxError::CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE, m_sHeadLine, column->m_sHeadLine, typeToString(m_type) + "/" + typeToString(column->m_type));
        }

        /////////////////////////////////////////////////
        /// \brief Delete the specified elements.
        ///
        /// \note Will trigger the shrinking algorithm.
        ///
        /// \param idx const VectorIndex&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void deleteElements(const VectorIndex& idx) override
        {
            idx.setOpenEndIndex(size()-1);

            // Shortcut, if everything shall be deleted
            if (idx.isExpanded() && idx.front() == 0 && idx.last() >= (int)m_data.size()-1)
            {
                m_data.clear();
                m_numElements = 0;
                return;
            }

            for (size_t i = 0; i < idx.size(); i++)
            {
                if (idx[i] >= 0 && idx[i] < (int)m_data.size())
                    m_data[idx[i]] = INVALID_VALUE;
            }

            // Update the number of elements
            if (idx.isExpanded() && idx.last() >= (int)m_data.size()-1)
                m_numElements = std::min(m_numElements, (size_t)idx.front());

            shrink();
        }

        /////////////////////////////////////////////////
        /// \brief Inserts as many as the selected
        /// elements at the desired position, if the
        /// column is already larger than the starting
        /// position.
        ///
        /// \param pos size_t
        /// \param elem size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void insertElements(size_t pos, size_t elem) override
        {
            if (pos < m_data.size())
            {
                m_data.insert(m_data.begin()+pos, elem, INVALID_VALUE);

                if (pos < m_numElements)
                    m_numElements += elem;
            }
        }

        /////////////////////////////////////////////////
        /// \brief Appends the number of elements.
        ///
        /// \param elem size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void appendElements(size_t elem) override
        {
            m_data.insert(m_data.end(), elem, INVALID_VALUE);
        }

        /////////////////////////////////////////////////
        /// \brief Removes the selected number of
        /// elements from the column and moving all
        /// following items forward.
        ///
        /// \param pos size_t
        /// \param elem size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void removeElements(size_t pos, size_t elem) override
        {
            if (pos < m_data.size())
            {
                m_data.erase(m_data.begin()+pos, m_data.begin()+pos+elem);

                if (pos < m_numElements)
                    m_numElements -= std::min(m_numElements - pos, elem);
            }
        }

        /////////////////////////////////////////////////
        /// \brief Resizes the internal array.
        ///
        /// \param elem size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void resize(size_t elem) override
        {
            if (!elem)
                m_data.clear();
            else
                m_data.resize(elem, INVALID_VALUE);

            m_numElements = std::min(m_numElements, elem);
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, if the selected element
        /// is a valid value.
        ///
        /// \param elem int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        virtual bool isValid(int elem) const override
        {
            if ((size_t)elem >= std::min(m_numElements, m_data.size()) || elem < 0 || mu::isnan(m_data[elem]))
                return false;

            return true;
        }

        /////////////////////////////////////////////////
        /// \brief Interprets the value as a boolean.
        ///
        /// \param elem int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        virtual bool asBool(int elem) const override
        {
            if (elem < 0 || (size_t)elem >= std::min(m_numElements, m_data.size()))
                return false;

            return m_data[elem] != T(0.0);
        }

        /////////////////////////////////////////////////
        /// \brief Returns the contents of this column
        /// converted to the new column type. Might even
        /// return itself.
        ///
        /// \param type ColumnType
        /// \return TableColumn*
        ///
        /////////////////////////////////////////////////
        virtual TableColumn* convert(ColumnType type = TableColumn::TYPE_NONE) override
        {
            TableColumn* col = nullptr;

            switch (type)
            {
                //case TableColumn::TYPE_NONE: // Disabled for correct autoconversion
                case TableColumn::TYPE_STRING:
                {
                    col = new StringColumn(m_data.size());

                    for (size_t i = 0; i < m_data.size(); i++)
                    {
                        if (!mu::isnan(m_data[i]))
                            col->setValue(i, toString(getValue(i), NumeReKernel::getInstance()->getSettings().getPrecision()));
                    }

                    break;
                }
                case TableColumn::TYPE_CATEGORICAL:
                {
                    col = new CategoricalColumn(m_data.size());

                    for (size_t i = 0; i < m_data.size(); i++)
                    {
                        if (!mu::isnan(m_data[i]))
                            col->setValue(i, toString(getValue(i), NumeReKernel::getInstance()->getSettings().getPrecision()));
                    }

                    break;
                }
                case TableColumn::TYPE_DATETIME:
                {
                    col = new DateTimeColumn(m_data.size());

                    for (size_t i = 0; i < m_data.size(); i++)
                    {
                        if (!mu::isnan(m_data[i]))
                            col->setValue(i, getValue(i));
                    }

                    break;
                }
                case TableColumn::TYPE_LOGICAL:
                {
                    col = new LogicalColumn(m_data.size());

                    for (size_t i = 0; i < m_data.size(); i++)
                    {
                        if (!mu::isnan(m_data[i]))
                            col->setValue(i, getValue(i));
                    }

                    break;
                }
                case COLTYPE:
                    return this;
                default:
                {
                    if (TableColumn::isValueType(type))
                        return convertNumericType(type, this);

                    return nullptr;
                }
            }

            col->assignMetaData(this);
            return col;
        }

        /////////////////////////////////////////////////
        /// \brief Return the number of bytes occupied by
        /// this column.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t getBytes() const override
        {
            return size() * sizeof(T) + m_sHeadLine.capacity() * sizeof(char);
        }

        /////////////////////////////////////////////////
        /// \brief Return the number of elements in this
        /// column (will also count invalid ones).
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t size() const override
        {
            return m_data.size();
        }
};

template <class T, TableColumn::ColumnType COLTYPE>
class BaseFloatColumn : public GenericValueColumn<T, COLTYPE>
{
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        BaseFloatColumn() : GenericValueColumn<T, COLTYPE>()
        {
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        BaseFloatColumn(size_t nElem) : BaseFloatColumn<T, COLTYPE>()
        {
            this->resize(nElem);
        }

        virtual ~BaseFloatColumn() {}

        /////////////////////////////////////////////////
        /// \brief Set a single numerical value.
        ///
        /// \param elem size_t
        /// \param vValue const std::complex<double>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void setValue(size_t elem, const std::complex<double>& vValue) override
        {
            if (elem >= this->m_data.size() && mu::isnan(vValue))
                return;

            if (elem >= this->m_data.size())
                this->m_data.resize(elem+1, this->INVALID_VALUE);

            this->m_data[elem] = vValue.real();
            this->m_numElements = std::max(elem+1, this->m_numElements);
        }

        /////////////////////////////////////////////////
        /// \brief Creates a copy of the selected part of
        /// this column. Can be used for simple
        /// extraction into a new table.
        ///
        /// \param idx const VectorIndex&
        /// \return BaseFloatColumn*
        ///
        /////////////////////////////////////////////////
        virtual BaseFloatColumn* copy(const VectorIndex& idx) const override
        {
            idx.setOpenEndIndex(this->getNumFilledElements()-1);

            BaseFloatColumn* col = new BaseFloatColumn(idx.size());
            col->assignMetaData(this);

            for (size_t i = 0; i < idx.size(); i++)
            {
                col->setValue(i, this->getValue(idx[i]));
            }

            return col;
        }

        /////////////////////////////////////////////////
        /// \brief Returns 0, if both elements are equal,
        /// -1 if element i is smaller than element j and
        /// 1 otherwise.
        ///
        /// \param i int
        /// \param j int
        /// \param unused bool
        /// \return int
        ///
        /////////////////////////////////////////////////
        virtual int compare(int i, int j, bool unused) const override
        {
            if ((int)this->m_data.size() <= std::max(i, j))
                return 0;

            if (this->m_data[i] == this->m_data[j])
                return 0;
            else if (this->m_data[i] < this->m_data[j])
                return -1;

            return 1;
        }
};

template <class T, TableColumn::ColumnType COLTYPE>
class BaseComplexColumn : public GenericValueColumn<T, COLTYPE>
{
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        BaseComplexColumn() : GenericValueColumn<T, COLTYPE>()
        {
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        BaseComplexColumn(size_t nElem) : BaseComplexColumn<T, COLTYPE>()
        {
            this->resize(nElem);
        }

        virtual ~BaseComplexColumn() {}

        /////////////////////////////////////////////////
        /// \brief Set a single numerical value.
        ///
        /// \param elem size_t
        /// \param vValue const std::complex<double>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void setValue(size_t elem, const std::complex<double>& vValue) override
        {
            if (elem >= this->m_data.size() && mu::isnan(vValue))
                return;

            if (elem >= this->m_data.size())
                this->m_data.resize(elem+1, this->INVALID_VALUE);

            this->m_data[elem] = vValue;
            this->m_numElements = std::max(elem+1, this->m_numElements);
        }

        /////////////////////////////////////////////////
        /// \brief Creates a copy of the selected part of
        /// this column. Can be used for simple
        /// extraction into a new table.
        ///
        /// \param idx const VectorIndex&
        /// \return BaseComplexColumn*
        ///
        /////////////////////////////////////////////////
        virtual BaseComplexColumn* copy(const VectorIndex& idx) const override
        {
            idx.setOpenEndIndex(this->getNumFilledElements()-1);

            BaseComplexColumn* col = new BaseComplexColumn(idx.size());
            col->assignMetaData(this);

            for (size_t i = 0; i < idx.size(); i++)
            {
                col->setValue(i, this->getValue(idx[i]));
            }

            return col;
        }

        /////////////////////////////////////////////////
        /// \brief Returns 0, if both elements are equal,
        /// -1 if element i is smaller than element j and
        /// 1 otherwise.
        ///
        /// \param i int
        /// \param j int
        /// \param unused bool
        /// \return int
        ///
        /////////////////////////////////////////////////
        virtual int compare(int i, int j, bool unused) const override
        {
            if ((int)this->m_data.size() <= std::max(i, j))
                return 0;

            if (this->m_data[i] == this->m_data[j])
                return 0;
            else if (this->m_data[i].real() < this->m_data[j].real())
                return -1;

            return 1;
        }
};


template <class T, TableColumn::ColumnType COLTYPE>
class BaseIntColumn : public GenericValueColumn<T, COLTYPE>
{
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        BaseIntColumn() : GenericValueColumn<T, COLTYPE>()
        {
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        BaseIntColumn(size_t nElem) : BaseIntColumn<T, COLTYPE>()
        {
            this->resize(nElem);
        }

        virtual ~BaseIntColumn() {}

        /////////////////////////////////////////////////
        /// \brief Set a single numerical value.
        ///
        /// \param elem size_t
        /// \param vValue const std::complex<double>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void setValue(size_t elem, const std::complex<double>& vValue) override
        {
            if (elem >= this->m_data.size() && mu::isnan(vValue))
                return;

            if (elem >= this->m_data.size())
                this->m_data.resize(elem+1, this->INVALID_VALUE);

            this->m_data[elem] = mu::isnan(vValue) ? this->INVALID_VALUE : genericIntCast<T>(vValue);

            if (mu::isnan(vValue) && this->m_numElements == elem+1)
                this->m_numElements--;
            else if (!mu::isnan(vValue))
                this->m_numElements = std::max(this->m_numElements, elem+1);
        }

        /////////////////////////////////////////////////
        /// \brief Creates a copy of the selected part of
        /// this column. Can be used for simple
        /// extraction into a new table.
        ///
        /// \param idx const VectorIndex&
        /// \return BaseIntColumn*
        ///
        /////////////////////////////////////////////////
        virtual BaseIntColumn* copy(const VectorIndex& idx) const override
        {
            long long int nNumElements = this->getNumFilledElements();
            idx.setOpenEndIndex(nNumElements-1);

            BaseIntColumn* col = new BaseIntColumn(idx.size());
            col->assignMetaData(this);

            for (size_t i = 0; i < idx.size(); i++)
            {
                if (idx[i] < nNumElements)
                    col->setValue(i, this->getValue(idx[i]));
            }

            col->shrink();
            return col;
        }

        /////////////////////////////////////////////////
        /// \brief Returns 0, if both elements are equal,
        /// -1 if element i is smaller than element j and
        /// 1 otherwise.
        ///
        /// \param i int
        /// \param j int
        /// \param unused bool
        /// \return int
        ///
        /////////////////////////////////////////////////
        virtual int compare(int i, int j, bool unused) const override
        {
            if ((int)this->m_data.size() <= std::max(i, j))
                return 0;

            if (this->m_data[i] == this->m_data[j])
                return 0;
            else if (this->m_data[i] < this->m_data[j])
                return -1;

            return 1;
        }
};


using I8ValueColumn = BaseIntColumn<int8_t, TableColumn::TYPE_VALUE_I8>;
using UI8ValueColumn = BaseIntColumn<uint8_t, TableColumn::TYPE_VALUE_UI8>;
using I16ValueColumn = BaseIntColumn<int16_t, TableColumn::TYPE_VALUE_I16>;
using UI16ValueColumn = BaseIntColumn<uint16_t, TableColumn::TYPE_VALUE_UI16>;
using I32ValueColumn = BaseIntColumn<int32_t, TableColumn::TYPE_VALUE_I32>;
using UI32ValueColumn = BaseIntColumn<uint32_t, TableColumn::TYPE_VALUE_UI32>;
using I64ValueColumn = BaseIntColumn<int64_t, TableColumn::TYPE_VALUE_I64>;
using UI64ValueColumn = BaseIntColumn<uint64_t, TableColumn::TYPE_VALUE_UI64>;

using F32ValueColumn = BaseFloatColumn<float, TableColumn::TYPE_VALUE_F32>;
using F64ValueColumn = BaseFloatColumn<double, TableColumn::TYPE_VALUE_F64>;

using CF32ValueColumn = BaseComplexColumn<std::complex<float>, TableColumn::TYPE_VALUE_CF32>;
using ValueColumn = BaseComplexColumn<std::complex<double>, TableColumn::TYPE_VALUE>;


/////////////////////////////////////////////////
/// \brief A table column containing numerical
/// values formatted as dates and times.
/////////////////////////////////////////////////
class DateTimeColumn : public TableColumn
{
    private:
        std::vector<double> m_data;

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        /// column's type.
        /////////////////////////////////////////////////
        DateTimeColumn() : TableColumn()
        {
            m_type = TableColumn::TYPE_DATETIME;
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        DateTimeColumn(size_t nElem) : DateTimeColumn()
        {
            resize(nElem);
        }

        virtual ~DateTimeColumn() {}

        virtual std::string getValueAsString(size_t elem) const override;
        virtual std::string getValueAsInternalString(size_t elem) const override;
        virtual std::string getValueAsParserString(size_t elem) const override;
        virtual std::string getValueAsStringLiteral(size_t elem) const override;
        virtual std::complex<double> getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const std::complex<double>& vValue) override;

        virtual DateTimeColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void insert(const VectorIndex& idx, const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;

        virtual void insertElements(size_t pos, size_t elem) override;
        virtual void appendElements(size_t elem) override;
        virtual void removeElements(size_t pos, size_t elem) override;
        virtual void resize(size_t elem) override;

        virtual int compare(int i, int j, bool unused) const override;
        virtual bool isValid(int elem) const override;
        virtual bool asBool(int elem) const override;

        /////////////////////////////////////////////////
        /// \brief Return the number of bytes occupied by
        /// this column.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t getBytes() const override
        {
            return size() * sizeof(double) + m_sHeadLine.capacity() * sizeof(char);
        }

        /////////////////////////////////////////////////
        /// \brief Return the number of elements in this
        /// column (will also count invalid ones).
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t size() const override
        {
            return m_data.size();
        }

        virtual TableColumn* convert(ColumnType type = TableColumn::TYPE_NONE) override;
};


/////////////////////////////////////////////////
/// \brief A table column containing logical
/// values.
/////////////////////////////////////////////////
class LogicalColumn : public TableColumn
{
    private:
        enum LogicalValue
        {
            LOGICAL_NAN = -1,
            LOGICAL_FALSE = 0,
            LOGICAL_TRUE = 1
        };

        std::vector<LogicalValue> m_data;

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        /// column's type.
        /////////////////////////////////////////////////
        LogicalColumn() : TableColumn()
        {
            m_type = TableColumn::TYPE_LOGICAL;
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        LogicalColumn(size_t nElem) : LogicalColumn()
        {
            resize(nElem);
        }

        virtual ~LogicalColumn() {}

        virtual std::string getValueAsString(size_t elem) const override;
        virtual std::string getValueAsInternalString(size_t elem) const override;
        virtual std::string getValueAsParserString(size_t elem) const override;
        virtual std::string getValueAsStringLiteral(size_t elem) const override;
        virtual std::complex<double> getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const std::complex<double>& vValue) override;

        virtual LogicalColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void insert(const VectorIndex& idx, const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;

        virtual void insertElements(size_t pos, size_t elem) override;
        virtual void appendElements(size_t elem) override;
        virtual void removeElements(size_t pos, size_t elem) override;
        virtual void resize(size_t elem) override;

        virtual int compare(int i, int j, bool unused) const override;
        virtual bool isValid(int elem) const override;
        virtual bool asBool(int elem) const override;

        /////////////////////////////////////////////////
        /// \brief Return the number of bytes occupied by
        /// this column.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t getBytes() const override
        {
            return size() * sizeof(LogicalValue) + m_sHeadLine.capacity() * sizeof(char);
        }

        /////////////////////////////////////////////////
        /// \brief Return the number of elements in this
        /// column (will also count invalid ones).
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t size() const override
        {
            return m_data.size();
        }

        virtual TableColumn* convert(ColumnType type = TableColumn::TYPE_NONE) override;
};


/////////////////////////////////////////////////
/// \brief A table column containing only strings
/// as values.
/////////////////////////////////////////////////
class StringColumn : public TableColumn
{
    private:
        std::vector<std::string> m_data;

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        /// column's type.
        /////////////////////////////////////////////////
        StringColumn() : TableColumn()
        {
            m_type = TableColumn::TYPE_STRING;
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        StringColumn(size_t nElem) : StringColumn()
        {
            resize(nElem);
        }

        virtual ~StringColumn() {}

        virtual std::string getValueAsString(size_t elem) const override;
        virtual std::string getValueAsInternalString(size_t elem) const override;
        virtual std::string getValueAsParserString(size_t elem) const override;
        virtual std::string getValueAsStringLiteral(size_t elem) const override;
        virtual std::complex<double> getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const std::complex<double>& vValue) override;

        virtual StringColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void insert(const VectorIndex& idx, const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;

        virtual void insertElements(size_t pos, size_t elem);
        virtual void appendElements(size_t elem);
        virtual void removeElements(size_t pos, size_t elem);
        virtual void resize(size_t elem) override;

        virtual int compare(int i, int j, bool caseinsensitive) const override;
        virtual bool isValid(int elem) const override;
        virtual bool asBool(int elem) const override;

        virtual size_t getBytes() const override;

        /////////////////////////////////////////////////
        /// \brief Returns the number of elements in this
        /// column (will also count invalid ones).
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t size() const override
        {
            return m_data.size();
        }

        virtual TableColumn* convert(ColumnType type = TableColumn::TYPE_NONE) override;
};


/////////////////////////////////////////////////
/// \brief A table column containing categorical
/// values.
/////////////////////////////////////////////////
class CategoricalColumn : public TableColumn
{
    private:
        enum {CATEGORICAL_NAN = -1};

        std::vector<int> m_data;
        std::vector<std::string> m_categories;

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        /// column's type.
        /////////////////////////////////////////////////
        CategoricalColumn() : TableColumn()
        {
            m_type = TableColumn::TYPE_CATEGORICAL;
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        CategoricalColumn(size_t nElem) : CategoricalColumn()
        {
            resize(nElem);
        }

        virtual ~CategoricalColumn() {}

        virtual std::string getValueAsString(size_t elem) const override;
        virtual std::string getValueAsInternalString(size_t elem) const override;
        virtual std::string getValueAsParserString(size_t elem) const override;
        virtual std::string getValueAsStringLiteral(size_t elem) const override;
        virtual std::complex<double> getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const std::complex<double>& vValue) override;

        virtual CategoricalColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void insert(const VectorIndex& idx, const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;

        virtual void insertElements(size_t pos, size_t elem);
        virtual void appendElements(size_t elem);
        virtual void removeElements(size_t pos, size_t elem);
        virtual void resize(size_t elem) override;

        virtual int compare(int i, int j, bool caseinsensitive) const override;
        virtual bool isValid(int elem) const override;
        virtual bool asBool(int elem) const override;

        virtual size_t getBytes() const override;

        /////////////////////////////////////////////////
        /// \brief Returns the number of elements in this
        /// column (will also count invalid ones).
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t size() const override
        {
            return m_data.size();
        }

        virtual TableColumn* convert(ColumnType type = TableColumn::TYPE_NONE) override;

        /////////////////////////////////////////////////
        /// \brief Returns the list of internal
        /// categories to be used within expressions and
        /// code.
        ///
        /// \return const std::vector<std::string>&
        ///
        /////////////////////////////////////////////////
        const std::vector<std::string>& getCategories() const
        {
            return m_categories;
        }

        void setCategories(const std::vector<std::string>& vCategories);
};


void convert_if_empty(TblColPtr& col, size_t colNo, TableColumn::ColumnType type);
bool convert_if_needed(TblColPtr& col, size_t colNo, TableColumn::ColumnType type, bool convertSimilarTypes = false);
void convert_for_overwrite(TblColPtr& col, size_t colNo, TableColumn::ColumnType type);


#endif // TABLECOLUMNIMPL_HPP




