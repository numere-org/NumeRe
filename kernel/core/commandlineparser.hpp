/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef COMMANDLINEPARSER_HPP
#define COMMANDLINEPARSER_HPP

#include <string>
#include <vector>

#include "structures.hpp"
#include "datamanagement/dataaccess.hpp"

/////////////////////////////////////////////////
/// \brief This class provides the functionality
/// to extract the different components of a
/// command line into the single elements:
/// command, expression and parameter list.
/// Furthermore, it shall simplify the
/// interpretation of the values in the
/// expression and the parameter list. This shall
/// avoid code duplication.
/////////////////////////////////////////////////
class CommandLineParser
{
    public:
        enum CommandSignature
        {
            CMD_PAR, ///< Command-parameter sequence
            CMD_DAT_PAR, ///< Command-dataobject-parameter sequence (e.g. fit)
            CMD_EXPR_set_PAR ///< Command-expression-set-parameter sequence
        };

    private:
        std::string m_cmd;
        std::string m_expr;
        std::string m_parlist;
        std::string m_returnValueStatement;
        std::string m_commandLine;

        void parse(const std::string& sCommandString, CommandSignature signature);

    public:
        CommandLineParser(const std::string& sCommandLine, CommandSignature signature);
        CommandLineParser(const std::string& sCommandLine, const std::string& sCommand, CommandSignature signature);

        /////////////////////////////////////////////////
        /// \brief Returns the command.
        ///
        /// \return const std::string&
        ///
        /////////////////////////////////////////////////
        inline const std::string& getCommand() const
        {
            return m_cmd;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the expression as plain value.
        ///
        /// \return const std::string&
        ///
        /////////////////////////////////////////////////
        inline const std::string& getExpr() const
        {
            return m_expr;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the parameter list.
        ///
        /// \return const std::string&
        ///
        /////////////////////////////////////////////////
        inline const std::string& getParameterList() const
        {
            return m_parlist;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the return value statement for
        /// commands with return values.
        ///
        /// \return const std::string&
        ///
        /////////////////////////////////////////////////
        inline const std::string& getReturnValueStatement() const
        {
            return m_returnValueStatement;
        }

        /////////////////////////////////////////////////
        /// \brief Sets the return value of the current
        /// command by simply appending it to the return
        /// value statement.
        ///
        /// \param sRetVal const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void setReturnValue(const std::string& sRetVal)
        {
            m_returnValueStatement += sRetVal;
        }

        void setReturnValue(const std::vector<double>& vRetVal);
        void setReturnValue(const std::vector<std::string>& vRetVal);

        /////////////////////////////////////////////////
        /// \brief Removes the return value statement.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void clearReturnValue()
        {
            m_returnValueStatement.clear();
        }

        /////////////////////////////////////////////////
        /// \brief Returns the command line used for
        /// constructing this instance (e.g. for errors).
        ///
        /// \return const std::string&
        ///
        /////////////////////////////////////////////////
        inline const std::string& getCommandLine() const
        {
            return m_commandLine;
        }

        bool exprContainsDataObjects() const;
        std::string getExprAsFileName(std::string sFileExt) const;
        DataAccessParser getExprAsDataObject() const;
        std::string getExprAsMathExpression(bool parseDataObjects = false) const;
        std::string parseExprAsString() const;
        std::vector<double> parseExprAsNumericalValues() const;
        std::vector<double> parseIntervals(bool bErase = false);
        std::string getTargetTable(Indices& _targetIndices, const std::string& sDefaultTableName);
        std::string getParameterValue(const std::string& sParameter) const;
        std::string getFileParameterValue(std::string sFileExt, const std::string& sBaseFolder = "", const std::string& sDefaultName = "") const;
        std::string getParameterValueAsString(const std::string& sParameter, const std::string& sDefaultValue) const;
        std::vector<double> getParameterValueAsNumericalValue(const std::string& sParameter) const;
        bool hasParam(const std::string& sParameter) const;
};

#endif // COMMANDLINEPARSER_HPP

