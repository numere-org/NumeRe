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

/////////////////////////////////////////////////
/// \brief A table column containing only
/// numerical values.
/////////////////////////////////////////////////
template <class T, TableColumn::ColumnType COLTYPE>
class BaseValueColumn : public TableColumn
{
    protected:
        std::vector<T> m_data;
        T INVALID_VALUE = T(NAN);

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        BaseValueColumn() : TableColumn()
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
        BaseValueColumn(size_t nElem) : BaseValueColumn()
        {
            resize(nElem);
        }

        virtual ~BaseValueColumn() {}

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
            if (elem < m_data.size())
                return toCmdString(m_data[elem]);

            return toString(mu::value_type(INVALID_VALUE));
        }

        /////////////////////////////////////////////////
        /// \brief Returns the contents as an internal
        /// string (i.e. without quotation marks).
        ///
        /// \param elem size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        virtual std::string getValueAsInternalString(size_t elem) const override
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
                return toString(mu::value_type(m_data[elem]), NumeReKernel::getInstance()->getSettings().getPrecision());

            return toString(mu::value_type(INVALID_VALUE));
        }

        /////////////////////////////////////////////////
        /// \brief Returns the selected value as a
        /// numerical type or an invalid value, if it
        /// does not exist.
        ///
        /// \param elem size_t
        /// \return mu::value_type
        ///
        /////////////////////////////////////////////////
        virtual mu::value_type getValue(size_t elem) const override
        {
            if (elem < m_data.size())
                return m_data[elem];

            return INVALID_VALUE;
        }

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
                TableColumn::setValue(elem, StrToCmplx(toInternalString(sValue)));
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
                m_sHeadLine = column->m_sHeadLine;
                m_data = static_cast<const BaseValueColumn*>(column)->m_data;
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
                TableColumn::setValue(idx, static_cast<const BaseValueColumn*>(column)->TableColumn::getValue(VectorIndex(0, VectorIndex::OPEN_END)));
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
                return;
            }

            for (size_t i = 0; i < idx.size(); i++)
            {
                if (idx[i] >= 0 && idx[i] < (int)m_data.size())
                    m_data[idx[i]] = INVALID_VALUE;
            }

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
                m_data.insert(m_data.begin()+pos, elem, INVALID_VALUE);
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
                m_data.erase(m_data.begin()+pos, m_data.begin()+pos+elem);
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
            if (elem >= (int)m_data.size() || elem < 0 || mu::isnan(m_data[elem]))
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
            if (elem < 0 || elem >= (int)m_data.size())
                return false;

            return m_data[elem] != 0.0;
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
                    return nullptr;
            }

            col->m_sHeadLine = m_sHeadLine;
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


class ValueColumn : public BaseValueColumn<mu::value_type, TableColumn::TYPE_VALUE>
{
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        ValueColumn() : BaseValueColumn()
        {
        }

        /////////////////////////////////////////////////
        /// \brief Generalized constructor. Will prepare
        /// a column with the specified size.
        ///
        /// \param nElem size_t
        ///
        /////////////////////////////////////////////////
        ValueColumn(size_t nElem) : ValueColumn()
        {
            resize(nElem);
        }

        virtual ~ValueColumn() {}

        /////////////////////////////////////////////////
        /// \brief Set a single numerical value.
        ///
        /// \param elem size_t
        /// \param vValue const mu::value_type&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void setValue(size_t elem, const mu::value_type& vValue) override
        {
            if (elem >= m_data.size() && mu::isnan(vValue))
                return;

            if (elem >= m_data.size())
                m_data.resize(elem+1, INVALID_VALUE);

            m_data[elem] = vValue;
        }

        /////////////////////////////////////////////////
        /// \brief Creates a copy of the selected part of
        /// this column. Can be used for simple
        /// extraction into a new table.
        ///
        /// \param idx const VectorIndex&
        /// \return ValueColumn*
        ///
        /////////////////////////////////////////////////
        virtual ValueColumn* copy(const VectorIndex& idx) const override
        {
            idx.setOpenEndIndex(size()-1);

            ValueColumn* col = new ValueColumn(idx.size());
            col->m_sHeadLine = m_sHeadLine;

            for (size_t i = 0; i < idx.size(); i++)
            {
                col->m_data[i] = getValue(idx[i]);
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
            if ((int)m_data.size() <= std::max(i, j))
                return 0;

            if (m_data[i] == m_data[j])
                return 0;
            else if (m_data[i].real() < m_data[j].real())
                return -1;

            return 1;
        }
};

template <class T, TableColumn::ColumnType COLTYPE>
class BaseIntColumn : public BaseValueColumn<T, COLTYPE>
{
    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        BaseIntColumn() : BaseValueColumn<T, COLTYPE>()
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
        /// \param vValue const mu::value_type&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void setValue(size_t elem, const mu::value_type& vValue) override
        {
            if (elem >= this->m_data.size() && mu::isnan(vValue))
                return;

            if (elem >= this->m_data.size())
                this->m_data.resize(elem+1, this->INVALID_VALUE);

            this->m_data[elem] = (T)intCast(vValue);
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
            idx.setOpenEndIndex(this->size()-1);

            BaseIntColumn* col = new BaseIntColumn(idx.size());
            col->m_sHeadLine = this->m_sHeadLine;

            for (size_t i = 0; i < idx.size(); i++)
            {
                col->m_data[i] = (T)intCast(this->getValue(idx[i]));
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

//using ValueColumn = BaseValueColumn<mu::value_type, TableColumn::TYPE_VALUE>;
using IntValueColumn = BaseIntColumn<int, TableColumn::TYPE_VALUE>;


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
        virtual mu::value_type getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const mu::value_type& vValue) override;

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
        virtual mu::value_type getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const mu::value_type& vValue) override;

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
        virtual mu::value_type getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const mu::value_type& vValue) override;

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
        virtual mu::value_type getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const mu::value_type& vValue) override;

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




