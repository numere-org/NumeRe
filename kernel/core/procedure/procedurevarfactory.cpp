/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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


#include "procedurevarfactory.hpp"
#include "procedure.hpp"
#include "../../kernel.hpp"

#define FLAG_EXPLICIT 1
#define FLAG_INLINE 2


/////////////////////////////////////////////////
/// \brief Static helper function to detect free
/// operators in a procedure argument. Those
/// arguments need to be surrounded by parentheses
/// as long as arguments are not converted into
/// real variables.
///
/// \param sString const string&
/// \return bool
///
/////////////////////////////////////////////////
static bool containsFreeOperators(const string& sString)
{
    size_t nQuotes = 0;
    static string sOperators = "+-*/&|?!^<>=";

    for (size_t i = 0; i < sString.length(); i++)
    {
        if (sString[i] == '"' && (!i || sString[i-1] != '\\'))
            nQuotes++;

        if (nQuotes % 2)
            continue;

        if (sString[i] == '(' || sString[i] == '[' || sString[i] == '{')
        {
            size_t nMatch;

            if ((nMatch = getMatchingParenthesis(sString.substr(i))) != string::npos)
                i += nMatch;
        }
        else if (sOperators.find(sString[i]) != string::npos)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Constructor
/////////////////////////////////////////////////
ProcedureVarFactory::ProcedureVarFactory()
{
    init();
}


/////////////////////////////////////////////////
/// \brief General constructor from an already
/// available procedure instance.
///
/// \param _procedure Procedure*
/// \param sProc const string&
/// \param currentProc unsigned int
/// \param _inliningMode bool
///
/////////////////////////////////////////////////
ProcedureVarFactory::ProcedureVarFactory(Procedure* _procedure, const string& sProc, unsigned int currentProc, bool _inliningMode)
{
    init();
    _currentProcedure = _procedure;

    sProcName = replaceProcedureName(sProc);
    nth_procedure = currentProc;
    inliningMode = _inliningMode;
}


/////////////////////////////////////////////////
/// \brief Destructor.
/////////////////////////////////////////////////
ProcedureVarFactory::~ProcedureVarFactory()
{
    reset();
}


/////////////////////////////////////////////////
/// \brief This member function is the
/// initializer function.
///
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::init()
{
    _currentProcedure = nullptr;

    // Get the addresses of the kernel objects
    _parserRef = &NumeReKernel::getInstance()->getParser();
    _dataRef = &NumeReKernel::getInstance()->getMemoryManager();
    _optionRef = &NumeReKernel::getInstance()->getSettings();
    _functionRef = &NumeReKernel::getInstance()->getDefinitions();
    _outRef = &NumeReKernel::getInstance()->getOutput();
    _pDataRef = &NumeReKernel::getInstance()->getPlottingData();
    _scriptRef = &NumeReKernel::getInstance()->getScript();

    sProcName = "";
    nth_procedure = 0;

    sArgumentMap = nullptr;
    sLocalVars = nullptr;
    sLocalStrings = nullptr;
    sLocalTables = nullptr;
    sLocalClusters = nullptr;

    dLocalVars = nullptr;

    nArgumentMapSize = 0;
    nLocalVarMapSize = 0;
    nLocalStrMapSize = 0;
    nLocalTableSize = 0;
    nLocalClusterSize = 0;

    inliningMode = false;
    sInlineVarDef.clear();
    sInlineStringDef.clear();
}


/////////////////////////////////////////////////
/// \brief Resets the object.
///
/// \return void
///
/// This member function will reset the object,
/// i.e. free up the allocated memory. Can be
/// used instead of deleting the object, however
/// due to design decisions, this object is heap-
/// allocated and recreated for each procedure.
/////////////////////////////////////////////////
void ProcedureVarFactory::reset()
{
    if (nArgumentMapSize)
    {
        for (size_t i = 0; i < nArgumentMapSize; i++)
            delete[] sArgumentMap[i];

        delete[] sArgumentMap;
        sArgumentMap = nullptr;
        nArgumentMapSize = 0;
    }

    // Clear the local copies of the arguments
    if (mLocalArgs.size())
    {
        for (auto iter : mLocalArgs)
        {
            if (iter.second == NUMTYPE)
                _parserRef->RemoveVar(iter.first);
            else if (iter.second == STRINGTYPE)
                NumeReKernel::getInstance()->getStringParser().removeStringVar(iter.first);
            else if (iter.second == CLUSTERTYPE)
                _dataRef->removeCluster(iter.first.substr(0, iter.first.find('{')));
        }

        mLocalArgs.clear();
    }

    if (nLocalVarMapSize)
    {
        for (size_t i = 0; i < nLocalVarMapSize; i++)
        {
            if (_parserRef)
                _parserRef->RemoveVar(sLocalVars[i][1]);

            delete[] sLocalVars[i];
        }

        delete[] sLocalVars;

        if (dLocalVars)
            delete[] dLocalVars;

        sLocalVars = nullptr;
        dLocalVars = nullptr;
        nLocalVarMapSize = 0;
    }

    if (nLocalStrMapSize)
    {
        for (size_t i = 0; i < nLocalStrMapSize; i++)
        {
            NumeReKernel::getInstance()->getStringParser().removeStringVar(sLocalStrings[i][1]);
            delete[] sLocalStrings[i];
        }

        delete[] sLocalStrings;
        sLocalStrings = nullptr;
        nLocalStrMapSize = 0;
    }

    if (nLocalTableSize)
    {
        for (size_t i = 0; i < nLocalTableSize; i++)
        {
            if (_dataRef)
                _dataRef->deleteTable(sLocalTables[i][1]);

            delete[] sLocalTables[i];
        }

        delete[] sLocalTables;
        sLocalTables = nullptr;
        nLocalTableSize = 0;
    }

    if (nLocalClusterSize)
    {
        for (size_t i = 0; i < nLocalClusterSize; i++)
        {
            if (_dataRef)
                _dataRef->removeCluster(sLocalClusters[i][1]);

            delete[] sLocalClusters[i];
        }

        delete[] sLocalClusters;
        sLocalClusters = nullptr;
        nLocalClusterSize = 0;
    }

    sInlineVarDef.clear();
    sInlineStringDef.clear();
}


/////////////////////////////////////////////////
/// \brief Returns whether the passed argument
/// representation (i.e. local variable name) is
/// actually a reference from the outside.
///
/// \param sArgName const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool ProcedureVarFactory::isReference(const std::string& sArgName) const
{
    return mLocalArgs.find(sArgName) == mLocalArgs.end();
}


/////////////////////////////////////////////////
/// \brief Replaces path characters and whitespaces
/// to create variable names fitting for an non-
/// relative procedure.
///
/// \param sProcedureName string
/// \return string
///
/////////////////////////////////////////////////
string ProcedureVarFactory::replaceProcedureName(string sProcedureName)
{
    for (size_t i = 0; i < sProcedureName.length(); i++)
    {
        if (sProcedureName[i] == ':' || sProcedureName[i] == '\\' || sProcedureName[i] == '/' || sProcedureName[i] == ' ')
            sProcedureName[i] = '_';
    }

    return sProcedureName;
}


/////////////////////////////////////////////////
/// \brief This private memebr function counts
/// the number of elements in the passed string
/// list.
///
/// \param sVarList const string&
/// \return unsigned int
///
/// This can be used to determine the size of the
/// to be allocated data object.
/////////////////////////////////////////////////
unsigned int ProcedureVarFactory::countVarListElements(const string& sVarList)
{
    int nParenthesis = 0;
    unsigned int nElements = 1;

    // Count every comma, which is not part of a parenthesis and
    // also not between two quotation marks
    for (unsigned int i = 0; i < sVarList.length(); i++)
    {
        if ((sVarList[i] == '(' || sVarList[i] == '{') && !isInQuotes(sVarList, i))
            nParenthesis++;

        if ((sVarList[i] == ')' || sVarList[i] == '}') && !isInQuotes(sVarList, i))
            nParenthesis--;

        if (sVarList[i] == ',' && !nParenthesis && !isInQuotes(sVarList, i))
            nElements++;
    }

    return nElements;
}


/////////////////////////////////////////////////
/// \brief This private member function checks,
/// whether the keywords "var", "str" or "tab"
/// are used in the current argument and throws
/// an exception, if this is the case.
///
/// \param sArgument const string&
/// \param sArgumentList const string&
/// \param nCurrentIndex unsigned int
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::checkKeywordsInArgument(const string& sArgument, const string& sArgumentList, unsigned int nCurrentIndex)
{
    string sCommand = findCommand(sArgument).sString;

    // Was a keyword used as a argument?
    if (sCommand == "var" || sCommand == "tab" || sCommand == "str")
    {
        // Free up memory
        for (unsigned int j = 0; j <= nCurrentIndex; j++)
            delete[] sArgumentMap[j];

        delete[] sArgumentMap;
        nArgumentMapSize = 0;

        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();
        // Gather all information in the debugger and throw
        // the exception
        _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
        _debugger.throwException(SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, sCommand));
    }
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the local variables for inlined procedures.
///
/// \param sVarList string
/// \return void
///
/// The created variables are redirected to
/// cluster items.
/////////////////////////////////////////////////
void ProcedureVarFactory::createLocalInlineVars(string sVarList)
{
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Get the number of declared variables
    nLocalVarMapSize = countVarListElements(sVarList);

    // Create a new temporary cluster
    string sTempCluster = _dataRef->createTemporaryCluster();
    sInlineVarDef = sTempCluster + " = {";

    sTempCluster.erase(sTempCluster.length()-2);

    // Get a reference to the temporary cluster
    NumeRe::Cluster& tempCluster = _dataRef->getCluster(sTempCluster);

    sLocalVars = new string*[nLocalVarMapSize];

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        sLocalVars[i] = new string[2];
        sLocalVars[i][0] = getNextArgument(sVarList, true);

        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (sLocalVars[i][0].find('=') != string::npos)
        {
            string sVarValue = sLocalVars[i][0].substr(sLocalVars[i][0].find('=')+1);
            sInlineVarDef += sVarValue + ",";

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                _debugger.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                for (unsigned int j = 0; j <= i; j++)
                {
                    delete[] sLocalVars[j];
                }

                delete[] sLocalVars;
                nLocalVarMapSize = 0;

                _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sVarList, SyntaxError::invalid_position));
            }

            try
            {
                if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sVarValue))
                {
                    string sTemp;
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sVarList, sTemp, true);
                }

                if (_dataRef->containsTablesOrClusters(sVarValue))
                {
                    getDataElements(sVarValue, *_parserRef, *_dataRef, *_optionRef);
                }

                sVarValue = resolveLocalVars(sVarValue, i);

                _parserRef->SetExpr(sVarValue);
                sLocalVars[i][0] = sLocalVars[i][0].substr(0, sLocalVars[i][0].find('='));
                tempCluster.setDouble(i, _parserRef->Eval());
            }
            catch (...)
            {
                _debugger.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                for (unsigned int j = 0; j <= i; j++)
                {
                    delete[] sLocalVars[j];
                }

                delete[] sLocalVars;
                nLocalVarMapSize = 0;

                _debugger.showError(current_exception());
                throw;
            }
        }
        else
        {
            tempCluster.setDouble(i, 0.0);
            sInlineVarDef += "0,";
        }

        StripSpaces(sLocalVars[i][0]);
        sLocalVars[i][1] = sTempCluster + "{" + toString(i+1) + "}";
    }

    sInlineVarDef.back() = '}';
}


