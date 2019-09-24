/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2016  Erik Haenel et al.

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



#include "debugger.hpp"
#include "../../kernel.hpp"
#include "../utils/tools.hpp"
#include "../procedure/procedurevarfactory.hpp"

// constructor
NumeReDebugger::NumeReDebugger()
{
    nLineNumber = string::npos;
    sErraticCommand = "";
    sErraticModule = "";
    sErrorMessage = "";
    bAlreadyThrown = false;
    bDebuggerActive = false;
    bExceptionHandled = false;
}

// This member function shows the debugger with the
// corresponding error message obtained by the passed
// exception_ptr
void NumeReDebugger::showError(exception_ptr e)
{
    if (!bDebuggerActive)
        return;

    // Rethrow the obtained exception to determine its
    // type and its message
    try
    {
        rethrow_exception(e);
    }
    catch (mu::Parser::exception_type& e)
    {
        // Parser exception
        sErrorMessage = _lang.get("ERR_MUP_HEAD") + "\n\n" + e.GetMsg();
        showError(_lang.get("ERR_MUP_HEAD_DBG"));
    }
    catch (const std::exception& e)
    {
        // C++ Standard exception
        sErrorMessage = _lang.get("ERR_STD_INTERNAL_HEAD") + "\n\n" + e.what();
        showError(_lang.get("ERR_STD_INTERNAL_HEAD_DBG"));
    }
    catch (SyntaxError& e)
    {
        // Internal exception
        if (e.errorcode == SyntaxError::PROCESS_ABORTED_BY_USER)
        {
            // Do nothing, if the user pressed ESC
            return;
        }
        else
        {
            if (e.getToken().length() && (e.errorcode == SyntaxError::PROCEDURE_THROW || e.errorcode == SyntaxError::LOOP_THROW))
            {
                sErrorMessage = _lang.get("ERR_NR_HEAD") + "\n\n" + e.getToken();
            }
            else
            {
                sErrorMessage = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(), toString(e.getIndices()[0]), toString(e.getIndices()[1]), toString(e.getIndices()[2]),
                                          toString(e.getIndices()[3]));

                if (sErrorMessage.substr(0, 7) == "ERR_NR_")
                {
                    sErrorMessage = _lang.get("ERR_GENERIC_0", toString((int)e.errorcode));
                }

                sErrorMessage = _lang.get("ERR_NR_HEAD") + "\n\n" + sErrorMessage;
            }

            showError(_lang.get("ERR_NR_HEAD_DBG"));
        }
    }
    catch (...)
    {
        return;
    }
}

// This member function shows the debugger with the
// passed error message
void NumeReDebugger::showError(const string& sTitle)
{
    if (bExceptionHandled)
        return;

    // Convert dollars into line breaks
    formatMessage();

    bExceptionHandled = true;
    showEvent(sTitle);
    reset();
}

// This member function shows the debugger with the
// corresponding error message obtained by the passed
// SyntaxError object and throws the corresponding
// exception afterwards
void NumeReDebugger::throwException(SyntaxError error)
{
    // do not show the debugger, if the user simply pressed ESC
    if (error.errorcode != SyntaxError::PROCESS_ABORTED_BY_USER)
    {
        if (error.getToken().length() && (error.errorcode == SyntaxError::PROCEDURE_THROW || error.errorcode == SyntaxError::LOOP_THROW))
        {
            sErrorMessage = _lang.get("ERR_NR_HEAD") + "\n\n" + error.getToken();
        }
        else
        {
            sErrorMessage = _lang.get("ERR_NR_" + toString((int)error.errorcode) + "_0_*", error.getToken(), toString(error.getIndices()[0]), toString(error.getIndices()[1]), toString(error.getIndices()[2]),
                                      toString(error.getIndices()[3]));

            if (sErrorMessage.substr(0, 7) == "ERR_NR_")
            {
                sErrorMessage = _lang.get("ERR_GENERIC_0", toString((int)error.errorcode));
            }
        }

        sErrorMessage = _lang.get("ERR_NR_HEAD") + "\n\n" + sErrorMessage;

        showError(_lang.get("ERR_NR_HEAD_DBG"));
    }

    throw error;
}

