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

#ifndef TABLE_HPP
#define TABLE_HPP

#include <vector>
#include <string>

using namespace std;

class Table
{
    private:
        vector<vector<double> > vTableData;
        vector<string> vTableHeadings;
        string sTableName;

        void setMinSize(size_t i, size_t j);

    public:
        Table();
        Table(const Table& _table);
        ~Table();

        void setSize(size_t i, size_t j);

        void setName(const string& _sName);
        void setHead(size_t i, const string& _sHead);
        void setValue(size_t i, size_t j, double _dValue);

        string getName();
        string getHead(size_t i);
        double getValue(size_t i, size_t j);

        size_t getLines();
        size_t getCols();
};

#endif // TABLE_HPP

