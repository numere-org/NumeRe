/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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


#include "procedure.hpp"
#include "../../kernel.hpp"
#include "procedurevarfactory.hpp"
#include <memory>
#define MAX_PROCEDURE_STACK_SIZE 2000000

extern value_type vAns;


/////////////////////////////////////////////////
/// \brief Default constructor
/////////////////////////////////////////////////
Procedure::Procedure() : FlowCtrl(), PackageManager(), _localDef(true)
{
    // init the object
    init();
}


/////////////////////////////////////////////////
/// \brief Custom constructor using the presets
/// from the passed procedure instance. Used as a
/// recursion functionality.
///
/// \param _procedure const Procedure&
///
/////////////////////////////////////////////////
Procedure::Procedure(const Procedure& _procedure) : FlowCtrl(), PackageManager(_procedure), _localDef(true)
{
    // Init the object
    init();

    // Copy the relevant data
    sPath = _procedure.sPath;
    sExecutablePath = _procedure.sExecutablePath;
    sCallingNameSpace = _procedure.sCallingNameSpace;
    sProcNames = _procedure.sProcNames;

    if (_procedure.nDebuggerCode == NumeReKernel::DEBUGGER_LEAVE || _procedure.nDebuggerCode == NumeReKernel::DEBUGGER_STEPOVER)
        nDebuggerCode = NumeReKernel::DEBUGGER_LEAVE;

    _localDef.setPredefinedFuncs(_procedure._localDef.getPredefinedFuncs());

    for (unsigned int i = 0; i < 6; i++)
    {
        sTokens[i][0] = _procedure.sTokens[i][0];
        sTokens[i][1] = _procedure.sTokens[i][1];
    }
}


/////////////////////////////////////////////////
/// \brief Destructor ensuring that the procedure
/// output file stream will be closed, if it is
/// still open.
/////////////////////////////////////////////////
Procedure::~Procedure()
{
    if (fProcedure.is_open())
        fProcedure.close();

    if (_varFactory)
    {
        delete _varFactory;
    }
}


/////////////////////////////////////////////////
/// \brief Private initializing member function.
/// Sets all variables to a reasonable default
/// value.
///
/// \return void
///
/////////////////////////////////////////////////
void Procedure::init()
{
    sCallingNameSpace = "main";
    bProcSupressAnswer = false;
    bWritingTofile = false;
    nCurrentLine = 0;
    nthBlock = 0;
    nFlags = 0;

    _varFactory = nullptr;
}


/////////////////////////////////////////////////
/// \brief This member function does the
/// evaluation stuff regarding strings and
/// numerical expressions for the current
/// procedure.
///
/// \param sLine string
/// \param sCurrentCommand string
/// \param nByteCode int&
/// \param _parser Parser&
/// \param _functions Define&
/// \param _data Datafile&
/// \param _option Settings&
/// \param _out Output&
/// \param _pData PlotData&
/// \param _script Script&
/// \return Returnvalue
///
/////////////////////////////////////////////////
Returnvalue Procedure::ProcCalc(string sLine, string sCurrentCommand, int& nByteCode, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script)
{
    string sCache = "";
    Indices _idx;
    bool bWriteToCache = false;
    bool bWriteToCluster = false;
    Returnvalue thisReturnVal;
    int nNum = 0;
    int nCurrentByteCode = nByteCode;
    value_type* v = nullptr;

    // Do not clear the vector variables, if we are currently part of a
    // loop, because the loop uses the cached vector variables for
    // speeding up the whole calculation process
    if (!_parser.ActiveLoopMode() || (!_parser.IsLockedPause() && !(nFlags & ProcedureCommandLine::FLAG_INLINE)))
        _parser.ClearVectorVars(true);

    // Check, whether the user pressed "ESC"
    if (NumeReKernel::GetAsyncCancelState())
    {
        throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
    }

    // Remove obsolete whitespaces
    StripSpaces(sLine);

    // Ignore empty lines
    if (!sLine.length() || sLine[0] == '@')
    {
        thisReturnVal.vNumVal.push_back(NAN);
        return thisReturnVal;
    }

    // Check for the "assert" command
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_ASSERT
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
    {
        if (findCommand(sLine, "assert").sString == "assert")
        {
            _assertionHandler.enable(sLine);
            sLine.erase(findCommand(sLine, "assert").nPos, 6);
            StripSpaces(sLine);

            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_ASSERT;
        }
    }

    // Handle the "to_cmd()" function, which is quite slow
    // Only handle this function, if we're not inside of a loop
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_TOCOMMAND
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
    {
        if (sLine.find("to_cmd(") != string::npos)
        {
            unsigned int nPos = 0;

            // As long as the "to_cmd()" function is found
            while (sLine.find("to_cmd(", nPos) != string::npos)
            {
                nPos = sLine.find("to_cmd(", nPos) + 6;

                if (isInQuotes(sLine, nPos))
                    continue;

                unsigned int nParPos = getMatchingParenthesis(sLine.substr(nPos));

                if (nParPos == string::npos)
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

                string sCmdString = sLine.substr(nPos + 1, nParPos - 1);
                StripSpaces(sCmdString);

                // Parse strings, if the argument contains some
                if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmdString))
                {
                    sCmdString += " -nq";
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmdString, sCache, true);
                    sCache = "";
                }

                // Replace the current command line
                sLine = sLine.substr(0, nPos - 6) + sCmdString + sLine.substr(nPos + nParPos + 1);
                nPos -= 5;

                if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_TOCOMMAND;
            }

            replaceLocalVars(sLine);
            sCurrentCommand = findCommand(sLine).sString;
        }
    }

    // Handle the "throw" command
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_THROWCOMMAND
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
    {
        if (sCurrentCommand == "throw")
        {
            string sErrorToken;

            if (sLine.length() > 6 && NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine))
            {
                if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sLine))
                    NumeReKernel::getInstance()->getStringParser().getStringValues(sLine);

                getStringArgument(sLine, sErrorToken);
                sErrorToken += " -nq";
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sErrorToken, sCache, true);
            }

            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_THROWCOMMAND;

            throw SyntaxError(SyntaxError::PROCEDURE_THROW, sLine, SyntaxError::invalid_position, sErrorToken);
        }
    }

    // Call the user prompt routine
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_PROMPT
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
    {
        if (sLine.find("??") != string::npos && sCurrentCommand != "help")
        {
            sLine = promptForUserInput(sLine);

            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_PROMPT;
        }
    }

    /* --> Die Keyword-Suche soll nur funktionieren, wenn keine Schleife eingegeben wird, oder wenn eine
     *     eine Schleife eingegeben wird, dann nur in den wenigen Spezialfaellen, die zum Nachschlagen
     *     eines Keywords noetig sind ("list", "help", "find", etc.) <--
     */
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_COMMAND
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
    {
        if (!FlowCtrl::isFlowCtrlStatement(sCurrentCommand)
            || sCurrentCommand == "help"
            || sCurrentCommand == "man"
            || sCurrentCommand == "quit"
            || sCurrentCommand == "list"
            || sCurrentCommand == "find"
            || sCurrentCommand == "search"
            || sCurrentCommand == "mode"
            || sCurrentCommand == "menue")
        {
            NumeReKernel::bSupressAnswer = bProcSupressAnswer;
            string sCommandBack = sLine;
            switch (commandHandler(sLine))
            {
                case COMMAND_HAS_RETURNVALUE:
                case NO_COMMAND:
                    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    {
                        StripSpaces(sCommandBack);
                        string sCurrentLine = sLine;
                        StripSpaces(sCurrentLine);
                        if (sCommandBack != sCurrentLine)
                            nByteCode |= ProcedureCommandLine::BYTECODE_COMMAND;
                    }
                    break; // Kein Keywort: Mit dem Parser auswerten
                case COMMAND_PROCESSED:        // Keywort: Naechster Schleifendurchlauf!
                    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                        nByteCode |= ProcedureCommandLine::BYTECODE_COMMAND;

                    thisReturnVal.vNumVal.push_back(NAN);
                    return thisReturnVal;
                default:
                    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                        nByteCode |= ProcedureCommandLine::BYTECODE_COMMAND;

                    thisReturnVal.vNumVal.push_back(NAN);
                    return thisReturnVal;  // Keywort "mode"
            }

            // It may be possible that the user entered "??" during some
            // command (i.e. "readline"). We'll handle this string at this
            // place
            if (sLine.find("??") != string::npos)
                sLine = promptForUserInput(sLine);

            // It may also be possible that some procedure occures at this
            // position. Handle them here
            if (sLine.find('$') != std::string::npos)
                procedureInterface(sLine, _parser, _functions, _data, _out, _pData, _script, _option, 0);
        }

    }

    // Call functions if we're not in a loop
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT))
    {
        if (sCurrentCommand != "for" && sCurrentCommand != "if" && sCurrentCommand != "while" && sCurrentCommand != "switch")
        {
            if (!_functions.call(sLine))
                throw SyntaxError(SyntaxError::FUNCTION_ERROR, sLine, SyntaxError::invalid_position);

            // Reduce surrounding white spaces
            StripSpaces(sLine);
        }
    }

    // Handle recursive expressions
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_RECURSIVEEXPRESSION
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
    {
        if (!getCurrentBlockDepth())
        {
            // Keep breakpoints and remove the token from the command line
            bool bBreakPoint = (sLine.substr(0, 2) == "|>");

            if (bBreakPoint)
            {
                sLine.erase(0, 2);
                StripSpaces(sLine);
            }

            // evaluate the recursive expression
            evalRecursiveExpressions(sLine);

            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_RECURSIVEEXPRESSION;
        }
    }

    // If we're already in a flow control statement
    // or the current command starts with a flow control
    // statement, then we pass the current command line
    // to the FlowCtrl class
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)
    {
        if (getCurrentBlockDepth() || FlowCtrl::isFlowCtrlStatement(sCurrentCommand))
        {
            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT;
            // Add the suppression semicolon, if necessary
            if (bProcSupressAnswer)
                sLine += ";";

            // Pass the command line to the FlowCtrl class
            setCommand(sLine, nCurrentLine);

            // Return now to the calling function
            thisReturnVal.vNumVal.push_back(NAN);
            return thisReturnVal;
        }
    }

    // Get elements from data access
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || nCurrentByteCode & ProcedureCommandLine::BYTECODE_DATAACCESS
        || nFlags & ProcedureCommandLine::FLAG_TEMPLATE)
    {
        if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine)
            && _data.containsTablesOrClusters(sLine))
        {
            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_DATAACCESS;

            sCache = getDataElements(sLine, _parser, _data, _option);

            if (sCache.length() && sCache.find('#') == string::npos)
                bWriteToCache = true;

            // Ad-hoc bytecode adaption
#warning NOTE (numere#1#08/21/21): Might need some adaption, if bytecode issues are experienced
            if (nCurrentByteCode && NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine))
                nCurrentByteCode |= ProcedureCommandLine::BYTECODE_STRING;
        }
        else if (isClusterCandidate(sLine, sCache))
        {
            bWriteToCache = true;

            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_DATAACCESS;
        }
    }

    // If the current line contains a string value or a string variable,
    // call the string parser and handle the return value
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || nCurrentByteCode & ProcedureCommandLine::BYTECODE_STRING
        || nFlags & ProcedureCommandLine::FLAG_TEMPLATE)
    {
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine))
        {
            auto retVal = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sLine, sCache, bProcSupressAnswer);
            NumeReKernel::getInstance()->getStringParser().removeTempStringVectorVars();

            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_STRING;

            // Handle the return value
            if (retVal == NumeRe::StringParser::STRING_SUCCESS)
            {
                // Only strings
                thisReturnVal.vStringVal.push_back(sLine);
                return thisReturnVal;
            }

            // Other: numerical values
            if (sCache.length() && _data.containsTablesOrClusters(sCache) && !bWriteToCache)
                bWriteToCache = true;

            // Ensure that the correct variables are available, because
            // the user might have used "to_value()" or something similar
            replaceLocalVars(sLine);
        }
    }

    // Get the target coordinates of the target cache,
    // if this is required
    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || nCurrentByteCode & ProcedureCommandLine::BYTECODE_DATAACCESS
        || nFlags & ProcedureCommandLine::FLAG_TEMPLATE)
    {
        if (bWriteToCache)
        {
            // Get the indices from the corresponding function
            getIndices(sCache, _idx, _parser, _data, _option);

            if (sCache[sCache.find_first_of("({")] == '{')
                bWriteToCluster = true;

            if (!isValidIndexSet(_idx))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "", _idx.row.to_string() + ", " + _idx.col.to_string());

            if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

            sCache.erase(sCache.find_first_of("({"));
            StripSpaces(sCache);
        }
    }

    // Set the expression and evaluate it
    if (!_parser.IsAlreadyParsed(sLine))
        _parser.SetExpr(sLine);

    v = _parser.Eval(nNum);
    _assertionHandler.checkAssertion(v, nNum);

    // Copy the return values
    if (nNum > 1)
    {
        for (int i = 0; i < nNum; ++i)
            thisReturnVal.vNumVal.push_back(v[i]);
    }
    else
    {
        thisReturnVal.vNumVal.push_back(v[0]);
    }

    vAns = v[0];
    NumeReKernel::getInstance()->getAns().clear();
    NumeReKernel::getInstance()->getAns().setDoubleArray(nNum, v);

    // Print the output to the console, if it isn't suppressed
    if (!bProcSupressAnswer)
        NumeReKernel::print(NumeReKernel::formatResultOutput(nNum, v));

    // Write the return values to cache
    if (bWriteToCache)
    {
        // Is it a cluster?
        if (bWriteToCluster)
        {
            NumeRe::Cluster& cluster = _data.getCluster(sCache);
            cluster.assignResults(_idx, nNum, v);
        }
        else
            _data.writeToTable(_idx, sCache, v, nNum);
    }

    // Clear the vector variables after the loop returned
    if (!_parser.ActiveLoopMode() || (!_parser.IsLockedPause() && !(nFlags & ProcedureCommandLine::FLAG_INLINE)))
        _parser.ClearVectorVars(true);

    return thisReturnVal;
}


