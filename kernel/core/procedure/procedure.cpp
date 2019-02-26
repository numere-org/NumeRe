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

// Default constructor
Procedure::Procedure() : FlowCtrl(), Plugin()
{
    // init the object
	init();
}

// Custom constructor using the presets from the passed
// procedure. Used as a recursion functionality
Procedure::Procedure(const Procedure& _procedure) : FlowCtrl(), Plugin(_procedure)
{
    // Init the object
	init();

	// Copy the relevant data
	sPath = _procedure.sPath;
	sWhere = _procedure.sWhere;
	sCallingNameSpace = _procedure.sCallingNameSpace;
	sProcNames = _procedure.sProcNames;

	for (unsigned int i = 0; i < 6; i++)
	{
		sTokens[i][0] = _procedure.sTokens[i][0];
		sTokens[i][1] = _procedure.sTokens[i][1];
	}
}

// Destructor ensuring that the procedure
// output file stream will be closed, if it is
// still open
Procedure::~Procedure()
{
	if (fProcedure.is_open())
		fProcedure.close();
}

// Private initializing member function. Sets all
// variables to a reasonable default value
void Procedure::init()
{
	sCallingNameSpace = "main";
	bProcSupressAnswer = false;
	bWritingTofile = false;
	nCurrentLine = 0;
	nthBlock = 0;
	nFlags = 0;

	sVars = nullptr;
	dVars = nullptr;
	sStrings = nullptr;
	sTables = nullptr;

	nVarSize = 0;
	nStrSize = 0;
	nTabSize = 0;
}

