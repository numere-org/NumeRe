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
    // Default constructor
    Table::Table()
    {
        sTableName.clear();
    }

    // Size constructor
    Table::Table(int nLines, int nCols) : Table()
    {
        setMinSize(nLines, nCols);
    }

    // Fill constructor
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

    // Copy constructor
    Table::Table(const Table& _table)
    {
        vTableData = _table.vTableData;
        vTableHeadings = _table.vTableHeadings;
        sTableName = _table.sTableName;
    }

    // Move constructor
    Table::Table(Table&& _table)
    {
        // We move by using the std::swap() functions
        std::swap(vTableData, _table.vTableData);
        std::swap(vTableHeadings, _table.vTableHeadings);
        sTableName = _table.sTableName;
    }

    // Destructor
    Table::~Table()
    {
        vTableData.clear();
        vTableHeadings.clear();
    }

    // Move assignment operator
    Table& Table::operator=(Table _table)
    {
        // We move by using the std::swap() functions
        std::swap(vTableData, _table.vTableData);
        std::swap(vTableHeadings, _table.vTableHeadings);
        sTableName = _table.sTableName;

        return *this;
    }

    // This member function prepares a table with
    // the minimal size of the selected lines and columns
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

    // This member function is a simple helper function
    // to determine, whether a passed value may be parsed
    // into a numerical value
    bool Table::isNumerical(const string& sValue)
    {
        static string sValidNumericalCharacters = "0123456789,.eE+- INFAinfa";
        return sValue.find_first_not_of(sValidNumericalCharacters) == string::npos;
    }

    // This member function cleares the contents of this table
    void Table::Clear()
    {
        vTableData.clear();
        vTableHeadings.clear();
        sTableName.clear();
    }

    // This member function simply redirects the control to
    // setMinSize()
    void Table::setSize(size_t i, size_t j)
    {
        this->setMinSize(i, j);
    }

    // Setter function for the table name
    void Table::setName(const string& _sName)
    {
        sTableName = _sName;
    }

    // Setter function for the selected column headline.
    // Will create missing headlines automatically
    void Table::setHead(size_t i, const string& _sHead)
    {
        while (vTableHeadings.size() <= i)
            vTableHeadings.push_back("");

        vTableHeadings[i] = _sHead;
    }

    // Setter function for the selected column headline
    // and the selected part of the headline (split using
    // linebreak characters). Will create the missing
    // headlines automatically.
    void Table::setHeadPart(size_t i, size_t part, const string& _sHead)
    {
        while (vTableHeadings.size() <= i)
            vTableHeadings.push_back("");

        string head;

        // Compose the new headline using the different
        // parts
        for (int j = 0; j < getHeadCount(); j++)
        {
            if (j == part)
                head += _sHead + "\\n";
            else
                head += getCleanHeadPart(i, j) + "\\n";
        }

        head.erase(head.find_last_not_of("\\n")+1);

        vTableHeadings[i] = head;
    }

    // This member function sets the data to the table.
    // It will resize the table automatically, if needed.
    void Table::setValue(size_t i, size_t j, double _dValue)
    {
        this->setMinSize(i+1, j+1);
        vTableData[i][j] = _dValue;
    }

    // This member function sets the data, which is passed
    // as a string, to the table. If the data is not a numerical
    // value, the data is assigned to the corresponding
    // column head.
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

    // Getter function for the table headline
    string Table::getName()
    {
        return sTableName;
    }

    // Getter function for the needed number of headlines
    // (depending on the number of linebreaks found in the
    // headlines)
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

    // Getter function for the selected column's headline
    string Table::getHead(size_t i)
    {
        if (vTableHeadings.size() > i)
            return vTableHeadings[i];

        return "";
    }

    // Getter function for the selected column's headline.
    // Underscores and masked headlines are replaced on-the-fly
    string Table::getCleanHead(size_t i)
    {
        if (vTableHeadings.size() > i)
        {
            string head = vTableHeadings[i];
            replaceAll(head, "_", " ");
            replaceAll(head, "\\n", "\n");
            return head;
        }

        return "";
    }

    // Getter function for the selected part of the selected
    // column's headline.
    // Underscores and masked headlines are replaced on-the-fly
    string Table::getCleanHeadPart(size_t i, size_t part)
    {
        if (vTableHeadings.size() > i || part >= getHeadCount())
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
            for (size_t i = 0; i < head.length(); i++)
            {
                if (head[i] == '\n')
                {
                    if (!part)
                        return head.substr(pos, i - pos);

                    part--;
                    pos = i+1;
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

    // Getter function for the value of the selected cell
    double Table::getValue(size_t i, size_t j)
    {
        if (vTableData.size() > i && vTableData[i].size() > j)
            return vTableData[i][j];

        return NAN;
    }

    // Getter function for the value of the selected cell.
    // The value is converted into a string.
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

    // Get the number of lines
    size_t Table::getLines()
    {
        return vTableData.size();
    }

    // Get the number of columns
    size_t Table::getCols()
    {
        return vTableData[0].size();
    }

    // Return, whether the table is empty
    bool Table::isEmpty()
    {
        return vTableData.empty();
    }

    // This member function inserts lines at the desired position
    bool Table::insertLines(size_t nPos, size_t nNum)
    {
        nPos -= getHeadCount();

        for (size_t i = 0; i < nNum; i++)
            vTableData.insert(vTableData.begin() + nPos, vector<double>(vTableData[0].size(), NAN));

        return true;
    }

    // This member function appends lines
    bool Table::appendLines(size_t nNum)
    {
        for (size_t i = 0; i < nNum; i++)
            vTableData.push_back(vector<double>(vTableData[0].size(), NAN));

        return true;
    }

    // This member function deletes lines at the desired position
    bool Table::deleteLines(size_t nPos, size_t nNum)
    {
        nPos -= getHeadCount();

        vTableData.erase(vTableData.begin() + nPos, vTableData.begin() + nPos + nNum);
        return true;
    }

    // This member function inserts columns at the desired position
    bool Table::insertCols(size_t nPos, size_t nNum)
    {
        for (size_t i = 0; i < vTableData.size(); i++)
            vTableData[i].insert(vTableData[i].begin() + nPos, nNum, NAN);

        vTableHeadings.insert(vTableHeadings.begin() + nPos, nNum, "");
        return true;
    }

    // This member function appends columns
    bool Table::appendCols(size_t nNum)
    {
        for (size_t i = 0; i < vTableData.size(); i++)
            vTableData[i].insert(vTableData[i].end(), nNum, NAN);

        vTableHeadings.insert(vTableHeadings.end(), nNum, "");
        return true;
    }

    // This member function delets columns at the desired position
    bool Table::deleteCols(size_t nPos, size_t nNum)
    {
        for (size_t i = 0; i < vTableData.size(); i++)
            vTableData[i].erase(vTableData[i].begin() + nPos, vTableData[i].begin() + nPos + nNum);

        vTableHeadings.erase(vTableHeadings.begin() + nPos, vTableHeadings.begin() + nPos + nNum);
        return true;
    }

}