/////////////////////////////////////////////////
/// \brief This member function is used to obtain
/// the procedure file name from the selected
/// procedure. It handles the "thisfile"
/// namespace directly.
///
/// \param sProc const string&
/// \param bInstallFileName bool
/// \return bool
///
/////////////////////////////////////////////////
bool Procedure::setProcName(const string& sProc, bool bInstallFileName)
{
    if (sProc.length())
    {
        string _sProc = sProc;

        // Handle the "thisfile" namespace by using the call stack
        // to obtain the corresponding file name
        if (sProcNames.length() && !bInstallFileName && _sProc.substr(0, 9) == "thisfile~")
        {
            sCurrentProcedureName = sProcNames.substr(sProcNames.rfind(';') + 1);
            sProcNames += ";" + sCurrentProcedureName;
            return true;
        }
        else if (sLastWrittenProcedureFile.length() && bInstallFileName && _sProc.substr(0, 9) == "thisfile~")
        {
            sCurrentProcedureName = sLastWrittenProcedureFile.substr(0, sLastWrittenProcedureFile.find('|'));
            return true;
        }
        else if (_sProc.substr(0, 9) == "thisfile~")
            return false;

        // Replace all tilde characters in the current path
        // string. Consider the special namespace "main", which
        // is a reference to the toplevel procedure folder
        for (size_t i = 0; i < _sProc.length(); i++)
        {
            if (_sProc[i] == '~')
            {
                if (_sProc.length() > 5 && i >= 4 && _sProc.substr(i - 4, 5) == "main~")
                    _sProc = _sProc.substr(0, i - 4) + _sProc.substr(i + 1);
                else
                    _sProc[i] = '/';
            }
        }

        // Create a valid file name from the procedure name
        sCurrentProcedureName = FileSystem::ValidFileName(_sProc, ".nprc");

        // Append the newly obtained procedure file name
        // to the call stack
        sProcNames += ";" + sCurrentProcedureName;
        return true;
    }
    else
        return false;
}


