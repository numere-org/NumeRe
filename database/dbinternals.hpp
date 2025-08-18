/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#ifndef DBINTERNALS_HPP
#define DBINTERNALS_HPP

#include <string>
#include <vector>
#include "../kernel/core/datamanagement/table.hpp"

enum DatabaseType
{
    DB_NONE,
    DB_SQLITE,
    DB_MYSQL,
    DB_POSTGRES,
    DB_ODBC
};

struct SqlStatement
{
    std::string stmt;
    std::vector<mu::Array> params;

    size_t affectedRows() const
    {
        size_t count = 1;

        for (const mu::Array& param : params)
        {
            count = std::max(count, param.size());
        }

        return count;
    }
};

int64_t openDbConnection(const std::string& fileName, DatabaseType type = DB_SQLITE);
int64_t openDbConnection(const std::string& host, const std::string& user, const std::string& password,
                         const std::string& dbname, size_t port = 0, DatabaseType type = DB_MYSQL, const std::string& driver = "", const std::string& sConnectionString = "");
bool closeDbConnection(int64_t dbId);
NumeRe::Table executeSql(int64_t dbId, const SqlStatement& statement);
std::vector<std::string> getOdbcDrivers();

#endif // DBINTERNALS_HPP


