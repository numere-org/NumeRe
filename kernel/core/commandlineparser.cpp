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

#include "commandlineparser.hpp"
#include "../kernel.hpp"
#include "utils/tools.hpp"            // For findCommand, extractCommandString and getMatchingParenthesis
#include "maths/parser_functions.hpp" // For evaluateTargetOptionInCommand and readAndParseIntervals -> shall be moved here

// Prototype needed for file name conversion
std::string removeQuotationMarks(const std::string& sString);



/////////////////////////////////////////////////
/// \brief CommandLineParser constructor from a
/// command line and the command signature. Will
/// use the standard findCommand() function to
/// detect a command.
///
/// \param sCommandLine const std::string&
/// \param signature CommandLineParser::CommandSignature
///
/////////////////////////////////////////////////
CommandLineParser::CommandLineParser(const std::string& sCommandLine, CommandLineParser::CommandSignature signature) : m_commandLine(sCommandLine)
{
    // Try to find any command
    Match _mMatch = findCommand(sCommandLine);

    // Parse the command line, if a command was found
    if (_mMatch.sString.length())
    {
        m_cmd = _mMatch.sString;
        m_returnValueStatement = sCommandLine.substr(0, _mMatch.nPos);

        parse(extractCommandString(sCommandLine, _mMatch), signature);
    }
}


/////////////////////////////////////////////////
/// \brief CommandLineParser constructor from the
/// command line, a specified command and the
/// signature. Will use the standard
/// findCommand() function in combination with
/// the specified command to locate it in the
/// command line.
///
/// \param sCommandLine const std::string&
/// \param sCommand const std::string&
/// \param signature CommandLineParser::CommandSignature
///
/////////////////////////////////////////////////
CommandLineParser::CommandLineParser(const std::string& sCommandLine, const std::string& sCommand, CommandLineParser::CommandSignature signature) : m_commandLine(sCommandLine)
{
    // Try to find the specified command
    Match _mMatch = findCommand(sCommandLine, sCommand);

    // Parse the command line, if the command was found
    if (_mMatch.sString.length())
    {
        m_cmd = sCommand;
        m_returnValueStatement = sCommandLine.substr(0, _mMatch.nPos);

        parse(extractCommandString(sCommandLine, _mMatch), signature);
    }
}


/////////////////////////////////////////////////
/// \brief Private helper member function used
/// for parsing the command line as specified by
/// the command line signature.
///
/// \param sCommandString const std::string&
/// \param signature CommandLineParser::CommandSignature
/// \return void
///
/////////////////////////////////////////////////
void CommandLineParser::parse(const std::string& sCommandString, CommandLineParser::CommandSignature signature)
{
    switch (signature)
    {
        // Command-parameter sequence
        case CMD_PAR:
            m_parlist = sCommandString.substr(sCommandString.find('-'));
            break;

        // Command-dataobject-parameter sequence
        case CMD_DAT_PAR:
            m_expr = sCommandString.substr(m_cmd.length()+1);
            StripSpaces(m_expr);

            for (size_t i = 0; i < m_expr.length(); i++)
            {
                if (m_expr[i] == '(' || m_expr[i] == '{')
                    i += getMatchingParenthesis(m_expr.substr(i));

                if (m_expr[i] == ' ' || m_expr[i] == '-')
                {
                    m_parlist = m_expr.substr(m_expr.find('-', i));
                    m_expr.erase(i);
                    break;
                }
            }

            break;

        // Command-expression-set-parameter sequence
        case CMD_EXPR_set_PAR:
            m_expr = sCommandString.substr(m_cmd.length()+1);
            StripSpaces(m_expr);

            size_t nQuotes = 0;

            for (size_t i = 0; i < m_expr.length(); i++)
            {
                if (m_expr[i] == '"' && (!i || m_expr[i-1] != '\\'))
                    nQuotes++;

                if (!(nQuotes % 2))
                {
                    if (m_expr[i] == '(' || m_expr[i] == '{')
                        i += getMatchingParenthesis(m_expr.substr(i));

                    if (m_expr[i] == ' ' || m_expr[i] == '-')
                    {
                        size_t pos = std::min(m_expr.find("-set", i), m_expr.find("--", i));

                        if (pos != std::string::npos)
                        {
                            m_parlist = m_expr.substr(pos);
                            m_expr.erase(pos);
                        }

                        break;
                    }
                }
            }

    }

    // Strip obsolete spaces
    StripSpaces(m_expr);
    StripSpaces(m_parlist);
}