// This member function does the calculation stuff for the current procedure
Returnvalue Procedure::ProcCalc(string sLine, string sCurrentCommand, int& nByteCode, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script)
{
	string sCache = "";
	Indices _idx;
	bool bWriteToCache = false;
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

	// Handle the "to_cmd()" function, which is quite slow
	// Only handle this function, if we're not inside of a loop
	if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_TOCOMMAND
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
    {
        if (sLine.find("to_cmd(") != string::npos && !getLoop())
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
                if (containsStrings(sCmdString) || _data.containsStringVars(sCmdString))
                {
                    sCmdString += " -nq";
                    parser_StringParser(sCmdString, sCache, _data, _parser, _option, true);
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
        if (sCurrentCommand == "throw" && !getLoop())
        {
            string sErrorToken;

            if (sLine.length() > 6 && (containsStrings(sLine) || _data.containsStringVars(sLine)))
            {
                if (_data.containsStringVars(sLine))
                    _data.getStringValues(sLine);

                getStringArgument(sLine, sErrorToken);
                sErrorToken += " -nq";
                parser_StringParser(sErrorToken, sCache, _data, _parser, _option, true);
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
        if (!getLoop() && sLine.find("??") != string::npos && sCurrentCommand != "help")
        {
            sLine = parser_Prompt(sLine);

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
        if (!getLoop()
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
            switch (BI_CommandHandler(sLine, _data, _out, _option, _parser, _functions, _pData, _script, true))
            {
                case  0:
                    if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    {
                        StripSpaces(sCommandBack);
                        string sCurrentLine = sLine;
                        StripSpaces(sCurrentLine);
                        if (sCommandBack != sCurrentLine)
                            nByteCode |= ProcedureCommandLine::BYTECODE_COMMAND;
                    }
                    break; // Kein Keywort: Mit dem Parser auswerten
                case  1:        // Keywort: Naechster Schleifendurchlauf!
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
                sLine = parser_Prompt(sLine);
        }

    }

	// Call functions if we're not in a loop
	if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT))
    {
        if (!getLoop() && sCurrentCommand != "for" && sCurrentCommand != "if" && sCurrentCommand != "while" && sCurrentCommand != "switch")
        {
            if (!_functions.call(sLine, _option))
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
        if (!getLoop())
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
        if (getLoop() || sCurrentCommand == "for" || sCurrentCommand == "if" || sCurrentCommand == "while" || sCurrentCommand == "switch")
        {
            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT;
            // Add the suppression semicolon, if necessary
            if (bProcSupressAnswer)
                sLine += ";";

            // Pass the command line to the FlowCtrl class
            setCommand(sLine, _parser, _data, _functions, _option, _out, _pData, _script);

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
        if (!containsStrings(sLine)
            && !_data.containsStringVars(sLine)
            && (sLine.find("data(") != string::npos || _data.containsCacheElements(sLine)))
        {
            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_DATAACCESS;

            sCache = getDataElements(sLine, _parser, _data, _option);

            if (sCache.length() && sCache.find('#') == string::npos)
                bWriteToCache = true;
        }
    }

	// If the current line contains a string value or a string variable,
	// call the string parser and handle the return value
	if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
        || nCurrentByteCode & ProcedureCommandLine::BYTECODE_STRING
        || nFlags & ProcedureCommandLine::FLAG_TEMPLATE)
    {
        if (containsStrings(sLine) || _data.containsStringVars(sLine))
        {
            int nReturn = parser_StringParser(sLine, sCache, _data, _parser, _option, bProcSupressAnswer);

            if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                nByteCode |= ProcedureCommandLine::BYTECODE_STRING;

            // Handle the return value
            if (nReturn == 1)
            {
                // Only strings
                thisReturnVal.vStringVal.push_back(sLine);
                return thisReturnVal;
            }
            else if (!nReturn)
            {
                // Error
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
            }

            // Other: numerical values
            if (sCache.length() && _data.containsCacheElements(sCache) && !bWriteToCache)
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
            _idx = parser_getIndices(sCache, _parser, _data, _option);

            if ((_idx.nI[0] < 0 || _idx.nJ[0] < 0) && !_idx.vI.size() && !_idx.vJ.size())
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "");

            if ((_idx.nI[1] == -2 && _idx.nJ[1] == -2))
                throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];

            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];

            sCache.erase(sCache.find('('));
            StripSpaces(sCache);
        }
    }

	// Set the expression and evaluate it
	if (!_parser.IsAlreadyParsed(sLine))
		_parser.SetExpr(sLine);

	v = _parser.Eval(nNum);

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

	// Print the output to the console, if it isn't suppressed
	if (!bProcSupressAnswer)
        NumeReKernel::print(NumeReKernel::formatResultOutput(nNum, v, _option));

    // Write the return values to cache
	if (bWriteToCache)
		_data.writeToCache(_idx, sCache, v, nNum);

    // Clear the vector variables after the loop returned
	if (!_parser.ActiveLoopMode() || (!_parser.IsLockedPause() && !(nFlags & ProcedureCommandLine::FLAG_INLINE)))
		_parser.ClearVectorVars(true);

	return thisReturnVal;
}


// This member function is used to obtain the procedure file name
// from the selected procedure. It handles the "thisfile" namespace
// directly
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

		// Create a valid file name from the procedure name
		sCurrentProcedureName = FileSystem::ValidFileName(sProc, ".nprc");

		// Replace tilde characters with path separators
		if (sCurrentProcedureName.find('~') != string::npos)
		{
			unsigned int nPos = sCurrentProcedureName.rfind('/');

            // Find the last path separator
			if (nPos < sCurrentProcedureName.rfind('\\') && sCurrentProcedureName.rfind('\\') != string::npos)
				nPos = sCurrentProcedureName.rfind('\\');

            // Replace all tilde characters in the current path
            // string. Consider the special namespace "main", which
            // is a reference to the toplevel procedure folder
			for (unsigned int i = nPos; i < sCurrentProcedureName.length(); i++)
			{
				if (sCurrentProcedureName[i] == '~')
				{
					if (sCurrentProcedureName.length() > 5 && i >= 4 && sCurrentProcedureName.substr(i - 4, 5) == "main~")
						sCurrentProcedureName = sCurrentProcedureName.substr(0, i - 4) + sCurrentProcedureName.substr(i + 1);
					else
						sCurrentProcedureName[i] = '/';
				}
			}
		}

		// Append the newly obtained procedure file name
		// to the call stack
		sProcNames += ";" + sCurrentProcedureName;
		return true;
	}
	else
		return false;
}

