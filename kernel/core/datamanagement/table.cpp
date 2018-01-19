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
#include <cmath>


Table::Table()
{

}

Table::Table(const Table& _table)
{
    vTableData = _table.vTableData;
    vTableHeadings = _table.vTableHeadings;
    sTableName = _table.sTableName;
}

Table::~Table()
{
    vTableData.clear();
    vTableHeadings.clear();
}

void Table::setMinSize(size_t i, size_t j)
{
    if (!vTableData.size())
    {
        vTableData = vector<vector<double> >(i, vector<double>(j, NAN));
    }
    else if (vTableData.size() < i || vTableData[0].size() < j)
    {
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
    while (vTableHeadings.size() <= j)
        vTableHeadings.push_back("");
}

void Table::setSize(size_t i, size_t j)
{
    this->setMinSize(i, j);
}

void Table::setName(const string& _sName)
{
    sTableName = _sName;
}

void Table::setHead(size_t i, const string& _sHead)
{
    while (vTableHeadings.size() <= i)
        vTableHeadings.push_back("");
    vTableHeadings[i] = _sHead;
}

void Table::setValue(size_t i, size_t j, double _dValue)
{
    this->setMinSize(i+1, j+1);
    vTableData[i][j] = _dValue;
}

string Table::getName()
{
    return sTableName;
}

string Table::getHead(size_t i)
{
    if (vTableHeadings.size() > i)
        return vTableHeadings[i];
    return "";
}

double Table::getValue(size_t i, size_t j)
{
    if (vTableData.size() > i && vTableData[i].size() > j)
        return vTableData[i][j];
    return NAN;
}

size_t Table::getLines()
{
    return vTableData.size();
}

size_t Table::getCols()
{
    return vTableData[0].size();
}

