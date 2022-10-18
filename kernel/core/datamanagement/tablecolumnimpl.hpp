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

// Forward declaration for ValueColumn::convert()
class StringColumn;

/////////////////////////////////////////////////
/// \brief A table column containing only
/// numerical values.
/////////////////////////////////////////////////
class ValueColumn : public TableColumn
{
    protected:
        std::vector<mu::value_type> m_data;

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Sets only the
        ///  type of the column.
        /////////////////////////////////////////////////
        ValueColumn() : TableColumn()
        {
            m_type = TableColumn::TYPE_VALUE;
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

        virtual std::string getValueAsString(size_t elem) const override;
        virtual std::string getValueAsInternalString(size_t elem) const override;
        virtual std::string getValueAsParserString(size_t elem) const override;
        virtual std::string getValueAsStringLiteral(size_t elem) const override;
        virtual mu::value_type getValue(size_t elem) const override;

        virtual void setValue(size_t elem, const std::string& sValue) override;
        virtual void setValue(size_t elem, const mu::value_type& vValue) override;

        virtual ValueColumn* copy(const VectorIndex& idx) const override;
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
            return size() * sizeof(mu::value_type) + m_sHeadLine.length() * sizeof(char);
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
            return size() * sizeof(double) + m_sHeadLine.length() * sizeof(char);
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
            return size() * sizeof(LogicalValue) + m_sHeadLine.length() * sizeof(char);
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

        std::vector<size_t> m_data;
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
void convert_if_needed(TblColPtr& col, size_t colNo, TableColumn::ColumnType type);
void convert_for_overwrite(TblColPtr& col, size_t colNo, TableColumn::ColumnType type);


#endif // TABLECOLUMNIMPL_HPP




