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

#include <variant>
#include <map>
#include <sstream>
#include <boost/nowide/convert.hpp>

#include "dbinternals.hpp"
#include "../kernel/core/ui/error.hpp"
#include "../kernel/core/io/logger.hpp"

#include "../externals/qtl/include/qtl_sqlite.hpp"
#include "../externals/qtl/include/qtl_mysql.hpp"
//#include "../externals/qtl/include/qtl_postgres.hpp"
#include "../externals/qtl/include/qtl_odbc.hpp"

// Just the prototype to avoid conflicts with the PostGreSQL date type
sys_time_point getTimePointFromTime_t(__time64_t t);
sys_time_point getTimePointFromYMD(int year, int month, int day);
sys_time_point getTimePointFromHMS(int hours, int minutes, int seconds, int milliseconds, int microseconds);

std::string toUpperCase(const std::string& sLowerCase);
void replaceAll(MutableStringView sToModify, const char* sToRep, const char* sNewValue, size_t nStart = 0, size_t nEnd = std::string::npos);

/////////////////////////////////////////////////
/// \brief This struct manages a single database
/// connection.
/////////////////////////////////////////////////
struct DatabaseInstance
{
    //std::variant<qtl::sqlite::database, qtl::mysql::database, qtl::postgres::database> database;
    std::variant<qtl::sqlite::database, qtl::mysql::database, qtl::odbc::database> database;
    std::string host;
    DatabaseType type = DB_NONE;
};

static std::map<int64_t, DatabaseInstance> activeInstances;
static qtl::odbc::environment odbcEnvironment;

