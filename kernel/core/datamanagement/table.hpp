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

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This data container is a copy-
    /// efficient table to interchange data between
    /// Kernel and GUI.
    /////////////////////////////////////////////////
    class Table
    {
        private:
            vector<vector<double> > vTableData;
            vector<string> vTableHeadings;
            string sTableName;

            void setMinSize(size_t i, size_t j);
            bool isNumerical(const string& sValue);

        public:
            Table();
            Table(int nLines, int nCols);
            Table(double** const dData, string* const sHeadings, long long int nLines, long long int nCols, const string& sName = "");
            Table(const Table& _table);
            Table(Table&& _table);
            ~Table();

            Table& operator=(Table _table);

            void Clear();

            void setSize(size_t i, size_t j);

            void setName(const string& _sName);
            void setHead(size_t i, const string& _sHead);
            void setHeadPart(size_t i, size_t part, const string& _sHead);
            void setValue(size_t i, size_t j, double _dValue);
            void setValueAsString(size_t i, size_t j, const string& _sValue);

            string getName() const;
            int getHeadCount();
            string getHead(size_t i) const;
            string getCleanHead(size_t i) const;
            string getCleanHeadPart(size_t i, size_t part = 0);
            double getValue(size_t i, size_t j);
            string getValueAsString(size_t i, size_t j);

            size_t getLines() const;
            size_t getCols();

            bool isEmpty() const;

            bool insertLines(size_t nPos = 0, size_t nNum = 1);
            bool appendLines(size_t nNum = 1);
            bool deleteLines(size_t nPos = 0, size_t nNum = 1);
            bool insertCols(size_t nPos = 0, size_t nNum = 1);
            bool appendCols(size_t nNum = 1);
            bool deleteCols(size_t nPos = 0, size_t nNum = 1);

    };

}
#endif // TABLE_HPP