/////////////////////////////////////////////////
/// \brief Converts the expression to a file name
/// and removes the surrounding quotation marks,
/// if any. The passed extension is used to
/// validate the file name.
///
/// \param sFileExt const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getExprAsFileName(const std::string& sFileExt) const
{
    // Make a copy
    std::string sFileName = m_expr;

    // Call functions first
    if (!NumeReKernel::getInstance()->getDefinitions().call(sFileName))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sFileName, SyntaxError::invalid_position);

    // Parse strings (if any)
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sFileName))
    {
        string sDummy = "";
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sFileName, sDummy, true);
    }

    // Strip spaces
    StripSpaces(sFileName);

    // Return the validated file name
    return NumeReKernel::getInstance()->getMemoryManager().ValidFileName(removeQuotationMarks(sFileName), sFileExt);
}


/////////////////////////////////////////////////
/// \brief Parses the expression to a
/// DataAccessParser, which will extract the
/// needed information for the current data
/// object.
///
/// \return DataAccessParser
///
/////////////////////////////////////////////////
DataAccessParser CommandLineParser::getExprAsDataObject() const
{
    return DataAccessParser(m_expr);
}


/////////////////////////////////////////////////
/// \brief Prepares the expression by calling
/// custom function definitions and resolving
/// vector braces so that the expression may be
/// used for numerical evaluation.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getExprAsMathExpression() const
{
    // Make a copy
    std::string sExpr = m_expr;

    // Call functions first
    if (!NumeReKernel::getInstance()->getDefinitions().call(sExpr))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, m_commandLine, sExpr);

    if (sExpr.find("??") != string::npos)
        sExpr = promptForUserInput(sExpr);

    if (sExpr.find('{') != std::string::npos)
        convertVectorToExpression(sExpr, NumeReKernel::getInstance()->getSettings());

    StripSpaces(sExpr);

    return sExpr;
}


/////////////////////////////////////////////////
/// \brief Parses the expression into numerical
/// values, returned as a vector of doubles.
///
/// \return std::vector<double>
///
/////////////////////////////////////////////////
std::vector<double> CommandLineParser::parseExprAsNumericalValues() const
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    // Make a copy
    std::string sValue = m_expr;

    // Call functions first
    if (!instance->getDefinitions().call(sValue))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sValue, SyntaxError::invalid_position);

    // Parse strings (if any)
    if (instance->getStringParser().isStringExpression(sValue))
    {
        string sDummy = "";
        instance->getStringParser().evalAndFormat(sValue, sDummy, true);
    }

    // Resolve table accesses
    if (instance->getMemoryManager().containsTablesOrClusters(sValue))
        getDataElements(sValue, instance->getParser(), instance->getMemoryManager(), instance->getSettings());

    instance->getParser().SetExpr(sValue);
    int nRes;
    value_type* v;

    v = instance->getParser().Eval(nRes);

    std::vector<double> vVals(v, v+nRes);

    return vVals;
}


/////////////////////////////////////////////////
/// \brief Parses intervals in the parameter list
/// and returns them as a vector. The interval
/// may be deleted, if the corresponding flag is
/// set to true. Only wraps
/// readAndParseIntervals().
///
/// \param bErase bool
/// \return std::vector<double>
///
/////////////////////////////////////////////////
std::vector<double> CommandLineParser::parseIntervals(bool bErase)
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    return readAndParseIntervals(m_parlist, instance->getParser(), instance->getMemoryManager(), instance->getDefinitions(), instance->getSettings(), bErase);
}


