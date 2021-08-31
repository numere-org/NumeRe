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
#include "maths/parser_functions.hpp" // For evaluateTargetOptionInCommand

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
        m_cmd = _mMatch.sString;
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
        {
            m_expr = sCommandString.substr(m_cmd.length()+1);
            StripSpaces(m_expr);

            size_t nQuotes = 0;

            for (size_t i = 0; i < m_expr.length(); i++)
            {
                if (m_expr[i] == '"' && (!i || m_expr[i-1] != '\\'))
                {
                    nQuotes++;
                    continue;
                }

                if (!(nQuotes % 2))
                {
                    // Handle parentheses
                    if (m_expr[i] == '(' || m_expr[i] == '{')
                        i += getMatchingParenthesis(m_expr.substr(i));

                    // Handle lists (will jump to the minus sign or
                    // a possible following expression part, if no list)
                    if ((m_expr[i] == ',' || m_expr[i] == ' ' || m_expr[i] == '+') && m_expr.find_first_not_of(",+ ", i) != std::string::npos)
                        i = m_expr.find_first_not_of(",+ ", i);

                    // If this is a quotation mark, increment the
                    // counter and continue
                    if (m_expr[i] == '"')
                    {
                        nQuotes++;
                        continue;
                    }

                    // Extract params
                    if (m_expr[i] == ' ' || m_expr[i] == '-')
                    {
                        m_parlist = m_expr.substr(m_expr.find('-', i));
                        m_expr.erase(i);
                        break;
                    }
                }
            }

            break;
        }
        // Command-expression-set-parameter sequence
        case CMD_EXPR_set_PAR:
        {
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
    }

    // Strip obsolete spaces
    StripSpaces(m_expr);
    StripSpaces(m_parlist);
}


/////////////////////////////////////////////////
/// \brief Common method to convert a prepared
/// string into a valid file or folder name.
///
/// \param sFileName std::string&
/// \param sFileExt std::string&
/// \param sBasePath const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::parseFileName(std::string& sFileName, std::string& sFileExt, const std::string& sBasePath) const
{
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    if (sBasePath.length() && sFileName.find("//") == std::string::npos && sFileName.find(':') == std::string::npos && sFileName.find('<') == std::string::npos)
        sFileName.insert(0, sBasePath + "/");

    // If the filename contains a extension, extract it here and declare it as a valid file type
    if (sFileName.find('.') != std::string::npos)
    {
        size_t nPos = sFileName.find_last_of("/\\");

        if (sFileName.find('.', nPos) != std::string::npos)
            sFileExt = sFileName.substr(sFileName.rfind('.'));
    }

    // There are some protected ones
    if (sFileExt == ".exe" || sFileExt == ".dll" || sFileExt == ".sys")
    {
        throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, m_commandLine, SyntaxError::invalid_position, sFileExt);
    }

    if (!sFileExt.length())
        return _fSys.ValidFolderName(sFileName);

    // Declare the extension
    _fSys.declareFileType(sFileExt);

	// Get a valid file name
    return _fSys.ValidFileName(sFileName, sFileExt);
}


/////////////////////////////////////////////////
/// \brief Simple wrapper around the vector
/// variable creation functionality of the
/// parser.
///
/// \param vRetVal const std::vector<mu::value_type>&
/// \return void
///
/////////////////////////////////////////////////
void CommandLineParser::setReturnValue(const std::vector<mu::value_type>& vRetVal)
{
    m_returnValueStatement += NumeReKernel::getInstance()->getParser().CreateTempVectorVar(vRetVal);
}


/////////////////////////////////////////////////
/// \brief Simple wrapper around the vector
/// variable creation functionality of the string
/// parser.
///
/// \param vRetVal const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void CommandLineParser::setReturnValue(const std::vector<std::string>& vRetVal)
{
    m_returnValueStatement += NumeReKernel::getInstance()->getStringParser().createTempStringVectorVar(vRetVal);
}


/////////////////////////////////////////////////
/// \brief Simply returns, whether the expression
/// contains any data objects.
///
/// \return bool
///
/////////////////////////////////////////////////
bool CommandLineParser::exprContainsDataObjects() const
{
    return NumeReKernel::getInstance()->getMemoryManager().containsTablesOrClusters(m_expr);
}