/////////////////////////////////////////////////
/// \brief This private member function creates
/// the local string variables for inlined
/// procedures.
///
/// \param sStringList string
/// \return void
///
/// the created variables are redirected to
/// cluster items.
/////////////////////////////////////////////////
void ProcedureVarFactory::createLocalInlineStrings(string sStringList)
{
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Get the number of declared variables
    nLocalStrMapSize = countVarListElements(sStringList);

    // Create a new temporary cluster
    string sTempCluster = _dataRef->createTemporaryCluster();
    sInlineStringDef = sTempCluster + " = {";
    sTempCluster.erase(sTempCluster.length()-2);

    // Get a reference to the temporary cluster
    NumeRe::Cluster& tempCluster = _dataRef->getCluster(sTempCluster);

    sLocalStrings = new string*[nLocalStrMapSize];

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        sLocalStrings[i] = new string[3];
        sLocalStrings[i][0] = getNextArgument(sStringList, true);

        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (sLocalStrings[i][0].find('=') != string::npos)
        {
            string sVarValue = sLocalStrings[i][0].substr(sLocalStrings[i][0].find('=')+1);
            sInlineStringDef += sVarValue + ",";

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                _debugger.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                for (unsigned int j = 0; j <= i; j++)
                {
                    delete[] sLocalStrings[j];
                }

                delete[] sLocalStrings;
                nLocalStrMapSize = 0;

                _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sStringList, SyntaxError::invalid_position));
            }

            try
            {
                sVarValue = resolveLocalStrings(sVarValue, i);

                if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sVarValue))
                {
                    string sTemp;
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sVarValue, sTemp, true);
                }

                sLocalStrings[i][0] = sLocalStrings[i][0].substr(0, sLocalStrings[i][0].find('='));
                sLocalStrings[i][2] = sVarValue;
                tempCluster.setString(i, sVarValue);
            }
            catch (...)
            {
                _debugger.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                for (unsigned int j = 0; j <= i; j++)
                {
                    delete[] sLocalStrings[j];
                }

                delete[] sLocalVars;
                nLocalStrMapSize = 0;

                _debugger.showError(current_exception());
                throw;
            }
        }
        else
        {
            tempCluster.setString(i, "\"\"");
            sInlineStringDef += "\"\",";
        }

        StripSpaces(sLocalStrings[i][0]);
        sLocalStrings[i][1] = sTempCluster + "{" + toString(i+1) + "}";
    }

    sInlineStringDef.back() = '}';
}


