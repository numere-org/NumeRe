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

#include "tableview.hpp"
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
    TableView::TableView()
    {
        sTableName.clear();
    }


    /////////////////////////////////////////////////
    /// \brief Size constructor
    ///
    /// \param nLines int
    /// \param nCols int
    /// \param sheets int
    ///
    /////////////////////////////////////////////////
    TableView::TableView(int nLines, int nCols, int sheets) : TableView()
    {
        setSize(nLines, nCols, sheets);
    }


    /////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    /// \param _table const Table&
    ///
    /////////////////////////////////////////////////
    TableView::TableView(const TableView& _table) : sTableName(_table.sTableName), m_meta(_table.m_meta)
    {
        vTableData = _table.vTableData;
    }


    /////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    /// \param _table TableData*
    ///
    /////////////////////////////////////////////////
    TableView::TableView(TableData* _table) : sTableName("")
    {
        vTableData.reset(_table);
    }


    /////////////////////////////////////////////////
    /// \brief Destructor
    /////////////////////////////////////////////////
    TableView::~TableView()
    {
        vTableData.reset();
    }


    /////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    /// \param _table Table
    /// \return Table&
    ///
    /////////////////////////////////////////////////
    TableView& TableView::operator=(TableView _table)
    {
        // We move by using the std::swap() functions
        vTableData = _table.vTableData;
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
    /// \param sheet size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setMinSize(size_t i, size_t j, size_t sheet)
    {
        if (!vTableData)
            vTableData.reset(new TableData);

        vTableData->reserve(sheet);

        for (size_t k = 0; k < sheet; k++)
        {
            vTableData->m_sheets[k]->reserve(j);
        }
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
    bool TableView::isNumerical(const std::string& sValue) const
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
    void TableView::Clear()
    {
        vTableData.reset();
        sTableName.clear();
    }


    /////////////////////////////////////////////////
    /// \brief This member function simply redirects
    /// the control to setMinSize().
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param sheet size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setSize(size_t i, size_t j, size_t sheet)
    {
        this->setMinSize(i, j, sheet);

        for (size_t k = 0; k < sheet; k++)
        {
            for (auto& col : vTableData->getSheet(k).getCols())
            {
                if (col)
                    col->resize(i);
                else
                    col.reset(new ValueColumn(i));
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the table name.
    ///
    /// \param _sName const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setName(const std::string& _sName)
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
    void TableView::setMetaData(const TableMetaData& meta)
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
    void TableView::setComment(const std::string& _comment)
    {
        m_meta.comment = _comment;
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the selected
    /// column headline. Will create missing
    /// headlines automatically.
    ///
    /// \param i size_t
    /// \param sheet size_t
    /// \param _sHead const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setHead(size_t i, size_t sheet, const std::string& _sHead)
    {
        if (!vTableData)
            vTableData.reset(new TableData);

        vTableData->reserve(sheet+1);
        TableSheet& currSheet = vTableData->getSheet(sheet);

        if (i >= currSheet.m_columns.size())
            currSheet.m_columns.resize(i+1);

        if (!currSheet.m_columns[i])
            currSheet.m_columns[i].reset(new ValueColumn);

        currSheet.m_columns[i]->m_sHeadLine = _sHead;
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the selected
    /// column headline and the selected part of the
    /// headline (split using linebreak characters).
    /// Will create the missing headlines
    /// automatically.
    ///
    /// \param i size_t
    /// \param sheet size_t
    /// \param part size_t
    /// \param _sHead const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setHeadPart(size_t i, size_t sheet, size_t part, const std::string& _sHead)
    {
        if (!vTableData)
            vTableData.reset(new TableData);

        vTableData->reserve(sheet+1);
        TableSheet& currSheet = vTableData->getSheet(sheet);

        if (i >= currSheet.m_columns.size())
            currSheet.m_columns.resize(i+1);

        if (!currSheet.m_columns[i])
            currSheet.m_columns[i].reset(new ValueColumn);

        std::string head;

        // Compose the new headline using the different
        // parts
        for (int j = 0; j < getHeadCount(sheet); j++)
        {
            if (j == (int)part)
                head += _sHead + '\n';
            else
                head += getCleanHeadPart(i, sheet, j) + '\n';
        }

		if (head.length())
			head.pop_back();

        currSheet.m_columns[i]->m_sHeadLine = head;
    }


    /////////////////////////////////////////////////
    /// \brief This member function sets the data to
    /// the table. It will resize the table
    /// automatically, if needed.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param sheet size_t
    /// \param _dValue const mu::Value&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::set(size_t i, size_t j, size_t sheet, const mu::Value& _dValue)
    {
        this->setMinSize(i+1, j+1, sheet+1);
        TableSheet& currSheet = vTableData->getSheet(sheet);

        convert_if_empty(currSheet.m_columns[j], j, to_column_type(_dValue));
        currSheet.m_columns[j]->set(i, _dValue);
    }


    /////////////////////////////////////////////////
    /// \brief This member function sets the data to
    /// the table. It will resize the table
    /// automatically, if needed.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param sheet size_t
    /// \param _dValue const std::complex<double>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setValue(size_t i, size_t j, size_t sheet, const std::complex<double>& _dValue)
    {
        this->setMinSize(i+1, j+1, sheet+1);
        TableSheet& currSheet = vTableData->getSheet(sheet);

        convert_if_empty(currSheet.m_columns[j], j, TableColumn::TYPE_VALUE_CF64);
        currSheet.m_columns[j]->setValue(i, _dValue);
    }


    /////////////////////////////////////////////////
    /// \brief This member function sets the data,
    /// which is passed as a string, to the table. If
    /// the data is not a numerical value, the data
    /// is assigned to the corresponding column head.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param sheet size_t
    /// \param _sValue const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setValueAsString(size_t i, size_t j, size_t sheet, const std::string& _sValue)
    {
        // Resize the table if needed
        this->setMinSize(i+1, j+1, sheet+1);
        TableSheet& currSheet = vTableData->getSheet(sheet);

        // Empty value means only deletion
        if (!_sValue.length())
        {
            if (currSheet.m_columns[j])
                currSheet.m_columns[j]->deleteElements(VectorIndex(i));

            return;
        }
        else if (currSheet.m_columns[j]
                 && currSheet.m_columns[j]->m_type == TableColumn::TYPE_CATEGORICAL
                 && static_cast<CategoricalColumn*>(currSheet.m_columns[j].get())->isCategory(toInternalString(_sValue)))
        {
            currSheet.m_columns[j]->setValue(i, toInternalString(_sValue));
        }
        else if (isConvertible(_sValue, CONVTYPE_DURATION))
        {
            if (currSheet.m_columns[j])
                currSheet.m_columns[j]->deleteElements(VectorIndex(i));

            convert_if_needed(currSheet.m_columns[j], j, TableColumn::TYPE_DURATION, false);
            currSheet.m_columns[j]->setValue(i, parseDuration(_sValue));
        }
        else if (isConvertible(_sValue, CONVTYPE_DATE_TIME))
        {
            if (currSheet.m_columns[j])
                currSheet.m_columns[j]->deleteElements(VectorIndex(i));

            convert_if_needed(currSheet.m_columns[j], j, TableColumn::TYPE_DATETIME, false);
            currSheet.m_columns[j]->setValue(i, to_double(StrToTime(_sValue)));
        }
        else if (isConvertible(_sValue, CONVTYPE_VALUE))
        {
            if (currSheet.m_columns[j])
                currSheet.m_columns[j]->deleteElements(VectorIndex(i));

            std::complex<double> val = StrToCmplx(_sValue);
            convert_if_needed(currSheet.m_columns[j], j, TableColumn::TYPE_VALUE_CF64, val.imag() != 0.0);
            currSheet.m_columns[j]->setValue(i, val);
        }
        else if (isConvertible(_sValue, CONVTYPE_LOGICAL))
        {
            if (currSheet.m_columns[j])
                currSheet.m_columns[j]->deleteElements(VectorIndex(i));

            convert_if_needed(currSheet.m_columns[j], j, TableColumn::TYPE_LOGICAL, false);
            currSheet.m_columns[j]->setValue(i, StrToLogical(_sValue));
        }
        else
        {
            if (currSheet.m_columns[j])
                currSheet.m_columns[j]->deleteElements(VectorIndex(i));

            convert_if_needed(currSheet.m_columns[j], j, TableColumn::TYPE_STRING, false);
            currSheet.m_columns[j]->setValue(i, toInternalString(_sValue));
        }
    }


    /////////////////////////////////////////////////
    /// \brief Assigns a whole column to the internal
    /// array.
    ///
    /// \param j size_t
    /// \param sheet size_t
    /// \param column TableColumn*
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TableView::setColumn(size_t j, size_t sheet, TableColumn* column)
    {
        if (!vTableData)
            vTableData.reset(new TableData);

        vTableData->reserve(sheet+1);
        TableSheet& currSheet = vTableData->getSheet(sheet);

        if (currSheet.m_columns.size() <= j)
            currSheet.m_columns.resize(j+1);

        currSheet.m_columns[j].reset(column);
        currSheet.m_columns[j]->shrink();
    }


    /////////////////////////////////////////////////
    /// \brief Tries to change the column type of the
    /// selected column.
    ///
    /// \param j size_t
    /// \param sheet size_t
    /// \param _type TableColumn::ColumnType
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::setColumnType(size_t j, size_t sheet, TableColumn::ColumnType _type)
    {
        if (!vTableData)
            vTableData.reset(new TableData);

        vTableData->reserve(sheet+1);
        TableSheet& currSheet = vTableData->getSheet(sheet);

        if (j >= currSheet.m_columns.size())
            return false;

        return convert_if_needed(currSheet.m_columns[j], j, _type, true);
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the table name.
    ///
    /// \return string
    ///
    /////////////////////////////////////////////////
    std::string TableView::getName() const
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
    TableMetaData TableView::getMetaData() const
    {
        return m_meta;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the table comment.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string TableView::getComment() const
    {
        return m_meta.comment;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the needed number
    /// of headlines (depending on the number of
    /// linebreaks found in the headlines).
    ///
    /// \param sheet size_t
    /// \return int
    ///
    /////////////////////////////////////////////////
    int TableView::getHeadCount(size_t sheet) const
    {
        if (sheet >= vTableData->sheets())
            return 0;

        const TableSheet& currSheet = vTableData->getSheet(sheet);

        int nHeadlineCount = 1;

        // Get the dimensions of the complete headline (i.e. including possible linebreaks)
        for (size_t j = 0; j < currSheet.m_columns.size(); j++)
        {
            if (!currSheet.m_columns[j])
                continue;

            // No linebreak? Continue
            if (currSheet.m_columns[j]->m_sHeadLine.find('\n') == std::string::npos)
                continue;

            int nLinebreak = 0;

            // Count all linebreaks
            for (size_t n = 0; n < currSheet.m_columns[j]->m_sHeadLine.length(); n++)
            {
                if (currSheet.m_columns[j]->m_sHeadLine[n] == '\n')
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
    /// \param sheet size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string TableView::getHead(size_t i, size_t sheet) const
    {
        if (sheet < vTableData->sheets() && vTableData->getSheet(sheet).isValidColumn(i))
            return vTableData->getSheet(sheet).m_columns[i]->m_sHeadLine;

        return TableColumn::getDefaultColumnHead(i);
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the selected
    /// column's headline. Underscores and masked
    /// headlines are replaced on-the-fly.
    ///
    /// \param i size_t
    /// \param sheet size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string TableView::getCleanHead(size_t i, size_t sheet) const
    {
        if (sheet < vTableData->sheets() && vTableData->getSheet(sheet).isValidColumn(i))
            return vTableData->getSheet(sheet).m_columns[i]->m_sHeadLine;

        return TableColumn::getDefaultColumnHead(i);
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the selected part
    /// of the selected column's headline.
    /// Underscores and masked headlines are replaced
    /// on-the-fly.
    ///
    /// \param i size_t
    /// \param sheet size_t
    /// \param part size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string TableView::getCleanHeadPart(size_t i, size_t sheet, size_t part) const
    {
        if (sheet < vTableData->sheets() && vTableData->getSheet(sheet).isValidColumn(i) && part < (size_t)getHeadCount(sheet))
        {
            // get the cleaned headline
            std::string head = getCleanHead(i, sheet);

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
    /// \param sheet size_t
    /// \return mu::Value
    ///
    /////////////////////////////////////////////////
    mu::Value TableView::get(size_t i, size_t j, size_t sheet) const
    {
        if (vTableData->sheets() > sheet && vTableData->getSheet(sheet).isValidColumn(j))
            return vTableData->getSheet(sheet).m_columns[j]->get(i);

        return mu::Value();
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the value of the
    /// selected cell.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param sheet size_t
    /// \return std::complex<double>
    ///
    /////////////////////////////////////////////////
    std::complex<double> TableView::getValue(size_t i, size_t j, size_t sheet) const
    {
        if (vTableData->sheets() > sheet && vTableData->getSheet(sheet).isValidColumn(j))
            return vTableData->getSheet(sheet).m_columns[j]->getValue(i);

        return NAN;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the value of the
    /// selected cell. The value is converted into a
    /// string.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param sheet size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string TableView::getValueAsString(size_t i, size_t j, size_t sheet) const
    {
        constexpr size_t MAXSTRINGLENGTH = 1024;

        if (vTableData->sheets() > sheet && vTableData->getSheet(sheet).isValidColumn(j))
        {
            // Invalid number
            if (!vTableData->getSheet(sheet).m_columns[j]->isValid(i))
                return "---";

            return truncString(vTableData->getSheet(sheet).m_columns[j]->getValueAsStringLiteral(i), MAXSTRINGLENGTH);
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
    /// \param sheet size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string TableView::getValueAsInternalString(size_t i, size_t j, size_t sheet) const
    {
        if (vTableData->sheets() > sheet && vTableData->getSheet(sheet).isValidColumn(j))
            return vTableData->getSheet(sheet).m_columns[j]->getValueAsInternalString(i);

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Returns a copy of the internal column
    /// array or a nullptr, if the column does not
    /// exist or is empty.
    ///
    /// \param j size_t
    /// \param sheet size_t
    /// \return TableColumn*
    ///
    /////////////////////////////////////////////////
    TableColumn* TableView::getColumn(size_t j, size_t sheet) const
    {
        if (vTableData->sheets() > sheet && vTableData->getSheet(sheet).isValidColumn(j))
            return vTableData->getSheet(sheet).m_columns[j]->copy();

        return nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Returns the type of the selected
    /// column.
    ///
    /// \param j size_t
    /// \param sheet size_t
    /// \return TableColumn::ColumnType
    ///
    /////////////////////////////////////////////////
    TableColumn::ColumnType TableView::getColumnType(size_t j, size_t sheet) const
    {
        if (vTableData->sheets() > sheet && vTableData->getSheet(sheet).isValidColumn(j))
            return vTableData->getSheet(sheet).m_columns[j]->m_type;

        return TableColumn::TYPE_NONE;
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of lines.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t TableView::getRows() const
    {
        return vTableData->rows();
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of lines.
    ///
    /// \param sheet size_t
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t TableView::getRows(size_t sheet) const
    {
        if (sheet < vTableData->sheets())
            return vTableData->getSheet(sheet).rows();

        return 0;
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of columns.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t TableView::getCols() const
    {
        return vTableData->cols();
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of columns.
    ///
    /// \param sheet size_t
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t TableView::getCols(size_t sheet) const
    {
        if (sheet < vTableData->sheets())
            return vTableData->getSheet(sheet).cols();

        return 0;
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of sheets.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t TableView::getSheets() const
    {
        return vTableData->sheets();
    }


    /////////////////////////////////////////////////
    /// \brief Return, whether the table is empty.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::isEmpty() const
    {
        return !vTableData || vTableData->empty();
    }


    /////////////////////////////////////////////////
    /// \brief This member function inserts rows at
    /// the desired position.
    ///
    /// \param sheet size_t
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::insertRows(size_t sheet, size_t nPos, size_t nNum)
    {
        if (sheet >= vTableData->sheets())
            return false;

        TableSheet& currSheet = vTableData->getSheet(sheet);

        for (TblColPtr& col : currSheet.m_columns)
        {
            if (col)
                col->insertElements(nPos, nNum);
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function appends rows.
    ///
    /// \param sheet size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::appendRows(size_t sheet, size_t nNum)
    {
        if (sheet >= vTableData->sheets())
            return false;

        TableSheet& currSheet = vTableData->getSheet(sheet);

        for (TblColPtr& col : currSheet.m_columns)
        {
            if (col)
                col->appendElements(nNum);
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function deletes rows at
    /// the desired position.
    ///
    /// \param sheet size_t
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::deleteRows(size_t sheet, size_t nPos, size_t nNum)
    {
        if (sheet >= vTableData->sheets())
            return false;

        TableSheet& currSheet = vTableData->getSheet(sheet);

        for (TblColPtr& col : currSheet.m_columns)
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
    /// \param sheet size_t
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::insertCols(size_t sheet, size_t nPos, size_t nNum)
    {
        if (sheet >= vTableData->sheets())
            return false;

        TableSheet& currSheet = vTableData->getSheet(sheet);

        TableColumnArray arr(nNum);
        currSheet.m_columns.insert(currSheet.m_columns.begin()+nPos,
                                   std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function appends columns.
    ///
    /// \param sheet size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::appendCols(size_t sheet, size_t nNum)
    {
        if (sheet >= vTableData->sheets())
            return false;

        TableSheet& currSheet = vTableData->getSheet(sheet);

        TableColumnArray arr(nNum);
        currSheet.m_columns.insert(currSheet.m_columns.end(),
                                   std::make_move_iterator(arr.begin()), std::make_move_iterator(arr.end()));
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function deletes columns at
    /// the desired position.
    ///
    /// \param sheet size_t
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::deleteCols(size_t sheet, size_t nPos, size_t nNum)
    {
        if (sheet >= vTableData->sheets())
            return false;

        TableSheet& currSheet = vTableData->getSheet(sheet);

        currSheet.m_columns.erase(currSheet.m_columns.begin()+nPos,
                                  currSheet.m_columns.begin()+nPos+nNum);
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function inserts sheets
    /// at the desired position.
    ///
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::insertSheets(size_t nPos, size_t nNum)
    {
        if (nPos > vTableData->sheets())
            return false;

        for (size_t s = nPos; s < nPos+nNum; s++)
        {
            vTableData->m_sheets.insert(vTableData->m_sheets.begin()+s,
                                        TblSheetPtr(new TableSheet("Sheet_" + toString(s+1))));
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function appends sheets.
    ///
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::appendSheets(size_t nNum)
    {
        vTableData->reserve(vTableData->sheets()+nNum);
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief This member function deletes sheets at
    /// the desired position.
    ///
    /// \param nPos size_t
    /// \param nNum size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool TableView::deleteSheets(size_t nPos, size_t nNum)
    {
        if (nPos >= vTableData->sheets())
            return false;

        vTableData->m_sheets.erase(vTableData->m_sheets.begin()+nPos,
                                   vTableData->m_sheets.begin()+nPos+nNum);
        return true;
    }

}