/////////////////////////////////////////////////
/// \brief This member function is central in the
/// execution of the currently selected procedure
/// as it handles all the logic.
///
/// \param sProc string
/// \param sVarList string
/// \param _parser Parser&
/// \param _functions Define&
/// \param _data Datafile&
/// \param _option Settings&
/// \param _out Output&
/// \param _pData PlotData&
/// \param _script Script&
/// \param nth_procedure unsigned int
/// \return Returnvalue
///
/////////////////////////////////////////////////
Returnvalue Procedure::execute(string sProc, string sVarList, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, unsigned int nth_procedure)
{
    // Measure the current stack size and ensure
    // that the current call won't exceed the
    // maximal stack size
    int stackMeasureVar;
    if ((stackMeasureVar = abs(&stackMeasureVar - NumeReKernel::baseStackPosition)) > MAX_PROCEDURE_STACK_SIZE)
        throw SyntaxError(SyntaxError::PROCEDURE_STACK_OVERFLOW, "$" + sProc + "(" + sVarList + ")", SyntaxError::invalid_position, "\\$" + sProc, nth_procedure);

    StripSpaces(sProc);
    NumeReKernel::getInstance()->getDebugger().pushStackItem(sProc + "(" + sVarList + ")", this);

    // Set the file name for the currently selected procedure
    if (!setProcName(sProc))
        throw SyntaxError(SyntaxError::INVALID_PROCEDURE_NAME, "$" + sProc + "(" + sVarList + ")", SyntaxError::invalid_position);

    ifstream fInclude;

    sProcCommandLine.clear();
    string sCmdCache = "";
    string sCurrentCommand = "";
    bool bReadingFromInclude = false;
    int nIncludeType = 0;
    int nByteCode = 0;
    int nCurrentByteCode = 0;
    Returnvalue _ReturnVal;

    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    sThisNameSpace = "";
    nCurrentLine = 0;
    nFlags = 0;
    nReturnType = 1;
    bReturnSignal = false;
    nthRecursion = nth_procedure;
    bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;

    // Prepare the procedure command line elements
    ProcedureElement* ProcElement;
    pair<int, ProcedureCommandLine> currentLine;
    currentLine.first = -1;

    // Prepare the var factory and obtain the current procedure file
    if (_varFactory)
        delete _varFactory;

    _varFactory = new ProcedureVarFactory(this, mangleName(sProc), nth_procedure);
    ProcElement = NumeReKernel::ProcLibrary.getProcedureContents(sCurrentProcedureName);

    // add spaces in front of and at the end of sVarList
    sVarList = " " + sVarList + " ";

    // Remove file name extension, if there's one in the procedure name
    if (sProc.length() > 5 && sProc.substr(sProc.length() - 5) == ".nprc")
        sProc = sProc.substr(0, sProc.rfind('.'));

    // Get the namespace of this procedure
    extractCurrentNamespace(sProc);

    // Separate the procedure name from the namespace
    if (sProc.find('~') != string::npos)
        sProc = sProc.substr(sProc.rfind('~') + 1);
    if (sProc.find('/') != string::npos)
        sProc = sProc.substr(sProc.rfind('/') + 1);
    if (sProc.find('\\') != string::npos)
        sProc = sProc.substr(sProc.rfind('\\') + 1);

    // find the current procedure line
    currentLine.first = ProcElement->gotoProcedure("$" + sProc);

    // if the procedure was not found, throw an error
    if (currentLine.first == -1)
    {
        sCallingNameSpace = "main";
        mVarMap.clear();

        if (_option.useDebugger())
            _debugger.popStackItem();

        throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, "", SyntaxError::invalid_position, sProc);
    }

    // Get the procedure head line
    currentLine = ProcElement->getCurrentLine(currentLine.first);

    // verify that this is a procedure headline
    if (currentLine.second.getType() != ProcedureCommandLine::TYPE_PROCEDURE_HEAD)
    {
        sCallingNameSpace = "main";
        mVarMap.clear();

        if (_option.useDebugger())
            _debugger.popStackItem();

        throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, "", SyntaxError::invalid_position, sProc);
    }

    // Get the flags
    nFlags = currentLine.second.getFlags();

    // verify that this was not a private procedure
    // or that the calling namespace is the same
    if (nFlags & ProcedureCommandLine::FLAG_PRIVATE && sThisNameSpace != sCallingNameSpace)
    {
        string sErrorToken;

        if (sCallingNameSpace == "main")
            sErrorToken = "\"" + sThisNameSpace + "\" aus dem globalen Namensraum";
        else
            sErrorToken = "\"" + sThisNameSpace + "\" aus dem Namensraum \"" + sCallingNameSpace + "\"";

        if (_option.useDebugger())
            _debugger.popStackItem();

        throw SyntaxError(SyntaxError::PRIVATE_PROCEDURE_CALLED, sProcCommandLine, SyntaxError::invalid_position, sErrorToken);
    }

    if (nFlags & ProcedureCommandLine::FLAG_MASK)
    {
        if (_option.systemPrints())
        {
            // if the print status is true, set it to false
            _option.enableSystemPrints(false);
            nFlags |= ProcedureCommandLine::FLAG_MASK;
        }
    }

    // Get the argument list and evaluate it
    string sVarDeclarationList = currentLine.second.getArgumentList();

    // Now the calling namespace is the current namespace
    sCallingNameSpace = sThisNameSpace;

    if (findCommand(sVarList, "var").sString == "var")
    {
        _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
        throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "var");
    }

    if (findCommand(sVarList, "str").sString == "str")
    {
        _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
        throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "str");
    }

    if (findCommand(sVarList, "tab").sString == "tab")
    {
        _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
        throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "tab");
    }

    if (findCommand(sVarList, "cst").sString == "cst")
    {
        _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
        throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "cst");
    }

    StripSpaces(sVarDeclarationList);
    StripSpaces(sVarList);

    try
    {
        // Evaluate the argument list for the current procedure
        mVarMap = _varFactory->createProcedureArguments(sVarDeclarationList, sVarList);
    }
    catch (...)
    {
        _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
        _debugger.showError(current_exception());

        resetProcedure(_parser, bSupressAnswer_back);
        throw;
    }

    _parser.mVarMapPntr = &mVarMap;

    if (nFlags & ProcedureCommandLine::FLAG_TEST)
    {
        sTestClusterName = _varFactory->createTestStatsCluster();
        baseline = _assertionHandler.getStats();
    }

    // As long as we didn't find the last line,
    // read the next line from the procedure and execute
    // this line
    while (!ProcElement->isLastLine(currentLine.first))
    {
        // Set the bytecode from the last calculation
        ProcElement->setByteCode(nCurrentByteCode | nByteCode, currentLine.first);
        bProcSupressAnswer = false;
        _assertionHandler.reset();
        updateTestStats();

        // Get the next line from one of the current active
        // command line sources
        if (!sCmdCache.length())
        {
            if (!bReadingFromInclude)
            {
                currentLine = ProcElement->getNextLine(currentLine.first);
                nCurrentLine = currentLine.first;
                sProcCommandLine = currentLine.second.getCommandLine();
                nCurrentByteCode = currentLine.second.getByteCode();
                nByteCode = nCurrentByteCode;

                // Obtain the current command from the command line
                sCurrentCommand = findCommand(sProcCommandLine).sString;

                if (_option.useDebugger() && _debugger.getBreakpointManager().isBreakpoint(sCurrentProcedureName, nCurrentLine) && sProcCommandLine.substr(0, 2) != "|>")
                {
                    sProcCommandLine.insert(0, "|> ");
                }
            }
            else
            {
                // Get the next command line from the included script
                try
                {
                    readFromInclude(fInclude, nIncludeType, _parser, _functions, _data, _out, _pData, _script, _option, nth_procedure);
                }
                catch (...)
                {
                    _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
                    _debugger.showError(current_exception());

                    resetProcedure(_parser, bSupressAnswer_back);
                    throw;
                }

                if (!fInclude.is_open())
                {
                    bReadingFromInclude = false;
                    nIncludeType = 0;
                }
            }

            // Stop the evaluation if the current procedure,
            // if we reach the endprocedure command
            if (currentLine.second.getType() == ProcedureCommandLine::TYPE_PROCEDURE_FOOT)
                break;

            // Remove the trailing output suppressing semicolon
            while (sProcCommandLine.back() == ';')
            {
                bProcSupressAnswer = true;
                sProcCommandLine.pop_back();
                StripSpaces(sProcCommandLine);
            }

            sProcCommandLine = " " + sProcCommandLine + " ";

            // if the current line doesn't contain a namespace command
            // resolve the local variables
            if (sCurrentCommand != "namespace" && sProcCommandLine[1] != '@')
            {
                sProcCommandLine = _varFactory->resolveVariables(sProcCommandLine);
            }
        }

        // Handle the command line cache
        if (sCmdCache.length() || sProcCommandLine.find(';') != string::npos)
        {
            if (sCmdCache.length())
            {
                while (sCmdCache.front() == ';' || sCmdCache.front() == ' ')
                    sCmdCache.erase(0, 1);

                if (!sCmdCache.length())
                    continue;

                if (sCmdCache.find(';') != string::npos)
                {
                    for (unsigned int i = 0; i < sCmdCache.length(); i++)
                    {
                        if (sCmdCache[i] == ';' && !isInQuotes(sCmdCache, i))
                        {
                            bProcSupressAnswer = true;
                            sProcCommandLine = sCmdCache.substr(0, i);
                            sCmdCache.erase(0, i + 1);
                            break;
                        }

                        if (i == sCmdCache.length() - 1)
                        {
                            sProcCommandLine = sCmdCache;
                            sCmdCache.clear();
                            break;
                        }
                    }
                }
                else
                {
                    sProcCommandLine = sCmdCache;
                    sCmdCache.clear();
                }
            }
            else if (sProcCommandLine.back() == ';')
            {
                bProcSupressAnswer = true;
                sProcCommandLine.pop_back();
            }
            else
            {
                for (unsigned int i = 0; i < sProcCommandLine.length(); i++)
                {
                    if (sProcCommandLine[i] == '(' || sProcCommandLine[i] == '[' || sProcCommandLine[i] == '{')
                    {
                        size_t parens = getMatchingParenthesis(sProcCommandLine.substr(i));

                        if (parens != string::npos)
                            i += parens;
                    }

                    if (sProcCommandLine[i] == ';' && !isInQuotes(sProcCommandLine, i))
                    {
                        if (i != sProcCommandLine.length() - 1)
                            sCmdCache = sProcCommandLine.substr(i + 1);

                        sProcCommandLine.erase(i);
                        bProcSupressAnswer = true;
                    }

                    if (i == sProcCommandLine.length() - 1)
                    {
                        break;
                    }
                }
            }
        }

        // Handle the defining process and the calling
        // of local functions
        if (sCurrentCommand == "lclfunc")
        {
            // This is a definition
            _localDef.defineFunc(sProcCommandLine.substr(sProcCommandLine.find("lclfunc")+7));
            sProcCommandLine.clear();
            continue;
        }
        else
        {
            // This is probably a call to a local function
            _localDef.call(sProcCommandLine);
        }

        // define the current command to be a flow control statement,
        // if the procedure was not parsed already
        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            && (FlowCtrl::isFlowCtrlStatement(sCurrentCommand) || getCurrentBlockDepth()))
        {
            nCurrentByteCode = ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT;
            nByteCode |= ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT;
        }

        // Handle breakpoints
        if (_option.useDebugger()
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT))
        {
            if ((sProcCommandLine.substr(sProcCommandLine.find_first_not_of(' '), 2) == "|>" || nDebuggerCode == NumeReKernel::DEBUGGER_STEP)
                && !getCurrentBlockDepth())
            {
                if (sProcCommandLine.substr(sProcCommandLine.find_first_not_of(' '), 2) == "|>")
                {
                    sProcCommandLine.erase(sProcCommandLine.find_first_not_of(' '), 2);
                    StripSpaces(sProcCommandLine);
                }

                if (nDebuggerCode != NumeReKernel::DEBUGGER_LEAVE)
                {
                    _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
                    nDebuggerCode = evalDebuggerBreakPoint(_parser, _option);
                }

                if (!sProcCommandLine.length())
                    continue;

                sProcCommandLine.insert(0, 1, ' ');
            }
        }

        // Handle the definition of local variables
        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || nCurrentByteCode & ProcedureCommandLine::BYTECODE_VARDEF)
        {
            try
            {
                if (handleVariableDefinitions(sProcCommandLine, sCurrentCommand))
                {
                    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                        nByteCode |= ProcedureCommandLine::BYTECODE_VARDEF;
                }
            }
            catch (...)
            {
                _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
                _debugger.showError(current_exception());

                resetProcedure(_parser, bSupressAnswer_back);
                throw;
            }

            if (!sProcCommandLine.length())
                continue;
        }
        // Handle the definition of namespaces
        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_NAMESPACE
                && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
        {
            if (sCurrentCommand == "namespace" && !getCurrentBlockDepth())
            {
                sNameSpace = decodeNameSpace(sProcCommandLine, sThisNameSpace);

                if (sNameSpace.length())
                    sNameSpace += "~";

                if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_NAMESPACE;

                sProcCommandLine = "";
                continue;
            }
        }

        // Handle include syntax
        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || nCurrentByteCode & ProcedureCommandLine::BYTECODE_INCLUDE)
        {
            try
            {
                nIncludeType = handleIncludeSyntax(sProcCommandLine, fInclude, bReadingFromInclude);

                if (fInclude.is_open())
                    bReadingFromInclude = true;
                else
                    bReadingFromInclude = false;

                if (bReadingFromInclude && nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_INCLUDE;

                if (!sProcCommandLine.length())
                    continue;
            }
            catch (...)
            {
                _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
                _debugger.showError(current_exception());

                resetProcedure(_parser, bSupressAnswer_back);
                throw;
            }
        }

        // Ensure that inline procedures don't contain flow control statements
        if (nFlags & ProcedureCommandLine::FLAG_INLINE)
        {
            if (sCurrentCommand == "for"
                || sCurrentCommand == "if"
                || sCurrentCommand == "switch"
                || sCurrentCommand == "while")
            {
                _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());

                try
                {
                    _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sProcCommandLine, SyntaxError::invalid_position));
                }
                catch (...)
                {
                    resetProcedure(_parser, bSupressAnswer_back);
                    throw;
                }
            }
        }

        // Only try to evaluate a procedure, if there's currently no active flow
        // control statement
        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_PROCEDUREINTERFACE
                && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
        {
            // Handle procedure calls and plugins in the common
            // virtual procedure interface function
            try
            {
                FlowCtrl::ProcedureInterfaceRetVal nRetVal = procedureInterface(sProcCommandLine, _parser, _functions, _data, _out, _pData, _script, _option, 0);

                // Only those two return values indicate that this line
                // does contain a procedure or a plugin
                if ((nRetVal == FlowCtrl::INTERFACE_EMPTY || nRetVal == FlowCtrl::INTERFACE_VALUE)
                    && nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                {
                    nByteCode |= ProcedureCommandLine::BYTECODE_PROCEDUREINTERFACE;
                }

                if (nRetVal == FlowCtrl::INTERFACE_ERROR || nRetVal == FlowCtrl::INTERFACE_EMPTY)
                    continue;
            }
            catch (...)
            {
                _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
                _debugger.showError(current_exception());

                nCurrentByteCode = 0;
                catchExceptionForTest(current_exception(), bSupressAnswer_back, GetCurrentLine());

                // If the error is converted, we have to skip
                // the remaining code, otherwise the procedure
                // is called again in ProcCalc(). If it's not
                // converted, this line won't be reached.
                continue;
            }
        }

        // Handle special commands
        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || nCurrentByteCode & ProcedureCommandLine::BYTECODE_EXPLICIT)
        {
            if (sCurrentCommand == "explicit")
            {
                sProcCommandLine.erase(findCommand(sProcCommandLine).nPos, 8);
                StripSpaces(sProcCommandLine);

                if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_EXPLICIT;
            }
        }

        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_THROWCOMMAND
                && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
        {
            if (sCurrentCommand == "throw")
            {
                string sErrorToken;

                if (sProcCommandLine.length() > 7 && NumeReKernel::getInstance()->getStringParser().isStringExpression(sProcCommandLine))
                {
                    if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sProcCommandLine))
                        NumeReKernel::getInstance()->getStringParser().getStringValues(sProcCommandLine);

                    getStringArgument(sProcCommandLine, sErrorToken);
                    sErrorToken += " -nq";
                    string sDummy = "";
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sErrorToken, sDummy, true);
                }

                if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_THROWCOMMAND;

                _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());

                try
                {
                    _debugger.throwException(SyntaxError(SyntaxError::PROCEDURE_THROW, sProcCommandLine, SyntaxError::invalid_position, sErrorToken));
                }
                catch (...)
                {
                    resetProcedure(_parser, bSupressAnswer_back);
                    throw;
                }
            }
        }

        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_RETURNCOMMAND
                && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
        {
            if (sCurrentCommand == "return")
            {
                try
                {
                    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                        nByteCode |= ProcedureCommandLine::BYTECODE_RETURNCOMMAND;

                    if (sProcCommandLine.find("void", sProcCommandLine.find("return") + 1) != string::npos)
                    {
                        string sReturnValue = sProcCommandLine.substr(sProcCommandLine.find("return") + 6);
                        StripSpaces(sReturnValue);

                        if (sReturnValue == "void")
                        {
                            nReturnType = 0;
                        }
                        else
                        {
                            sReturnValue += " ";
                            _ReturnVal = ProcCalc(sReturnValue, sCurrentCommand, nCurrentByteCode, _parser, _functions, _data, _option, _out, _pData, _script);
                        }
                    }
                    else if (sProcCommandLine.length() > 6)
                        _ReturnVal = ProcCalc(sProcCommandLine.substr(sProcCommandLine.find("return") + 6), sCurrentCommand, nCurrentByteCode, _parser, _functions, _data, _option, _out, _pData, _script);

                    break;
                }
                catch (...)
                {
                    _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
                    _debugger.showError(current_exception());

                    nCurrentByteCode = 0;
                    catchExceptionForTest(current_exception(), bSupressAnswer_back, GetCurrentLine());
                }
            }
        }

        // Evaluate the command line using the ProcCalc
        // member function
        try
        {
            ProcCalc(sProcCommandLine, sCurrentCommand, nCurrentByteCode, _parser, _functions, _data, _option, _out, _pData, _script);

            if (getReturnSignal())
            {
                _ReturnVal = getReturnValue();
                break;
            }
        }
        catch (...)
        {
            _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());
            _debugger.showError(current_exception());

            nCurrentByteCode = 0;
            catchExceptionForTest(current_exception(), bSupressAnswer_back, GetCurrentLine());
        }

        sProcCommandLine.clear();
    }



    // Ensure that all loops are closed now
    if (getCurrentBlockDepth())
    {
        _debugger.gatherInformations(_varFactory, sProcCommandLine, sCurrentProcedureName, GetCurrentLine());

        try
        {
            _debugger.throwException(SyntaxError(SyntaxError::IF_OR_LOOP_SEEMS_NOT_TO_BE_CLOSED, "endprocedure", SyntaxError::invalid_position, "\\$" + sProc + "()"));
        }
        catch (...)
        {
            resetProcedure(_parser, bSupressAnswer_back);
            throw;
        }
    }

    if (nFlags & ProcedureCommandLine::FLAG_MASK)
    {
        // reset the print status
        _option.enableSystemPrints();
    }

    // Reset this procedure
    resetProcedure(_parser, bSupressAnswer_back);

    // Determine the return value
    if (nReturnType && !_ReturnVal.vNumVal.size() && !_ReturnVal.vStringVal.size())
        _ReturnVal.vNumVal.push_back(1.0);

    return _ReturnVal;
}


