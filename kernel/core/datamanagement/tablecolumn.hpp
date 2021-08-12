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

#ifndef TABLECOLUMN_HPP
#define TABLECOLUMN_HPP

#include <string>
#include <vector>
#include "../ParserLib/muParserDef.h"
#include "../structures.hpp"

/////////////////////////////////////////////////
/// \brief Abstract table column, which allows
/// using it to compose the data table in each
/// Memory instance.
/////////////////////////////////////////////////
struct TableColumn
{
    enum ColumnType
    {
        TYPE_NONE,
        TYPE_VALUE,
        TYPE_STRING
    };

    std::string m_sHeadLine;
    ColumnType m_type;

    TableColumn() : m_type(TYPE_NONE) {}
    virtual ~TableColumn() {}

    std::vector<std::string> getValueAsString(const VectorIndex& idx) const;
    std::vector<mu::value_type> getValue(const VectorIndex& idx) const;

    virtual std::string getValueAsString(int elem) const = 0;
    virtual mu::value_type getValue(int elem) const = 0;

    virtual void setValue(const VectorIndex& idx, const std::vector<std::string>& sValue) = 0;
    virtual void setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue) = 0;
    virtual void setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum) = 0;

    virtual TableColumn* copy(const VectorIndex& idx) const = 0;
    virtual void assign(const TableColumn* column) = 0;
    virtual void deleteElements(const VectorIndex& idx) = 0;
    virtual size_t size() const = 0;
    virtual size_t getBytes() const = 0;
};


/////////////////////////////////////////////////
/// \brief A table column containing only
/// numerical values.
/////////////////////////////////////////////////
class ValueColumn : public TableColumn
{
    private:
        std::vector<mu::value_type> m_data;
        void shrink();

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
        virtual mu::value_type getValue(int elem) const override;
        virtual void setValue(const VectorIndex& idx, const std::vector<std::string>& vValue) override;
        virtual void setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue) override;
        virtual void setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum) override;

        virtual ValueColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;

        /////////////////////////////////////////////////
        /// \brief Return the number of bytes occupied by
        /// this column.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        virtual size_t getBytes() const override
        {
            return size() * sizeof(mu::value_type);
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


/////////////////////////////////////////////////
/// \brief A table column containing only strings
/// as values.
/////////////////////////////////////////////////
class StringColumn : public TableColumn
{
    private:
        std::vector<std::string> m_data;
        void shrink();

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
        virtual mu::value_type getValue(int elem) const override;
        virtual void setValue(const VectorIndex& idx, const std::vector<std::string>& vValue) override;
        virtual void setValue(const VectorIndex& idx, const std::vector<mu::value_type>& vValue) override;
        virtual void setValue(const VectorIndex& idx, mu::value_type* _dData, unsigned int _nNum) override;

        virtual StringColumn* copy(const VectorIndex& idx) const override;
        virtual void assign(const TableColumn* column) override;
        virtual void deleteElements(const VectorIndex& idx) override;
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
};


#endif // TABLECOLUMN_HPP