/////////////////////////////////////////////////
/// \brief This simple function is used to
/// detect, whether a SQL statement needs
/// prepared mode or can be run in simple mode.
///
/// \param sqlCommand std::string
/// \return bool
///
/// \note This seems only be required by MySQL
/// and MariaDB connections. SQLite, PostGreSQL
/// and ODBC do not seem to have this limitation.
/////////////////////////////////////////////////
static bool isSimpleQuery(std::string sqlCommand)
{
    sqlCommand = toUpperCase(sqlCommand);

    static std::string prepared[30] = {"ALTER", "ANALYZE", "CALL", "CHANGE", "CHECKSUM",
        "COMMIT", "CREATE", "DROP", "RENAME", "DELETE", "DO", "EXECUTE", "FLUSH", "GRANT",
        "INSERT", "INSTALL", "KILL", "OPTIMIZE", "PREPARE", "REPAIR", "REPLACE", "REPLICA",
        "RESET", "REVOKE", "SELECT", "SET", "SHOW", "TRUNCATE", "UNINSTALL", "UPDATE"};

    for (size_t i = 0; i < 30; i++)
    {
        if (sqlCommand.starts_with(prepared[i] + " "))
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Open a SQLite database connection and
/// return the connection ID.
///
/// \param fileName const std::string&
/// \param type DatabaseType
/// \return int64_t
///
/////////////////////////////////////////////////
int64_t openDbConnection(const std::string& fileName, DatabaseType type)
{
    if (type != DB_SQLITE)
        return -1;

    int64_t id = 0;

    if (activeInstances.size())
        id = activeInstances.rbegin()->first+1;

    qtl::sqlite::database db;
    g_logger.info("Connecting with SQLite/CONNECT=" + fileName);

    try
	{
		db.open(fileName.c_str());
		g_logger.info("Connected to " + fileName + " via SQLite");
	}
	catch (qtl::sqlite::error& e)
	{
		throw SyntaxError(SyntaxError::DATABASE_ERROR_SQLITE, "SQLite/CONNECT="+fileName, SyntaxError::invalid_position, e.what());
	}

	activeInstances[id].type = DB_SQLITE;
	activeInstances[id].host = fileName;
    activeInstances[id].database = std::move(db);
    return id;
}


/////////////////////////////////////////////////
/// \brief Apply the ODBC connection string
/// escaping rule to a value.
///
/// \param value std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string odbcEscape(std::string value)
{
    replaceAll(value, "}", "}}");
    return "{" + value + "}";
}


/////////////////////////////////////////////////
/// \brief Create a string representation of the
/// connection parameters for logging and error
/// messages.
///
/// \param host const std::string&
/// \param user const std::string&
/// \param password const std::string&
/// \param dbname const std::string&
/// \param port size_t
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createConnectionAlias(const std::string& host, const std::string& user, const std::string& password, const std::string& dbname, size_t port)
{
    std::string connection = "CONNECT="+host;

    if (port)
        connection += ":" + std::to_string(port);

    if (user.length())
        connection += "/USR="+user;

    if (password.length())
        connection += "/PWD=******";

    if (dbname.length())
        connection += "/DB=" + dbname;

    return connection;
}


/////////////////////////////////////////////////
/// \brief Open all of the non-SQLite database
/// connections, i.e. MySQL, MariaDB, ODBC, and
/// PostGreSQL (once enabled).
///
/// \param host const std::string&
/// \param user const std::string&
/// \param password const std::string&
/// \param dbname const std::string&
/// \param port size_t
/// \param type DatabaseType
/// \param driver const std::string&
/// \param sConnectionString const std::string&
/// \return int64_t
///
/////////////////////////////////////////////////
int64_t openDbConnection(const std::string& host, const std::string& user, const std::string& password, const std::string& dbname, size_t port, DatabaseType type, const std::string& driver, const std::string& sConnectionString)
{
    if (type == DB_SQLITE)
        return openDbConnection(host, type);

    int64_t id = 0;

    if (activeInstances.size())
        id = activeInstances.rbegin()->first+1;

    if (type == DB_MYSQL)
    {
        qtl::mysql::database db;
        std::string connection = "MySQL/"+createConnectionAlias(host, user, password, dbname, port);

        g_logger.info("Connecting with: " + connection);

        if (!db.open(host.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 0, port))
            throw SyntaxError(SyntaxError::DATABASE_ERROR_MYSQL, connection, SyntaxError::invalid_position, mysql_error(db.handle()));
        else
            g_logger.info("Connected to " + host + " via MySQL");

        activeInstances[id].type = type;
        activeInstances[id].host = host;
        activeInstances[id].database = std::move(db);
    }
    else if (type == DB_POSTGRES)
    {
        /*qtl::postgres::database db;

        if (!db.open(host.c_str(), user.c_str(), password.c_str(), !port ? 5432 : port, dbname.c_str()))
        {*/
            std::string connection = "PosGreSQL/"+createConnectionAlias(host, user, password, dbname, port);

            throw SyntaxError(SyntaxError::DATABASE_ERROR_POSTGRES, connection, SyntaxError::invalid_position, "NOT IMPLEMENTED");
        /*}

        activeInstances[id].type = type;
        activeInstances[id].database = std::move(db);*/
    }
    else if (type == DB_ODBC)
    {
        qtl::odbc::database db(odbcEnvironment);
        std::string connection = "ODBC{" + driver + "}/"+createConnectionAlias(host, user, password, dbname, port);

        if (sConnectionString.length())
            connection += "/CSTCONNSTR=" + sConnectionString;

        g_logger.info("Connecting with: " + connection);

        try
        {
            std::string conn = "DRIVER={" + driver + "}";

            if (sConnectionString.length())
                conn += ";" + sConnectionString;
            else
                conn += ";SERVER=" + host + ";TRUSTED_CONNECTION=no";

            if (user.length())
                conn += ";UID=" + odbcEscape(user);

            if (password.length())
                conn += ";PWD=" + odbcEscape(password);

            if (dbname.length())
                conn += ";DATABASE=" + dbname;

            if (port)
                conn += ";PORT=" + std::to_string(port);

            db.open(conn);
            g_logger.info("Connected to " + host + " via ODBC");
        }
        catch (qtl::odbc::error& e)
        {
            throw SyntaxError(SyntaxError::DATABASE_ERROR_ODBC, connection, SyntaxError::invalid_position, e.what());
        }

        activeInstances[id].type = type;
        activeInstances[id].host = host;
        activeInstances[id].database = std::move(db);
    }
    else
        return -1;

    return id;
}


/////////////////////////////////////////////////
/// \brief Close a connection based upon the
/// connection ID.
///
/// \param dbId int64_t
/// \return bool
///
/////////////////////////////////////////////////
bool closeDbConnection(int64_t dbId)
{
    auto iter = activeInstances.find(dbId);

    if (iter != activeInstances.end())
    {
        if (iter->second.type == DB_SQLITE)
            g_logger.info("Closing SQLite connection with " + iter->second.host);
        else if (iter->second.type == DB_MYSQL)
            g_logger.info("Closing MySQL connection with " + iter->second.host);
        else if (iter->second.type == DB_POSTGRES)
            g_logger.info("Closing PostGreSQL connection with " + iter->second.host);
        else if (iter->second.type == DB_ODBC)
            g_logger.info("Closing ODBC connection with " + iter->second.host);
        else
            g_logger.info("Closing UNSPECIFIED connection with " + iter->second.host);

        activeInstances.erase(iter);
        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Execute a SQL statement on a SQLite
/// database.
///
/// \param statement const SqlStatement&
/// \param db qtl::sqlite::database&
/// \param host const std::string&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
static NumeRe::Table executeSql(const SqlStatement& statement, qtl::sqlite::database& db, const std::string& host)
{
    NumeRe::Table result;

    try
    {
        qtl::sqlite::statement stmt = db.open_command(statement.stmt);

        bool paramIsMat = statement.params.size() == 1ull && statement.params[0].cols() > 1;
        size_t nParamCount = paramIsMat ? statement.params[0].cols() : statement.params.size();

        for (size_t i = 0; i < statement.affectedRows(); i++)
        {
            for (size_t j = 0; j < nParamCount; j++)
            {
                const mu::Value& val = paramIsMat ? statement.params[0].get(i, j) : statement.params[j].get(i);

                if (val.getType() == mu::TYPE_VOID || val.getType() == mu::TYPE_NEUTRAL)
                    stmt.bind_param(j, nullptr);
                else if (val.getType() == mu::TYPE_STRING || val.getType() == mu::TYPE_CATEGORY)
                    stmt.bind_param(j, val.getStr());
                else if (val.getType() == mu::TYPE_NUMERICAL)
                {
                    const mu::Numerical& numVal = val.getNum();
                    mu::TypeInfo info = numVal.getInfo();

                    if (info.m_flags & (mu::TypeInfo::TYPE_FLOAT
                                        | mu::TypeInfo::TYPE_COMPLEX
                                        | mu::TypeInfo::TYPE_DURATION
                                        | mu::TypeInfo::TYPE_DATETIME))
                        stmt.bind_param(j, numVal.asF64());
                    else
                        stmt.bind_param(j, numVal.asI64());
                }
                else
                    throw SyntaxError(SyntaxError::DATABASE_ERROR_SQLITE, "SQLite@" + host + "/SQLSTMT=" + statement.stmt,
                                      SyntaxError::invalid_position, "Parameter ?" + toString((long long int)j+1) + " has an unsupported type: " + val.getTypeAsString());
            }

            stmt.fetch();
            int columnCount = stmt.get_column_count();

            if (columnCount)
            {
                //std::vector<int> types;
                result.setSize(0, columnCount);

                for (int j = 0; j < columnCount; j++)
                {
                    //types.push_back(stmt.get_column_type(j));
                    result.setHead(j, stmt.get_column_name(j));
                }

                size_t i = 0;

                do
                {
                    for (int j = 0; j < columnCount; j++)
                    {
                        //switch (types[j])
                        switch (stmt.get_column_type(j))
                        {
                            case SQLITE_NULL:
                                break;
                            case SQLITE_INTEGER:
                                result.set(i, j, mu::Value(stmt.get_value_i64(j)));
                                break;
                            case SQLITE_FLOAT:
                                result.set(i, j, mu::Value(stmt.get_value_f64(j)));
                                break;
                            case SQLITE_TEXT:
                                result.set(i, j, mu::Value(stmt.get_text_value<char>(j)));
                                break;
                            case SQLITE_BLOB:
                                result.set(i, j, mu::Value("BLOB"));
                                break;
                        }
                    }

                    i++;
                } while (stmt.fetch());
            }

            stmt.reset();
        }
    }
    catch (qtl::sqlite::error& e)
    {
        throw SyntaxError(SyntaxError::DATABASE_ERROR_SQLITE, "SQLite@" + host + "/SQLSTMT=" + statement.stmt,
                          SyntaxError::invalid_position, e.what());
    }

    return result;
}


/////////////////////////////////////////////////
/// \brief Execute a SQL statement on a MySQL or
/// MariaDB database.
///
/// \param statement const SqlStatement&
/// \param db qtl::mysql::database&
/// \param host const std::string&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
static NumeRe::Table executeSql(const SqlStatement& statement, qtl::mysql::database& db, const std::string& host)
{
    NumeRe::Table result;

    try
    {
        if (isSimpleQuery(statement.stmt))
        {
            db.simple_execute(statement.stmt.c_str(), statement.stmt.length(), nullptr);
            return result;
        }

        bool paramIsMat = statement.params.size() == 1ull && statement.params[0].cols() > 1;
        size_t nParamCount = paramIsMat ? statement.params[0].cols() : statement.params.size();

        std::vector<std::any> paramStorage(nParamCount);

        qtl::mysql::statement stmt = db.open_command(statement.stmt);
        size_t numParams = stmt.get_parameter_count();

        for (size_t i = 0; i < statement.affectedRows(); i++)
        {
            // Binders are reused for reading, therefore they have to be
            // resized for every data set
            stmt.resize_binders(numParams);

            for (size_t j = 0; j < std::min(nParamCount, numParams); j++)
            {
                const mu::Value& val = paramIsMat ? statement.params[0].get(i, j) : statement.params[j].get(i);

                if (val.getType() == mu::TYPE_VOID || val.getType() == mu::TYPE_NEUTRAL)
                    stmt.bind_param(j, nullptr);
                else if (val.getType() == mu::TYPE_STRING || val.getType() == mu::TYPE_CATEGORY)
                {
                    paramStorage[j] = val.getStr();
                    stmt.bind_param(j, std::any_cast<const std::string&>(paramStorage[j]).c_str(),
                                    val.getStr().length());
                }
                else if (val.getType() == mu::TYPE_NUMERICAL)
                {
                    const mu::Numerical& numVal = val.getNum();

                    switch (numVal.getType())
                    {
                        case mu::LOGICAL:
                            paramStorage[j] = (bool)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const bool&>(paramStorage[j]));
                            break;
                        case mu::UI8:
                            paramStorage[j] = (uint8_t)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint8_t&>(paramStorage[j]));
                            break;
                        case mu::UI16:
                            paramStorage[j] = (uint16_t)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint16_t&>(paramStorage[j]));
                            break;
                        case mu::UI32:
                            paramStorage[j] = (uint32_t)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint32_t&>(paramStorage[j]));
                            break;
                        case mu::UI64:
                            paramStorage[j] = numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint64_t&>(paramStorage[j]));
                            break;
                        case mu::I8:
                            paramStorage[j] = (int8_t)numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int8_t&>(paramStorage[j]));
                            break;
                        case mu::I16:
                            paramStorage[j] = (int16_t)numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int16_t&>(paramStorage[j]));
                            break;
                        case mu::I32:
                            paramStorage[j] = (int32_t)numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int32_t&>(paramStorage[j]));
                            break;
                        case mu::I64:
                            paramStorage[j] = numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int64_t&>(paramStorage[j]));
                            break;
                        case mu::F32:
                        case mu::CF32:
                            paramStorage[j] = (float)numVal.asF64();
                            stmt.bind_param(j, std::any_cast<const float&>(paramStorage[j]));
                            break;
                        case mu::F64:
                        case mu::CF64:
                        case mu::DURATION:
                            paramStorage[j] = numVal.asF64();
                            stmt.bind_param(j, std::any_cast<const double&>(paramStorage[j]));
                            break;
                        case mu::DATETIME:
                        {
                            MYSQL_TIME t;
                            time_stamp ts = getTimeStampFromTimePoint(to_timePoint(numVal.asF64()));
                            t.year = (int)ts.m_ymd.year();
                            t.month = (unsigned)ts.m_ymd.month();
                            t.day = (unsigned)ts.m_ymd.day();
                            t.hour = ts.m_hours.count();
                            t.minute = ts.m_minutes.count();
                            t.second = ts.m_seconds.count();
                            t.second_part = ts.m_millisecs.count()*1000 + ts.m_microsecs.count();
                            paramStorage[j] = t;
                            qtl::mysql::binder* binder = stmt.get_binder(j);
                            binder->bind(std::any_cast<MYSQL_TIME&>(paramStorage[j]), MYSQL_TYPE_DATETIME);
                            break;
                        }

                    }
                }
                else
                    throw SyntaxError(SyntaxError::DATABASE_ERROR_MYSQL, "MySQL@" + host + "/SQLSTMT=" + statement.stmt,
                                      SyntaxError::invalid_position, "Parameter ?" + toString((long long int)j+1) + " has an unsupported type: " + val.getTypeAsString());


            }

            stmt.execute();
            int columnCount = stmt.get_column_count();

            if (columnCount)
            {
                stmt.auto_bind_fetch();
                std::vector<enum_field_types> types;
                result.setSize(0, columnCount);

                for (int j = 0; j < columnCount; j++)
                {
                    types.push_back(stmt.get_column_type(j));
                    result.setHead(j, stmt.get_column_name(j));
                }

                size_t i = 0;

                do
                {
                    for (int j = 0; j < columnCount; j++)
                    {
                        if (stmt.is_null(j))
                            continue;

                        switch (types[j])
                        {
                            case MYSQL_TYPE_NULL:
                                break;
                            case MYSQL_TYPE_BIT:
                                result.set(i, j, mu::Value(std::any_cast<bool>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_YEAR:
                            case MYSQL_TYPE_TINY:
                                result.set(i, j, mu::Value(std::any_cast<int8_t>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_SHORT:
                                result.set(i, j, mu::Value(std::any_cast<int16_t>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_INT24:
                            case MYSQL_TYPE_LONG:
                                result.set(i, j, mu::Value(std::any_cast<int32_t>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_LONGLONG:
                                result.set(i, j, mu::Value(std::any_cast<int64_t>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_FLOAT:
                                result.set(i, j, mu::Value(std::any_cast<float>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_DOUBLE:
                                result.set(i, j, mu::Value(std::any_cast<double>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_DATE:
                            case MYSQL_TYPE_TIME:
                            case MYSQL_TYPE_DATETIME:
                            case MYSQL_TYPE_TIMESTAMP:
                            case MYSQL_TYPE_TIMESTAMP2:
                            case MYSQL_TYPE_DATETIME2:
                            case MYSQL_TYPE_TIME2:
                            {
                                qtl::mysql::time t = std::any_cast<qtl::mysql::time>(stmt.get_value(j));
                                result.set(i, j, mu::Value(getTimePointFromTime_t(t.get_time()))+mu::Value(t.second_part*1e-6));
                                break;
                            }
                            case MYSQL_TYPE_VARCHAR:
                            case MYSQL_TYPE_VAR_STRING:
                            case MYSQL_TYPE_STRING:
                            case MYSQL_TYPE_ENUM:
    #if LIBMYSQL_VERSION_ID >= 50700
                            case MYSQL_TYPE_JSON:
    #endif
                            case MYSQL_TYPE_DECIMAL:
                            case MYSQL_TYPE_NEWDECIMAL:
                            case MYSQL_TYPE_GEOMETRY:
                                result.set(i, j, mu::Value(std::any_cast<std::string>(stmt.get_value(j))));
                                break;
                            case MYSQL_TYPE_TINY_BLOB:
                            case MYSQL_TYPE_MEDIUM_BLOB:
                            case MYSQL_TYPE_BLOB:
                            case MYSQL_TYPE_LONG_BLOB:
                            {
                                std::any& blob = stmt.get_value(j);

                                if (!stmt.is_null(j) && blob.has_value())
                                {
                                    if (stmt.blob_is_text(j))
                                    {
                                        qtl::mysql::blobbuf& buf = std::any_cast<qtl::mysql::blobbuf&>(blob);
                                        std::stringstream s;
                                        s << &buf << std::flush;
                                        result.set(i, j, mu::Value(s.str()));
                                    }
                                    else
                                        result.set(i, j, mu::Value("BINARY"));
                                }

                                break;
                            }
                        }
                    }

                    i++;
                } while (stmt.fetch());
            }
        }
    }
    catch (qtl::mysql::error& e)
    {
        throw SyntaxError(SyntaxError::DATABASE_ERROR_MYSQL, "MySQL@" + host + "/SQLSTMT=" + statement.stmt,
                          SyntaxError::invalid_position, e.what());
    }
    catch (std::bad_any_cast& e)
    {
        throw SyntaxError(SyntaxError::DATABASE_ERROR_MYSQL, "MySQL@" + host + "/SQLSTMT=" + statement.stmt,
                          SyntaxError::invalid_position, e.what());
    }

    return result;
}

/*
static NumeRe::Table executeSql(const SqlStatement& statement, qtl::postgres::database& db, const std::string& host)
{
    try
    {
        qtl::postgres::statement stmt = db.open_command(sqlCommand);
        stmt.execute();
        qtl::postgres::result& res = stmt.get_result();
        int columnCount = res.get_column_count();
        std::cout << "Execution returned " << columnCount << " column(s)." << std::endl;

        if (columnCount)
        {
            std::vector<Oid> types;

            for (int i = 0; i < columnCount; i++)
            {
                std::cout << res.get_column_name(i) << "\t";
                types.push_back(res.get_column_type(i));
            }

            std::cout << std::endl;
            std::cout << "--------------------------------------------------------------------------" << std::endl;

            for (int64_t i = 0; i < res.get_row_count(); i++)
            {
                for (int j = 0; j < columnCount; j++)
                {
                    const char* value = res.get_value(i, j);
                    Oid type = types[j];
                    switch (types[j])
                    {
                        case BOOLOID:
                            std::cout << *(bool*)res.get_value(i, j);
                            break;
                        case CHAROID:
                        case TEXTOID:
                        case NAMEOID:
                        case VARCHAROID:
                            std::cout << res.get_value(i, j);
                            break;
                        case FLOAT4OID:
                            std::cout << *(float*)res.get_value(i, j);
                            break;
                        case FLOAT8OID:
                            std::cout << *(double*)res.get_value(i, j);
                            break;
                        case INT2OID:
                            std::cout << qtl::postgres::detail::ntoh(*(int16_t*)res.get_value(i, j));
                            break;
                        case INT4OID:
                            std::cout << qtl::postgres::detail::ntoh(*(int32_t*)res.get_value(i, j));
                            break;
                        case INT8OID:
                            std::cout << qtl::postgres::detail::ntoh(*(int64_t*)res.get_value(i, j));
                            break;
                        case DATEOID: // Days since 1.1.2000
                            std::cout << qtl::postgres::detail::ntoh(*(int32_t*)res.get_value(i, j));
                            break;
                        case TIMEOID: // microseconds
                            std::cout << qtl::postgres::detail::ntoh(*(int64_t*)res.get_value(i, j));
                            break;
                        case TIMESTAMPOID: // somehow microseconds since 1.1.2000
                            std::cout << qtl::postgres::detail::ntoh(*(int64_t*)res.get_value(i, j));
                            break;
                        case TIMESTAMPTZOID:
                            std::cout << qtl::postgres::detail::ntoh(*(int64_t*)res.get_value(i, j));
                            break;
  //                      case qtl::postgres::object_traits<timestamp>::type_id:
  //                          value = field_cast<timestamp>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<interval>::type_id:
  //                          value = field_cast<interval>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<date>::type_id:
  //                          value = field_cast<date>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<std::vector<uint8_t>>::type_id:
  //                          value = field_cast<std::vector<uint8_t>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<bool>::array_type_id:
  //                          value = field_cast<std::vector<bool>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<char>::array_type_id:
  //                          std::cout << res.get_value(i, j);
  //                          break;
  //                      case qtl::postgres::object_traits<float>::array_type_id:
  //                          value = field_cast<std::vector<float>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<double>::array_type_id:
  //                          value = field_cast<std::vector<double>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<int16_t>::array_type_id:
  //                          value = field_cast<std::vector<int16_t>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<int32_t>::array_type_id:
  //                          value = field_cast<std::vector<int32_t>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<int64_t>::array_type_id:
  //                          value = field_cast<std::vector<int64_t>>(index);
  //                          break;
  //                      case qtl::postgres::qtl::postgres::object_traits<Oid>::array_type_id:
  //                          value = field_cast<std::vector<Oid>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<std::string>::array_type_id:
  //                          value = field_cast<std::vector<std::string>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<timestamp>::array_type_id:
  //                          value = field_cast<std::vector<timestamp>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<interval>::array_type_id:
  //                          value = field_cast<std::vector<interval>>(index);
  //                          break;
  //                      case qtl::postgres::object_traits<date>::array_type_id:
  //                          value = field_cast<std::vector<date>>(index);
  //                          break;
                        default:
                            std::cout << "NOT IMPLEMENTED";
                            break;
                    }
                    std::cout << "\t";

                }
                std::cout << std::endl;
            }

            std::cout << "--------------------------------------------------------------------------" << std::endl;
        }

        std::cout << "DONE.\n" << std::endl;
    }
    catch (qtl::postgres::error& e)
    {
        std::cout << e.what() << "\n" << std::endl;
    }
}

*/


/////////////////////////////////////////////////
/// \brief Execute a SQL statement on a ODBC
/// connection.
///
/// \param statement const SqlStatement&
/// \param db qtl::odbc::database&
/// \param host const std::string&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
static NumeRe::Table executeSql(const SqlStatement& statement, qtl::odbc::database& db, const std::string& host)
{
    NumeRe::Table result;

    try
    {
        bool paramIsMat = statement.params.size() == 1ull && statement.params[0].cols() > 1;
        size_t nParamCount = paramIsMat ? statement.params[0].cols() : statement.params.size();

        std::vector<std::any> paramStorage(nParamCount);

        qtl::odbc::statement stmt = db.open_command(statement.stmt);
        size_t numParams = stmt.get_parameter_count();

        stmt.resize_binders(numParams);

        for (size_t i = 0; i < statement.affectedRows(); i++)
        {
            for (size_t j = 0; j < std::min(nParamCount, numParams); j++)
            {
                const mu::Value& val = paramIsMat ? statement.params[0].get(i, j) : statement.params[j].get(i);

                if (val.getType() == mu::TYPE_VOID || val.getType() == mu::TYPE_NEUTRAL)
                    stmt.bind_param(j, nullptr);
                else if (val.getType() == mu::TYPE_STRING || val.getType() == mu::TYPE_CATEGORY)
                {
                    paramStorage[j] = val.getStr();
                    stmt.bind_param(j, std::any_cast<const std::string&>(paramStorage[j]).c_str());
                }
                else if (val.getType() == mu::TYPE_NUMERICAL)
                {
                    const mu::Numerical& numVal = val.getNum();

                    switch (numVal.getType())
                    {
                        case mu::LOGICAL:
                            paramStorage[j] = (bool)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const bool&>(paramStorage[j]));
                            break;
                        case mu::UI8:
                            paramStorage[j] = (uint8_t)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint8_t&>(paramStorage[j]));
                            break;
                        case mu::UI16:
                            paramStorage[j] = (uint16_t)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint16_t&>(paramStorage[j]));
                            break;
                        case mu::UI32:
                            paramStorage[j] = (uint32_t)numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint32_t&>(paramStorage[j]));
                            break;
                        case mu::UI64:
                            paramStorage[j] = numVal.asUI64();
                            stmt.bind_param(j, std::any_cast<const uint64_t&>(paramStorage[j]));
                            break;
                        case mu::I8:
                            paramStorage[j] = (int8_t)numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int8_t&>(paramStorage[j]));
                            break;
                        case mu::I16:
                            paramStorage[j] = (int16_t)numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int16_t&>(paramStorage[j]));
                            break;
                        case mu::I32:
                            paramStorage[j] = (int32_t)numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int32_t&>(paramStorage[j]));
                            break;
                        case mu::I64:
                            paramStorage[j] = numVal.asI64();
                            stmt.bind_param(j, std::any_cast<const int64_t&>(paramStorage[j]));
                            break;
                        case mu::F32:
                        case mu::CF32:
                            paramStorage[j] = (float)numVal.asF64();
                            stmt.bind_param(j, std::any_cast<const float&>(paramStorage[j]));
                            break;
                        case mu::F64:
                        case mu::CF64:
                        case mu::DURATION:
                            paramStorage[j] = numVal.asF64();
                            stmt.bind_param(j, std::any_cast<const double&>(paramStorage[j]));
                            break;
                        case mu::DATETIME:
                        {
                            TIMESTAMP_STRUCT t;
                            time_stamp ts = getTimeStampFromTimePoint(to_timePoint(numVal.asF64()));
                            t.year = (int)ts.m_ymd.year();
                            t.month = (unsigned)ts.m_ymd.month();
                            t.day = (unsigned)ts.m_ymd.day();
                            t.hour = ts.m_hours.count();
                            t.minute = ts.m_minutes.count();
                            t.second = ts.m_seconds.count();
                            t.fraction = ts.m_millisecs.count()*1000 + ts.m_microsecs.count();
                            paramStorage[j] = t;
                            stmt.bind_param(j, std::any_cast<const TIMESTAMP_STRUCT&>(paramStorage[j]));
                            break;
                        }

                    }
                }
                else
                    throw SyntaxError(SyntaxError::DATABASE_ERROR_ODBC, "ODBC@" + host + "/SQLSTMT=" + statement.stmt,
                                      SyntaxError::invalid_position, "Parameter ?" + toString((long long int)j+1) + " has an unsupported type: " + val.getTypeAsString());


            }

            stmt.execute();
            int columnCount = stmt.get_column_count();

            if (columnCount)
            {
                std::vector<SQLLEN> types;
                result.setSize(0, columnCount);

                for (int j = 0; j < columnCount; j++)
                {
                    types.push_back(stmt.get_column_type(j));
                    result.setHead(j, stmt.get_column_name(j));
                }

                stmt.fetch();
                size_t i = 0;

                do
                {
                    for (int j = 0; j < columnCount; j++)
                    {
                        /*if (stmt.is_null(j))
                            continue;*/

                        switch (types[j])
                        {
                            case SQL_BIT:
                                result.set(i, j, mu::Value(stmt.get_value<bool, SQL_C_BIT>(j)));
                                break;
                            case SQL_TINYINT:
                                result.set(i, j, mu::Value(stmt.get_value<int8_t, SQL_TINYINT>(j)));
                                break;
                            case SQL_SMALLINT:
                                result.set(i, j, mu::Value(stmt.get_value<int16_t, SQL_C_SSHORT>(j)));
                                break;
                            case SQL_INTEGER:
                                result.set(i, j, mu::Value(stmt.get_value<int32_t, SQL_C_SLONG>(j)));
                                break;
                            case SQL_BIGINT:
                                result.set(i, j, mu::Value(stmt.get_value<int64_t, SQL_C_SBIGINT>(j)));
                                break;
                            case SQL_FLOAT:
                                result.set(i, j, mu::Value(stmt.get_value<float, SQL_C_FLOAT>(j)));
                                break;
                            case SQL_DOUBLE:
                                result.set(i, j, mu::Value(stmt.get_value<double,SQL_C_DOUBLE>(j)));
                                break;
                            /*case SQL_NUMERIC:
                                std::cout << stmt.get_value<SQL_NUMERIC_STRUCT, SQL_NUMERIC>(j);
                                break;*/
                            case SQL_TIME:
                            {
                                SQL_TIME_STRUCT tm = stmt.get_value<SQL_TIME_STRUCT,SQL_TIME>(j);
                                result.set(i, j, mu::Value(getTimePointFromHMS(tm.hour, tm.minute, tm.second)));
                                break;
                            }
                            case SQL_DATE:
                            {
                                SQL_DATE_STRUCT dt = stmt.get_value<SQL_DATE_STRUCT,SQL_DATE>(j);
                                result.set(i, j, mu::Value(getTimePointFromYMD(dt.year, dt.month, dt.day)));
                                break;
                            }
                            case SQL_TIMESTAMP:
                            {
                                qtl::odbc::timestamp dt = stmt.get_value<SQL_TIMESTAMP_STRUCT,SQL_TIMESTAMP>(j);
                                result.set(i, j, mu::Value(dt.get_time())+mu::Value(dt.fraction*1e-9));
                                break;
                            }
                            /*case SQL_TIMESTAMP:
                                std::cout << stmt.get_value<SQL_TIMESTAMP_STRUCT,SQL_TIMESTAMP>(j);
                                break;*/
                            /*case SQL_INTERVAL_MONTH:
                            case SQL_INTERVAL_YEAR:
                            case SQL_INTERVAL_YEAR_TO_MONTH:
                            case SQL_INTERVAL_DAY:
                            case SQL_INTERVAL_HOUR:
                            case SQL_INTERVAL_MINUTE:
                            case SQL_INTERVAL_SECOND:
                            case SQL_INTERVAL_DAY_TO_HOUR:
                            case SQL_INTERVAL_DAY_TO_MINUTE:
                            case SQL_INTERVAL_DAY_TO_SECOND:
                            case SQL_INTERVAL_HOUR_TO_MINUTE:
                            case SQL_INTERVAL_HOUR_TO_SECOND:
                            case SQL_INTERVAL_MINUTE_TO_SECOND:
                                value.emplace<SQL_INTERVAL_STRUCT>();
                                bind_field(index, std::forward<SQL_INTERVAL_STRUCT>(std::any_cast<SQL_INTERVAL_STRUCT&>(value)));
                                break;*/
                            case SQL_CHAR:
                            case SQL_VARCHAR:
                            case SQL_LONGVARCHAR:
                                result.set(i, j, mu::Value(stmt.get_str_value(j)));
                                break;
                            case SQL_WCHAR:
                            case SQL_WVARCHAR:
                            case SQL_WLONGVARCHAR:
                            {
                                result.set(i, j, mu::Value(boost::nowide::narrow(stmt.get_wstr_value(j))));
                                break;
                            }
                            /*case SQL_GUID:
                                value.emplace<SQLGUID>();
                                bind_field(index, std::forward<SQLGUID>(std::any_cast<SQLGUID&>(value)));
                                break;
                            case SQL_BINARY:
                                value.emplace<blobbuf>();
                                bind_field(index, std::forward<blobbuf>(std::any_cast<blobbuf&>(value)));
                                break;*/
                            default:
                                break;
                        }
                    }

                    i++;
                } while (stmt.fetch());
            }
        }
    }
    catch (qtl::odbc::error& e)
    {
        throw SyntaxError(SyntaxError::DATABASE_ERROR_ODBC, "ODBC@" + host + "/SQLSTMT=" + statement.stmt,
                          SyntaxError::invalid_position, e.what());
    }

    return result;
}


/////////////////////////////////////////////////
/// \brief Execute a SQL statement on a database
/// connection passed as an ID.
///
/// \param dbId int64_t
/// \param statement const SqlStatement&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table executeSql(int64_t dbId, const SqlStatement& statement)
{
    auto iter = activeInstances.find(dbId);

    if (iter != activeInstances.end())
    {
        if (iter->second.type == DB_SQLITE)
            return executeSql(statement, std::get<qtl::sqlite::database>(iter->second.database), iter->second.host);
        else if (iter->second.type == DB_MYSQL)
            return executeSql(statement, std::get<qtl::mysql::database>(iter->second.database), iter->second.host);
        //else if (iter->second.type == DB_POSTGRES)
        //    return executeSql(statement, std::get<qtl::postgres::database>(iter->second.database), iter->second.host);
        else if (iter->second.type == DB_ODBC)
            return executeSql(statement, std::get<qtl::odbc::database>(iter->second.database), iter->second.host);

        return NumeRe::Table();
    }

    return NumeRe::Table();
}


/////////////////////////////////////////////////
/// \brief Return a list of installed ODBC
/// drivers.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> getOdbcDrivers()
{
    std::vector<qtl::odbc::driver> drivers = odbcEnvironment.drivers();

    std::vector<std::string> ret;

    for (const auto& driver : drivers)
    {
        ret.push_back(driver.description);
    }

    return ret;
}