/////////////////////////////////////////////////
/// \brief This member function handles the calls
/// for procedures and plugins, resolves them and
/// executes the called procedures by constructing
/// a new instance of this class on the heap.
///
/// \param sLine string&
/// \param _parser Parser&
/// \param _functions Define&
/// \param _data Datafile&
/// \param _out Output&
/// \param _pData PlotData&
/// \param _script Script&
/// \param _option Settings&
/// \param nth_command int
/// \return FlowCtrl::ProcedureInterfaceRetVal
///
/////////////////////////////////////////////////
FlowCtrl::ProcedureInterfaceRetVal Procedure::procedureInterface(string& sLine, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, int nth_command)
{
    // Create a new procedure object on the heap
    std::unique_ptr<Procedure> _procedure(new Procedure(*this));
    FlowCtrl::ProcedureInterfaceRetVal nReturn = FlowCtrl::INTERFACE_NONE;

    // Handle procedure calls first
    if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos)
    {
        // Ensure that the current procedure is no inline procedure
        if (nFlags & ProcedureCommandLine::FLAG_INLINE)
            throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sLine, SyntaxError::invalid_position);

        sLine += " ";
        unsigned int nPos = 0;
        int nProc = 0;

        // Handle all procedure calls one after the other
        while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
        {
            nPos = sLine.find('$', nPos) + 1;
            string __sName = sLine.substr(nPos, sLine.find('(', nPos) - nPos);
            string __sVarList = "";

            if (!isInQuotes(sLine, nPos, true))
            {
                unsigned int nParPos = 0;

                // Add namespaces, where necessary
                if (__sName.find('~') == string::npos)
                    __sName = sNameSpace + __sName;

                if (__sName.substr(0, 5) == "this~")
                    __sName.replace(0, 4, sThisNameSpace);


                // Handle explicit procedure file names
                if (sLine[nPos] == '\'')
                {
                    __sName = sLine.substr(nPos + 1, sLine.find('\'', nPos + 1) - nPos - 1);
                    nParPos = sLine.find('(', nPos + 1 + __sName.length());
                }
                else
                    nParPos = sLine.find('(', nPos);

                // Get the variable list
                __sVarList = sLine.substr(nParPos);

                // Ensure that each parenthesis has its counterpart
                if (getMatchingParenthesis(sLine.substr(nParPos)) == string::npos)
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nParPos);

                nParPos += getMatchingParenthesis(sLine.substr(nParPos));
                __sVarList = __sVarList.substr(1, getMatchingParenthesis(__sVarList) - 1);
                unsigned int nVarPos = 0;

                // Try to find other procedure calls in the argument
                // list and prepend the current namespace
                while (__sVarList.find('$', nVarPos) != string::npos)
                {
                    // Find the position of the next procedure candidate
                    // and increment 1
                    nVarPos = __sVarList.find('$', nVarPos) + 1;

                    // If this candidate is not part of a string literal,
                    // prepend the current namespace
                    if (!isInQuotes(__sVarList, nVarPos-1) && __sVarList.substr(nVarPos, __sVarList.find('(', nVarPos) - nVarPos).find('~') == string::npos)
                        __sVarList = __sVarList.substr(0, nVarPos) + sNameSpace + __sVarList.substr(nVarPos);
                }

                // Call the current procedure
                Returnvalue tempreturnval = _procedure->execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion + 1);

                // Evaluate the return value of the called procedure
                if (!_procedure->nReturnType)
                    sLine = sLine.substr(0, nPos - 1) + sLine.substr(nParPos + 1);
                else
                {
                    nPos += replaceReturnVal(sLine, _parser, tempreturnval, nPos - 1, nParPos + 1, "_~PROC~[" + mangleName(__sName) + "~" + toString(nProc) + "_" + toString((int)nthRecursion) + "_" + toString((int)(nth_command + nthRecursion)) + "]");
                    nProc++;
                }

                nReturnType = 1;
            }
            else
                nPos += __sName.length() + 1;
        }

        nReturn = FlowCtrl::INTERFACE_VALUE;
        updateTestStats();
        StripSpaces(sLine);
        _parser.mVarMapPntr = &mVarMap;

        if (nFlags & ProcedureCommandLine::FLAG_MASK)
            _option.enableSystemPrints(false);

        if (!sLine.length())
            return FlowCtrl::INTERFACE_EMPTY;
    }
    else if (sLine.find('$') != string::npos)
    {
        int nQuotes = 0;

        // Ensure that this is no "wrong" procedure call
        for (size_t i = 0; i < sLine.length(); i++)
        {
            if (sLine[i] == '"' && (!i || sLine[i - 1] != '\\'))
                nQuotes++;

            if (sLine[i] == '$' && !(nQuotes % 2))
            {
                sLine.clear();
                return FlowCtrl::INTERFACE_ERROR;
            }
        }

        return nReturn;
    }

    // Handle plugin calls
    if (!(nFlags & ProcedureCommandLine::FLAG_EXPLICIT) && _procedure->isPluginCmd(sLine))
    {
        if (_procedure->evalPluginCmd(sLine))
        {
            Returnvalue _return;

            // Call the plugin routines
            if (!_option.systemPrints())
            {
                _return = _procedure->execute(_procedure->getPluginProcName(), _procedure->getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion + 1);

                if (nFlags & ProcedureCommandLine::FLAG_MASK)
                    _option.enableSystemPrints(false);
            }
            else
            {
                _option.enableSystemPrints(false);
                _return = _procedure->execute(_procedure->getPluginProcName(), _procedure->getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion + 1);
                _option.enableSystemPrints(true);
            }

            _parser.mVarMapPntr = &mVarMap;
            updateTestStats();

            // Handle the plugin return values
            if (sLine.length())
            {
                if (sLine.find("<<RETURNVAL>>") != string::npos)
                {
                    if (_return.vStringVal.size())
                    {
                        string sReturn = "{";

                        for (unsigned int v = 0; v < _return.vStringVal.size(); v++)
                            sReturn += _return.vStringVal[v] + ",";

                        sReturn.back() = '}';
                        sLine.replace(sLine.find("<<RETURNVAL>>"), 13, sReturn);
                    }
                    else
                    {
                        _parser.SetVectorVar("_~PLUGIN[" + _procedure->getPluginProcName() + "~" + toString((int)nthRecursion) + "]", _return.vNumVal);
                        sLine.replace(sLine.find("<<RETURNVAL>>"), 13, "_~PLUGIN[" + _procedure->getPluginProcName() + "~" + toString((int)(nth_command + nthRecursion)) + "]");
                    }
                }
            }
            else
                return FlowCtrl::INTERFACE_EMPTY;
        }
        else
            return FlowCtrl::INTERFACE_ERROR;
    }

    return nReturn;
}


