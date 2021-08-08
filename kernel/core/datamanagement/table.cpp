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
#include <cmath>
#include <cstdio>

double StrToDb(const string& sString);

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
        setMinSize(nLines, nCols);
    }


    /////////////////////////////////////////////////
    /// \brief Fill constructor
    ///
    /// \param dData double** const
    /// \param sHeadings string* const
    /// \param nLines long longint
    /// \param nCols long longint
    /// \param sName const string&
    ///
    /////////////////////////////////////////////////
    Table::Table(double** const dData, string* const sHeadings, long long int nLines, long long int nCols, const string& sName)
    {
        // Empty table with predefined headlines
        if (nCols && !nLines)
            nLines = 1;

        // Prepare the table
        setMinSize(nLines, nCols);

        // Copy the data
        for (long long int i = 0; i < nLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                vTableData[i][j] = dData[i][j];
            }
        }

        // Copy the headlines
        for (long long int j = 0; j < nCols; j++)
            vTableHeadings[j] = sHeadings[j];

        sTableName = sName;
    }


    /////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    /// \param _table const Table&
    ///
    /////////////////////////////////////////////////
    Table::Table(const Table& _table) : vTableData(_table.vTableData), vTableHeadings(_table.vTableHeadings), sTableName(_table.sTableName)
    {
    }


    /////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    /// \param _table Table&&
    ///
    /////////////////////////////////////////////////
    Table::Table(Table&& _table) : sTableName(_table.sTableName)
    {
        // We move by using the std::swap() functions
        std::swap(vTableData, _table.vTableData);
        std::swap(vTableHeadings, _table.vTableHeadings);
    }


    /////////////////////////////////////////////////
    /// \brief Destructor
    /////////////////////////////////////////////////
    Table::~Table()
    {
        vTableData.clear();
        vTableHeadings.clear();
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
        std::swap(vTableHeadings, _table.vTableHeadings);
        sTableName = _table.sTableName;

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
        if (!vTableData.size())
        {
            // Simple case: table is empty
            vTableData = vector<vector<double> >(i, vector<double>(j, NAN));
        }
        else if (vTableData.size() < i || vTableData[0].size() < j)
        {
            // complex case: table is not empty
            vector<vector<double> > vTempData = vector<vector<double> >(i, vector<double>(j, NAN));

            for (size_t ii = 0; ii < vTableData.size(); ii++)
            {
                for (size_t jj = 0; jj < vTableData[ii].size(); jj++)
                {
                    vTempData[ii][jj] = vTableData[ii][jj];
                }
            }

            vTableData = vTempData;
        }

        // Prepare the column headlines
        while (vTableHeadings.size() <= j)
            vTableHeadings.push_back("Spalte_" + toString(vTableHeadings.size()+1));
    }


    /////////////////////////////////////////////////
    /// \brief This member function is a simple
    /// helper function to determine, whether a
    /// passed value may be parsed into a numerical
    /// value.
    ///
    /// \param sValue const string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Table::isNumerical(const string& sValue)
    {
        static string sValidNumericalCharacters = "0123456789,.eE+- INFAinfa";
        return sValue.find_first_not_of(sValidNumericalCharacters) == string::npos;
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
        vTableHeadings.clear();
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
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the table name.
    ///
    /// \param _sName const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setName(const string& _sName)
    {
        sTableName = _sName;
    }


    /////////////////////////////////////////////////
    /// \brief Setter function for the selected
    /// column headline. Will create missing
    /// headlines automatically.
    ///
    /// \param i size_t
    /// \param _sHead const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setHead(size_t i, const string& _sHead)
    {
        while (vTableHeadings.size() <= i)
            vTableHeadings.push_back("");

        vTableHeadings[i] = _sHead;
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
    /// \param _sHead const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setHeadPart(size_t i, size_t part, const string& _sHead)
    {
        while (vTableHeadings.size() <= i)
            vTableHeadings.push_back("");

        string head;

        // Compose the new headline using the different
        // parts
        for (int j = 0; j < getHeadCount(); j++)
        {
            if (j == (int)part)
                head += _sHead + "\\n";
            else
                head += getCleanHeadPart(i, j) + "\\n";
        }

        head.erase(head.find_last_not_of("\\n")+1);

        vTableHeadings[i] = head;
    }


    /////////////////////////////////////////////////
    /// \brief This member function sets the data to
    /// the table. It will resize the table
    /// automatically, if needed.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \param _dValue double
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Table::setValue(size_t i, size_t j, double _dValue)
    {
        this->setMinSize(i+1, j+1);
        vTableData[i][j] = _dValue;
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
    void Table::setValueAsString(size_t i, size_t j, const string& _sValue)
    {
        // Is it a numerical value?
        if (!isNumerical(_sValue))
        {
            if (vTableHeadings[j].length())
                vTableHeadings[j] += "\\n";

            vTableHeadings[j] += _sValue;

            return;
        }

        // Resize the table if needed
        this->setMinSize(i+1, j+1);
        double _dValue = 0.0;
        string sValue = _sValue;

        // Parse the value as a numerical value
        if (!_sValue.length())
            _dValue = NAN;
        else if (_sValue == "NAN" || _sValue == "NaN" || _sValue == "nan" || _sValue == "---")
            _dValue = NAN;
        else if (_sValue == "inf")
            _dValue = INFINITY;
        else if (_sValue == "-inf")
            _dValue = -INFINITY;
        else
        {
            while (sValue.find(',') != string::npos)
                sValue[sValue.find(',')] = '.';

            _dValue = StrToDb(sValue);
        }

        vTableData[i][j] = _dValue;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the table
    /// headline.
    ///
    /// \return string
    ///
    /////////////////////////////////////////////////
    string Table::getName() const
    {
        return sTableName;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the needed number
    /// of headlines (depending on the number of
    /// linebreaks found in the headlines).
    ///
    /// \return int
    ///
    /////////////////////////////////////////////////
    int Table::getHeadCount()
    {
        int nHeadlineCount = 1;
        // Get the dimensions of the complete headline (i.e. including possible linebreaks)
        for (long long int j = 0; j < vTableHeadings.size(); j++)
        {
            // No linebreak? Continue
            if (vTableHeadings[j].find("\\n") == string::npos)
                continue;

            int nLinebreak = 0;

            // Count all linebreaks
            for (unsigned int n = 0; n < vTableHeadings[j].length() - 2; n++)
            {
                if (vTableHeadings[j].substr(n, 2) == "\\n")
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
    /// \return string
    ///
    /////////////////////////////////////////////////
    string Table::getHead(size_t i) const
    {
        if (vTableHeadings.size() > i)
            return vTableHeadings[i];

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the selected
    /// column's headline. Underscores and masked
    /// headlines are replaced on-the-fly.
    ///
    /// \param i size_t
    /// \return string
    ///
    /////////////////////////////////////////////////
    string Table::getCleanHead(size_t i) const
    {
        if (vTableHeadings.size() > i)
        {
            string head = vTableHeadings[i];
            replaceAll(head, "\\n", "\n");
            return head;
        }

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the selected part
    /// of the selected column's headline.
    /// Underscores and masked headlines are replaced
    /// on-the-fly.
    ///
    /// \param i size_t
    /// \param part size_t
    /// \return string
    ///
    /////////////////////////////////////////////////
    string Table::getCleanHeadPart(size_t i, size_t part)
    {
        if (vTableHeadings.size() > i || part >= (size_t)getHeadCount())
        {
            // get the cleaned headline
            string head = getCleanHead(i);

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

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the value of the
    /// selected cell.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return double
    ///
    /////////////////////////////////////////////////
    double Table::getValue(size_t i, size_t j)
    {
        if (vTableData.size() > i && vTableData[i].size() > j)
            return vTableData[i][j];

        return NAN;
    }


    /////////////////////////////////////////////////
    /// \brief Getter function for the value of the
    /// selected cell. The value is converted into a
    /// string.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return string
    ///
    /////////////////////////////////////////////////
    string Table::getValueAsString(size_t i, size_t j)
    {
        char cBuffer[50];

        if (vTableData.size() > i && vTableData[i].size() > j)
        {
            // Invalid number
            if (isnan(vTableData[i][j]))
                return "---";

            // +/- infinity
            if (isinf(vTableData[i][j]) && vTableData[i][j] > 0)
                return "inf";
            else if (isinf(vTableData[i][j]))
                return "-inf";

            // Explicit conversion
            sprintf(cBuffer, "%.*g", 7, vTableData[i][j]);
            return string(cBuffer);
        }

        return "---";
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of lines.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Table::getLines() const
    {
        return vTableData.size();
    }


    /////////////////////////////////////////////////
    /// \brief Get the number of columns.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t Table::getCols()
    {
        return vTableData[0].size();
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

        for (size_t i = 0; i < nNum; i++)
            vTableData.insert(vTableData.begin() + nPos, vector<double>(vTableData[0].size(), NAN));

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
        for (size_t i = 0; i < nNum; i++)
            vTableData.push_back(vector<double>(vTableData[0].size(), NAN));

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

        vTableData.erase(vTableData.begin() + nPos, vTableData.begin() + nPos + nNum);
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
        for (size_t i = 0; i < vTableData.size(); i++)
            vTableData[i].insert(vTableData[i].begin() + nPos, nNum, NAN);

        vTableHeadings.insert(vTableHeadings.begin() + nPos, nNum, "");
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
        for (size_t i = 0; i < vTableData.size(); i++)
            vTableData[i].insert(vTableData[i].end(), nNum, NAN);

        vTableHeadings.insert(vTableHeadings.end(), nNum, "");
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
        for (size_t i = 0; i < vTableData.size(); i++)
            vTableData[i].erase(vTableData[i].begin() + nPos, vTableData[i].begin() + nPos + nNum);

        vTableHeadings.erase(vTableHeadings.begin() + nPos, vTableHeadings.begin() + nPos + nNum);
        return true;
    }

}

