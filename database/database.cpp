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

#include "database.hpp"
#include "dbinternals.hpp"
#include "../kernel/kernel.hpp"


/////////////////////////////////////////////////
/// \brief This is an interface function for
/// accessing one of the supported types of
/// database, either for connecting or for
/// running queries.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void databaseCommand(CommandLineParser& cmdParser)
{
    std::vector<mu::Array> expression = cmdParser.parseExpr();
    std::string sType = cmdParser.getParameterValue("type");

    if (expression.front().getCommonType() != mu::TYPE_NUMERICAL && sType.length())
    {
        // Open a database connection
        DatabaseType type = DB_NONE;

        if (sType == "sqlite")
            type = DB_SQLITE;
        else if (sType == "mysql" || sType == "mariadb")
            type = DB_MYSQL;
        else if (sType == "postgres")
            type = DB_POSTGRES;
        else if (sType == "odbc")
            type = DB_ODBC;

        if (type == DB_SQLITE)
        {
            std::string sFileName = expression.front().front().getStr();

            if (sFileName.find('.') != std::string::npos)
                sFileName = sFileName.substr(sFileName.rfind('.')+1);
            else
                sFileName = "sqlite";

            cmdParser.setReturnValue(mu::Value(openDbConnection(cmdParser.getExprAsFileName(sFileName), type)));
        }
        else if (type == DB_MYSQL || type == DB_POSTGRES)
        {
            std::string sUser = cmdParser.getParsedParameterValueAsString("usr", "", true);
            std::string sPassword = cmdParser.getParsedParameterValueAsString("pwd", "", true);
            std::string sDatabase = cmdParser.getParsedParameterValueAsString("usedb", "", true);
            mu::Array port = cmdParser.getParsedParameterValue("port");

            cmdParser.setReturnValue(mu::Value(openDbConnection(expression.front().front().printVal(),
                                                                sUser,
                                                                sPassword,
                                                                sDatabase,
                                                                port.size() ? port.getAsScalarInt() : 0,
                                                                type)));
        }
        else if (type == DB_ODBC)
        {
            std::string sUser = cmdParser.getParsedParameterValueAsString("usr", "", true);
            std::string sPassword = cmdParser.getParsedParameterValueAsString("pwd", "", true);
            std::string sDatabase = cmdParser.getParsedParameterValueAsString("usedb", "", true);
            std::string sDriver = cmdParser.getParsedParameterValueAsString("driver", "", true);
            mu::Array port = cmdParser.getParsedParameterValue("port");

            cmdParser.setReturnValue(mu::Value(openDbConnection(expression.front().front().printVal(),
                                                                sUser,
                                                                sPassword,
                                                                sDatabase,
                                                                port.size() ? port.getAsScalarInt() : 0,
                                                                type,
                                                                sDriver)));
        }
        else
            cmdParser.setReturnValue(mu::Value(false));
    }
    else if (expression.front().getCommonType() == mu::TYPE_NUMERICAL && !sType.length() && cmdParser.hasParam("sql"))
    {
        mu::Array sqlCommands = cmdParser.getParsedParameterValue("sql");

        for (size_t i = 0; i < sqlCommands.size(); i++)
        {
            NumeRe::Table result = executeSql(expression.front().getAsScalarInt(),
                                              sqlCommands.get(i).getStr());

            // TODO How to handle multiple results for an array of SQL statements
            if (i+1 < sqlCommands.size())
                continue;

            if (cmdParser.hasParam("target") && !result.isEmpty())
            {
                Indices _idx;
                std::string sTable = cmdParser.getTargetTable(_idx, "");
                NumeReKernel::getInstance()->getMemoryManager().insertCopiedTable(std::move(result), sTable, _idx.row, _idx.col);
            }
            else if (!result.isEmpty())
            {
                // print a summary here
                Output& _out = NumeReKernel::getInstance()->getOutput();

                _out.setCompact(true);

                NumeReKernel::toggleTableStatus();
                make_hline();
                NumeReKernel::print("NUMERE: QUERY RESULT");
                make_hline();
                _out.format(result, 7, 25);
                _out.reset();
                NumeReKernel::toggleTableStatus();
                make_hline();
            }
        }

        cmdParser.setReturnValue(mu::Value(true));
    }
    else if (expression.front().getCommonType() == mu::TYPE_NUMERICAL && cmdParser.hasParam("close"))
        cmdParser.setReturnValue(mu::Value(closeDbConnection(expression.front().getAsScalarInt())));
    else
        cmdParser.setReturnValue(mu::Value(false));
}