/////////////////////////////////////////////////
/// \brief This member function will create the
/// procedure arguments for the current procedure.
///
/// \param sArgumentList string
/// \param sArgumentValues string
/// \return map<string,string>
///
/////////////////////////////////////////////////
map<string,string> ProcedureVarFactory::createProcedureArguments(string sArgumentList, string sArgumentValues)
{
    map<string,string> mVarMap;
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    if (!sArgumentList.length() && sArgumentValues.length())
    {
        if (_optionRef->useDebugger())
            _debugger.popStackItem();

        throw SyntaxError(SyntaxError::TOO_MANY_ARGS, sArgumentList, SyntaxError::invalid_position);
    }
    else if (!sArgumentList.length())
        return mVarMap;

    if (!validateParenthesisNumber(sArgumentList))
    {
        _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
        _debugger.throwException(SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sArgumentList, SyntaxError::invalid_position));
    }

    // Get the number of argument of this procedure
    nArgumentMapSize = countVarListElements(sArgumentList);
    sArgumentMap = new string*[nArgumentMapSize];

    // Decode the argument list
    for (unsigned int i = 0; i < nArgumentMapSize; i++)
    {
        sArgumentMap[i] = new string[2];
        sArgumentMap[i][0] = getNextArgument(sArgumentList);
        StripSpaces(sArgumentMap[i][0]);

        checkKeywordsInArgument(sArgumentMap[i][0], sArgumentList, i);

        // Fill in the value of the argument by either
        // using the default value or the passed value
        sArgumentMap[i][1] = getNextArgument(sArgumentValues);
        StripSpaces(sArgumentMap[i][1]);

        checkKeywordsInArgument(sArgumentMap[i][1], sArgumentList, i);

        if (sArgumentMap[i][1].length() && sArgumentMap[i][0].find('=') != string::npos)
        {
            sArgumentMap[i][0].erase(sArgumentMap[i][0].find('='));
            StripSpaces(sArgumentMap[i][0]);
        }
        else if (!sArgumentMap[i][1].length() && sArgumentMap[i][0].find('=') != string::npos)
        {
            sArgumentMap[i][1] = sArgumentMap[i][0].substr(sArgumentMap[i][0].find('=')+1);
            sArgumentMap[i][0].erase(sArgumentMap[i][0].find('='));
            StripSpaces(sArgumentMap[i][0]);
            StripSpaces(sArgumentMap[i][1]);
        }
        else if (!sArgumentMap[i][1].length() && sArgumentMap[i][0].find('=') == string::npos)
        {
            string sErrorToken = sArgumentMap[i][0];

            for (unsigned int j = 0; j <= i; j++)
                delete[] sArgumentMap[j];

            delete[] sArgumentMap;
            nArgumentMapSize = 0;

            if (_optionRef->useDebugger())
                _debugger.popStackItem();

            throw SyntaxError(SyntaxError::MISSING_DEFAULT_VALUE, sArgumentList, sErrorToken, sErrorToken);
        }

        if (containsFreeOperators(sArgumentMap[i][1]))
            sArgumentMap[i][1] = "(" + sArgumentMap[i][1] + ")";
    }

    // Evaluate procedure calls and the parentheses of the
    // passed tables and clusters
    evaluateProcedureArguments(sArgumentList);

    for (unsigned int i = 0; i < nArgumentMapSize; i++)
    {
        mVarMap[sArgumentMap[i][0]] = sArgumentMap[i][1];
    }

    return mVarMap;
}