/////////////////////////////////////////////////
/// \brief Virtual member function allowing to
/// identify and evaluate some special procedure
/// commands. Currently it is only used for the
/// namespace command.
///
/// \param sLine string&
/// \return int
///
/// The commands "var", "str", "tab" and "cst"
/// are recognized but not evaluated.
/////////////////////////////////////////////////
int Procedure::procedureCmdInterface(string& sLine)
{
    // Find the current command
    string sCommand = findCommand(sLine).sString;

    // Try to identify the command
    if (sCommand == "var" || sCommand == "str" || sCommand == "tab" || sCommand == "cst")
    {
        // Only recognized
        return 1;
    }
    else if (sCommand == "namespace")
    {
        sLine = sLine.substr(sLine.find("namespace") + 9);
        StripSpaces(sLine);

        // Evaluate the namespace name
        if (sLine.length())
        {
            if (sLine.find(' ') != string::npos)
                sLine = sLine.substr(0, sLine.find(' '));

            if (sLine.substr(0, 5) == "this~" || sLine == "this")
                sLine.replace(0, 4, sThisNameSpace);

            if (sLine != "main")
            {
                sNameSpace = sLine;
                if (sNameSpace[sNameSpace.length() - 1] != '~')
                    sNameSpace += "~";
            }
            else
                sNameSpace = "";
        }
        else
            sNameSpace = "";

        return 1;
    }
    return -1;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// procedure installation process by governing
/// the file stream and passing the corresponding
/// procedure linewards to the stream.
///
/// \param sProcedureLine string
/// \return bool
///
/////////////////////////////////////////////////
bool Procedure::writeProcedure(string sProcedureLine)
{
    string sAppendedLine = "";

    // Check, whether the current line is a procedure head,
    // a procedure foot or the actual body of the procedure
    if (sProcedureLine.substr(0, 9) == "procedure"
            && sProcedureLine.find('$') != string::npos
            && sProcedureLine.find('(', sProcedureLine.find('$')) != string::npos)
    {
        // This is the handling code for the procedure head.
        // It will determine the correct file path from the
        // procedure name and its namespace
        bool bAppend = false;

        nthBlock = 0;

        // Get the procedure name and its namespace
        string sFileName = sProcedureLine.substr(sProcedureLine.find('$') + 1, sProcedureLine.find('(', sProcedureLine.find('$')) - sProcedureLine.find('$') - 1);
        StripSpaces(sFileName);

        // Try to evaluate it using the setProcName
        // member function
        if (!setProcName(sFileName))
            return false;

        // If the procedure belongs to the "thisfile"
        // namespace, then it has to be appended to the
        // procedure, which was opened lastly
        if (sFileName.substr(0, 9) == "thisfile~")
            bAppend = true;

        // Create a corresponding folder from the
        // namespace
        if (sCurrentProcedureName.find_last_of("~/") != string::npos)
        {
            FileSystem _fSys;
            _fSys.setPath(sCurrentProcedureName.substr(0, sCurrentProcedureName.find_last_of("~/")), true, sTokens[5][1]);
        }

        // If the procedure shall be appended, open the
        // filestream in read mode, read everything and
        // truncate the file afterwards, otherwise truncate
        // the whole file in advance
        if (bAppend)
        {
            fProcedure.open(sCurrentProcedureName.c_str(), ios_base::in);

            if (!fProcedure.good())
            {
                fProcedure.close();
                return false;
            }

            string sLineTemp;
            vector<string> vProcedureFile;

            // Read the whole file
            while (!fProcedure.eof())
            {
                getline(fProcedure, sLineTemp);
                vProcedureFile.push_back(sLineTemp);
            }

            fProcedure.close();

            // find the last "endprocedure" command and
            // erase everything after it
            for (int i = vProcedureFile.size()-1; i >= 0; i--)
            {
                if (vProcedureFile[i] == "endprocedure")
                {
                    vProcedureFile.erase(vProcedureFile.begin()+i+1, vProcedureFile.end());
                    break;
                }
            }

            // Open the file in out mode and truncate it
            fProcedure.open(sCurrentProcedureName.c_str(), ios_base::out | ios_base::trunc);

            // Write the stored contents to the file
            for (size_t i = 0; i < vProcedureFile.size(); i++)
            {
                fProcedure << vProcedureFile[i] << endl;
            }

            // Append two line breaks to separate the procedures
            fProcedure << endl << endl;
        }
        else
        {
            fProcedure.open(sCurrentProcedureName.c_str(), ios_base::out | ios_base::trunc);

            // Ensure that the file stream could be opened
            if (fProcedure.fail())
            {
                fProcedure.close();
                return false;
            }

            string sProcName = "";

            if (sFileName.find('~') != string::npos)
                sProcName = sFileName.substr(sFileName.rfind('~') + 1);
            else
                sProcName = sFileName;

            // Write the procedure head comment
            unsigned int nLength = _lang.get("PROC_FOOTER").length();
            fProcedure << "#**" << std::setfill('*') << std::setw(nLength) << '*' << endl;
            fProcedure << " * NUMERE-" << toUpperCase(_lang.get("COMMON_PROCEDURE")) << ": $" << sProcName << "()" << endl;
            fProcedure << " * " << std::setfill('=') << std::setw(nLength) << '=' << endl;
            fProcedure << " * " << _lang.get("PROC_ADDED_DATE") << ": " << getTimeStamp(false) << " *#" << endl;
            fProcedure << endl;
        }

        sLastWrittenProcedureFile = sCurrentProcedureName;
        bWritingTofile = true;

        // Print prefixed documentation strings, which were
        // appended, first
        if (sProcedureLine.find("##!") != string::npos)
        {
            // Line comments
            fProcedure << sProcedureLine.substr(sProcedureLine.find("##!"));
            sProcedureLine.erase(sProcedureLine.find("##!"));
        }
        else if (sProcedureLine.find("#*!") != string::npos)
        {
            // block comments
            fProcedure << sProcedureLine.substr(sProcedureLine.find("#*!"));
            sProcedureLine.erase(sProcedureLine.find("#*!"));
        }

        fProcedure << "procedure $";

        // Write the procedure name (without the namespace)
        if (sFileName.find('~') != string::npos)
            fProcedure << sFileName.substr(sFileName.rfind('~') + 1);
        else
            fProcedure << sFileName;

        // Write the argument list
        fProcedure << sProcedureLine.substr(sProcedureLine.find('(')) << endl;
        return true;
    }
    else if (sProcedureLine.substr(0, 12) == "endprocedure")
        bWritingTofile = false;
    else
    {
        // This is the handling code for the procedure body
        // The first cases try to split multiple flow
        // control statements, which are passed as a single
        // line into multiple lines. This is done by pushing the
        // remaining part of the current line into a string cache.
        if (sProcedureLine.find('(') != string::npos
                && (sProcedureLine.substr(0, 3) == "for"
                    || sProcedureLine.substr(0, 3) == "if "
                    || sProcedureLine.substr(0, 3) == "if("
                    || sProcedureLine.substr(0, 6) == "elseif"
                    || sProcedureLine.substr(0, 6) == "switch"
                    || sProcedureLine.substr(0, 5) == "while"))
        {
            sAppendedLine = sProcedureLine.substr(getMatchingParenthesis(sProcedureLine) + 1);
            sProcedureLine.erase(getMatchingParenthesis(sProcedureLine) + 1);
        }
        else if (sProcedureLine.find(':', 5) != string::npos
                 && (sProcedureLine.substr(0, 5) == "case "
                     || sProcedureLine.substr(0, 8) == "default "
                     || sProcedureLine.substr(0, 8) == "default:")
                 && sProcedureLine.find_first_not_of(' ', sProcedureLine.find(':', 5)) != string::npos)
        {
            sAppendedLine = sProcedureLine.substr(sProcedureLine.find(':', 5)+1);
            sProcedureLine.erase(sProcedureLine.find(':', 5)+1);
        }
        else if (sProcedureLine.find(' ', 4) != string::npos
                 && (sProcedureLine.substr(0, 5) == "else "
                     || sProcedureLine.substr(0, 6) == "endif "
                     || sProcedureLine.substr(0, 10) == "endswitch "
                     || sProcedureLine.substr(0, 7) == "endfor "
                     || sProcedureLine.substr(0, 9) == "endwhile ")
                 && sProcedureLine.find_first_not_of(' ', sProcedureLine.find(' ', 4)) != string::npos
                 && sProcedureLine[sProcedureLine.find_first_not_of(' ', sProcedureLine.find(' ', 4))] != '-')
        {
            sAppendedLine = sProcedureLine.substr(sProcedureLine.find(' ', 4));
            sProcedureLine.erase(sProcedureLine.find(' ', 4));
        }
        else if (sProcedureLine.find(" for ") != string::npos
                 || sProcedureLine.find(" for(") != string::npos
                 || sProcedureLine.find(" endfor") != string::npos
                 || sProcedureLine.find(" if ") != string::npos
                 || sProcedureLine.find(" if(") != string::npos
                 || sProcedureLine.find(" else") != string::npos
                 || sProcedureLine.find(" elseif ") != string::npos
                 || sProcedureLine.find(" elseif(") != string::npos
                 || sProcedureLine.find(" endif") != string::npos
                 || sProcedureLine.find(" switch ") != string::npos
                 || sProcedureLine.find(" switch(") != string::npos
                 || sProcedureLine.find(" case") != string::npos
                 || sProcedureLine.find(" default") != string::npos
                 || sProcedureLine.find(" endswitch") != string::npos
                 || sProcedureLine.find(" while ") != string::npos
                 || sProcedureLine.find(" while(") != string::npos
                 || sProcedureLine.find(" endwhile") != string::npos)
        {
            for (unsigned int n = 0; n < sProcedureLine.length(); n++)
            {
                if (sProcedureLine[n] == ' ' && !isInQuotes(sProcedureLine, n))
                {
                    if (sProcedureLine.substr(n, 5) == " for "
                            || sProcedureLine.substr(n, 5) == " for("
                            || sProcedureLine.substr(n, 7) == " endfor"
                            || sProcedureLine.substr(n, 4) == " if "
                            || sProcedureLine.substr(n, 4) == " if("
                            || sProcedureLine.substr(n, 5) == " else"
                            || sProcedureLine.substr(n, 8) == " elseif "
                            || sProcedureLine.substr(n, 8) == " elseif("
                            || sProcedureLine.substr(n, 6) == " endif"
                            || sProcedureLine.substr(n, 8) == " switch "
                            || sProcedureLine.substr(n, 8) == " switch("
                            || sProcedureLine.substr(n, 6) == " case "
                            || sProcedureLine.substr(n, 9) == " default "
                            || sProcedureLine.substr(n, 9) == " default:"
                            || sProcedureLine.substr(n, 10) == " endswitch"
                            || sProcedureLine.substr(n, 7) == " while "
                            || sProcedureLine.substr(n, 7) == " while("
                            || sProcedureLine.substr(n, 9) == " endwhile")
                    {
                        sAppendedLine = sProcedureLine.substr(n + 1);
                        sProcedureLine.erase(n);
                        break;
                    }
                }
            }
        }

        // Decrement the block count for every
        // endBLOCK command
        if (findCommand(sProcedureLine).sString == "endif"
                || findCommand(sProcedureLine).sString == "endwhile"
                || findCommand(sProcedureLine).sString == "endfor"
                || findCommand(sProcedureLine).sString == "endcompose"
                || findCommand(sProcedureLine).sString == "endswitch"
                || findCommand(sProcedureLine).sString == "case"
                || findCommand(sProcedureLine).sString == "default"
                || findCommand(sProcedureLine).sString == "elseif"
                || findCommand(sProcedureLine).sString == "else")
            nthBlock--;

        string sTabs = "\t";

        for (int i = 0; i < nthBlock; i++)
            sTabs += '\t';

        // Create the procedure line
        sProcedureLine = sTabs + sProcedureLine;

        // Increment the block count for every
        // BLOCK command
        if (findCommand(sProcedureLine).sString == "if"
                || findCommand(sProcedureLine).sString == "while"
                || findCommand(sProcedureLine).sString == "for"
                || findCommand(sProcedureLine).sString == "compose"
                || findCommand(sProcedureLine).sString == "switch"
                || findCommand(sProcedureLine).sString == "case"
                || findCommand(sProcedureLine).sString == "default"
                || findCommand(sProcedureLine).sString == "elseif"
                || findCommand(sProcedureLine).sString == "else")
            nthBlock++;
    }

    // Write the actual line to the file
    if (fProcedure.is_open())
        fProcedure << sProcedureLine << endl;

    // If this was the last line, write the final comment lines
    // to the procedure file and close it afterwards
    if (!bWritingTofile && fProcedure.is_open())
    {
        fProcedure << endl;
        fProcedure << "#**" << _lang.get("PROC_END_OF_PROCEDURE") << endl;
        fProcedure << " * " << _lang.get("PROC_FOOTER") << endl;
        fProcedure << " * https://www.numere.org/" << endl;
        fProcedure << " **" << std::setfill('*') << std::setw(_lang.get("PROC_FOOTER").length() + 1) << "#" << endl;

        fProcedure.close();

        // This ensures that all blocks were closed
        if (nthBlock)
            throw SyntaxError(SyntaxError::IF_OR_LOOP_SEEMS_NOT_TO_BE_CLOSED, sProcedureLine, SyntaxError::invalid_position, sCurrentProcedureName);

         sCurrentProcedureName = "";
    }

    StripSpaces(sAppendedLine);

    // If there are currently contents cached,
    // call this member function recursively.
    if (sAppendedLine.length())
        return writeProcedure(sAppendedLine);

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function extracts
/// procedure name, argument list and the
/// corresponding file name from the passed
/// position in the command line.
///
/// \param sCmdLine const string&
/// \param nPos size_t
/// \param sProcName string&
/// \param sArgList string&
/// \param sFileName string&
/// \return void
///
/////////////////////////////////////////////////
void Procedure::extractProcedureInformation(const string& sCmdLine, size_t nPos, string& sProcName, string& sArgList, string& sFileName)
{
    string __sName = sCmdLine.substr(nPos, sCmdLine.find('(', nPos) - nPos);

    // Get the argument list
    sArgList = sCmdLine.substr(sCmdLine.find('(', nPos));
    sArgList.erase(getMatchingParenthesis(sArgList));
    sArgList.erase(0, 1);

    if (__sName.find('~') == string::npos)
        __sName = sNameSpace + __sName;

    if (__sName.substr(0, 5) == "this~")
        __sName.replace(0, 4, sThisNameSpace);

    // Handle the special case of absolute paths
    if (sCmdLine[nPos] == '\'')
    {
        __sName = sCmdLine.substr(nPos + 1, sCmdLine.find('\'', nPos + 1) - nPos - 1);
    }

    // Create a valid filename first
    sFileName = __sName;

    // Now remove the namespace stuff
    if (__sName.find('~') != string::npos)
        sProcName = __sName.substr(__sName.rfind('~')+1);
    else
        sProcName = __sName;

    // Handle namespaces
    if (sFileName.find('~') != string::npos)
    {
        if (sFileName.substr(0, 9) == "thisfile~")
        {
            if (sProcNames.length())
                sFileName = sProcNames.substr(sProcNames.rfind(';') + 1);
            else
            {
                throw SyntaxError(SyntaxError::PRIVATE_PROCEDURE_CALLED, sCmdLine, SyntaxError::invalid_position, "thisfile");
            }
        }
        else
        {
            for (unsigned int i = 0; i < sFileName.length(); i++)
            {
                if (sFileName[i] == '~')
                {
                    if (sFileName.length() > 5 && i >= 4 && sFileName.substr(i - 4, 5) == "main~")
                        sFileName = sFileName.substr(0, i - 4) + sFileName.substr(i + 1);
                    else
                        sFileName[i] = '/';
                }
            }
        }
    }

    // Use the filesystem to determine a valid file name
    sFileName = ValidFileName(sFileName, ".nprc");

    if (sFileName[1] != ':')
    {
        sFileName = "<procpath>/" + sFileName;
        sFileName = ValidFileName(sFileName, ".nprc");
    }
}


/////////////////////////////////////////////////
/// \brief This virtual member function checks,
/// whether the procedures in the current line
/// are declared as inline and whether they are
/// inlinable.
///
/// \param sProc const string&
/// \return int one of ProcedureCommendLine::Inlineable values
///
/////////////////////////////////////////////////
int Procedure::isInline(const string& sProc)
{
    // No procedures?
    if (sProc.find('$') == string::npos)
        return ProcedureCommandLine::INLINING_IMPOSSIBLE;

    size_t nProcedures = 0;
    int nInlineable = ProcedureCommandLine::INLINING_POSSIBLE;

    if (sProc.find('$') != string::npos && sProc.find('(', sProc.find('$')) != string::npos)
    {
        size_t nPos = 0;

        // Examine all procedures, which may be found in the
        // current command string
        while (sProc.find('$', nPos) != string::npos && sProc.find('(', sProc.find('$', nPos)) != string::npos)
        {
            // Extract the name of the procedure
            nPos = sProc.find('$', nPos) + 1;

            // Only evaluate the current match, if it is not part of a string
            if (!isInQuotes(sProc, nPos, true))
            {
                string __sFileName;
                string __sProcName;
                string __sArgList;

                // Obtain procedure name, argument list and the corresponding
                // file name of the procedure
                extractProcedureInformation(sProc, nPos, __sProcName, __sArgList, __sFileName);

                int nInlineFlag = 0;

                // Here happens the hard work: get a procedure element from the library, find
                // the procedure definition line, obtain it and examine the already parsed
                // flags of this procedure. Additionally, determine, whether the current procedure
                // is inlinable.
                nInlineable = max(isInlineable(__sProcName, __sFileName, &nInlineFlag), nInlineable);
                nProcedures++;

                // If the current procedure is not flagged as inline, return the corresponding
                // value - we do not need to inspect the current command line further
                if (!nInlineFlag)
                    return ProcedureCommandLine::INLINING_IMPOSSIBLE;
            }
        }
    }
    else
        return ProcedureCommandLine::INLINING_IMPOSSIBLE;

    // All procedures were declared as inline
    return nInlineable;
}


/////////////////////////////////////////////////
/// \brief This virtual private member function
/// expands all procedures in the current command
/// line, which were declared as "inline" and
/// which are inlinable, into a vector of
/// command lines, which will be inserted in the
/// flow control command array before the current
/// command line.
///
/// \param sProc string&
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Procedure::expandInlineProcedures(string& sProc)
{
    vector<string> vExpandedProcedures;

    // No procedures?
    if (sProc.find('$') == string::npos)
        return vExpandedProcedures;

    size_t nProcedures = countProceduresInLine(sProc);

    if (sProc.find('$') != string::npos && sProc.find('(', sProc.find('$')) != string::npos)
    {
        size_t nPos = 0;

        // Examine all procedures, which may be found in the
        // current command string
        while (sProc.find('$', nPos) != string::npos && sProc.find('(', sProc.find('$', nPos)) != string::npos && nProcedures)
        {
            // Extract the name of the procedure
            nPos = sProc.find('$', nPos) + 1;

            // Only evaluate the current match, if it is not part of a string
            if (!isInQuotes(sProc, nPos, true))
            {
                string __sFileName;
                string __sProcName;
                string __sArgList;

                // Obtain procedure name, argument list and the corresponding
                // file name of the procedure
                extractProcedureInformation(sProc, nPos, __sProcName, __sArgList, __sFileName);

                // Pre-parse procedures, which are part of the current
                // argument list
                if (__sArgList.find('$') != string::npos)
                {
                    // Call member function recursively
                    vector<string> vExpandedArgList = expandInlineProcedures(__sArgList);

                    // Insert the returned list, if it is non-empty
                    if (vExpandedArgList.size())
                    {
                        vExpandedProcedures.insert(vExpandedProcedures.end(), vExpandedArgList.begin(), vExpandedArgList.end());
                    }
                }

                // Ensure that the current procedure is inlinable
                // (won't be re-evaluated here, because the result
                // of the last evaluation is cached)
                if (isInlineable(__sProcName, __sFileName))
                {
                    // Get the inlined representation as a vector
                    vector<string> vInlinedRepresentation = getInlined(__sProcName, __sArgList, __sFileName, nProcedures);

                    // Replace the return value and insert the
                    // stuff before the return value in the overall
                    // expansion
                    sProc.replace(nPos-1, getMatchingParenthesis(sProc.substr(nPos-1))+1, vInlinedRepresentation.back());
                    vExpandedProcedures.insert(vExpandedProcedures.end(), vInlinedRepresentation.begin(), vInlinedRepresentation.end()-1);
                }

                nProcedures--;
            }
        }
    }

    // All procedures were expanded
    return vExpandedProcedures;
}


/////////////////////////////////////////////////
/// \brief This private member function evaluates,
/// whether the current procedure is inlineable,
/// i.e. whether it fulfills the internal inlining
/// rules.
///
/// \param sProc const string&
/// \param sFileName const string&
/// \param nInlineFlag int*
/// \return int one of ProcedureCommendLine::Inlineable values
///
/////////////////////////////////////////////////
int Procedure::isInlineable(const string& sProc, const string& sFileName, int* nInlineFlag)
{
    // Get procedure element and goto to the corresponding line
    ProcedureElement* element = NumeReKernel::ProcLibrary.getProcedureContents(sFileName);
    int nProcedureLine = element->gotoProcedure("$" + sProc);
    int nInlineable = ProcedureCommandLine::INLINING_IMPOSSIBLE;
    string sArgumentList;

    const int nMAX_INLINING_LINES = 7;

    if (nProcedureLine < 0)
        throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, sProc, SyntaxError::invalid_position, "$" + sProc);

    // Get the procedure head
    pair<int, ProcedureCommandLine> currentline = element->getCurrentLine(nProcedureLine);

    // If the inline flag pointer was passed, store the inline flag
    // value here
    if (nInlineFlag)
    {
        *nInlineFlag = currentline.second.getFlags() & ProcedureCommandLine::FLAG_INLINE;
    }

    // Extract information about inlinability and the argument list
    nInlineable = currentline.second.isInlineable();
    sArgumentList = currentline.second.getArgumentList();

    if (nInlineable == ProcedureCommandLine::INLINING_UNKNOWN)
    {
        size_t nLines = 0;

        // Apply the rules and update the procedure definition
        while (!element->isLastLine(currentline.first))
        {
            currentline = element->getNextLine(currentline.first);

            // Apply the internal inlining rule set
            nInlineable = applyInliningRuleset(currentline.second.getCommandLine(), sArgumentList);

            // If the procedure either is not inlinable or we've reached
            // the end of the procedure, break the loop
            if (!nInlineable || currentline.second.getType() == ProcedureCommandLine::TYPE_PROCEDURE_FOOT || findCommand(currentline.second.getCommandLine()).sString == "return")
            {
                break;
            }

            nLines++;
        }

        // Ensure that we don't have too many lines
        if (nLines > nMAX_INLINING_LINES)
            nInlineable = ProcedureCommandLine::INLINING_IMPOSSIBLE;

        // Go to the procedure head again and update the
        // inlinability information
        currentline = element->getCurrentLine(nProcedureLine);
        currentline.second.setInlineable(nInlineable);

        return nInlineable;
    }

    return nInlineable;
}


/////////////////////////////////////////////////
/// \brief This private member function applies
/// the internal inlining rule set for a single
/// procedure command line and returns the
/// corresponding enumeration flags.
///
/// \param sCommandLine const string&
/// \param sArgumentList const string&
/// \return int one of ProcedureCommendLine::Inlineable values
///
/////////////////////////////////////////////////
int Procedure::applyInliningRuleset(const string& sCommandLine, const string& sArgumentList)
{
    static const string sINVALID_INLINING_COMMANDS = " cst tab namespace for if while switch ifndef ifndefined def define lclfunc redef redefine undef undefine ";
    string command = findCommand(sCommandLine).sString;

    // Check for invalid inlining commands
    if (sINVALID_INLINING_COMMANDS.find(" " + command + " ") != string::npos)
        return ProcedureCommandLine::INLINING_IMPOSSIBLE;

    // Check for procedures in the current line
    if (sCommandLine.find("$") != string::npos)
    {
        size_t nQuotes = 0;

        // Go through the line and search for dollars,
        // while considering the quotation marks
        for (size_t i = 0; i < sCommandLine.length(); i++)
        {
            if (sCommandLine[i] == '"' && (!i || sCommandLine[i-1] != '\\'))
                nQuotes++;

            if (sCommandLine[i] == '$' && !(nQuotes % 2))
                return ProcedureCommandLine::INLINING_IMPOSSIBLE;
        }
    }

    return ProcedureCommandLine::INLINING_POSSIBLE;
}


/////////////////////////////////////////////////
/// \brief This private member function simply
/// counts the number of procedures,  which may
/// be found in the current command line.
///
/// \param sCommandLine const string&
/// \return size_t
///
/////////////////////////////////////////////////
size_t Procedure::countProceduresInLine(const string& sCommandLine)
{
    size_t nProcedures = 0;
    size_t nPos = 0;

    // Only do something, if there are candidates for procedures
    if (sCommandLine.find('$') != string::npos && sCommandLine.find('(', sCommandLine.find('$')) != string::npos)
    {

        // Examine all procedures candidates, which may be found in the
        // current command string
        while (sCommandLine.find('$', nPos) != string::npos && sCommandLine.find('(', sCommandLine.find('$', nPos)) != string::npos)
        {
            nPos = sCommandLine.find('$', nPos) + 1;

            // Only count the current match, if it is not part of a string
            if (!isInQuotes(sCommandLine, nPos, true))
            {
                nProcedures++;
            }
        }
    }

    // Return the number of strings
    return nProcedures;
}


/////////////////////////////////////////////////
/// \brief This virtual private member function
/// returns the inlined representation of the
/// selected procedure as a vector containing the
/// single commands.
///
/// \param sProc const string&
/// \param sArgumentList const string&
/// \param sFileName const string&
/// \param nProcedures size_t
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Procedure::getInlined(const string& sProc, const string& sArgumentList, const string& sFileName, size_t nProcedures)
{
    // Prepare a variable factory and get the procedure
    // element
    ProcedureVarFactory varFactory(this, sProc, nthRecursion, true);
    ProcedureElement* element = NumeReKernel::ProcLibrary.getProcedureContents(sFileName);

    // Goto to the corresponding procedure head
    int nProcedureLine = element->gotoProcedure("$" + sProc);
    vector<string> vProcCommandLines;
    string sCommandLine;
    static const string sSPECIALRETURNVALS = " true false nan inf -inf ";

    // Get a reference to the debugger. This is needed to insert
    // the applied breakpoints at the correct location
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    // Ensure that the procedure was found in the file
    if (nProcedureLine < 0)
        throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, sProc, SyntaxError::invalid_position, "$" + sProc);

    // Get the procedure head
    pair<int, ProcedureCommandLine> currentline = element->getCurrentLine(nProcedureLine);

    // Create the filled argument list in the variable factory
    varFactory.createProcedureArguments(currentline.second.getArgumentList(), sArgumentList);

    // If there are some argument copies needed, we'll
    // insert them now
    if (varFactory.vInlineArgDef.size())
        vProcCommandLines.insert(vProcCommandLines.end(), varFactory.vInlineArgDef.begin(), varFactory.vInlineArgDef.end());

    // Read each line, replace the arguments with their
    // values and push the result in the vector
    while (!element->isLastLine(currentline.first))
    {
        // Get next line and replace the argument occurences
        currentline = element->getNextLine(currentline.first);
        sCommandLine = varFactory.resolveVariables(" " + currentline.second.getCommandLine() + " ");

        // Local variables and strings are allowed and will be redirected
        // into temporary cluster elements
        if (findCommand(sCommandLine).sString == "var")
        {
            varFactory.createLocalVars(sCommandLine.substr(findCommand(sCommandLine).nPos + 4));
            sCommandLine = varFactory.sInlineVarDef + ";";
        }
        else if (findCommand(sCommandLine).sString == "str")
        {
            varFactory.createLocalStrings(sCommandLine.substr(findCommand(sCommandLine).nPos + 4));
            sCommandLine = varFactory.sInlineStringDef + ";";
        }

        // Insert a breakpoint, if the breakpoint manager
        // contains a reference to this line
        if (_debugger.getBreakpointManager().isBreakpoint(sFileName, currentline.first))
            sCommandLine = "|> " + sCommandLine;

        // If the current line is the last procedure line,
        // simply push a "true" into the vector
        if (currentline.second.getType() == ProcedureCommandLine::TYPE_PROCEDURE_FOOT)
        {
            vProcCommandLines.push_back("true");
            break;
        }

        // If the current line is a "return" statement,
        // remove the statement and inspect the return
        // value
        if (findCommand(sCommandLine).sString == "return")
        {
            // Get the return value
            size_t pos = findCommand(sCommandLine).nPos;
            sCommandLine.erase(0, pos+7);

            if (sCommandLine.find(';') != string::npos)
                sCommandLine.erase(sCommandLine.rfind(';'));

            // Strip all spaces from the return value
            StripSpaces(sCommandLine);

            // Push a or the return value depending on the
            // type into the vector. We try to exclude all
            // constant cases, which will increase the speed
            // of the inlined procedure even more
            if (sCommandLine == "void")
                vProcCommandLines.push_back("");
            else if (!sCommandLine.length())
                vProcCommandLines.push_back("true");
            else if (sSPECIALRETURNVALS.find(" " + sCommandLine + " ") != string::npos || _parser.GetConst().find(sCommandLine) != _parser.GetConst().end())
                vProcCommandLines.push_back(sCommandLine);
            else if (isMultiValue(sCommandLine))
            {
                // Multi value return value
                if (nProcedures > 1)
                {
                    // Save the return value in a cluster
                    string sTempCluster = NumeReKernel::getInstance()->getMemoryManager().createTemporaryCluster();
                    vProcCommandLines.push_back(sTempCluster + " = " + sCommandLine + ";");
                    vProcCommandLines.push_back(sTempCluster);
                }
                else
                    vProcCommandLines.push_back("{" + sCommandLine + "}");
            }
            else
            {
                // Single value return value
                if (nProcedures > 1)
                {
                    // Save the return value in a cluster
                    string sTempCluster = NumeReKernel::getInstance()->getMemoryManager().createTemporaryCluster();
                    vProcCommandLines.push_back(sTempCluster + " = " + sCommandLine + ";");
                    vProcCommandLines.push_back(sTempCluster);
                }
                else
                    vProcCommandLines.push_back("(" + sCommandLine + ")");
            }

            break;
        }
        else
            vProcCommandLines.push_back(sCommandLine);
    }

    return vProcCommandLines;
}