/////////////////////////////////////////////////
/// \brief Evaluates any target=TABLE()
/// statements in the parameter list and returns
/// the needed information. Wraps the
/// evaluateTargetOptionInCommand() function.
///
/// \param _targetIndices Indices&
/// \param sDefaultTableName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getTargetTable(Indices& _targetIndices, const std::string& sDefaultTableName)
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    return evaluateTargetOptionInCommand(m_parlist, sDefaultTableName, _targetIndices, instance->getParser(), instance->getMemoryManager(), instance->getSettings());
}


/////////////////////////////////////////////////
/// \brief Parses the value of the common "file"
/// command line parameter and returns a valid
/// filename.
///
/// \param sFileExt const std::string&
/// \param sBaseFolder const std::string&
/// \param sDefaultName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getFileParameterValue(const std::string& sFileExt, const std::string& sBaseFolder, const std::string& sDefaultName) const
{
    int nParPos = findParameter(m_parlist, "file", '=');

    if (!nParPos)
        return NumeReKernel::getInstance()->getMemoryManager().ValidFileName(removeQuotationMarks(sDefaultName), sFileExt);

    std::string arg = getArgAtPos(m_parlist, nParPos+4, ARGEXTRACT_NONE);
    NumeReKernel* instance = NumeReKernel::getInstance();

    // Function call
    if (!instance->getDefinitions().call(arg))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, arg, "");

    // String evaluation
    if (instance->getStringParser().isStringExpression(arg))
    {
        std::string dummy;
        instance->getStringParser().evalAndFormat(arg, dummy, true);
    }

    if (arg.length())
    {
        if (arg.find('/') == string::npos && arg.find('\\') == string::npos)
            arg.insert(0, sBaseFolder);

        return NumeReKernel::getInstance()->getMemoryManager().ValidFileName(removeQuotationMarks(arg), sFileExt);
    }

    return NumeReKernel::getInstance()->getMemoryManager().ValidFileName(removeQuotationMarks(sDefaultName), sFileExt);
}


/////////////////////////////////////////////////
/// \brief Parses the selected parameter value as
/// a string and returns it. If the parameter is
/// not found, the default value is returned.
///
/// \param sParameter const std::string&
/// \param sDefaultValue const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getParameterValueAsString(const std::string& sParameter, const std::string& sDefaultValue) const
{
    int nParPos = findParameter(m_parlist, sParameter, '=');

    if (!nParPos)
        return sDefaultValue;

    std::string arg = getArgAtPos(m_parlist, nParPos+sParameter.length(), ARGEXTRACT_PARSED);
    StripSpaces(arg);

    if (arg.find(",") != std::string::npos && arg.find("\"") != std::string::npos)
        return arg;
    else if (arg.front() == '"' && arg.back() == '"')
        return removeQuotationMarks(arg);

    return arg;
}


/////////////////////////////////////////////////
/// \brief Parses the selected parameter as (one
/// or more) numerical value(s) and returns them
/// as a vector of doubles. If the parameter is
/// not found, an empty vector is returned.
///
/// \param sParameter const std::string&
/// \return std::vector<double>
///
/////////////////////////////////////////////////
std::vector<double> CommandLineParser::getParameterValueAsNumericalValue(const std::string& sParameter) const
{
    int nParPos = findParameter(m_parlist, sParameter, '=');

    if (!nParPos)
        return std::vector<double>();

    std::string arg = getArgAtPos(m_parlist, nParPos+sParameter.length(), ARGEXTRACT_PARSED | ARGEXTRACT_STRIPPED);
    std::vector<double> vArgs;

    while (arg.length())
    {
        vArgs.push_back(StrToDb(getNextArgument(arg, true)));
    }

    return vArgs;
}