// This member function shows the debugger for the
// current breakpoint and returns the debugger code
// obtained from the user
int NumeReDebugger::showBreakPoint()
{
    int nDebuggerCode = showEvent(_lang.get("DBG_HEADLINE"));
    resetBP();

    return nDebuggerCode;
}

// This private member function shows the debugger for
// the current selected event and returns the debugger
// code obtained from the user
int NumeReDebugger::showEvent(const string& sTitle)
{
    if (!bDebuggerActive)
        return NumeReKernel::DEBUGGER_CONTINUE;

    // Show the debugger, if valid information is
    // available
    if (validDebuggingInformations())
    {
        NumeReKernel::showDebugEvent(sTitle, getStackTrace());
        NumeReKernel::gotoLine(sErraticModule, nLineNumber);

        // Return the obtained debugger code
        return NumeReKernel::waitForContinue();
    }

    // Always return CONTINUE - we don't want to
    // block the kernel due to lacking debugger
    // information
    return NumeReKernel::DEBUGGER_CONTINUE;
}

// This function replaces unmasked dollars with regular
// line break characters and also removes the masking
// characters in front of masked dollars
void NumeReDebugger::formatMessage()
{
    for (size_t i = 1; i < sErrorMessage.length(); i++)
    {
        if (sErrorMessage[i] == '$' && sErrorMessage[i-1] != '\\')
            sErrorMessage[i] = '\n';
        else if (sErrorMessage[i] == '$' && sErrorMessage[i-1] == '\\')
        {
            sErrorMessage.erase(i-1, 1);
            i--;
        }
    }
}

// This private member function decodes the type of the arguments
// by looking at their values and apply some guesses
string NumeReDebugger::decodeType(string& sArgumentValue)
{
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    // Only try to decode the arguments, if the user decided to
    // do so
    if (!NumeReKernel::getInstance()->getSettings().getTryToDecodeProcedureArguments())
        return "\t1 x 1\t(...)\t";

    // Is the current argument value a table?
    if (_data.isTable(sArgumentValue) || sArgumentValue.substr(0, 5) == "data(")
    {
        string sCache = sArgumentValue.substr(0, sArgumentValue.find('('));

        // Replace the value with its actual value and mark the
        // argument type as reference
        sArgumentValue = "{" + toString(_data.min(sCache, "")[0], 5) + ", ..., " + toString(_data.max(sCache, "")[0], 5) + "}";
        return "\t" + toString(_data.getLines(sCache, false)) + " x " + toString(_data.getCols(sCache, false)) + "\t(&) double\t";
    }

    // Is the current argument value a cluster?
    if (_data.isCluster(sArgumentValue))
    {
        NumeRe::Cluster& cluster = _data.getCluster(sArgumentValue.substr(0, sArgumentValue.find('{')));

        // Replace the value with its actual value and mark the
        // argument type as reference
        sArgumentValue = cluster.getShortVectorRepresentation();
        return "\t" + toString(cluster.size()) + " x 1\t(&) cluster\t";
    }

    // Is the current value surrounded from braces?
    if (sArgumentValue.front() == '{' && sArgumentValue.back() == '}')
    {
        size_t nDim = 0;
        string sArg = sArgumentValue.substr(1, sArgumentValue.length()-2);

        while (sArg.length())
        {
            nDim++;
            getNextArgument(sArg, true);
        }

        return "\t" + toString(nDim) + " x 1\tcluster\t";
    }

    // Is the current argument a string variable?
    if (_data.getStringVars().find(sArgumentValue) != _data.getStringVars().end())
    {
        // Replace the value with its actual value and mark the
        // argument type as reference
        sArgumentValue = "\"" + (_data.getStringVars().find(sArgumentValue)->second) + "\"";
        return "\t1 x 1\t(&) string\t";
    }

    // Equals the current argument the string table?
    if (sArgumentValue.substr(0, 7) == "string(")
    {
        // Replace the value with its actual value and mark the
        // argument type as reference
        sArgumentValue = "{\"" + replaceControlCharacters(_data.minString()) + "\", ..., \"" + replaceControlCharacters(_data.maxString()) + "\"}";
        return "\t" + toString(_data.getStringSize()) + " x " + toString(_data.getStringCols()) + "\t(&) string\t";
    }

    // Is the current argument a numerical variable?
    if (_parser.GetVar().find(sArgumentValue) != _parser.GetVar().end())
    {
        // Replace the value with its actual value and mark the
        // argument type as reference
        sArgumentValue = toString(*(_parser.GetVar().find(sArgumentValue)->second), 5);
        return "\t1 x 1\t(&) double\t";
    }

    // Is it a constant numerical expression or value?
    if (sArgumentValue.find_first_not_of("0123456789.eE-+*/^!&|<>=(){},") == string::npos)
        return "\t1 x 1\tdouble\t";

    // Is the current argument a string expression?
    // This heuristic is potentially wrong
    if (containsStrings(sArgumentValue))
        return "\t1 x 1\tstring\t";

    // Does it not contain any special characters?
    if (sArgumentValue.find_first_of("$\"#?:") == string::npos)
        return "\t1 x 1\tdouble\t";

    // We cannot decode it
    return "\t1 x 1\t(...)\t";
}