// This member function handles the most part of the execution
// of the currently selected procedure.
Returnvalue Procedure::execute(string sProc, string sVarList, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, unsigned int nth_procedure)
{
    // Measure the current stack size and ensure
    // that the current call won't exceed the
    // maximal stack size
    int stackMeasureVar;
    if ((stackMeasureVar = abs(&stackMeasureVar - NumeReKernel::baseStackPosition)) > MAX_PROCEDURE_STACK_SIZE)
        throw SyntaxError(SyntaxError::PROCEDURE_STACK_OVERFLOW, "$" + sProc + "(" + sVarList + ")", SyntaxError::invalid_position, "\\$" + sProc, nth_procedure);

	StripSpaces(sProc);
	_option._debug.pushStackItem(sProc + "(" + sVarList + ")");

	// Set the file name for the currently selected procedure
	if (!setProcName(sProc))
		throw SyntaxError(SyntaxError::INVALID_PROCEDURE_NAME, "$" + sProc + "(" + sVarList + ")", SyntaxError::invalid_position);

	ifstream fInclude;

	string sProcCommandLine = "";
	string sCmdCache = "";
	string sCurrentCommand = "";
	bool bReadingFromInclude = false;
	int nIncludeType = 0;
	int nByteCode = 0;
	int nCurrentByteCode = 0;
	Returnvalue _ReturnVal;

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
	ProcedureVarFactory _varfactory(this, &_parser, &_data, &_option, &_functions, &_out, &_pData, &_script, sProc, nth_procedure);
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
		if (_option.getUseDebugger())
			_option._debug.popStackItem();
		throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, "", SyntaxError::invalid_position, sProc);
    }

    // Get the procedure head line
    currentLine = ProcElement->getCurrentLine(currentLine.first);

    // verify that this is a procedure headline
    if (currentLine.second.getType() != ProcedureCommandLine::TYPE_PROCEDURE_HEAD)
    {
        sCallingNameSpace = "main";
		mVarMap.clear();
		if (_option.getUseDebugger())
			_option._debug.popStackItem();
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
        if (_option.getUseDebugger())
            _option._debug.popStackItem();
        throw SyntaxError(SyntaxError::PRIVATE_PROCEDURE_CALLED, sProcCommandLine, SyntaxError::invalid_position, sErrorToken);
    }

    if (nFlags & ProcedureCommandLine::FLAG_MASK)
    {
        if (_option.getSystemPrintStatus())
        {
            // if the print status is true, set it to false
            _option.setSystemPrintStatus(false);
            nFlags |= ProcedureCommandLine::FLAG_MASK;
        }
    }

    // Get the argument list and evaluate it
    string sVarDeclarationList = currentLine.second.getArgumentList();

    // Now the calling namespace is the current namespace
    sCallingNameSpace = sThisNameSpace;

    if (findCommand(sVarList, "var").sString == "var")
    {
        if (_option.getUseDebugger())
            _option._debug.gatherInformations(nullptr, 0, nullptr, nullptr, 0, nullptr, 0, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
        throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "var");
    }
    if (findCommand(sVarList, "str").sString == "str")
    {
        if (_option.getUseDebugger())
            _option._debug.gatherInformations(nullptr, 0, nullptr, nullptr, 0, nullptr, 0, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
        throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "str");
    }
    if (findCommand(sVarList, "tab").sString == "tab")
    {
        if (_option.getUseDebugger())
            _option._debug.gatherInformations(nullptr, 0, nullptr, nullptr, 0, nullptr, 0, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
        throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "tab");
    }

    StripSpaces(sVarDeclarationList);
    StripSpaces(sVarList);

    // Evaluate the argument list for the current procedure
    mVarMap = _varfactory.createProcedureArguments(sVarDeclarationList, sVarList);

    _parser.mVarMapPntr = &mVarMap;



	// As long as we didn't find the last line,
    // read the next line from the procedure and execute
    // this line
	while (!ProcElement->isLastLine(currentLine.first))
	{
	    // Set the bytecode from the last calculation
	    ProcElement->setByteCode(nCurrentByteCode | nByteCode, currentLine.first);
		bProcSupressAnswer = false;

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

				if (_option.getUseDebugger() && NumeReKernel::_messenger.isBreakpoint(sCurrentProcedureName, nCurrentLine) && sProcCommandLine.substr(0, 2) != "|>")
					sProcCommandLine.insert(0, "|> ");
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
			        if (_option.getUseDebugger())
                        _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);

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
				sProcCommandLine = _varfactory.resolveVariables(sProcCommandLine);
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

		// Handle breakpoints
		if (_option.getUseDebugger()
            && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT))
        {
            if (sProcCommandLine.substr(sProcCommandLine.find_first_not_of(' '), 2) == "|>" && !getLoop())
            {
                sProcCommandLine.erase(sProcCommandLine.find_first_not_of(' '), 2);
                StripSpaces(sProcCommandLine);

                _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                evalDebuggerBreakPoint(_parser, _option);

                if (!sProcCommandLine.length())
                    continue;

                sProcCommandLine.insert(0, 1, ' ');
            }
        }

        // Handle the definition of local variables
        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || nCurrentByteCode & ProcedureCommandLine::BYTECODE_VARDEF)
        {
            if (handleVariableDefinitions(sProcCommandLine, sCurrentCommand, _varfactory))
            {
                if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_VARDEF;
            }
            if (!sProcCommandLine.length())
                continue;
        }
		// Handle the definition of namespaces
		if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_NAMESPACE
                && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
        {
            if (sCurrentCommand == "namespace" && !getLoop())
            {
                sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("namespace") + 9);
                StripSpaces(sProcCommandLine);

                if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_NAMESPACE;

                if (sProcCommandLine.length())
                {
                    if (sProcCommandLine.find(' ') != string::npos)
                        sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find(' '));

                    if (sProcCommandLine.substr(0, 5) == "this~" || sProcCommandLine == "this")
                        sProcCommandLine.replace(0, 4, sThisNameSpace);

                    if (sProcCommandLine != "main")
                    {
                        sNameSpace = sProcCommandLine;

                        if (sNameSpace[sNameSpace.length() - 1] != '~')
                            sNameSpace += "~";
                    }
                    else
                        sNameSpace = "";
                }
                else
                    sNameSpace = "";

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
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);

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
				if (_option.getUseDebugger())
					_option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);

				resetProcedure(_parser, bSupressAnswer_back);
				throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sProcCommandLine, SyntaxError::invalid_position);
			}
		}

		// Only try to evaluate a procedure, if there's currently no active flow
		// control statement
		if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_PROCEDUREINTERFACE
                && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
        {
            if (!getLoop())
            {
                // Handle procedure calls and plugins in the common
                // virtual procedure interface function
                try
                {
                    int nRetVal = procedureInterface(sProcCommandLine, _parser, _functions, _data, _out, _pData, _script, _option, nth_procedure, 0);

                    // Only those two return values indicate that this line
                    // does contain a procedure or a plugin
                    if ((nRetVal == -2 || nRetVal == 2)
                        && nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    {
                        nByteCode |= ProcedureCommandLine::BYTECODE_PROCEDUREINTERFACE;
                    }

                    if (nRetVal < 0)
                        continue;
                }
                catch (...)
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);

                    resetProcedure(_parser, bSupressAnswer_back);
                    throw;
                }
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
            if (sCurrentCommand == "throw" && !getLoop())
            {
                string sErrorToken;
                if (sProcCommandLine.length() > 7 && (containsStrings(sProcCommandLine) || _data.containsStringVars(sProcCommandLine)))
                {
                    if (_data.containsStringVars(sProcCommandLine))
                        _data.getStringValues(sProcCommandLine);
                    getStringArgument(sProcCommandLine, sErrorToken);
                    sErrorToken += " -nq";
                    string sDummy = "";
                    parser_StringParser(sErrorToken, sDummy, _data, _parser, _option, true);
                }

                if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED)
                    nByteCode |= ProcedureCommandLine::BYTECODE_THROWCOMMAND;

                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                resetProcedure(_parser, bSupressAnswer_back);

                throw SyntaxError(SyntaxError::PROCEDURE_THROW, sProcCommandLine, SyntaxError::invalid_position, sErrorToken);
            }
        }

        if (nCurrentByteCode == ProcedureCommandLine::BYTECODE_NOT_PARSED
            || (nCurrentByteCode & ProcedureCommandLine::BYTECODE_RETURNCOMMAND
                && !(nCurrentByteCode & ProcedureCommandLine::BYTECODE_FLOWCTRLSTATEMENT)))
        {
            if (sCurrentCommand == "return" && !getLoop())
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
                    if (_option.getUseDebugger())
                        _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                    resetProcedure(_parser, bSupressAnswer_back);
                    throw;
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
            if (_option.getUseDebugger())
                _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
            resetProcedure(_parser, bSupressAnswer_back);
            throw;
        }

		sProcCommandLine.clear();
	}



	// Ensure that all loops are closed now
	if (getLoop())
	{
		if (_option.getUseDebugger())
			_option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _varfactory.sLocalTables, _varfactory.nLocalTableSize, sProcCommandLine, sCurrentProcedureName, nCurrentLine);
		resetProcedure(_parser, bSupressAnswer_back);
		throw SyntaxError(SyntaxError::IF_OR_LOOP_SEEMS_NOT_TO_BE_CLOSED, "endprocedure", SyntaxError::invalid_position, "\\$" + sProc + "()");
	}

	// Remove the current procedure from the call stack
	_option._debug.popStackItem();

	if (nFlags & ProcedureCommandLine::FLAG_MASK)
	{
		// reset the print status
		_option.setSystemPrintStatus();
	}

	// Reset this procedure
	resetProcedure(_parser, bSupressAnswer_back);

	// Determine the return value
	if (nReturnType && !_ReturnVal.vNumVal.size() && !_ReturnVal.vStringVal.size())
		_ReturnVal.vNumVal.push_back(1.0);
	return _ReturnVal;
}