/////////////////////////////////////////////////
/// \brief Converts the expression to a file name
/// and removes the surrounding quotation marks,
/// if any. The passed extension is used to
/// validate the file name.
///
/// \param sFileExt std::string
/// \param sBasePath const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getExprAsFileName(std::string sFileExt, const std::string& sBasePath) const
{
    // Make a copy
    std::string sFileName = m_expr;

    // Call functions first
    if (!NumeReKernel::getInstance()->getDefinitions().call(sFileName))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sFileName, SyntaxError::invalid_position);

    if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sFileName))
        NumeReKernel::getInstance()->getStringParser().getStringValues(sFileName);

    if (NumeReKernel::getInstance()->getMemoryManager().containsTablesOrClusters(sFileName))
        getDataElements(sFileName, NumeReKernel::getInstance()->getParser(), NumeReKernel::getInstance()->getMemoryManager(), NumeReKernel::getInstance()->getSettings());

    // Strip the spaces and ensure that there's something left
    StripSpaces(sFileName);

    if (!sFileName.length())
        return "";

    // If there's a string in the file name, parse it here
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sFileName))
    {
        string sDummy;
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sFileName, sDummy, true);
    }

    sFileName = removeQuotationMarks(sFileName);

    // Parse the prepared file path
    return parseFileName(sFileName, sFileExt, sBasePath);
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
    std::string sExpr = m_expr;

    // Call functions first
    if (!NumeReKernel::getInstance()->getDefinitions().call(sExpr))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, m_commandLine, sExpr);

    if (sExpr.find("??") != string::npos)
        sExpr = promptForUserInput(sExpr);

    return DataAccessParser(sExpr);
}


/////////////////////////////////////////////////
/// \brief Prepares the expression by calling
/// custom function definitions and resolving
/// vector braces so that the expression may be
/// used for numerical evaluation.
///
/// \param parseDataObjects bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getExprAsMathExpression(bool parseDataObjects) const
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    // Make a copy
    std::string sExpr = m_expr;

    // Call functions first
    if (!instance->getDefinitions().call(sExpr))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, m_commandLine, sExpr);

    if (sExpr.find("??") != string::npos)
        sExpr = promptForUserInput(sExpr);

    if (parseDataObjects && instance->getMemoryManager().containsTablesOrClusters(sExpr))
        getDataElements(sExpr, instance->getParser(), instance->getMemoryManager(), instance->getSettings());

    if (sExpr.find('{') != std::string::npos)
        convertVectorToExpression(sExpr, instance->getSettings());

    StripSpaces(sExpr);

    return sExpr;
}


/////////////////////////////////////////////////
/// \brief Prepares the expression by handling
/// all string operations and removing the
/// surrounding quotation marks.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::parseExprAsString() const
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    // Make a copy
    std::string sExpr = m_expr;

    // Call functions first
    if (!instance->getDefinitions().call(sExpr))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, m_commandLine, sExpr);

    StripSpaces(sExpr);

    if (sExpr.find("??") != string::npos)
        sExpr = promptForUserInput(sExpr);

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sExpr))
    {
        sExpr += " -nq";
        string sDummy;
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sExpr, sDummy, true);
    }

    return sExpr;
}


/////////////////////////////////////////////////
/// \brief Parses the expression into numerical
/// values, returned as a vector of doubles.
///
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> CommandLineParser::parseExprAsNumericalValues() const
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
    mu::value_type* v;

    v = instance->getParser().Eval(nRes);

    std::vector<mu::value_type> vVals(v, v+nRes);

    return vVals;
}


