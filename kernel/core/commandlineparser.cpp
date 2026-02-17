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
#include "utils/filecheck.hpp"

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
                if (isQuotationMark(m_expr, i))
                {
                    nQuotes++;
                    continue;
                }

                if (!(nQuotes % 2))
                {
                    // Handle parentheses
                    if (m_expr[i] == '(' || m_expr[i] == '{')
                        i += getMatchingParenthesis(StringView(m_expr, i));

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
                if (isQuotationMark(m_expr, i))
                    nQuotes++;

                // Are we not in a string literal?
                if (!(nQuotes % 2))
                {
                    if (m_expr[i] == '(' || m_expr[i] == '{')
                        i += getMatchingParenthesis(StringView(m_expr, i));

                    // Is this the start of the parameter list?
                    if (m_expr.substr(i, 2) == "--" || m_expr.substr(i, 5) == "-set ")
                    {
                        m_parlist = m_expr.substr(i);
                        m_expr.erase(i);

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

    if (sBasePath.length()
        && sFileName.find("\\\\") == std::string::npos
        && sFileName.find("//") == std::string::npos
        && sFileName.find(':') == std::string::npos
        && sFileName.find('<') == std::string::npos)
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
        throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, m_commandLine, SyntaxError::invalid_position, sFileExt);

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
/// \param vRetVal const mu::Array&
/// \return void
///
/////////////////////////////////////////////////
void CommandLineParser::setReturnValue(const mu::Array& vRetVal)
{
    m_returnValueStatement += NumeReKernel::getInstance()->getParser().CreateTempVar(vRetVal);
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
    std::string parsedForFileOperation = getExprForFileOperation();

    // Parse the prepared file path
    return parseFileName(parsedForFileOperation, sFileExt, sBasePath);
}


/////////////////////////////////////////////////
/// \brief Converts the expression to a string
/// but does not resolve the path completely
/// because the callee might require to know that
/// wildcards have been used within the file path.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getExprForFileOperation() const
{
    // Make a copy
    std::string sFilePath = m_expr;

    // Call functions first
    if (!NumeReKernel::getInstance()->getDefinitions().call(sFilePath))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sFilePath, SyntaxError::invalid_position);

    if (NumeReKernel::getInstance()->getMemoryManager().containsTables(sFilePath))
        getDataElements(sFilePath, NumeReKernel::getInstance()->getParser(), NumeReKernel::getInstance()->getMemoryManager());

    // Strip the spaces and ensure that there's something left
    StripSpaces(sFilePath);

    if (!sFilePath.length())
        return "";

    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

    // It is mostly possible to supply a file path without being enclosed
    // in quotation marks. is_dir checks for that
    if (!is_dir(sFilePath) || _parser.ContainsStringVars(sFilePath))
    {
        // String evaluation
        _parser.SetExpr(sFilePath);
        mu::Array v = _parser.Eval();
        sFilePath = v.front().getStr();
    }

    return sFilePath;
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

    if (sExpr.find("??") != std::string::npos)
        sExpr = promptForUserInput(sExpr);

    return DataAccessParser(sExpr, false);
}


/////////////////////////////////////////////////
/// \brief Parses the expression to a
/// DataView, which will either contain a
/// DataAccessParser, if the expression contains
/// a table or the result of the mathematical
/// expression.
///
/// \return DataView
///
/////////////////////////////////////////////////
DataView CommandLineParser::getExprAsDataView() const
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    std::string sExpr = m_expr;

    // Call functions first
    if (!instance->getDefinitions().call(sExpr))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, m_commandLine, sExpr);

    if (sExpr.find("??") != std::string::npos)
        sExpr = promptForUserInput(sExpr);

    StripSpaces(sExpr);

    if (instance->getMemoryManager().isTable(sExpr, true))
        return DataView(DataAccessParser(sExpr, false));

    // Resolve table accesses
    if (instance->getMemoryManager().containsTables(sExpr))
        getDataElements(sExpr, instance->getParser(), instance->getMemoryManager());

    StripSpaces(sExpr);

    instance->getParser().SetExpr(sExpr);
    return DataView(instance->getParser().Eval(), sExpr);
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

    if (sExpr.find("??") != std::string::npos)
        sExpr = promptForUserInput(sExpr);

    if (parseDataObjects && instance->getMemoryManager().containsTables(sExpr))
        getDataElements(sExpr, instance->getParser(), instance->getMemoryManager());

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
    return parseExpr().front().printVals();
}