// This member function handles the calls for procedures and plugins
int Procedure::procedureInterface(string& sLine, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_procedure, int nth_command)
{
    // Create a new procedure object on the heap
	std::unique_ptr<Procedure> _procedure(new Procedure(*this));
	int nReturn = 1;

	// Handle procedure calls first
	if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos)
	{
	    // Ensure that the current procedure is no inline procedure
	    if (nFlags & ProcedureCommandLine::FLAG_INLINE)
        {
            throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sLine, SyntaxError::invalid_position);
        }

		sLine += " ";
		unsigned int nPos = 0;
		int nProc = 0;

		// Handle all procedure calls one after the other
		while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
		{
			unsigned int nParPos = 0;
			nPos = sLine.find('$', nPos) + 1;
			string __sName = sLine.substr(nPos, sLine.find('(', nPos) - nPos);

			// Add namespaces, where necessary
			if (__sName.find('~') == string::npos)
				__sName = sNameSpace + __sName;

			if (__sName.substr(0, 5) == "this~")
				__sName.replace(0, 4, sThisNameSpace);

			string __sVarList = "";

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
			{
				throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nParPos);
			}

			nParPos += getMatchingParenthesis(sLine.substr(nParPos));
			__sVarList = __sVarList.substr(1, getMatchingParenthesis(__sVarList) - 1);

			if (!isInQuotes(sLine, nPos, true))
			{
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
                    {
                        __sVarList = __sVarList.substr(0, nVarPos) + sNameSpace + __sVarList.substr(nVarPos);
                    }
                }

                // Call the current procedure
				Returnvalue tempreturnval = _procedure->execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion + 1);

				// Evaluate the return value of the called procedure
				if (!_procedure->nReturnType)
					sLine = sLine.substr(0, nPos - 1) + sLine.substr(nParPos + 1);
				else
				{
					replaceReturnVal(sLine, _parser, tempreturnval, nPos - 1, nParPos + 1, "PROC~[" + __sName + "~" + toString(nProc) + "_" + toString((int)nth_procedure) + "_" + toString((int)(nth_command + nth_procedure)) + "]");
				}

				nReturnType = 1;
			}
			nPos += __sName.length() + __sVarList.length() + 1;
		}

		nReturn = 2;
		StripSpaces(sLine);
		_parser.mVarMapPntr = &mVarMap;

		if (nFlags & ProcedureCommandLine::FLAG_MASK)
            _option.setSystemPrintStatus(false);

		if (!sLine.length())
			return -2;
	}
	else if (sLine.find('$') != string::npos)
	{
		int nQuotes = 0;

		// Ensure that this is no "wrong" procedure call
		for (size_t i = 0; i < sLine.length(); i++)
		{
			if (sLine[i] == '"' && (!i || (i && sLine[i - 1] != '\\')))
				nQuotes++;

			if (sLine[i] == '$' && !(nQuotes % 2))
			{
				sLine.clear();
				return -1;
			}
		}
		return 1;
	}

	// Handle plugin calls
	if (!(nFlags & ProcedureCommandLine::FLAG_EXPLICIT) && _procedure->isPluginCmd(sLine))
	{
		if (_procedure->evalPluginCmd(sLine))
		{
			Returnvalue _return;

			// Call the plugin routines
			if (!_option.getSystemPrintStatus())
            {
				_return = _procedure->execute(_procedure->getPluginProcName(), _procedure->getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion + 1);
                if (nFlags & ProcedureCommandLine::FLAG_MASK)
                    _option.setSystemPrintStatus(false);
            }
			else
			{
				_option.setSystemPrintStatus(false);
				_return = _procedure->execute(_procedure->getPluginProcName(), _procedure->getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion + 1);
				_option.setSystemPrintStatus(true);
			}

			_parser.mVarMapPntr = &mVarMap;

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
						_parser.SetVectorVar("_~PLUGIN[" + _procedure->getPluginProcName() + "~" + toString((int)nth_procedure) + "]", _return.vNumVal);
						sLine.replace(sLine.find("<<RETURNVAL>>"), 13, "_~PLUGIN[" + _procedure->getPluginProcName() + "~" + toString((int)(nth_command + nth_procedure)) + "]");
					}
				}
			}
			else
				return -2;
		}
		else
		{
			return -1;
		}
	}

	return nReturn;
}