// This member function can be used to select a
// specific element in the current stack trace to
// read the state at this position
bool NumeReDebugger::select(size_t nStackElement)
{
    // Ensure that the selected element exists
    if (nStackElement >= vStackTrace.size())
        return false;

    // Get the procedure pointer
    Procedure* _curProcedure = vStackTrace[nStackElement].second;

    // Do nothing, if it does not exist
    // (for some reason)
    if (!_curProcedure)
        return false;

    // Clear the breakpoint information
    resetBP();

    // If the current procedure is evaluating a flow control
    // statement, obtain the corresponding information here
    if (_curProcedure->bEvaluatingFlowControlStatements)
    {
        gatherLoopBasedInformations(_curProcedure->getCurrentCommand(), _curProcedure->getCurrentLineNumber(), _curProcedure->mVarMap, _curProcedure->vVarArray, _curProcedure->sVarArray, _curProcedure->nVarArray);
    }

    // Obtain the remaining information here
    gatherInformations(_curProcedure->_varFactory, _curProcedure->sProcCommandLine, _curProcedure->sCurrentProcedureName, _curProcedure->GetCurrentLine());

    // Jump to the selected file and the selected line number
    NumeReKernel::gotoLine(sErraticModule, nLineNumber);

    return true;
}

// This member function resets the debugger after a
// thrown and displayed error
void NumeReDebugger::reset()
{
    vStackTrace.clear();
    sErrorMessage.clear();
    resetBP();
    return;
}

// This member function resets the debugger after
// an evaluated breakpoint. This excludes resetting
// the stacktrace
void NumeReDebugger::resetBP()
{
    nLineNumber = string::npos;
    sErraticCommand = "";
    sErraticModule = "";
    mLocalVars.clear();
    mLocalStrings.clear();
    mLocalTables.clear();
    mLocalClusters.clear();
    mArguments.clear();
    bAlreadyThrown = false;
    return;
}

// This member function adds a new stack item to the
// monitored stack. Additionally, it cleanes the procedure's
// names for the stack trace display
void NumeReDebugger::pushStackItem(const string& sStackItem, Procedure* _currentProcedure)
{
    vStackTrace.push_back(pair<string, Procedure*>(sStackItem, _currentProcedure));

    // Insert a leading backslash, if it missing
    for (unsigned int i = 0; i < vStackTrace.back().first.length(); i++)
    {
        if ((!i && vStackTrace.back().first[i] == '$') || (i && vStackTrace.back().first[i] == '$' && vStackTrace.back().first[i-1] != '\\'))
            vStackTrace.back().first.insert(i,1,'\\');
    }

    // Insert the single quotation marks for explicit procedure
    // paths
    if (vStackTrace.back().first.find('/') != string::npos && vStackTrace.back().first.find('/') < vStackTrace.back().first.find('('))
    {
        vStackTrace.back().first.insert(vStackTrace.back().first.find('$')+1, "'");
        vStackTrace.back().first.insert(vStackTrace.back().first.find('('), "'");
    }

    return;
}

// This member function removes the last item from the stack
void NumeReDebugger::popStackItem()
{
    if (vStackTrace.size())
        vStackTrace.pop_back();

    return;
}

