/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector>
#include <map>

#include "../io/filesystem.hpp"

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class is an implementation of a
    /// database. It will handle the *.ndb data
    /// format an provides an interface to its
    /// contained data.
    /////////////////////////////////////////////////
    class DataBase : public FileSystem
    {
        private:
            std::vector<std::vector<std::string>> m_dataBase;
            std::string m_dataBaseFile;

            std::vector<std::string> getDBFileContent();
            void readDataBase();
            size_t findOrCreateRecord(const std::string& sRecord);

        public:
            DataBase();
            DataBase(const std::string& sDataBaseFile);
            DataBase(const DataBase& data);
            DataBase(const std::vector<std::string>& vDataColumn);

            void addData(const std::string& sDataBaseFile);

            size_t size() const
            {
                return m_dataBase.size();
            }

            size_t getCols() const
            {
                if (!m_dataBase.size())
                    return 0;

                return m_dataBase[0].size();
            }

            std::string getElement(size_t i, size_t j) const;
            std::vector<std::string> getColumn(size_t j) const;
            std::vector<std::string>& operator[](size_t i);
            DataBase& operator=(const DataBase& data);
            size_t randomRecord() const;

            size_t findRecord(const std::string& _sRecord) const;
            std::map<size_t,std::vector<size_t>> find(const std::string& _sSearchString, bool findOnlyFirst = false) const;
            std::map<double,std::vector<size_t>> findRecordsUsingRelevance(const std::string& _sSearchString, std::vector<double> vWeighting = std::vector<double>()) const;
    };
}


#endif // DATABASE_HPP