// Virtual member function allowing to identify and evaluate
// some special procedure commands. Currently it is only used
// for the namespace command.
// The commands "var", "str" and "tab" are recognized but
// not evaluated.
int Procedure::procedureCmdInterface(string& sLine)
{
    // Find the current command
	string sCommand = findCommand(sLine).sString;

	// Try to identify the command
	if (sCommand == "var" || sCommand == "str" || sCommand == "tab")
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

// This member function handles the procedure installation
// process by governing the file stream and passing the
// passed procedure line onwards to the stream.
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
		bool bNamespaceline = false;
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

		if (sLastWrittenProcedureFile.find("|namespace") != string::npos)
			bNamespaceline = true;

		// Create a corresponding folder from the
		// namespace
		if (sCurrentProcedureName.find('~') != string::npos)
		{
			FileSystem _fSys;
			_fSys.setPath(sCurrentProcedureName.substr(0, sCurrentProcedureName.rfind('~')), true, sTokens[5][1]);
		}

		if (sCurrentProcedureName.find('/') != string::npos)
		{
			FileSystem _fSys;
			_fSys.setPath(sCurrentProcedureName.substr(0, sCurrentProcedureName.rfind('/')), true, sTokens[5][1]);
		}

		// If the procedure shall be appended, open the
		// filestream in append mode, otherwise truncate
		// the whole file in advance
		if (bAppend)
			fProcedure.open(sCurrentProcedureName.c_str(), ios_base::out | ios_base::app);
		else
			fProcedure.open(sCurrentProcedureName.c_str(), ios_base::out | ios_base::trunc);

		// Ensure that the file stream could be opened
		if (fProcedure.fail())
		{
			fProcedure.close();
			return false;
		}
		else
		{
			string sProcName = "";

			if (sFileName.find('~') != string::npos)
				sProcName = sFileName.substr(sFileName.rfind('~') + 1);
			else
				sProcName = sFileName;

			sLastWrittenProcedureFile = sCurrentProcedureName;
			bWritingTofile = true;

            // Add a warning that the procedures after the
            // first procedure are file static
			if (bAppend && !bNamespaceline)
			{
				unsigned int nLength = _lang.get("PROC_NAMESPACE_THISFILE_MESSAGE").length();
				fProcedure << endl << endl
						   << "#**" << std::setfill('*') << std::setw(nLength + 2) << "**" << endl
						   << " * NAMESPACE: THISFILE" << std::setfill(' ') << std::setw(nLength - 17) << " *" << endl
						   << " * " << _lang.get("PROC_NAMESPACE_THISFILE_MESSAGE") << " *" << endl
						   << " **" << std::setfill('*') << std::setw(nLength + 3) << "**#" << endl << endl << endl;
				sLastWrittenProcedureFile += "|namespace";
			}
			else if (bAppend)
				fProcedure << endl << endl;

			// Write the procedure head comment
			unsigned int nLength = _lang.get("COMMON_PROCEDURE").length();
            fProcedure << "#*********" << std::setfill('*') << std::setw(nLength + 2) << "***" << std::setfill('*') << std::setw(max(21u, sProcName.length() + 2)) << "*" << endl;
			fProcedure << " * NUMERE-" << toUpperCase(_lang.get("COMMON_PROCEDURE")) << ": $" << sProcName << "()" << endl;
			fProcedure << " * =======" << std::setfill('=') << std::setw(nLength + 2) << "===" << std::setfill('=') << std::setw(max(21u, sProcName.length() + 2)) << "=" << endl;
			fProcedure << " * " << _lang.get("PROC_ADDED_DATE") << ": " << getTimeStamp(false) << " *#" << endl;
			fProcedure << endl;
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
				 || sProcedureLine.find(" endif") != string::npos
				 || sProcedureLine.find(" endif") != string::npos
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
							|| sProcedureLine.substr(n, 8) == " default "
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
		fProcedure << "#* " << _lang.get("PROC_END_OF_PROCEDURE") << endl;
		fProcedure << " * " << _lang.get("PROC_FOOTER") << endl;
		fProcedure << " * https://sites.google.com/site/numereframework/" << endl;
		fProcedure << " **" << std::setfill('*') << std::setw(_lang.get("PROC_FOOTER").length() + 1) << "#" << endl;

		fProcedure.close();

		// This ensures that all blocks were closed
		if (nthBlock)
		{
			throw SyntaxError(SyntaxError::IF_OR_LOOP_SEEMS_NOT_TO_BE_CLOSED, sProcedureLine, SyntaxError::invalid_position, sCurrentProcedureName);
		}
		sCurrentProcedureName = "";
	}

	StripSpaces(sAppendedLine);

	// If there are currently contents cached,
	// call this member function recursively.
	if (sAppendedLine.length())
		return writeProcedure(sAppendedLine);

	return true;
}