/////////////////////////////////////////////////
/// \brief This memberfunction will evaluate the
/// passed procedure arguments and convert them
/// to local variables if necessary.
///
/// \param sArgumentList const std::string&
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::evaluateProcedureArguments(const std::string& sArgumentList)
{
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();
    NumeRe::StringParser& _stringParser = NumeReKernel::getInstance()->getStringParser();
    bool isTemplate = _currentProcedure->getProcedureFlags() & ProcedureCommandLine::FLAG_TEMPLATE;
    bool isMacro = _currentProcedure->getProcedureFlags() & ProcedureCommandLine::FLAG_MACRO || inliningMode;

    for (unsigned int i = 0; i < nArgumentMapSize; i++)
    {
        // Evaluate procedure calls first
        if (sArgumentMap[i][1].find('$') != string::npos && sArgumentMap[i][1].find('(') != string::npos)
        {
            if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
            {
                _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sArgumentList, SyntaxError::invalid_position));
            }

            int nReturn = _currentProcedure->procedureInterface(sArgumentMap[i][1], *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

            if (nReturn == -1)
            {
                _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.throwException(SyntaxError(SyntaxError::PROCEDURE_ERROR, sArgumentList, SyntaxError::invalid_position));
            }
            else if (nReturn == -2)
                sArgumentMap[i][1] = "false";
        }

        bool isRef = sArgumentMap[i][0].front() == '&' || sArgumentMap[i][0].back() == '&';

        // Determine, if this is a reference (and
        // remove the ampersand in this case)
        if (isRef)
        {
            sArgumentMap[i][0].erase(sArgumentMap[i][0].find('&'), 1);
            StripSpaces(sArgumentMap[i][0]);
        }

        if (sArgumentMap[i][0].length() > 2 && sArgumentMap[i][0].substr(sArgumentMap[i][0].length()-2) == "()")
        {
            sArgumentMap[i][0].pop_back();

            if (sArgumentMap[i][1].find('(') != string::npos)
                sArgumentMap[i][1].erase(sArgumentMap[i][1].find('('));
        }
        else if (sArgumentMap[i][0].length() > 2 && sArgumentMap[i][0].substr(sArgumentMap[i][0].length()-2) == "{}")
        {
            sArgumentMap[i][0].pop_back();

            if (isRef && !_dataRef->isCluster(sArgumentMap[i][1]))
            {
                _debugger.gatherInformations(this, sArgumentMap[i][1], _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.throwException(SyntaxError(SyntaxError::CANNOT_PASS_LITERAL_PER_REFERENCE, sArgumentMap[i][1], "", sArgumentMap[i][0]));
            }
            else if (!isRef && !isMacro) // Macros do the old copy-paste logic
            {
                // Create a local variable
                std::string sNewArgName = "_~"+sProcName+"_~A_"+toString((int)nth_procedure)+"_"+sArgumentMap[i][0].substr(0, sArgumentMap[i][0].length()-1);
                NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);

                // Copy, if it is already a cluster
                if (_dataRef->isCluster(sArgumentMap[i][1]))
                    newCluster = _dataRef->getCluster(sArgumentMap[i][1].substr(0, sArgumentMap[i][1].find('{')));
                else
                {
                    // Evaluate the expression and create a new
                    // cluster
                    try
                    {
                        std::string sCurrentValue = sArgumentMap[i][1];

                        if (_dataRef->containsTablesOrClusters(sCurrentValue))
                            getDataElements(sCurrentValue, *_parserRef, *_dataRef, *_optionRef, false);

                        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentValue))
                        {
                            string sCluster = sNewArgName + "{}";
                            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCurrentValue, sCluster, true);
                        }
                        else
                        {
                            value_type* v = nullptr;
                            int nResults;
                            _parserRef->SetExpr(sCurrentValue);
                            v = _parserRef->Eval(nResults);
                            newCluster.setDoubleArray(nResults, v);
                        }
                    }
                    catch (...)
                    {
                        _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                        _debugger.showError(current_exception());
                        throw;
                    }
                }

                // Clusters have to be stored as names without
                // their braces
                sArgumentMap[i][1] = sNewArgName;
                mLocalArgs[sNewArgName] = CLUSTERTYPE;
            }

            sArgumentMap[i][1] = sArgumentMap[i][1].substr(0, sArgumentMap[i][1].find('{'));
        }
        else
        {
            if (isRef)
            {
                const auto& varMap = _parserRef->GetVar();
                const auto& stringMap = _stringParser.getStringVars();

                // Reference
                if (varMap.find(sArgumentMap[i][1]) == varMap.end()
                    && stringMap.find(sArgumentMap[i][1]) == stringMap.end()
                    && !(isTemplate && _dataRef->isCluster(sArgumentMap[i][1]))
                    && !(isTemplate && sArgumentMap[i][1].substr(0, 7) == "string(")
                    && !(isTemplate && _dataRef->isTable(sArgumentMap[i][1])))
                {
                    _debugger.gatherInformations(this, sArgumentMap[i][1], _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.throwException(SyntaxError(SyntaxError::CANNOT_PASS_LITERAL_PER_REFERENCE, sArgumentMap[i][1], "", sArgumentMap[i][0]));
                }
            }
            else if (!isMacro) // macros do the old copy-paste logic
            {
                // Create a local variable
                std::string sNewArgName = "_~"+sProcName+"_~A_"+toString((int)nth_procedure)+"_"+sArgumentMap[i][0];

                if (!_functionRef->call(sArgumentMap[i][1]))
                {
                    _debugger.gatherInformations(this, sArgumentMap[i][1], _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.throwException(SyntaxError(SyntaxError::FUNCTION_ERROR, sArgumentMap[i][0], SyntaxError::invalid_position));
                }

                try
                {
                    // Templates allow also other types
                    if (isTemplate)
                    {
                        StripSpaces(sArgumentMap[i][1]);

                        if (sArgumentMap[i][1].find("()") != std::string::npos && _dataRef->isTable(sArgumentMap[i][1]))
                            continue; // Tables are always references
                        else if (sArgumentMap[i][1].find("{}") != std::string::npos && _dataRef->isCluster(sArgumentMap[i][1]))
                        {
                            // Copy clusters
                            NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);
                            newCluster = _dataRef->getCluster(sArgumentMap[i][1].substr(0, sArgumentMap[i][1].find('{')));

                            sArgumentMap[i][1] = sNewArgName + "{}";
                            mLocalArgs[sNewArgName + "{}"] = CLUSTERTYPE;
                            continue;
                        }

                        // Get data, if necessary
                        if (_dataRef->containsTablesOrClusters(sArgumentMap[i][1]))
                            getDataElements(sArgumentMap[i][1], *_parserRef, *_dataRef, *_optionRef);

                        // Evaluate strings
                        if (_stringParser.isStringExpression(sArgumentMap[i][1]))
                        {
                            NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);
                            std::string sCluster = sNewArgName + "{}";
                            NumeRe::StringParser::StringParserRetVal ret =  _stringParser.evalAndFormat(sArgumentMap[i][1], sCluster, true);

                            if (ret == NumeRe::StringParser::STRING_SUCCESS)
                            {
                                if (newCluster.size() == 1)
                                {
                                    _stringParser.setStringValue(sNewArgName, newCluster.getString(0));
                                    _dataRef->removeCluster(sNewArgName);
                                    sArgumentMap[i][1] = sNewArgName;
                                    mLocalArgs[sNewArgName] = STRINGTYPE;
                                }
                                else
                                {
                                    sArgumentMap[i][1] = sNewArgName + "{}";
                                    mLocalArgs[sNewArgName + "{}"] = CLUSTERTYPE;
                                }

                                continue;
                            }
                        }

                        // Evaluate numerical expressions
                        _parserRef->SetExpr(sArgumentMap[i][1]);
                        int nRes = 0;
                        value_type* v = _parserRef->Eval(nRes);

                        if (nRes > 1)
                        {
                            NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);
                            newCluster.setDoubleArray(nRes, v);
                            sArgumentMap[i][1] = sNewArgName + "{}";
                            mLocalArgs[sNewArgName + "{}"] = CLUSTERTYPE;
                        }
                        else
                        {
                            value_type* newVar = new value_type;
                            *newVar = v[0];
                            _parserRef->m_lDataStorage.push_back(newVar);
                            _parserRef->DefineVar(sNewArgName, newVar);
                            sArgumentMap[i][1] = sNewArgName;
                            mLocalArgs[sNewArgName] = NUMTYPE;
                        }

                        continue;
                    }
                    else
                    {
                        // Get data, if necessary
                        if (_dataRef->containsTablesOrClusters(sArgumentMap[i][1]))
                            getDataElements(sArgumentMap[i][1], *_parserRef, *_dataRef, *_optionRef);

                        // Evaluate strings
                        if (_stringParser.isStringExpression(sArgumentMap[i][1]))
                        {
                            std::string dummy;
                            NumeRe::StringParser::StringParserRetVal ret =  _stringParser.evalAndFormat(sArgumentMap[i][1], dummy, true);

                            if (ret == NumeRe::StringParser::STRING_SUCCESS)
                            {
                                _stringParser.setStringValue(sNewArgName, getNextArgument(sArgumentMap[i][1]));
                                sArgumentMap[i][1] = sNewArgName;
                                mLocalArgs[sNewArgName] = STRINGTYPE;
                                continue;
                            }
                        }

                        // Evaluate numerical expressions
                        _parserRef->SetExpr(sNewArgName + " = " + sArgumentMap[i][1]);
                        _parserRef->Eval();
                    }
                }
                catch (...)
                {
                    _debugger.gatherInformations(this, sArgumentMap[i][1], _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.showError(current_exception());
                    throw;
                }

                sArgumentMap[i][1] = sNewArgName;
                mLocalArgs[sNewArgName] = NUMTYPE;
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function will create the
/// local numerical variables for the current
/// procedure.
///
/// \param sVarList string
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::createLocalVars(string sVarList)
{
    // already declared the local vars?
    if (nLocalVarMapSize)
        return;

    if (!_currentProcedure)
        return;

    if (inliningMode)
    {
        createLocalInlineVars(sVarList);
        return;
    }

    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Get the number of declared variables
    nLocalVarMapSize = countVarListElements(sVarList);

    sLocalVars = new string*[nLocalVarMapSize];
    dLocalVars = new double[nLocalVarMapSize];

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        sLocalVars[i] = new string[2];
        sLocalVars[i][0] = getNextArgument(sVarList, true);

        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (sLocalVars[i][0].find('=') != string::npos)
        {
            string sVarValue = sLocalVars[i][0].substr(sLocalVars[i][0].find('=')+1);

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    _debugger.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            _parserRef->RemoveVar(sLocalVars[j][1]);

                        delete[] sLocalVars[j];
                    }

                    delete[] sLocalVars;
                    nLocalVarMapSize = 0;

                    _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sVarList, SyntaxError::invalid_position));
                }

                try
                {
                    int nReturn = _currentProcedure->procedureInterface(sVarValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                    if (nReturn == -1)
                    {
                        throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sVarList, SyntaxError::invalid_position);
                    }
                    else if (nReturn == -2)
                        sVarValue = "false";
                }
                catch (...)
                {
                    _debugger.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            _parserRef->RemoveVar(sLocalVars[j][1]);

                        delete[] sLocalVars[j];
                    }

                    delete[] sLocalVars;
                    nLocalVarMapSize = 0;

                    _debugger.showError(current_exception());
                    throw;
                }
            }

            try
            {
                sVarValue = resolveLocalVars(sVarValue, i);

                if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sVarValue) &&  _dataRef->containsTablesOrClusters(sVarValue))
                {
                    getDataElements(sVarValue, *_parserRef, *_dataRef, *_optionRef);
                }

                if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sVarValue))
                {
                    string sTemp;
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sVarValue, sTemp, true);
                }

                _parserRef->SetExpr(sVarValue);
                sLocalVars[i][0] = sLocalVars[i][0].substr(0,sLocalVars[i][0].find('='));
                dLocalVars[i] = _parserRef->Eval();
            }
            catch (...)
            {
                _debugger.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                for (unsigned int j = 0; j <= i; j++)
                {
                    if (j < i)
                        _parserRef->RemoveVar(sLocalVars[j][1]);

                    delete[] sLocalVars[j];
                }

                delete[] sLocalVars;
                nLocalVarMapSize = 0;

                _debugger.showError(current_exception());
                throw;
            }
        }
        else
            dLocalVars[i] = 0.0;

        StripSpaces(sLocalVars[i][0]);
        sLocalVars[i][1] = "_~"+sProcName+"_"+toString((int)nth_procedure)+"_"+sLocalVars[i][0];

        _parserRef->DefineVar(sLocalVars[i][1], &dLocalVars[i]);
    }
}


