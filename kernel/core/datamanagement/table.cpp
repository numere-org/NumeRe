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

#include "table.hpp"
#include "../utils/tools.hpp"
#include "tablecolumnimpl.hpp"
#include <cmath>
#include <cstdio>

using namespace std;

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief Default constructor
    /////////////////////////////////////////////////
    Table::Table()
    {
        sTableName.clear();
    }


    /////////////////////////////////////////////////
    /// \brief Size constructor
    ///
    /// \param nLines int
    /// \param nCols int
    ///
    /////////////////////////////////////////////////
    Table::Table(int nLines, int nCols) : Table()
    {
        setSize(nLines, nCols);
    }


    /////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    /// \param _table const Table&
    ///
    /////////////////////////////////////////////////
    Table::Table(const Table& _table) : sTableName(_table.sTableName), m_meta(_table.m_meta)
    {
        vTableData.resize(_table.vTableData.size());

        for (size_t i = 0; i < vTableData.size(); i++)
        {
            if (_table.vTableData[i])
                vTableData[i].reset(_table.vTableData[i]->copy());
        }
    }


    /////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    /// \param _table Table&&
    ///
    /////////////////////////////////////////////////
    Table::Table(Table&& _table) : sTableName(_table.sTableName), m_meta(_table.m_meta)
    {
        // We move by using the std::swap() functions
        std::swap(vTableData, _table.vTableData);
    }


    /////////////////////////////////////////////////
    /// \brief Destructor
    /////////////////////////////////////////////////
    Table::~Table()
    {
        vTableData.clear();
    }


    /////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    /// \param _table Table
    /// \return Table&
    ///
    /////////////////////////////////////////////////
    Table& Table::operator=(Table _table)
    {
        // We move by using the std::swap() functions
        std::swap(vTableData, _table.vTableData);
        sTableName = _table.sTableName;
        m_meta = _table.m_meta;

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief This member function prepares a table
    /// with the minimal size of the selected lines
    /// and columns.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setMinSize(size_t i, size_t j)
    {
        if (vTableData.size() <= j)
            vTableData.resize(j);
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a simple
    /// helper function to determine, whether a
    /// passed value may be parsed into a numerical
    /// value.
    ///
    /// \param sValue const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::isNumerical(const std::string& sValue) const
    {
        return isConvertible(sValue, CONVTYPE_VALUE);
    }


    /////////////////////////////////////////////////
    /// \brief This member function cleares the
    /// contents of this table.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::Clear()
    {
        vTableData.clear();
        sTableName.clear();
    }


    /////////////////////////////////////////////////
    /// \brief This member function simply redirects
    /// the control to setMinSize().
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setSize(size_t i, size_t j)
    {
        this->setMinSize(i, j);

        for (auto& col : vTableData)
        {
            if (col)
                col->resize(i);
            else
                col.reset(new ValueColumn(i));
        }
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the table name.
    ///
    /// \param _sName const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setName(const std::string& _sName)
    {
        sTableName = _sName;
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the table meta
    /// data.
    ///
    /// \param meta const TableMetaData&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setMetaData(const TableMetaData& meta)
    {
        m_meta = meta;
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the table comment.
    ///
    /// \param _comment const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setComment(const std::string& _comment)
    {
        m_meta.comment = _comment;
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the selected
    /// column headline. Will create missing
    /// headlines automatically.
    ///
    /// \param i size_t
    /// \param _sHead const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setHead(size_t i, const std::string& _sHead)
    {
        if (i >= vTableData.size())
            vTableData.resize(i+1);

        if (!vTableData[i])
            vTableData[i].reset(new ValueColumn);

        vTableData[i]->m_sHeadLine = _sHead;
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the selected
    /// column headline and the selected part of the
    /// headline (split using linebreak characters).
    /// Will create the missing headlines
    /// automatically.
    ///
    /// \param i size_t
    /// \param part size_t
    /// \param _sHead const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setHeadPart(size_t i, size_t part, const std::string& _sHead)
    {
        if (i >= vTableData.size())
            vTableData.resize(i+1);

        if (!vTableData[i])
            vTableData[i].reset(new ValueColumn);

        std::string head;

        // Compose the new headline using the different
        // parts
        for (int j = 0; j < getHeadCount(); j++)
        {
            if (j == (int)part)
                head += _sHead + '\n';
            else
                head += getCleanHeadPart(i, j) + '\n';
        }

        head.pop_back();

        vTableData[i]->m_sHeadLine = head;
    }


    /////////////////////////////////////////////////
    /// \brief This member function sets the data to
    /// the table. It will resize the table
    /// automatically, if needed.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param _dValue const mu::value_type&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setValue(size_t i, size_t j, const mu::value_type& _dValue)
    {
        this->setMinSize(i+1, j+1);

        convert_if_empty(vTableData[j], j, TableColumn::TYPE_VALUE);
        vTableData[j]->setValue(i, _dValue);
    }


    /////////////////////////////////////////////////
    /// \brief This member function sets the data,
    /// which is passed as a string, to the table. If
    /// the data is not a numerical value, the data
    /// is assigned to the corresponding column head.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param _sValue const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setValueAsString(size_t i, size_t j, const std::string& _sValue)
    {
        // Resize the table if needed
        this->setMinSize(i+1, j+1);

        if (vTableData[j])
            vTableData[j]->deleteElements(VectorIndex(i));

        // Empty value means only deletion
        if (!_sValue.length())
            return;

        // Is it a numerical value?
        if (isConvertible(_sValue, CONVTYPE_DATE_TIME))
        {
            convert_if_needed(vTableData[j], j, TableColumn::TYPE_DATETIME);
            vTableData[j]->setValue(i, to_double(StrToTime(_sValue)));
        }
        else if (isConvertible(_sValue, CONVTYPE_VALUE))
        {
            convert_if_needed(vTableData[j], j, TableColumn::TYPE_VALUE);
            vTableData[j]->setValue(i, StrToCmplx(_sValue));
        }
        else if (isConvertible(_sValue, CONVTYPE_LOGICAL))
        {
            convert_if_needed(vTableData[j], j, TableColumn::TYPE_LOGICAL);
            vTableData[j]->setValue(i, StrToLogical(_sValue));
        }
        else
        {
            convert_if_needed(vTableData[j], j, TableColumn::TYPE_STRING);
            vTableData[j]->setValue(i, _sValue);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Assigns a whole column to the internal
    /// array.
    ///
    /// \param j size_t
    /// \param column TableColumn*
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setColumn(size_t j, TableColumn* column)
    {
        if (vTableData.size() <= j)
            vTableData.resize(j+1);

        vTableData[j].reset(column);
    }


    /////////////////////////////////////////////////
    /// \brief Tries to change the column type of the
    /// selected column.
    ///
    /// \param j size_t
    /// \param _type TableColumn::ColumnType
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::setColumnType(size_t j, TableColumn::ColumnType _type)
    {
        if (j >= vTableData.size())
            return false;

        return convert_if_needed(vTableData[j], j, _type, true);
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the table name.
    ///
    /// \return string
    ///
    /////////////////////////////////////////////////
    std::string Table::getName() const
    {
        return sTableName;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the table meta
    /// data.
    ///
    /// \return TableMetaData
    ///
    /////////////////////////////////////////////////
    TableMetaData Table::getMetaData() const
    {
        return m_meta;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the table comment.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Table::getComment() const
    {
        return m_meta.comment;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the needed number
    /// of headlines (depending on the number of
    /// linebreaks found in the headlines).
    ///
    /// \return int
    ///
    /////////////////////////////////////////////////
    int Table::getHeadCount() const
    {
        int nHeadlineCount = 1;

        // Get the dimensions of the complete headline (i.e. including possible linebreaks)
        for (size_t j = 0; j < vTableData.size(); j++)
        {
            if (!vTableData[j])
                continue;

            // No linebreak? Continue
            if (vTableData[j]->m_sHeadLine.find('\n') == std::string::npos)
                continue;

            int nLinebreak = 0;

            // Count all linebreaks
            for (unsigned int n = 0; n < vTableData[j]->m_sHeadLine.length() - 2; n++)
            {
                if (vTableData[j]->m_sHeadLine[n] == '\n')
                    nLinebreak++;
            }

            // Save the maximal number
            if (nLinebreak + 1 > nHeadlineCount)
                nHeadlineCount = nLinebreak + 1;
        }

        return nHeadlineCount;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the selected
    /// column's headline.
    ///
    /// \param i size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Table::getHead(size_t i) const
    {
        if (vTableData.size() > i && vTableData[i])
            return vTableData[i]->m_sHeadLine;

        return TableColumn::getDefaultColumnHead(i);
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the selected
    /// column's headline. Underscores and masked
    /// headlines are replaced on-the-fly.
    ///
    /// \param i size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Table::getCleanHead(size_t i) const
    {
        if (vTableData.size() > i && vTableData[i])
            return vTableData[i]->m_sHeadLine;

        return TableColumn::getDefaultColumnHead(i);
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the selected part
    /// of the selected column's headline.
    /// Underscores and masked headlines are replaced
    /// on-the-fly.
    ///
    /// \param i size_t
    /// \param part size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Table::getCleanHeadPart(size_t i, size_t part) const
    {
        if (vTableData.size() > i && vTableData[i] && part < (size_t)getHeadCount())
        {
            // get the cleaned headline
            std::string head = getCleanHead(i);

            // Simple case: only one line
            if (head.find('\n') == string::npos)
            {
                if (part)
                    return "";
                return head;
            }

            size_t pos = 0;

            // Complex case: find the selected part
            for (size_t j = 0; j < head.length(); j++)
            {
                if (head[j] == '\n')
                {
                    if (!part)
                        return head.substr(pos, j - pos);

                    part--;
                    pos = j+1;
                }
            }

            // part is zero and position not: return
            // the last part
            if (!part && pos)
                return head.substr(pos);

            return "";
        }

        if (!part)
            return TableColumn::getDefaultColumnHead(i);

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the value of the
    /// selected cell.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return mu::value_type
    ///
    /////////////////////////////////////////////////
    mu::value_type Table::getValue(size_t i, size_t j) const
    {
        if (vTableData.size() > j && vTableData[j])
            return vTableData[j]->getValue(i);

        return NAN;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the value of the
    /// selected cell. The value is converted into a
    /// string.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Table::getValueAsString(size_t i, size_t j) const
    {
        if (vTableData.size() > j && vTableData[j])
        {
            // Invalid number
            if (!vTableData[j]->isValid(i))
                return "---";

            return vTableData[j]->getValueAsStringLiteral(i);
        }

        return "---";
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the value of the
    /// selected cell. The value is converted into an
    /// internal string.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Table::getValueAsInternalString(size_t i, size_t j) const
    {
        if (vTableData.size() > j && vTableData[j] && vTableData[j]->isValid(i))
            return vTableData[j]->getValueAsInternalString(i);

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Returns a copy of the internal column
    /// array or a nullptr, if the column does not
    /// exist or is empty.
    ///
    /// \param j size_t
    /// \return TableColumn*
    ///
    /////////////////////////////////////////////////
    TableColumn* Table::getColumn(size_t j) const
    {
        if (j < vTableData.size() && vTableData[j])
            return vTableData[j]->copy();

        return nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Returns the type of the selected
    /// column.
    ///
    /// \param j size_t
    /// \return TableColumn::ColumnType
    ///
    /////////////////////////////////////////////////
    TableColumn::ColumnType Table::getColumnType(size_t j) const
    {
        if (j < vTableData.size() && vTableData[j])
            return vTableData[j]->m_type;

        return TableColumn::TYPE_NONE;
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of lines.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Table::getLines() const
    {
        size_t lines = 0;

        for (const TblColPtr& col : vTableData)
        {
            if (col && col->size() > lines)
                lines = col->size();
        }

        return lines;
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of columns.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Table::getCols() const
    {
        return vTableData.size();
    }


    /////////////////////////////////////////////////
    /// \brief Return, whether the table is empty.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::isEmpty() const
    {
        return vTableData.empty();
    }


    /////////////////////////////////////////////////
    /// \brief This member function inserts lines at
    /// the desired position.
    ///
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::insertLines(size_t nPos, size_t nNum)
    {
        nPos -= getHeadCount();

        for (TblColPtr& col : vTableData)
        {
            if (col)
                col->insertElements(nPos, nNum);
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function appends lines.
    ///
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::appendLines(size_t nNum)
    {
        for (TblColPtr& col : vTableData)
        {
            if (col)
                col->appendElements(nNum);
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function deletes lines at
    /// the desired position.
    ///
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::deleteLines(size_t nPos, size_t nNum)
    {
        nPos -= getHeadCount();

        for (TblColPtr& col : vTableData)
        {
            if (col)
                col->removeElements(nPos, nNum);
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function inserts columns
    /// at the desired position.
    ///
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::insertCols(size_t nPos, size_t nNum)
    {
        TableColumnArray arr(nNum);
        vTableData.insert(vTableData.begin()+nPos, std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function appends columns.
    ///
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::appendCols(size_t nNum)
    {
        TableColumnArray arr(nNum);
        vTableData.insert(vTableData.end(), std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function delets columns at
    /// the desired position.
    ///
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::deleteCols(size_t nPos, size_t nNum)
    {
        vTableData.erase(vTableData.begin()+nPos, vTableData.begin()+nPos+nNum);
        return true;
    }

}