// This virtual member function checks, whether the procedures in the
// current line are declared as inline
bool Procedure::isInline(const string& sProc)
{
    // No procedures?
    if (sProc.find('$') == string::npos)
		return false;

	if (sProc.find('$') != string::npos && sProc.find('(', sProc.find('$')) != string::npos)
	{
		unsigned int nPos = 0;

		// Examine all procedures, which may be found in the
		// current command string
		while (sProc.find('$', nPos) != string::npos && sProc.find('(', sProc.find('$', nPos)) != string::npos)
		{
		    // Extract the name of the procedure
			nPos = sProc.find('$', nPos) + 1;
			string __sName = sProc.substr(nPos, sProc.find('(', nPos) - nPos);
			string __sProcName;

			if (__sName.find('~') == string::npos)
				__sName = sNameSpace + __sName;

			if (__sName.substr(0, 5) == "this~")
				__sName.replace(0, 4, sThisNameSpace);

            // Handle the special case of absolute paths
			if (sProc[nPos] == '\'')
			{
				__sName = sProc.substr(nPos + 1, sProc.find('\'', nPos + 1) - nPos - 1);
			}

			// Only evaluate the current match, if it is not part of a string
			if (!isInQuotes(sProc, nPos, true))
			{
			    // Create a valid filename first
				string __sFileName = __sName;

				// Now remove the namespace stuff
				if (__sName.find('~') != string::npos)
                    __sProcName = __sName.substr(__sName.rfind('~')+1);
                else
                    __sProcName = __sName;

				// Handle namespaces
				if (__sFileName.find('~') != string::npos)
				{
					if (__sFileName.substr(0, 9) == "thisfile~")
					{
						if (sProcNames.length())
							__sFileName = sProcNames.substr(sProcNames.rfind(';') + 1);
						else
						{
							throw SyntaxError(SyntaxError::PRIVATE_PROCEDURE_CALLED, sProc, SyntaxError::invalid_position, "thisfile");
						}
					}
					else
					{
						for (unsigned int i = 0; i < __sFileName.length(); i++)
						{
							if (__sFileName[i] == '~')
							{
								if (__sFileName.length() > 5 && i >= 4 && __sFileName.substr(i - 4, 5) == "main~")
									__sFileName = __sFileName.substr(0, i - 4) + __sFileName.substr(i + 1);
								else
									__sFileName[i] = '/';
							}
						}
					}
				}

				// Use the filesystem to determine a valid file name
				__sFileName = ValidFileName(__sFileName, ".nprc");

				if (__sFileName[1] != ':')
				{
					__sFileName = "<procpath>/" + __sFileName;
					__sFileName = ValidFileName(__sFileName, ".nprc");
				}

				// Here happens the hard work: get a procedure element from the library, find
				// the procedure definition line, obtain it and examine the already parsed
				// flags of this procedure
				ProcedureElement* element = NumeReKernel::ProcLibrary.getProcedureContents(__sFileName);
                int nProcedureLine = element->gotoProcedure("$" + __sProcName);
                if (nProcedureLine < 0)
                    throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, sProc, SyntaxError::invalid_position, "$" + __sName);
                pair<int, ProcedureCommandLine> header = element->getCurrentLine(nProcedureLine);

                // Only return false, if this procedure is NOT inline,
                // otherwise continue
                if (!(header.second.getFlags() & ProcedureCommandLine::FLAG_INLINE))
                    return false;
			}
		}
	}
	else
		return false;

    // All procedures were declared as inline
	return true;
}