/////////////////////////////////////////////////
/// \brief Parses the expression into numerical
/// values, returned as a vector of doubles.
///
/// \return std::vector<mu::Array>
///
/////////////////////////////////////////////////
std::vector<mu::Array> CommandLineParser::parseExpr() const
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    // Make a copy
    std::string sValue = m_expr;

    // Call functions first
    if (!instance->getDefinitions().call(sValue))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sValue, SyntaxError::invalid_position);

    // Resolve table accesses
    if (instance->getMemoryManager().containsTables(sValue))
        getDataElements(sValue, instance->getParser(), instance->getMemoryManager());

    StripSpaces(sValue);

    if (sValue.find("??") != std::string::npos)
        sValue = promptForUserInput(sValue);

    instance->getParser().SetExpr(sValue);
    int nRes;
    const mu::StackItem* v;
    v = instance->getParser().Eval(nRes);
    return mu::make_vector(v, nRes);
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
    return evaluateTargetOptionInCommand(m_parlist, sDefaultTableName, _targetIndices,
                                         instance->getParser(), instance->getMemoryManager(), instance->getSettings());
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
    NumeReKernel* instance = NumeReKernel::getInstance();

    std::string sParams = m_parlist;

    // Call functions first
    if (!instance->getDefinitions().call(sParams))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sParams, SyntaxError::invalid_position);

    if (instance->getMemoryManager().containsTables(sParams))
        getDataElements(sParams, instance->getParser(), instance->getMemoryManager());

    int nParPos = findParameter(sParams, "file", '=');

    if (!nParPos && sDefaultName.length())
        return _fSys.ValidFileName(removeQuotationMarks(sDefaultName), sFileExt, false);
    else if (!nParPos)
        return "";

    std::string sFileName = getArgAtPos(sParams, nParPos+4, ARGEXTRACT_NONE);

    // It is mostly possible to supply a file path without being enclosed
    // in quotation marks. is_dir checks for that
    if (!is_dir(sFileName))
    {
        // String evaluation
        mu::Parser& _parser = instance->getParser();
        _parser.SetExpr(sFileName);
        mu::Array v = _parser.Eval();
        sFileName = v.front().getStr();
    }

    // If a filename had been found, parse it here
    if (sFileName.length())
        return parseFileName(sFileName, sFileExt, sBaseFolder);
    else if (sDefaultName.length())
        return _fSys.ValidFileName(removeQuotationMarks(sDefaultName), sFileExt, false);

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

    return _fSys.ValidizeAndPrepareName(getFileParameterValue(sFileExt, sBaseFolder, sDefaultName), "");
}


/////////////////////////////////////////////////
/// \brief Parses the selected parameter value as
/// a string and returns it. If the parameter is
/// not found, the default value is returned.
///
/// \param sParameter const std::string&
/// \param sDefaultValue const std::string&
/// \param stripAlways bool Always strip quotation marks
/// \param onlyStringEvaluation bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string CommandLineParser::getParsedParameterValueAsString(const std::string& sParameter, const std::string& sDefaultValue, bool stripAlways, bool onlyStringEvaluation) const
{
    int nParPos = findParameter(m_parlist, sParameter, '=');

    if (!nParPos)
        return sDefaultValue;

    return getParsedParameterValue(sParameter).printVals();
}


/////////////////////////////////////////////////
/// \brief Parses the selected parameter and
/// returns them as a mu::Array. If the parameter
/// is not found, an empty mu::Array is returned.
///
/// \param sParameter const std::string&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array CommandLineParser::getParsedParameterValue(const std::string& sParameter) const
{
    int nParPos = findParameter(m_parlist, sParameter, '=');

    if (!nParPos)
        return mu::Array();

    std::string sArg = getArgAtPos(m_parlist,
                                   nParPos+sParameter.length(),
                                   ARGEXTRACT_NONE);

    NumeReKernel* instance = NumeReKernel::getInstance();

    // Function call
    if (!instance->getDefinitions().call(sArg))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sArg, "");

    // Read data
    if (instance->getMemoryManager().containsTables(sArg))
        getDataElements(sArg, instance->getParser(), instance->getMemoryManager());

    StripSpaces(sArg);

    // Handler for old syntax
    if (sArg.front() == '[' && sArg.back() == ']')
    {
        sArg.front() = '{';
        sArg.back() = '}';
    }

    // Numerical evaluation
    instance->getParser().SetExpr(sArg);

    //int nPrec = instance->getSettings().getPrecision();
    return instance->getParser().Eval();
}


/////////////////////////////////////////////////
/// \brief Parses the values within the
/// "params=[VALUES]" definition and returns
/// them. If the parameter is not found, an empty
/// vector is returned.
///
/// \return std::vector<mu::Array>
///
/////////////////////////////////////////////////
std::vector<mu::Array> CommandLineParser::getParamsValues() const
{
    if (!hasParam("params"))
        return std::vector<mu::Array>();

    std::string sParams = getParameterValue("params");

    NumeReKernel* instance = NumeReKernel::getInstance();

    // Function call
    if (!instance->getDefinitions().call(sParams))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sParams, "");

    // Read data
    if (instance->getMemoryManager().containsTables(sParams))
        getDataElements(sParams, instance->getParser(), instance->getMemoryManager());

    StripSpaces(sParams);

    // Numerical evaluation
    if (sParams.front() == '[' && sParams.back() == ']')
        instance->getParser().SetExpr(StringView(sParams, 1, sParams.length()-2));
    else
        instance->getParser().SetExpr(sParams);

    int nResults = 0;
    const mu::StackItem* res = instance->getParser().Eval(nResults);

    return mu::make_vector(res, nResults);
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