/////////////////////////////////////////////////
/// \brief This member function will create the
/// local string variables for the current
/// procedure.
///
/// \param sStringList string
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::createLocalStrings(string sStringList)
{
    // already declared the local vars?
    if (nLocalStrMapSize)
        return;

    if (!_currentProcedure)
        return;

    if (inliningMode)
    {
        createLocalInlineStrings(sStringList);
        return;
    }

    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Get the number of declared variables
    nLocalStrMapSize = countVarListElements(sStringList);
    sLocalStrings = new string*[nLocalStrMapSize];

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        sLocalStrings[i] = new string[3];
        sLocalStrings[i][0] = getNextArgument(sStringList, true);

        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (sLocalStrings[i][0].find('=') != string::npos)
        {
            string sVarValue = sLocalStrings[i][0].substr(sLocalStrings[i][0].find('=')+1);

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {

                    _debugger.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            NumeReKernel::getInstance()->getStringParser().removeStringVar(sLocalStrings[j][1]);

                        delete[] sLocalStrings[j];
                    }

                    delete[] sLocalStrings;
                    nLocalStrMapSize = 0;

                    _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sStringList, SyntaxError::invalid_position));
                }

                try
                {
                    int nReturn = _currentProcedure->procedureInterface(sVarValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                    if (nReturn == -1)
                    {
                        throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sStringList, SyntaxError::invalid_position);
                    }
                    else if (nReturn == -2)
                        sVarValue = "false";
                }
                catch (...)
                {
                    _debugger.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            NumeReKernel::getInstance()->getStringParser().removeStringVar(sLocalStrings[j][1]);

                        delete[] sLocalStrings[j];
                    }

                    delete[] sLocalStrings;
                    nLocalStrMapSize = 0;

                    _debugger.showError(current_exception());
                    throw;
                }
            }

            try
            {
                sVarValue = resolveLocalStrings(sVarValue, i);

                if (_dataRef->containsTablesOrClusters(sVarValue))
                {
                    getDataElements(sVarValue, *_parserRef, *_dataRef, *_optionRef);
                }

                if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sVarValue))
                {
                    string sTemp;
                    NumeReKernel::getInstance()->getStringParser().evalAndFormat(sVarValue, sTemp, true);
                }

                sLocalStrings[i][0] = sLocalStrings[i][0].substr(0,sLocalStrings[i][0].find('='));
                sLocalStrings[i][2] = sVarValue;
            }
            catch (...)
            {
                for (unsigned int j = 0; j <= i; j++)
                {
                    if (j < i)
                        NumeReKernel::getInstance()->getStringParser().removeStringVar(sLocalStrings[j][1]);

                    delete[] sLocalStrings[j];
                }

                delete[] sLocalStrings;
                nLocalStrMapSize = 0;
                throw;
            }
        }
        else
            sLocalStrings[i][2] = "";

        StripSpaces(sLocalStrings[i][0]);
        sLocalStrings[i][1] = "_~"+sProcName+"_"+toString((int)nth_procedure)+"_"+sLocalStrings[i][0];

        try
        {
            NumeReKernel::getInstance()->getStringParser().setStringValue(sLocalStrings[i][1], sLocalStrings[i][2]);
        }
        catch (...)
        {
            _debugger.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, sLocalTables, nLocalTableSize, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

            for (unsigned int j = 0; j <= i; j++)
            {
                if (j < i)
                    NumeReKernel::getInstance()->getStringParser().removeStringVar(sLocalStrings[j][1]);

                delete[] sLocalStrings[j];
            }

            delete[] sLocalStrings;
            nLocalStrMapSize = 0;

            _debugger.showError(current_exception());
            throw;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function will create the
/// local tables for the current procedure.
///
/// \param sTableList string
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::createLocalTables(string sTableList)
{
    // already declared the local vars?
    if (nLocalTableSize)
        return;

    if (!_currentProcedure)
        return;

    if (inliningMode)
        return;

    // Get the number of declared variables
    nLocalTableSize = countVarListElements(sTableList);
    sLocalTables = new string*[nLocalTableSize];

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalTableSize; i++)
    {
        sLocalTables[i] = new string[2];
        sLocalTables[i][0] = getNextArgument(sTableList, true);

        if (sLocalTables[i][0].find('(') != string::npos)
            sLocalTables[i][0].erase(sLocalTables[i][0].find('('));

        StripSpaces(sLocalTables[i][0]);
        sLocalTables[i][1] = "_~"+sProcName+"_"+toString((int)nth_procedure)+"_"+sLocalTables[i][0];

        try
        {
            _dataRef->addTable(sLocalTables[i][1], *_optionRef);
        }
        catch (...)
        {
            NumeReKernel::getInstance()->getDebugger().gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, nLocalStrMapSize, sLocalTables, i, sLocalClusters, nLocalClusterSize, sArgumentMap, nArgumentMapSize, sTableList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

            for (unsigned int j = 0; j <= i; j++)
            {
                if (j < i)
                    _dataRef->deleteTable(sLocalTables[j][1]);

                delete[] sLocalTables[j];
            }

            delete[] sLocalTables;
            nLocalTableSize = 0;

            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            throw;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function will create the
/// local clusters for the current procedure.
///
/// \param sClusterList string
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::createLocalClusters(string sClusterList)
{
    // already declared the local vars?
    if (nLocalClusterSize)
        return;

    if (!_currentProcedure)
        return;

    if (inliningMode)
        return;

    // Get the number of declared variables
    nLocalClusterSize = countVarListElements(sClusterList);
    sLocalClusters = new string*[nLocalClusterSize];
    string sCurrentValue;

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalClusterSize; i++)
    {
        sLocalClusters[i] = new string[2];
        sLocalClusters[i][0] = getNextArgument(sClusterList, true);

        if (sLocalClusters[i][0].find('=') != string::npos)
        {
            sCurrentValue = sLocalClusters[i][0].substr(sLocalClusters[i][0].find('=')+1);
        }

        if (sLocalClusters[i][0].find('{') != string::npos)
            sLocalClusters[i][0].erase(sLocalClusters[i][0].find('{'));

        StripSpaces(sLocalClusters[i][0]);
        sLocalClusters[i][1] = "_~"+sProcName+"_"+toString((int)nth_procedure)+"_"+sLocalClusters[i][0];

        //sLocalClusters[i][0] += "{";

        try
        {
            NumeRe::Cluster& cluster = _dataRef->newCluster(sLocalClusters[i][1]);

            if (!sCurrentValue.length())
                continue;

            sCurrentValue = resolveLocalClusters(sCurrentValue, i);

            if (sCurrentValue.find('$') != string::npos && sCurrentValue.find('(', sCurrentValue.find('$')+1))
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sClusterList, SyntaxError::invalid_position);
                }

                int nReturn = _currentProcedure->procedureInterface(sCurrentValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                if (nReturn == -1)
                {
                    throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sClusterList, SyntaxError::invalid_position);
                }
                else if (nReturn == -2)
                    sCurrentValue = "false";
            }

            if (_dataRef->containsTablesOrClusters(sCurrentValue))
                getDataElements(sCurrentValue, *_parserRef, *_dataRef, *_optionRef, false);

            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentValue))
            {
                string sCluster = sLocalClusters[i][1] + "{}";
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCurrentValue, sCluster, true);
            }
            else
            {
                value_type* v = nullptr;
                int nResults;
                _parserRef->SetExpr(sCurrentValue);
                v = _parserRef->Eval(nResults);
                cluster.setDoubleArray(nResults, v);
            }
        }
        catch (...)
        {
            NumeReKernel::getInstance()->getDebugger().gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, nLocalStrMapSize, sLocalTables, nLocalTableSize, sLocalClusters, i, sArgumentMap, nArgumentMapSize, sClusterList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

            for (unsigned int j = 0; j <= i; j++)
            {
                if (j < i)
                    _dataRef->removeCluster(sLocalClusters[j][1]);

                delete[] sLocalClusters[j];
            }

            delete[] sLocalClusters;
            nLocalClusterSize = 0;

            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            throw;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// resolve the calls to arguments in the passed
/// procedure command line.
///
/// \param sProcedureCommandLine string
/// \param nMapSize size_t
/// \return string
///
/////////////////////////////////////////////////
string ProcedureVarFactory::resolveArguments(string sProcedureCommandLine, size_t nMapSize)
{
    if (!nMapSize)
        return sProcedureCommandLine;

    if (nMapSize == string::npos)
        nMapSize = nArgumentMapSize;

    for (unsigned int i = 0; i < nMapSize; i++)
    {
        unsigned int nPos = 0;
        size_t nArgumentBaseLength = sArgumentMap[i][0].length();

        if (sArgumentMap[i][0].back() == '(' || sArgumentMap[i][0].back() == '{')
            nArgumentBaseLength--;

        while ((nPos = sProcedureCommandLine.find(sArgumentMap[i][0].substr(0, nArgumentBaseLength), nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~',nPos-1)] != '#')
                || (sArgumentMap[i][0].back() != '(' && sProcedureCommandLine[nPos+nArgumentBaseLength] == '(')
                || (sArgumentMap[i][0].back() != '{' && sProcedureCommandLine[nPos+nArgumentBaseLength] == '{'))
            {
                nPos += sArgumentMap[i][0].length();
                continue;
            }

            if (checkDelimiter(sProcedureCommandLine.substr(nPos-1, nArgumentBaseLength+2), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true)
                    || isToCmd(sProcedureCommandLine, nPos)))
            {
                if (sArgumentMap[i][1].front() == '{' && sArgumentMap[i][0].back() == '{')
                    sProcedureCommandLine.replace(nPos, getMatchingParenthesis(sProcedureCommandLine.substr(nPos))+1, sArgumentMap[i][1]);
                else
                    sProcedureCommandLine.replace(nPos, nArgumentBaseLength, sArgumentMap[i][1]);

                nPos += sArgumentMap[i][1].length();
            }
            else
                nPos += nArgumentBaseLength;
        }
    }

    return sProcedureCommandLine;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// resolve the calls to numerical variables in
/// the passed procedure command line.
///
/// \param sProcedureCommandLine string
/// \param nMapSize size_t
/// \return string
///
/////////////////////////////////////////////////
string ProcedureVarFactory::resolveLocalVars(string sProcedureCommandLine, size_t nMapSize)
{
    if (!nMapSize)
        return sProcedureCommandLine;

    if (nMapSize == string::npos)
        nMapSize = nLocalVarMapSize;

    for (size_t i = 0; i < nMapSize; i++)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(sLocalVars[i][0], nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos+sLocalVars[i][0].length()] == '(')
            {
                nPos += sLocalVars[i][0].length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, sLocalVars[i][0].length() + 1 + nPos - nDelimCheck))
                && (!isInQuotes(sProcedureCommandLine, nPos, true)
                    || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, sLocalVars[i][0].length(), sLocalVars[i][1]);
                nPos += sLocalVars[i][1].length();
            }
            else
                nPos += sLocalVars[i][0].length();
        }
    }

    return sProcedureCommandLine;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// resolve the calls to string variables in the