// This virtual member function handles the gathering of all
// relevant information for the debugger for the currently
// found break point
void Procedure::evalDebuggerBreakPoint(Parser& _parser, Settings& _option)
{
	// if the stack is empty, it has to be a breakpoint from a script
	// This is only valid, if the script contained flow control statements
	if (!_option._debug.getStackSize())
	{
		NumeReKernel::evalDebuggerBreakPoint(_option, "", &_parser);
		return;
	}

	// Gather all information needed by the debugger
	_option._debug.gatherInformations(sVars, nVarSize, dVars, sStrings, nStrSize, sTables, nTabSize, "", sCurrentProcedureName, nCurrentLine);

	// Let the kernel display the debugger window and jump to the
	// corresponding line in the procedure file
	NumeReKernel::showDebugEvent(_lang.get("DBG_HEADLINE"), _option._debug.getModuleInformations(), _option._debug.getStackTrace(), _option._debug.getNumVars(), _option._debug.getStringVars(), _option._debug.getTables());
	NumeReKernel::gotoLine(_option._debug.getErrorModule(), _option._debug.getLineNumber() + 1);

	// Rese the debugger and wait for the user action
	_option._debug.resetBP();
	NumeReKernel::waitForContinue();
	return;
}

// This member function replaces the procedure occurence
// between the both passed positions using akronymed version
// of the called procedure. It also declares the numerical
// return value as a vector to the parser.
void Procedure::replaceReturnVal(string& sLine, Parser& _parser, const Returnvalue& _return, unsigned int nPos, unsigned int nPos2, const string& sReplaceName)
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
	}
	else if (_return.isNumeric())
	{
	    // Numerical value, use the procedure name
	    // to derive a vector name and declare the
	    // corresponding vector
		string __sRplcNm = sReplaceName;

		if (sReplaceName.find('\\') != string::npos || sReplaceName.find('/') != string::npos || sReplaceName.find(':') != string::npos)
		{
			for (unsigned int i = 0; i < __sRplcNm.length(); i++)
			{
				if (__sRplcNm[i] == '\\' || __sRplcNm[i] == '/' || __sRplcNm[i] == ':')
					__sRplcNm[i] = '~';
			}
		}

		_parser.SetVectorVar(__sRplcNm, _return.vNumVal);
		sLine = sLine.substr(0, nPos) + __sRplcNm +  sLine.substr(nPos2);
	}
	else
		sLine = sLine.substr(0, nPos) + "nan" + sLine.substr(nPos2);
	return;
}