// This member function gathers all information from the current
// workspace and stores them internally to display them to the user.
// This member function is a wrapper for the more complicated signature
// further below
void NumeReDebugger::gatherInformations(ProcedureVarFactory* _varFactory, const string& _sErraticCommand, const string& _sErraticModule, unsigned int _nLineNumber)
{
    if (!bDebuggerActive)
        return;

    gatherInformations(_varFactory->sLocalVars, _varFactory->nLocalVarMapSize, _varFactory->dLocalVars, _varFactory->sLocalStrings, _varFactory->nLocalStrMapSize, _varFactory->sLocalTables,
                       _varFactory->nLocalTableSize, _varFactory->sLocalClusters, _varFactory->nLocalClusterSize, _varFactory->sArgumentMap, _varFactory->nArgumentMapSize, _sErraticCommand, _sErraticModule,
                       _nLineNumber);
}

// This member function gathers all information from the current
// workspace and stores them internally to display them to the user
void NumeReDebugger::gatherInformations(string** sLocalVars, size_t nLocalVarMapSize, double* dLocalVars, string** sLocalStrings, size_t nLocalStrMapSize, string** sLocalTables,
                                        size_t nLocalTableMapSize, string** sLocalClusters, size_t nLocalClusterMapSize, string** sArgumentMap, size_t nArgumentMapSize, const string& _sErraticCommand,
                                        const string& _sErraticModule, unsigned int _nLineNumber)
{
    if (!bDebuggerActive)
        return;

    if (bAlreadyThrown)
        return;

    // Get the instance of the kernel
    NumeReKernel* instance = NumeReKernel::getInstance();

    // If the instance is zero, something really bad happened
    if (!instance)
        return;

    // Store the command line containing the error
    if (!sErraticCommand.length())
        sErraticCommand = _sErraticCommand;

    // Removes the leading and trailing whitespaces
    StripSpaces(sErraticCommand);

    sErraticModule = _sErraticModule;

    if (nLineNumber == string::npos)
        nLineNumber = _nLineNumber;

    bAlreadyThrown = true;

    // Store the local numerical variables and replace their
    // occurence with their definition in the command lines
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        // Replace the occurences
        if (sLocalVars[i][0] != sLocalVars[i][1])
        {
            while (sErraticCommand.find(sLocalVars[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalVars[i][1]), sLocalVars[i][1].length(), sLocalVars[i][0]);
        }

        mLocalVars[sLocalVars[i][0] + "\t" + sLocalVars[i][1]] = dLocalVars[i];
    }

    // Store the local string variables and replace their
    // occurence with their definition in the command lines
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        // Replace the occurences
        if (sLocalStrings[i][0] != sLocalStrings[i][1])
        {
            while (sErraticCommand.find(sLocalStrings[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalStrings[i][1]), sLocalStrings[i][1].length(), sLocalStrings[i][0]);
        }

        mLocalStrings[sLocalStrings[i][0] + "\t" + sLocalStrings[i][1]] = replaceControlCharacters(instance->getData().getStringVars().at(sLocalStrings[i][1]));
    }

    // Store the local tables and replace their
    // occurence with their definition in the command lines
    for (unsigned int i = 0; i < nLocalTableMapSize; i++)
    {
        // Replace the occurences
        if (sLocalTables[i][0] != sLocalTables[i][1])
        {
            while (sErraticCommand.find(sLocalTables[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalTables[i][1]), sLocalTables[i][1].length(), sLocalTables[i][0]);
        }

        string sTableData;

        // Extract the minimal and maximal values of the tables
        // to display them in the variable viewer panel
        if (sLocalTables[i][1] == "string")
        {
            sTableData = toString(instance->getData().getStringElements()) + " x " + toString(instance->getData().getStringCols());
            sTableData += "\tstring\t{\"" + replaceControlCharacters(instance->getData().minString()) + "\", ..., \"" + replaceControlCharacters(instance->getData().maxString()) + "\"}\tstring()";
        }
        else
        {
            sTableData = toString(instance->getData().getLines(sLocalTables[i][1], false)) + " x " + toString(instance->getData().getCols(sLocalTables[i][1], false));
            sTableData += "\tdouble\t{" + toString(instance->getData().min(sLocalTables[i][1], "")[0], 5) + ", ..., " + toString(instance->getData().max(sLocalTables[i][1], "")[0], 5) + "}\t" + sLocalTables[i][1] + "()";
        }

        mLocalTables[sLocalTables[i][0] + "()"] = sTableData;
    }

    // Store the local clusters and replace their
    // occurence with their definition in the command lines
    for (unsigned int i = 0; i < nLocalClusterMapSize; i++)
    {
        // Replace the occurences
        if (sLocalClusters[i][0] != sLocalClusters[i][1])
        {
            while (sErraticCommand.find(sLocalClusters[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalClusters[i][1]), sLocalClusters[i][1].length(), sLocalClusters[i][0].substr(0, sLocalClusters[i][0].length()-1));
        }

        string sTableData;

        // Extract the minimal and maximal values of the tables
        // to display them in the variable viewer panel
        sTableData = toString(instance->getData().getCluster(sLocalClusters[i][1]).size()) + " x 1";
        sTableData += "\tcluster\t" + instance->getData().getCluster(sLocalClusters[i][1]).getShortVectorRepresentation() + "\t" + sLocalClusters[i][1] + "}";

        mLocalClusters[sLocalClusters[i][0] + "}"] = sTableData;
    }

    // Store the arguments
    for (unsigned int i = 0; i < nArgumentMapSize; i++)
    {
        if (sArgumentMap[i][0].back() == '(')
            mArguments[sArgumentMap[i][0] + ")\t" + sArgumentMap[i][1]] = sArgumentMap[i][1];
        else if (sArgumentMap[i][0].back() == '{')
            mArguments[sArgumentMap[i][0] + "}\t" + sArgumentMap[i][1]] = sArgumentMap[i][1];
        else
            mArguments[sArgumentMap[i][0] + "\t" + sArgumentMap[i][1]] = sArgumentMap[i][1];
    }

    return;
}

// This member funciton gathers the necessary debugging informations
// from the current executed control flow block
void NumeReDebugger::gatherLoopBasedInformations(const string& _sErraticCommand, unsigned int _nLineNumber, map<string, string>& mVarMap, double** vVarArray, string* sVarArray, int nVarArray)
{
    if (!bDebuggerActive)
        return;

    if (bAlreadyThrown)
        return;

    // Store command line and line number
    sErraticCommand = _sErraticCommand;

    if (sErraticCommand.substr(sErraticCommand.find_first_not_of(' '), 2) == "|>")
    {
        sErraticCommand.erase(sErraticCommand.find_first_not_of(' '), 2);
    }


    nLineNumber = _nLineNumber;

    // store variable names and replace their occurences with
    // their definitions
    for (int i = 0; i < nVarArray; i++)
    {
        for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
        {
            if (iter->second == sVarArray[i])
            {
                // Store the variables
                mLocalVars[iter->first + "\t" + iter->second] = vVarArray[i][0];

                // Replace the variables
                while (sErraticCommand.find(iter->second) != string::npos)
                    sErraticCommand.replace(sErraticCommand.find(iter->second), (iter->second).length(), iter->first);
            }
        }
    }

    return;
}

// This member function returns the module informations as a vector
vector<string> NumeReDebugger::getModuleInformations()
{
    vector<string> vModule;
    vModule.push_back(sErraticCommand);
    vModule.push_back(sErraticModule);
    vModule.push_back(toString(nLineNumber+1));
    vModule.push_back(sErrorMessage);
    return vModule;
}

// This member function returns the stack trace as a vector
vector<string> NumeReDebugger::getStackTrace()
{
    vector<string> vStack;

    // Return a corresponding message, if the stack is empty
    // Append the line number and the file name, though
    if (!vStackTrace.size())
    {
        vStack.push_back(_lang.get("DBG_STACK_EMPTY") + "\t" + sErraticModule + "\t" + toString(nLineNumber+1));
        return vStack;
    }

    // Return the stack and append the current procedure
    // name and the line number
    for (int i = vStackTrace.size()-1; i >= 0; i--)
    {
        Procedure* _curProc = vStackTrace[i].second;
        vStack.push_back("$" + vStackTrace[i].first + "\t" + _curProc->sCurrentProcedureName + "\t" + toString(_curProc->GetCurrentLine()+1));
    }

    return vStack;
}

// This member function returns the numerical variables as a vector
vector<string> NumeReDebugger::getNumVars()
{
    vector<string> vNumVars;

    for (auto iter = mLocalVars.begin(); iter != mLocalVars.end(); ++iter)
    {
        vNumVars.push_back((iter->first).substr(0, (iter->first).find('\t')) + "\t1 x 1\tdouble\t" + toString(iter->second, 7) + (iter->first).substr((iter->first).find('\t')));
    }

    return vNumVars;
}

// This member function returns the string variables as a vector
vector<string> NumeReDebugger::getStringVars()
{
    vector<string> vStringVars;

    for (auto iter = mLocalStrings.begin(); iter != mLocalStrings.end(); ++iter)
    {
        vStringVars.push_back((iter->first).substr(0, (iter->first).find('\t')) + "\t1 x 1\tstring\t\"" + iter->second + "\"" + (iter->first).substr((iter->first).find('\t')));
    }

    return vStringVars;
}

// This member function returns the tables as a vector
vector<string> NumeReDebugger::getTables()
{
    vector<string> vTables;

    for (auto iter = mLocalTables.begin(); iter != mLocalTables.end(); ++iter)
    {
        vTables.push_back(iter->first + "\t" + iter->second);
    }

    return vTables;
}

// This member function returns the clusters as a vector
vector<string> NumeReDebugger::getClusters()
{
    vector<string> vClusters;

    for (auto iter = mLocalClusters.begin(); iter != mLocalClusters.end(); ++iter)
    {
        vClusters.push_back(iter->first + "\t" + iter->second);
    }

    return vClusters;
}

// This member function returns the procedure argument as a vector
vector<string> NumeReDebugger::getArguments()
{
    vector<string> vArguments;

    for (auto iter = mArguments.begin(); iter != mArguments.end(); ++iter)
    {
        string sValue = iter->second;
        vArguments.push_back((iter->first).substr(0, (iter->first).find('\t')) + decodeType(sValue) + sValue + (iter->first).substr((iter->first).find('\t')));
    }

    return vArguments;
}

// This member function returns the current global
// variables as a vector
vector<string> NumeReDebugger::getGlobals()
{
    vector<string> vGlobals;

    if (!vStackTrace.size())
        return vGlobals;

    map<string,string> mGlobals;

    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    // Is there valid data?
    if (_data.isValid())
    {
        mGlobals["data()"] = toString(_data.getLines("data", false)) + " x " + toString(_data.getCols("data", false))
                             + "\tdouble\t{" + toString(_data.min("data", "")[0], 5) + ", ...," + toString(_data.max("data", "")[0], 5) + "}";
    }

    // Is there anything in the string object?
    if (_data.getStringElements())
    {
        mGlobals["string()"] = toString(_data.getStringElements()) + " x " + toString(_data.getStringCols())
                               + "\tstring\t{\"" + _data.minString() + "\", ...,\"" + _data.maxString() + "\"}";
    }

    // List all relevant caches
    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
    {
        if (iter->first.substr(0, 2) != "_~")
        {
            mGlobals[iter->first + "()"] = toString(_data.getLines(iter->first, false)) + " x " + toString(_data.getCols(iter->first, false))
                                           + "\tdouble\t{" + toString(_data.min(iter->first, "")[0], 5) + ", ..., " + toString(_data.max(iter->first, "")[0], 5) + "}";
        }
    }

    // List all relevant clusters
    for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
    {
        if (iter->first.substr(0, 2) != "_~")
        {
            mGlobals[iter->first + "{}"] = toString(iter->second.size()) + " x 1" + "\tcluster\t" + iter->second.getShortVectorRepresentation();
        }
    }

    // List all relevant string variables
    for (auto iter = _data.getStringVars().begin(); iter != _data.getStringVars().end(); ++iter)
    {
        if (iter->first.substr(0, 2) != "_~")
        {
            mGlobals[iter->first] = "1 x 1\tstring\t\"" + iter->second + "\"";
        }
    }

    // List all relevant numerical variables
    for (auto iter = _parser.GetVar().begin(); iter != _parser.GetVar().end(); ++iter)
    {
        if (iter->first.substr(0, 2) != "_~")
        {
            mGlobals[iter->first] = "1 x 1\tdouble\t" + toString(*iter->second, 5);
        }
    }

    // Push everything into the vector
    for (auto iter = mGlobals.begin(); iter != mGlobals.end(); ++iter)
    {
        vGlobals.push_back(iter->first + "\t" + iter->second + "\t" + iter->first);
    }

    // Return the vector filled with the global variables
    return vGlobals;
}


