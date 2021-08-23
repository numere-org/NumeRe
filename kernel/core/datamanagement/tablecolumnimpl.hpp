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
    private:
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
            m_data.resize(nElem);
        }

        virtual ~ValueColumn() {}

        virtual std::string getValueAsString(int elem) const override;
        virtual std::string getValueAsInternalString(int elem) const override;
        virtual mu::value_type getValue(int elem) const override;

        virtual void setValue(int elem, const std::string& sValue) override;
        virtual void setValue(int elem, const mu::value_type& vValue) override;
        virtual void setValue(const VectorIndex& idx, const std::vector<std::string>& vValue) override;
        virtual void setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue) override;
        virtual void setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum) override;

        virtual ValueColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void insert(const VectorIndex& idx, const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;
        virtual void shrink() override;

        virtual void insertElements(size_t pos, size_t elem);
        virtual void appendElements(size_t elem);
        virtual void removeElements(size_t pos, size_t elem);

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

        StringColumn* convert() const;
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
        /// columns type.
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
            m_data.resize(nElem);
        }

        virtual ~StringColumn() {}

        virtual std::string getValueAsString(int elem) const override;
        virtual std::string getValueAsInternalString(int elem) const override;
        virtual mu::value_type getValue(int elem) const override;

        virtual void setValue(int elem, const std::string& sValue) override;
        virtual void setValue(int elem, const mu::value_type& vValue) override;
        virtual void setValue(const VectorIndex& idx, const std::vector<std::string>& vValue) override;
        virtual void setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue) override;
        virtual void setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum) override;

        virtual StringColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void insert(const VectorIndex& idx, const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;
        virtual void shrink() override;

        virtual void insertElements(size_t pos, size_t elem);
        virtual void appendElements(size_t elem);
        virtual void removeElements(size_t pos, size_t elem);

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

        ValueColumn* convert() const;
};


void convert_if_empty(TblColPtr& col, size_t colNo, TableColumn::ColumnType type);
void convert_if_needed(TblColPtr& col, size_t colNo, TableColumn::ColumnType type);
void convert_for_overwrite(TblColPtr& col, size_t colNo, TableColumn::ColumnType type);


#endif // TABLECOLUMNIMPL_HPP