/////////////////////////////////////////////////
/// \brief Parses intervals in the parameter list
/// and returns them as a vector. The interval
/// may be deleted, if the corresponding flag is
/// set to true.
///
/// \param bErase bool
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet CommandLineParser::parseIntervals(bool bErase)
{
    return IntervalSet(m_parlist);
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
/// \brief Returns a vector containing all
/// parameters with values in the current
/// parameter list. Does not return parameters,
/// which do not have any value.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> CommandLineParser::getAllParametersWithValues() const
{
    size_t pos = 0;
    std::vector<std::string> vParams;

    while ((pos = m_parlist.find('=', pos)) != std::string::npos)
    {
        if (!isInQuotes(m_parlist, pos))
        {
            size_t lastChar = m_parlist.find_last_not_of(" =", pos);

            if (lastChar)
            {
                size_t firstChar = m_parlist.find_last_of(" -", lastChar) + 1;
                vParams.push_back(m_parlist.substr(firstChar, lastChar + 1 - firstChar));
            }
        }

        pos++;
    }

    return vParams;
}


/////////////////////////////////////////////////
/// \brief Simply returns the parameter value or
/// an empty string. Does not do any parsing
/// steps.
///
/// \param sParameter const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getParameterValue(const std::string& sParameter) const
{
    int nParPos = findParameter(m_parlist, sParameter, '=');

    if (!nParPos)
        return "";

    return getArgAtPos(m_parlist, nParPos+sParameter.length(), ARGEXTRACT_NONE);
}


/////////////////////////////////////////////////
/// \brief Parses the value of the common "file"
/// command line parameter and returns a valid
/// filename.
///
/// \param sFileExt std::string
/// \param sBaseFolder const std::string&
/// \param sDefaultName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getFileParameterValue(std::string sFileExt, const std::string& sBaseFolder, const std::string& sDefaultName) const
{
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    std::string sParams = m_parlist;

    // Call functions first
    if (!NumeReKernel::getInstance()->getDefinitions().call(sParams))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sParams, SyntaxError::invalid_position);

    if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sParams))
        NumeReKernel::getInstance()->getStringParser().getStringValues(sParams);

    if (NumeReKernel::getInstance()->getMemoryManager().containsTablesOrClusters(sParams))
        getDataElements(sParams, NumeReKernel::getInstance()->getParser(), NumeReKernel::getInstance()->getMemoryManager(), NumeReKernel::getInstance()->getSettings());

    int nParPos = findParameter(sParams, "file", '=');

    if (!nParPos && sDefaultName.length())
        return _fSys.ValidFileName(removeQuotationMarks(sDefaultName), sFileExt);
    else if (!nParPos)
        return "";

    std::string sFileName = getArgAtPos(sParams, nParPos+4, ARGEXTRACT_NONE);
    NumeReKernel* instance = NumeReKernel::getInstance();

    // String evaluation
    if (instance->getStringParser().isStringExpression(sFileName))
    {
        std::string dummy;
        instance->getStringParser().evalAndFormat(sFileName, dummy, true);
    }

    sFileName = removeQuotationMarks(sFileName);

    // If a filename had been found, parse it here
    if (sFileName.length())
        return parseFileName(sFileName, sFileExt, sBaseFolder);
    else if (sDefaultName.length())
        return _fSys.ValidFileName(removeQuotationMarks(sDefaultName), sFileExt);

    return "";
}


/////////////////////////////////////////////////
/// \brief Parses the value of the common "file"
/// command line parameter and returns a valid
/// filename. Ensures that the target folder
/// actually exists.
///
/// \param sFileExt std::string
/// \param sBaseFolder const std::string&
/// \param sDefaultName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getFileParameterValueForSaving(std::string sFileExt, const std::string& sBaseFolder, const std::string& sDefaultName) const
{
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    return _fSys.ValidizeAndPrepareName(getFileParameterValue(sFileExt, sBaseFolder, sDefaultName), sFileExt);
}


/////////////////////////////////////////////////
/// \brief Parses the selected parameter value as
/// a string and returns it. If the parameter is
/// not found, the default value is returned.
///
/// \param sParameter const std::string&
/// \param sDefaultValue const std::string&
/// \param stripAlways bool Always strip quotation marks
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getParameterValueAsString(const std::string& sParameter, const std::string& sDefaultValue, bool stripAlways) const
{
    int nParPos = findParameter(m_parlist, sParameter, '=');

    if (!nParPos)
        return sDefaultValue;

    std::string arg = getArgAtPos(m_parlist, nParPos+sParameter.length(), ARGEXTRACT_PARSED);
    StripSpaces(arg);

    if (!stripAlways && arg.find(",") != std::string::npos && arg.find("\"") != std::string::npos)
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
/// \return std::vector<mu::value_type>
///
/////////////////////////////////////////////////
std::vector<mu::value_type> CommandLineParser::getParameterValueAsNumericalValue(const std::string& sParameter) const
{
    int nParPos = findParameter(m_parlist, sParameter, '=');

    if (!nParPos)
        return std::vector<mu::value_type>();

    std::string arg = getArgAtPos(m_parlist, nParPos+sParameter.length(), ARGEXTRACT_PARSED | ARGEXTRACT_STRIPPED);
    std::vector<mu::value_type> vArgs;

    while (arg.length())
    {
        vArgs.push_back(StrToDb(getNextArgument(arg, true)));
    }

    return vArgs;
}


/////////////////////////////////////////////////
/// \brief Simple wrapper around findParameter(),
/// if used as a boolean flag.
///
/// \param sParameter const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool CommandLineParser::hasParam(const std::string& sParameter) const
{
    return findParameter(m_parlist, sParameter) != 0;
}