// This member function sets the current procedure object
// to its original state. It will be called at the end of
// the executed procedure
void Procedure::resetProcedure(Parser& _parser, bool bSupressAnswer)
{
	sCallingNameSpace = "main";
	sNameSpace.clear();
	sThisNameSpace.clear();
	mVarMap.clear();
	NumeReKernel::bSupressAnswer = bSupressAnswer;
	_parser.mVarMapPntr = 0;

	// Remove the last procedure in the current stack
	if (sProcNames.length())
	{
		sProcNames.erase(sProcNames.rfind(';'));
	}

	sVars = nullptr;
	sStrings = nullptr;
	dVars = nullptr;

	nVarSize = 0;
	nStrSize = 0;

	return;
}

// This member function extracts the namespace of the currently
// executed procedure
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

// This method handles the definitions of local variables. It will
// return true, if a definition occured, otherwise false. The return
// value will be used to create a corresponding byte code
bool Procedure::handleVariableDefinitions(string& sProcCommandLine, const string& sCommand, ProcedureVarFactory& _varfactory)
{
    // Is it a variable declaration?
    if (sCommand == "var" && sProcCommandLine.length() > 6)
    {
        _varfactory.createLocalVars(sProcCommandLine.substr(sProcCommandLine.find("var") + 3));

        sProcCommandLine = "";
        sVars = _varfactory.sLocalVars;
        dVars = _varfactory.dLocalVars;
        nVarSize = _varfactory.nLocalVarMapSize;
        return true;
    }

    // Is it a string declaration?
    if (sCommand == "str" && sProcCommandLine.length() > 6)
    {
        _varfactory.createLocalStrings(sProcCommandLine.substr(sProcCommandLine.find("str") + 3));

        sProcCommandLine = "";
        sStrings = _varfactory.sLocalStrings;
        nStrSize = _varfactory.nLocalStrMapSize;
        return true;
    }

    // Is it a table declaration?
    if (sCommand == "tab" && sProcCommandLine.length() > 6)
    {
        _varfactory.createLocalTables(sProcCommandLine.substr(sProcCommandLine.find("tab") + 3));
        sTables = _varfactory.sLocalTables;
        nTabSize = _varfactory.nLocalTableSize;
        sProcCommandLine = "";
        return true;
    }

    // No local variable declaration in this command line
    return false;
}

// This member function reads the lines from the included file. It
// acts quite independent from the rest of the procedure
void Procedure::readFromInclude(ifstream& fInclude, int nIncludeType, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_procedure)
{
    string sProcCommandLine;
    bool bSkipNextLine = false;
    bool bAppendNextLine = false;
    bool bBlockComment = false;
    int nCurrentByteCode = 0;

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
                int nReturn = procedureInterface(sProcCommandLine, _parser, _functions, _data, _out, _pData, _script, _option, nth_procedure);

                if (nReturn == -1)
                {
                    throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sProcCommandLine, SyntaxError::invalid_position);
                }
                else if (nReturn == -2)
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

// This member function handles the script include syntax, which
// one may use in other procedures. The included file is indicated
// with an "@" and its file name afterwards. This function will
// decode this syntax and open the corresponding file stream.
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