/// passed procedure command line.
///
/// \param sProcedureCommandLine string
/// \param nMapSize size_t
/// \return string
///
/////////////////////////////////////////////////
string ProcedureVarFactory::resolveLocalStrings(string sProcedureCommandLine, size_t nMapSize)
{
    if (!nMapSize)
        return sProcedureCommandLine;

    if (nMapSize == string::npos)
        nMapSize = nLocalStrMapSize;

    for (size_t i = 0; i < nMapSize; i++)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(sLocalStrings[i][0], nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos+sLocalStrings[i][0].length()] == '(')
            {
                nPos += sLocalStrings[i][0].length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, sLocalStrings[i][0].length() + 1 + nPos - nDelimCheck), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true) || isToCmd(sProcedureCommandLine, nPos)))
            {
                if (inliningMode)
                    replaceStringMethod(sProcedureCommandLine, nPos, sLocalStrings[i][0].length(), sLocalStrings[i][1]);
                else
                    sProcedureCommandLine.replace(nPos, sLocalStrings[i][0].length(), sLocalStrings[i][1]);

                nPos += sLocalStrings[i][1].length();
            }
            else
                nPos += sLocalStrings[i][0].length();
        }
    }

    return sProcedureCommandLine;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// resolve the calls to local tables in the
/// passed procedure command line.
///
/// \param sProcedureCommandLine string
/// \param nMapSize size_t
/// \return string
///
/////////////////////////////////////////////////
string ProcedureVarFactory::resolveLocalTables(string sProcedureCommandLine, size_t nMapSize)
{
    if (!nMapSize)
        return sProcedureCommandLine;

    if (nMapSize == string::npos)
        nMapSize = nLocalTableSize;

    for (size_t i = 0; i < nMapSize; i++)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(sLocalTables[i][0] + "(", nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos-1] == '$')
            {
                nPos += sLocalTables[i][0].length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, sLocalTables[i][0].length() + 1 + nPos - nDelimCheck), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true) || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, sLocalTables[i][0].length(), sLocalTables[i][1]);
                nPos += sLocalTables[i][1].length();
            }
            else
                nPos += sLocalTables[i][0].length();
        }
    }

    return sProcedureCommandLine;
}


/////////////////////////////////////////////////
/// \brief This private member function will
/// resolve the calls to local clusters in the
/// passed procedure command line.
///
/// \param sProcedureCommandLine string
/// \param nMapSize size_t
/// \return string
///
/////////////////////////////////////////////////
string ProcedureVarFactory::resolveLocalClusters(string sProcedureCommandLine, size_t nMapSize)
{
    if (!nMapSize)
        return sProcedureCommandLine;

    if (nMapSize == string::npos)
        nMapSize = nLocalClusterSize;

    for (size_t i = 0; i < nMapSize; i++)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(sLocalClusters[i][0] + "{", nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#'))
            {
                nPos += sLocalClusters[i][0].length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, sLocalClusters[i][0].length() + 1 + nPos - nDelimCheck), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true) || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, sLocalClusters[i][0].length(), sLocalClusters[i][1]);
                nPos += sLocalClusters[i][1].length();
            }
            else
                nPos += sLocalClusters[i][0].length();
        }
    }

    return sProcedureCommandLine;
}