/////////////////////////////////////////////////
/// \brief Mangles a procedure name to be used as
/// a usual variable.
///
/// \param sProcedureName std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string Procedure::mangleName(std::string sProcedureName)
{
    for (size_t i = 0; i < sProcedureName.length(); i++)
    {
        if (!isalnum(sProcedureName[i]) && sProcedureName[i] != '_' && sProcedureName[i] != '~')
            sProcedureName[i] = '_';
    }

    return sProcedureName;
}


/////////////////////////////////////////////////
/// \brief This virtual member function handles
/// the gathering of all relevant information for
/// the debugger for the currently found
/// breakpoint.
///
/// \param _parser Parser&
/// \param _option Settings&
/// \return int
///
/////////////////////////////////////////////////
int Procedure::evalDebuggerBreakPoint(Parser& _parser, Settings& _option)
{
    // if the stack is empty, it has to be a breakpoint from a script
    // This is only valid, if the script contained flow control statements
    if (!NumeReKernel::getInstance()->getDebugger().getStackSize())
        return NumeReKernel::evalDebuggerBreakPoint("");

    // Get a reference to the debugger object
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Gather all information needed by the debugger
    _debugger.gatherInformations(_varFactory, "", sCurrentProcedureName, GetCurrentLine());

    // Let the kernel display the debugger window and jump to the
    // corresponding line in the procedure file
    return _debugger.showBreakPoint();
}


/////////////////////////////////////////////////
/// \brief This virtual member function handles
/// the gathering of all relevant information for
/// the debugger for the currently found error.
///
/// \return int
///
/////////////////////////////////////////////////
int Procedure::getErrorInformationForDebugger()
{
    // if the stack is empty, it has to be a breakpoint from a script
    // This is only valid, if the script contained flow control statements
    if (!NumeReKernel::getInstance()->getDebugger().getStackSize())
        return 0;

    // Get a reference to the debugger object
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Gather all information needed by the debugger
    _debugger.gatherInformations(_varFactory, "", sCurrentProcedureName, GetCurrentLine());

    return 1;
}


/////////////////////////////////////////////////
/// \brief This virtual member function is
/// inserted in some automatically catchable
/// locations to convert an error into a warning
/// to avoid that the calculation is aborted.
/// This will only get active, if the
/// corresponding procedure is flagged as "test".
///
/// \param e_ptr exception_ptr
/// \param bSupressAnswer_back bool
/// \param nLine int
/// \return int
///
/////////////////////////////////////////////////
int Procedure::catchExceptionForTest(exception_ptr e_ptr, bool bSupressAnswer_back, int nLine)
{
    // Assure that the procedure is flagges as "test"
    if (nFlags & ProcedureCommandLine::FLAG_TEST)
    {
        try
        {
            // Rethrow to determine the exception type
            rethrow_exception(e_ptr);
        }
        catch (mu::Parser::exception_type& e)
        {
            // Catch and convert parser errors
            NumeReKernel::getInstance()->getDebugger().finalizeCatched();
            NumeReKernel::failMessage("@" + toString(nLine+1) + " | FAILED EXPRESSION: '" + e.GetExpr() + "'");
        }
        catch (SyntaxError& e)
        {
            // Catch and convert syntax errors with the exception
            // of a user abort request
            if (e.errorcode == SyntaxError::PROCESS_ABORTED_BY_USER)
            {
                // Rethrow the abort request
                resetProcedure(NumeReKernel::getInstance()->getParser(), bSupressAnswer_back);
                throw;
            }
            else if (e.getToken().length() && (e.errorcode == SyntaxError::PROCEDURE_THROW || e.errorcode == SyntaxError::LOOP_THROW))
            {
                // Display custom errors with their message
                NumeReKernel::getInstance()->getDebugger().finalizeCatched();
                NumeReKernel::failMessage("@" + toString(nLine+1) + " | ERROR CAUGHT: " + e.getToken());
            }
            else
            {
                // Mark default errors only with the failing expression
                NumeReKernel::getInstance()->getDebugger().finalizeCatched();
                NumeReKernel::failMessage("@" + toString(nLine+1) + " | FAILED EXPRESSION: '" + e.getExpr() + "'");
            }
        }
        catch (...)
        {
            // All other exceptions are not catchable, because they refer
            // to internal issues, for which it might not possible to
            // handle them here
            resetProcedure(NumeReKernel::getInstance()->getParser(), bSupressAnswer_back);
            throw;
        }
    }
    else
    {
        // If not a test, then simply reset the current procedure
        // and rethrow the error
        resetProcedure(NumeReKernel::getInstance()->getParser(), bSupressAnswer_back);
        rethrow_exception(e_ptr);
    }

    return 0;
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// current line number depending on whether a
/// flow control statement is evaluated or not.
///
/// \return unsigned int
///
/////////////////////////////////////////////////
unsigned int Procedure::GetCurrentLine() const
{
    // Get the line number from FlowCtrl
    if (bEvaluatingFlowControlStatements)
        return getCurrentLineNumber();

    // Use the internal line number
    return nCurrentLine;
}


/////////////////////////////////////////////////
/// \brief This member function replaces the
/// procedure occurence between the both passed
/// positions using akronymed version of the
/// called procedure. It also declares the
/// numerical return value as a vector to the
/// parser.
///
/// \param sLine string&
/// \param _parser Parser&
/// \param _return const Returnvalue&
/// \param nPos unsigned int
/// \param nPos2 unsigned int
/// \param sReplaceName const string&
/// \return size_t
///
/////////////////////////////////////////////////
size_t Procedure::replaceReturnVal(string& sLine, Parser& _parser, const Returnvalue& _return, unsigned int nPos, unsigned int nPos2, const string& sReplaceName)
{
    // Replace depending on the type
    if (_return.isString())
    {
        // String value, transform the return value
        // into a string vector
        string sReturn = "{";

        for (unsigned int v = 0; v < _return.vStringVal.size(); v++)
            sReturn += _return.vStringVal[v] + ",";

        sReturn.back() = '}';
        sLine = sLine.substr(0, nPos) + sReturn + sLine.substr(nPos2);

        return sReturn.length();
    }
    else if (_return.isNumeric())
    {
        // Numerical value, use the procedure name
        // to derive a vector name and declare the
        // corresponding vector
        _parser.SetVectorVar(sReplaceName, _return.vNumVal);
        sLine = sLine.substr(0, nPos) + sReplaceName +  sLine.substr(nPos2);

        return sReplaceName.length();
    }

    sLine = sLine.substr(0, nPos) + "nan" + sLine.substr(nPos2);
    return 3;
}


/////////////////////////////////////////////////
/// \brief This member function sets the current
/// procedure object to its original state. It
/// will be called at the end of the executed
/// procedure.
///
/// \param _parser Parser&
/// \param bSupressAnswer bool
/// \return void
///
/////////////////////////////////////////////////
void Procedure::resetProcedure(Parser& _parser, bool bSupressAnswer)
{
    // Remove the current procedure from the call stack
    NumeReKernel::getInstance()->getDebugger().popStackItem();

    sCallingNameSpace = "main";
    sNameSpace.clear();
    sThisNameSpace.clear();
    mVarMap.clear();
    NumeReKernel::bSupressAnswer = bSupressAnswer;
    _parser.mVarMapPntr = 0;
    _localDef.reset();
    nDebuggerCode = 0;
    nFlags = 0;
    sTestClusterName.clear();
    nthRecursion = 0;

    // Delete the variable factory for the current procedure
    if (_varFactory)
    {
        delete _varFactory;
        _varFactory = nullptr;
    }

    // Remove the last procedure in the current stack
    if (sProcNames.length())
    {
        sProcNames.erase(sProcNames.rfind(';'));
    }

    return;
}


/////////////////////////////////////////////////
/// \brief This member function extracts the
/// namespace of the currently executed procedure.
///
/// \param sProc const string&
/// \return void
///
/////////////////////////////////////////////////
void Procedure::extractCurrentNamespace(const string& sProc)
{
    for (unsigned int i = sProc.length() - 1; i >= 0; i--)
    {
        if (sProc[i] == '\\' || sProc[i] == '/' || sProc[i] == '~')
        {
            sThisNameSpace = sProc.substr(0, i);

            // If the namespace doesn't contain a colon
            // replace all path separators with a tilde
            // character
            if (sThisNameSpace.find(':') == string::npos)
            {
                for (unsigned int j = 0; j < sThisNameSpace.length(); j++)
                {
                    if (sThisNameSpace[j] == '\\' || sThisNameSpace[j] == '/')
                        sThisNameSpace[j] = '~';
                }
            }

            break;
        }

        if (!i)
        {
            sThisNameSpace = "main";
            break;
        }
    }

    // If the current namespace is "thisfile", use the calling namespace
    if (sThisNameSpace == "thisfile")
        sThisNameSpace = sCallingNameSpace;
}


/////////////////////////////////////////////////
/// \brief This method handles the definitions of
/// local variables.
///
/// \param sProcCommandLine string&
/// \param sCommand const string&
/// \return bool
///
/// It will return true, if a definition occured,
/// otherwise false. The return value will be
/// used to create a corresponding byte code.
/////////////////////////////////////////////////
bool Procedure::handleVariableDefinitions(string& sProcCommandLine, const string& sCommand)
{
    // Is it a variable declaration?
    if (sCommand == "var" && sProcCommandLine.length() > 6)
    {
        _varFactory->createLocalVars(sProcCommandLine.substr(sProcCommandLine.find("var") + 3));

        sProcCommandLine = "";
        return true;
    }

    // Is it a string declaration?
    if (sCommand == "str" && sProcCommandLine.length() > 6)
    {
        _varFactory->createLocalStrings(sProcCommandLine.substr(sProcCommandLine.find("str") + 3));

        sProcCommandLine = "";
        return true;
    }

    // Is it a table declaration?
    if (sCommand == "tab" && sProcCommandLine.length() > 6)
    {
        _varFactory->createLocalTables(sProcCommandLine.substr(sProcCommandLine.find("tab") + 3));

        sProcCommandLine = "";
        return true;
    }

    // Is it a cluster declaration?
    if (sCommand == "cst" && sProcCommandLine.length() > 6)
    {
        _varFactory->createLocalClusters(sProcCommandLine.substr(sProcCommandLine.find("cst") + 3));

        sProcCommandLine = "";
        return true;
    }

    // No local variable declaration in this command line
    return false;
}


/////////////////////////////////////////////////
/// \brief This member function reads the lines
/// from the included file. It acts quite
/// independent from the rest of the procedure.
///
/// \param fInclude ifstream&
/// \param nIncludeType int
/// \param _parser Parser&
/// \param _functions Define&
/// \param _data Datafile&
/// \param _out Output&
/// \param _pData PlotData&
/// \param _script Script&
/// \param _option Settings&
/// \param nth_procedure unsigned int
/// \return void
///
/////////////////////////////////////////////////
void Procedure::readFromInclude(ifstream& fInclude, int nIncludeType, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_procedure)
{
    string sProcCommandLine;
    bool bSkipNextLine = false;
    bool bAppendNextLine = false;
    bool bBlockComment = false;
    int nCurrentByteCode;

    // Read as long as the end of the included script was not reached
    while (!fInclude.eof())
    {
        nCurrentByteCode = 0;
        getline(fInclude, sProcCommandLine);

        StripSpaces(sProcCommandLine);

        // Ignore empty lines and line comments
        if (!sProcCommandLine.length())
            continue;

        if (sProcCommandLine.substr(0, 2) == "##")
            continue;

        // Ignore install sections
        if (sProcCommandLine.substr(0, 9) == "<install>"
            || (findCommand(sProcCommandLine).sString == "global" && sProcCommandLine.find("<install>") != string::npos))
        {
            while (!fInclude.eof())
            {
                getline(fInclude, sProcCommandLine);

                StripSpaces(sProcCommandLine);

                if (sProcCommandLine.substr(0, 12) == "<endinstall>"
                        || (findCommand(sProcCommandLine).sString == "global" && sProcCommandLine.find("<endinstall>") != string::npos))
                    break;
            }

            sProcCommandLine = "";
            continue;
        }

        // Ignore remaining comments
        if (sProcCommandLine.find("##") != string::npos)
            sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find("##"));

        if (sProcCommandLine.substr(0, 2) == "#*" && sProcCommandLine.find("*#", 2) == string::npos)
        {
            bBlockComment = true;
            sProcCommandLine = "";
            continue;
        }

        // Handle block comments
        if (bBlockComment && sProcCommandLine.find("*#") != string::npos)
        {
            bBlockComment = false;

            if (sProcCommandLine.find("*#") == sProcCommandLine.length() - 2)
            {
                sProcCommandLine = "";
                continue;
            }
            else
                sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("*#") + 2);
        }
        else if (bBlockComment && sProcCommandLine.find("*#") == string::npos)
        {
            sProcCommandLine = "";
            continue;
        }

        // Skip the next line, if the current one ends with a doubled backslash
        if (bSkipNextLine && sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length() - 2) == "\\\\")
        {
            sProcCommandLine = "";
            continue;
        }
        else if (bSkipNextLine)
        {
            bSkipNextLine = false;
            sProcCommandLine = "";
            continue;
        }

        // If the current line doesn't contain any of the included commands
        // ignore it
        if (findCommand(sProcCommandLine).sString != "define"
                && findCommand(sProcCommandLine).sString != "ifndef"
                && findCommand(sProcCommandLine).sString != "ifndefined"
                && findCommand(sProcCommandLine).sString != "redefine"
                && findCommand(sProcCommandLine).sString != "redef"
                && findCommand(sProcCommandLine).sString != "lclfunc"
                && findCommand(sProcCommandLine).sString != "global"
                && !bAppendNextLine)
        {
            if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length() - 2) == "\\\\")
                bSkipNextLine = true;

            sProcCommandLine = "";
            continue;
        }

        // Depending on the include type, ignore the other commands
        if (nIncludeType == 1
                && findCommand(sProcCommandLine).sString != "define"
                && findCommand(sProcCommandLine).sString != "ifndef"
                && findCommand(sProcCommandLine).sString != "ifndefined"
                && findCommand(sProcCommandLine).sString != "redefine"
                && findCommand(sProcCommandLine).sString != "redef"
                && findCommand(sProcCommandLine).sString != "lclfunc"
                && !bAppendNextLine)
        {
            if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length() - 2) == "\\\\")
                bSkipNextLine = true;

            sProcCommandLine = "";
            continue;
        }
        else if (nIncludeType == 2
                 && findCommand(sProcCommandLine).sString != "global"
                 && !bAppendNextLine)
        {
            if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length() - 2) == "\\\\")
                bSkipNextLine = true;

            sProcCommandLine = "";
            continue;
        }

        // Append the next line, if the current one ends with a doubled backslash
        if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length() - 2) == "\\\\")
            bAppendNextLine = true;
        else
            bAppendNextLine = false;

        // Handle the trailing semicolon
        if (sProcCommandLine.back() == ';')
        {
            bProcSupressAnswer = true;
            sProcCommandLine.pop_back();
        }

        // Execure procedures, if necessary
        if (sProcCommandLine.find('$') != string::npos && sProcCommandLine.find('(', sProcCommandLine.find('$')) != string::npos)
        {
            if (nFlags & ProcedureCommandLine::FLAG_INLINE)
            {

                throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sProcCommandLine, SyntaxError::invalid_position);
            }

            try
            {
                FlowCtrl::ProcedureInterfaceRetVal nReturn = procedureInterface(sProcCommandLine, _parser, _functions, _data, _out, _pData, _script, _option, nth_procedure);

                if (nReturn == FlowCtrl::INTERFACE_ERROR)
                    throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sProcCommandLine, SyntaxError::invalid_position);
                else if (nReturn == FlowCtrl::INTERFACE_EMPTY)
                {
                    sProcCommandLine = "";
                    bProcSupressAnswer = false;
                    continue;
                }
            }
            catch (...)
            {
                throw;
            }
        }

        // Evaluate the current line
        try
        {
            ProcCalc(sProcCommandLine, findCommand(sProcCommandLine).sString, nCurrentByteCode, _parser, _functions, _data, _option, _out, _pData, _script);
        }
        catch (...)
        {
            throw;
        }

        sProcCommandLine = "";

        if (bProcSupressAnswer)
            bProcSupressAnswer = false;
    }

    if (fInclude.eof())
    {
        fInclude.close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// script include syntax, which one may use in
/// other procedures.
///
/// \param sProcCommandLine string&
/// \param fInclude ifstream&
/// \param bReadingFromInclude bool
/// \return int
///
/// The included file is indicated with an "@"
/// and its file name afterwards. This function
/// will decode this syntax and open the
/// corresponding file stream.
/////////////////////////////////////////////////
int Procedure::handleIncludeSyntax(string& sProcCommandLine, ifstream& fInclude, bool bReadingFromInclude)
{
    int nIncludeType = 0;

    // Try to find the include syntax in the current procedure command line
    if (!bReadingFromInclude && sProcCommandLine[1] == '@' && sProcCommandLine[2] != ' ')
    {
        StripSpaces(sProcCommandLine);
        string sIncludeFileName = "";

        // Extract the include file name
        if (sProcCommandLine[1] == '"')
            sIncludeFileName = sProcCommandLine.substr(2, sProcCommandLine.find('"', 2) - 2);
        else
            sIncludeFileName = sProcCommandLine.substr(1, sProcCommandLine.find(' ') - 1);

        // Extract the include type
        if (sProcCommandLine.find(':') != string::npos)
        {
            if (sProcCommandLine.find("defines", sProcCommandLine.find(':') + 1) != string::npos)
            {
                nIncludeType = 1;
            }
            else if (sProcCommandLine.find("globals", sProcCommandLine.find(':') + 1) != string::npos)
            {
                nIncludeType = 2;
            }
            else if (sProcCommandLine.find("procedures", sProcCommandLine.find(':') + 1) != string::npos)
            {
                nIncludeType = 3;
            }
        }

        // Remove everything after the last colon,
        // if it is not the colon after the drive
        // letter
        if (sIncludeFileName.find(':') != string::npos)
        {
            for (int __i = sIncludeFileName.length() - 1; __i >= 0; __i--)
            {
                if (sIncludeFileName[__i] == ':'
                        && (__i > 1
                            || (__i == 1 && sIncludeFileName.length() > (unsigned int)__i + 1 && sIncludeFileName[__i + 1] != '/')))
                {
                    sIncludeFileName.erase(sIncludeFileName.find(':'));
                    break;
                }
            }
        }

        // Create a valid file name from the extracted file name
        if (sIncludeFileName.length())
            sIncludeFileName = ValidFileName(sIncludeFileName, ".nscr");
        else
        {
            sProcCommandLine.clear();
            return 0;
        }

        // Open the file stream
        fInclude.clear();
        fInclude.open(sIncludeFileName.c_str());

        if (fInclude.fail())
        {
            fInclude.close();
            throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sProcCommandLine, SyntaxError::invalid_position, sIncludeFileName);
        }

        sProcCommandLine = "";
    }
    else if (sProcCommandLine[1] == '@')
    {
        sProcCommandLine = "";
    }

    // Return the obtained include type
    return nIncludeType;
}


