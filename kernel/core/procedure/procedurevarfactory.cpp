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

    sProcName = sProc;
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
            else if (iter.second == TABLETYPE)
                _dataRef->deleteTable(iter.first.substr(0, iter.first.find('(')));
        }

        mLocalArgs.clear();
    }

    if (mLocalVars.size())
    {
        for (auto iter : mLocalVars)
        {
            if (_parserRef)
                _parserRef->RemoveVar(iter.second.first);

            // Deleting a nullptr is harmless
            delete iter.second.second;
        }

        mLocalVars.clear();
    }

    if (mLocalStrings.size())
    {
        for (auto iter : mLocalStrings)
            NumeReKernel::getInstance()->getStringParser().removeStringVar(iter.second.first);

        mLocalStrings.clear();
    }

    if (mLocalTables.size())
    {
        for (auto iter : mLocalTables)
            _dataRef->deleteTable(iter.second);

        mLocalTables.clear();
    }

    if (mLocalClusters.size())
    {
        for (auto iter : mLocalClusters)
            _dataRef->removeCluster(iter.second);

        mLocalClusters.clear();
    }

    sInlineVarDef.clear();
    sInlineStringDef.clear();
    vInlineArgDef.clear();

    mArguments.clear();
}


/////////////////////////////////////////////////
/// \brief Searches for a local table or a local
/// table in the arguments with the
/// corresponding identifier, deletes its name
/// from the map and returns whether the local
/// table was found.
///
/// \param sTableName const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool ProcedureVarFactory::delayDeletionOfReturnedTable(const std::string& sTableName)
{
    // Search for a corresponding local table
    for (auto iter = mLocalTables.begin(); iter != mLocalTables.end(); ++iter)
    {
        if (iter->second == sTableName)
        {
            mLocalTables.erase(iter);
            return true;
        }
    }

    // Sarch for a corresponding local argument table
    auto iter = mLocalArgs.find(sTableName);

    if (iter != mLocalArgs.end() && iter->second == TABLETYPE)
    {
        mLocalArgs.erase(iter);
        return true;
    }

    return false;
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
string ProcedureVarFactory::replaceProcedureName(string sProcedureName) const
{
    for (size_t i = 0; i < sProcedureName.length(); i++)
    {
        if (sProcedureName[i] == ':' || sProcedureName[i] == '\\' || sProcedureName[i] == '/' || sProcedureName[i] == ' ')
            sProcedureName[i] = '_';
    }

    return sProcedureName;
}


/////////////////////////////////////////////////
/// \brief Creates a mangled name for an argument.
///
/// \param sDefinedName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string ProcedureVarFactory::createMangledArgName(const std::string& sDefinedName) const
{
    return "_~"+sProcName+"_~A_"+toString((int)nth_procedure)+"_"+sDefinedName;
}


/////////////////////////////////////////////////
/// \brief Creates a mangled name for a variable.
///
/// \param sDefinedName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string ProcedureVarFactory::createMangledVarName(const std::string& sDefinedName) const
{
    return "_~"+sProcName+"_"+toString((int)nth_procedure)+"_"+sDefinedName;
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
/// are used in the current argument or if the
/// argument name contains invalid characters and
/// throws an exception, if this is the case.
///
/// \param sArgument const string&
/// \param sArgumentList const string&
/// \param nCurrentIndex unsigned int
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::checkArgument(const string& sArgument, const string& sArgumentList, unsigned int nCurrentIndex)
{
    string sCommand = findCommand(sArgument).sString;

    // Was a keyword used as a argument?
    if (sCommand == "var" || sCommand == "tab" || sCommand == "str")
    {
        // Free up memory
        mArguments.clear();

        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();
        // Gather all information in the debugger and throw
        // the exception
        _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
        _debugger.throwException(SyntaxError(SyntaxError::WRONG_ARG_NAME, "$" + sProcName + "(" + sArgumentList + ")", SyntaxError::invalid_position, sCommand));
    }

    // Copy the argument to avoid an segmentation violation due to the deletion
    sCommand = sArgument.substr(0, sArgument.find('='));

    // Check for invalid argument names
    if (!checkSymbolName(sCommand))
    {
        // Free up memory
        mArguments.clear();

        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();
        // Gather all information in the debugger and throw
        // the exception
        _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
        _debugger.throwException(SyntaxError(SyntaxError::INVALID_SYM_NAME, "$" + sProcName + "(" + sArgumentList + ")", SyntaxError::invalid_position, sCommand));
    }
}


/////////////////////////////////////////////////
/// \brief This private member function checks,
/// whether the keywords "var", "str" or "tab"
/// are used in the current argument value and
/// throws an exception, if this is the case.
///
/// \param sArgument const string&
/// \param sArgumentList const string&
/// \param nCurrentIndex unsigned int
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::checkArgumentValue(const string& sArgument, const string& sArgumentList, unsigned int nCurrentIndex)
{
    string sCommand = findCommand(sArgument).sString;

    // Was a keyword used as a argument?
    if (sCommand == "var" || sCommand == "tab" || sCommand == "str")
    {
        // Free up memory
        mArguments.clear();

        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();
        // Gather all information in the debugger and throw
        // the exception
        _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
        _debugger.throwException(SyntaxError(SyntaxError::WRONG_ARG_NAME, "$" + sProcName + "(" + sArgumentList + ")", SyntaxError::invalid_position, sCommand));
    }
}


/////////////////////////////////////////////////
/// \brief Checks for invalid characters or
/// similar.
///
/// \param sSymbolName const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool ProcedureVarFactory::checkSymbolName(const std::string& sSymbolName) const
{
    return sSymbolName.length() && !isdigit(sSymbolName[0]) && toLowerCase(sSymbolName).find_first_not_of(" abcdefghijklmnopqrstuvwxyz1234567890_(){}&") == std::string::npos;
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
    size_t nLocalVarMapSize = countVarListElements(sVarList);

    // Create a new temporary cluster
    string sTempCluster = _dataRef->createTemporaryCluster();
    sInlineVarDef = sTempCluster + " = {";

    sTempCluster.erase(sTempCluster.length()-2);

    // Get a reference to the temporary cluster
    NumeRe::Cluster& tempCluster = _dataRef->getCluster(sTempCluster);

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        std::string currentDef = getNextArgument(sVarList, true);

        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (currentDef.find('=') != string::npos)
        {
            string sVarValue = currentDef.substr(currentDef.find('=')+1);
            sInlineVarDef += sVarValue + ",";

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                _debugger.gatherInformations(this, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
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
                currentDef.erase(currentDef.find('='));
                tempCluster.setDouble(i, _parserRef->Eval());
            }
            catch (...)
            {
                _debugger.gatherInformations(this, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.showError(current_exception());
                throw;
            }
        }
        else
        {
            tempCluster.setDouble(i, 0.0);
            sInlineVarDef += "0,";
        }

        StripSpaces(currentDef);
        std::string currentVar = sTempCluster + "{" + toString(i+1) + "}";

        mLocalVars[currentDef] = std::make_pair(currentVar, nullptr);
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
    size_t nLocalStrMapSize = countVarListElements(sStringList);

    // Create a new temporary cluster
    string sTempCluster = _dataRef->createTemporaryCluster();
    sInlineStringDef = sTempCluster + " = {";
    sTempCluster.erase(sTempCluster.length()-2);

    // Get a reference to the temporary cluster
    NumeRe::Cluster& tempCluster = _dataRef->getCluster(sTempCluster);

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        std::string currentDef = getNextArgument(sStringList, true);
        std::string sVarValue;

        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (currentDef.find('=') != string::npos)
        {
            sVarValue = currentDef.substr(currentDef.find('=')+1);
            sInlineStringDef += sVarValue + ",";

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                _debugger.gatherInformations(this, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                mLocalStrings.clear();
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

                currentDef.erase(currentDef.find('='));
                tempCluster.setString(i, sVarValue);
            }
            catch (...)
            {
                _debugger.gatherInformations(this, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                mLocalStrings.clear();
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

        StripSpaces(currentDef);
        mLocalStrings[currentDef] = std::make_pair(sTempCluster + "{" + toString(i+1) + "}", sVarValue);
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
    size_t nArgumentMapSize = countVarListElements(sArgumentList);

    std::string sArgListBack = sArgumentList;

    // Decode the argument list
    for (unsigned int i = 0; i < nArgumentMapSize; i++)
    {
        std::string currentArg = getNextArgument(sArgumentList);
        StripSpaces(currentArg);

        checkArgument(currentArg, sArgListBack, i);

        // Fill in the value of the argument by either
        // using the default value or the passed value
        std::string currentValue = getNextArgument(sArgumentValues);
        StripSpaces(currentValue);

        checkArgumentValue(currentValue, sArgListBack, i);

        if (currentValue.length() && currentArg.find('=') != string::npos)
        {
            currentArg.erase(currentArg.find('='));
            StripSpaces(currentArg);
        }
        else if (!currentValue.length() && currentArg.find('=') != string::npos)
        {
            currentValue = currentArg.substr(currentArg.find('=')+1);
            currentArg.erase(currentArg.find('='));
            StripSpaces(currentArg);
            StripSpaces(currentValue);
        }
        else if (!currentValue.length() && currentArg.find('=') == string::npos)
        {
            string sErrorToken = currentArg;
            nArgumentMapSize = 0;

            if (_optionRef->useDebugger())
                _debugger.popStackItem();

            throw SyntaxError(SyntaxError::MISSING_DEFAULT_VALUE, sArgListBack, sErrorToken, sErrorToken);
        }

        if (containsFreeOperators(currentValue))
            currentValue = "(" + currentValue + ")";

        // Evaluate procedure calls and the parentheses of the
        // passed tables and clusters
        evaluateProcedureArguments(currentArg, currentValue, sArgListBack);
    }


    mVarMap.insert(mArguments.begin(), mArguments.end());
    return mVarMap;
}


/////////////////////////////////////////////////
/// \brief Determines, whether the user has
/// passed a complete cluster or a cluster with
/// some indices.
///
/// \param sArgumentValue StringView
/// \param _dataRef MemoryManager*
/// \return bool
///
/////////////////////////////////////////////////
static bool isCompleteCluster(StringView sArgumentValue, MemoryManager* _dataRef)
{
    size_t pos = sArgumentValue.find('{');

    return _dataRef->isCluster(sArgumentValue) && (sArgumentValue.subview(pos, 2) == "{}" || sArgumentValue.subview(pos, 3) == "{:}");
}


/////////////////////////////////////////////////
/// \brief Determines, whether the user has
/// passed a complete table or a table with
/// some indices.
///
/// \param sArgumentValue StringView
/// \param _dataRef MemoryManager*
/// \return bool
///
/////////////////////////////////////////////////
static bool isCompleteTable(StringView sArgumentValue, MemoryManager* _dataRef)
{
    size_t pos = sArgumentValue.find('(');

    return _dataRef->isTable(sArgumentValue.to_string()) && (sArgumentValue.subview(pos, 2) == "()"
                                                             || sArgumentValue.subview(pos, 3) == "(:)"
                                                             || sArgumentValue.subview(pos, 5) == "(:,:)");
}


/////////////////////////////////////////////////
/// \brief This memberfunction will evaluate the
/// passed procedure arguments and convert them
/// to local variables if necessary.
///
/// \param currentArg std::string&
/// \param currentValue std::string&
/// \param sArgumentList const std::string&
/// \return void
///
/////////////////////////////////////////////////
void ProcedureVarFactory::evaluateProcedureArguments(std::string& currentArg, std::string& currentValue, const std::string& sArgumentList)
{
    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();
    NumeRe::StringParser& _stringParser = NumeReKernel::getInstance()->getStringParser();
    bool isTemplate = _currentProcedure->getProcedureFlags() & ProcedureCommandLine::FLAG_TEMPLATE;
    bool isMacro = _currentProcedure->getProcedureFlags() & ProcedureCommandLine::FLAG_MACRO || inliningMode;

    // Evaluate procedure calls first (but not for tables)
    if (currentValue.find('$') != string::npos
        && currentValue.find('(') != string::npos
        && currentArg.substr(currentArg.length()-2) != "()")
    {
        if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
        {
            _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sArgumentList, SyntaxError::invalid_position));
        }

        FlowCtrl::ProcedureInterfaceRetVal nReturn = _currentProcedure->procedureInterface(currentValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

        if (nReturn == FlowCtrl::INTERFACE_ERROR)
        {
            _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            _debugger.throwException(SyntaxError(SyntaxError::PROCEDURE_ERROR, sArgumentList, SyntaxError::invalid_position));
        }
        else if (nReturn == FlowCtrl::INTERFACE_EMPTY)
            currentValue = "false";
    }

    bool isRef = currentArg.front() == '&' || currentArg.back() == '&';

    // Determine, if this is a reference (and
    // remove the ampersand in this case)
    if (isRef)
    {
        currentArg.erase(currentArg.find('&'), 1);
        StripSpaces(currentArg);
    }

    if (currentArg.length() > 2 && currentArg.substr(currentArg.length()-2) == "()")
    {
        currentArg.pop_back();

#warning TODO (numere#1#02/27/22): This behavior will be deprecated with v1.1.5
        if (!_optionRef->getSetting(SETTING_B_FUTURE).active() && !isRef)
        {
            isRef = true;
            NumeReKernel::issueWarning(_currentProcedure->getCurrentProcedureName() + ": " + _lang.get("PROCEDURE_WARN_TABLE_REFERENCE"));
        }

        if (isRef)
        {
            if (!isCompleteTable(currentValue, _dataRef))
            {
                _debugger.gatherInformations(this, currentValue, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.throwException(SyntaxError(SyntaxError::CANNOT_PASS_LITERAL_PER_REFERENCE, currentValue, "", currentArg + ")"));
            }

            if (currentValue.find('(') != string::npos)
                currentValue.erase(currentValue.find('('));
        }
        else if (!isRef && !isMacro) // Macros do the old copy-paste logic
        {
            // Create a local variable
            std::string sNewArgName = "_~"+sProcName+"_~A_"+toString((int)nth_procedure)+"_"+currentArg.substr(0, currentArg.length()-1);

            // Evaluate procedure calls first
            if (currentValue.find('$') != string::npos
                && currentValue.find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sArgumentList, SyntaxError::invalid_position));
                }

                currentValue.insert(0, sNewArgName + "() = ");
                FlowCtrl::ProcedureInterfaceRetVal nReturn = _currentProcedure->procedureInterface(currentValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                if (nReturn == FlowCtrl::INTERFACE_ERROR)
                {
                    _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.throwException(SyntaxError(SyntaxError::PROCEDURE_ERROR, sArgumentList, SyntaxError::invalid_position));
                }
                else if (nReturn == FlowCtrl::INTERFACE_EMPTY)
                    currentValue = "false";
                else
                {
                    if (currentValue.substr(0, sNewArgName.length()+5) != sNewArgName+"() = ")
                    {
                        currentValue = sNewArgName;
                        mLocalArgs[sNewArgName] = TABLETYPE;
                        mArguments[currentArg] = currentValue;
                        return;
                    }
                    else
                        currentValue.erase(0, sNewArgName.length()+5);
                }
            }

            if (isCompleteTable(currentValue, _dataRef))
                _dataRef->copyTable(currentValue.substr(0, currentValue.find('(')), sNewArgName);
            else
            {
                // Evaluate the expression and create a new
                // table
                try
                {
                    std::string sCurrentValue = currentValue;
                    _dataRef->addTable(sNewArgName, *_optionRef);

                    if (_dataRef->containsTablesOrClusters(sCurrentValue))
                        getDataElements(sCurrentValue, *_parserRef, *_dataRef, *_optionRef, false);

                    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentValue))
                    {
                        std::string sTable = sNewArgName + "(:,1)";
                        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCurrentValue, sTable, true);
                    }
                    else
                    {
                        mu::value_type* v = nullptr;
                        int nResults;
                        _parserRef->SetExpr(sCurrentValue);
                        v = _parserRef->Eval(nResults);
                        Indices _idx;
                        _idx.row = VectorIndex(0, VectorIndex::OPEN_END);
                        _idx.col = VectorIndex(0);
                        _dataRef->writeToTable(_idx, sNewArgName, v, nResults);
                    }
                }
                catch (...)
                {
                    _debugger.gatherInformations(this, sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.showError(current_exception());
                    throw;
                }
            }

            // Tables have to be stored as names without
            // their braces
            currentValue = sNewArgName;
            mLocalArgs[sNewArgName] = TABLETYPE;
        }
    }
    else if (currentArg.length() > 2 && currentArg.substr(currentArg.length()-2) == "{}")
    {
        currentArg.pop_back();

        if (isRef && !isCompleteCluster(currentValue, _dataRef))
        {
            _debugger.gatherInformations(this, currentValue, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            _debugger.throwException(SyntaxError(SyntaxError::CANNOT_PASS_LITERAL_PER_REFERENCE, currentValue, "", currentArg));
        }
        else if (!isRef && !isMacro) // Macros do the old copy-paste logic
        {
            // Create a local variable
            std::string sNewArgName = "_~"+sProcName+"_~A_"+toString((int)nth_procedure)+"_"+currentArg.substr(0, currentArg.length()-1);
            NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);

            // Copy, if it is already a (complete!) cluster
            if (isCompleteCluster(currentValue, _dataRef))
                newCluster = _dataRef->getCluster(currentValue.substr(0, currentValue.find('{')));
            else
            {
                // Evaluate the expression and create a new
                // cluster
                try
                {
                    std::string sCurrentValue = currentValue;

                    if (_dataRef->containsTablesOrClusters(sCurrentValue))
                        getDataElements(sCurrentValue, *_parserRef, *_dataRef, *_optionRef, false);

                    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentValue))
                    {
                        std::string sCluster = sNewArgName + "{}";
                        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCurrentValue, sCluster, true);
                    }
                    else
                    {
                        mu::value_type* v = nullptr;
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
            currentValue = sNewArgName;
            mLocalArgs[sNewArgName] = CLUSTERTYPE;
        }
        else if (inliningMode && (currentValue.length() < 3 || currentValue.substr(currentValue.length()-2) != "{}"))
        {
            std::string sTempCluster = _dataRef->createTemporaryCluster();
            vInlineArgDef.push_back(sTempCluster + " = " + currentValue + ";");
            currentValue = sTempCluster;
        }

        if (currentValue.find('{') != string::npos)
            currentValue.erase(currentValue.find('{'));
    }
    else
    {
        if (isRef)
        {
            const auto& varMap = _parserRef->GetVar();
            const auto& stringMap = _stringParser.getStringVars();

            // Reference
            if (varMap.find(currentValue) == varMap.end()
                && stringMap.find(currentValue) == stringMap.end()
                && !(isTemplate && _dataRef->isCluster(currentValue))
                && !(isTemplate && currentValue.substr(0, 7) == "string(")
                && !(isTemplate && _dataRef->isTable(currentValue)))
            {
                _debugger.gatherInformations(this, currentValue, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.throwException(SyntaxError(SyntaxError::CANNOT_PASS_LITERAL_PER_REFERENCE, currentValue, "", currentArg));
            }
        }
        else if (!isMacro) // macros do the old copy-paste logic
        {
            // Create a local variable
            std::string sNewArgName = createMangledArgName(currentArg);

            if (!_functionRef->call(currentValue))
            {
                _debugger.gatherInformations(this, currentValue, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.throwException(SyntaxError(SyntaxError::FUNCTION_ERROR, currentArg, SyntaxError::invalid_position));
            }

            try
            {
                // Templates allow also other types
                if (isTemplate)
                {
                    StripSpaces(currentValue);

                    if (currentValue.find("()") != std::string::npos && _dataRef->isTable(currentValue))
                    {
                        mArguments[currentArg] = currentValue; // Tables are always references
                        return;
                    }
                    else if (currentValue.find("{}") != std::string::npos && _dataRef->isCluster(currentValue))
                    {
                        // Copy clusters
                        NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);
                        newCluster = _dataRef->getCluster(currentValue.substr(0, currentValue.find('{')));

                        currentValue = sNewArgName + "{}";
                        mLocalArgs[sNewArgName + "{}"] = CLUSTERTYPE;
                        mArguments[currentArg] = currentValue;
                        return;
                    }

                    // Get data, if necessary
                    if (_dataRef->containsTablesOrClusters(currentValue))
                        getDataElements(currentValue, *_parserRef, *_dataRef, *_optionRef);

                    // Evaluate strings
                    if (_stringParser.isStringExpression(currentValue))
                    {
                        NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);
                        std::string sCluster = sNewArgName + "{}";
                        NumeRe::StringParser::StringParserRetVal ret =  _stringParser.evalAndFormat(currentValue, sCluster, true);

                        if (ret == NumeRe::StringParser::STRING_SUCCESS)
                        {
                            if (newCluster.size() == 1)
                            {
                                _stringParser.setStringValue(sNewArgName, newCluster.getString(0));
                                _dataRef->removeCluster(sNewArgName);
                                currentValue = sNewArgName;
                                mLocalArgs[sNewArgName] = STRINGTYPE;
                            }
                            else
                            {
                                currentValue = sNewArgName + "{}";
                                mLocalArgs[sNewArgName + "{}"] = CLUSTERTYPE;
                            }

                            mArguments[currentArg] = currentValue;
                            return;
                        }
                    }

                    // Evaluate numerical expressions
                    _parserRef->SetExpr(currentValue);
                    int nRes = 0;
                    mu::value_type* v = _parserRef->Eval(nRes);

                    if (nRes > 1)
                    {
                        NumeRe::Cluster& newCluster = _dataRef->newCluster(sNewArgName);
                        newCluster.setDoubleArray(nRes, v);
                        currentValue = sNewArgName + "{}";
                        mLocalArgs[sNewArgName + "{}"] = CLUSTERTYPE;
                    }
                    else
                    {
                        mu::value_type* newVar = new mu::value_type;
                        *newVar = v[0];
                        _parserRef->m_lDataStorage.push_back(newVar);
                        _parserRef->DefineVar(sNewArgName, newVar);
                        currentValue = sNewArgName;
                        mLocalArgs[sNewArgName] = NUMTYPE;
                    }

                    mArguments[currentArg] = currentValue;
                    return;
                }
                else
                {
                    // Get data, if necessary
                    if (_dataRef->containsTablesOrClusters(currentValue))
                        getDataElements(currentValue, *_parserRef, *_dataRef, *_optionRef);

                    // Evaluate strings
                    if (_stringParser.isStringExpression(currentValue))
                    {
                        std::string dummy;
                        NumeRe::StringParser::StringParserRetVal ret =  _stringParser.evalAndFormat(currentValue, dummy, true);

                        if (ret == NumeRe::StringParser::STRING_SUCCESS)
                        {
                            _stringParser.setStringValue(sNewArgName, getNextArgument(currentValue));
                            currentValue = sNewArgName;
                            mLocalArgs[sNewArgName] = STRINGTYPE;
                            mArguments[currentArg] = currentValue;
                            return;
                        }
                    }

                    // Evaluate numerical expressions
                    _parserRef->SetExpr(sNewArgName + " = " + currentValue);
                    _parserRef->Eval();
                }
            }
            catch (...)
            {
                _debugger.gatherInformations(this, currentValue, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.showError(current_exception());
                throw;
            }

            currentValue = sNewArgName;
            mLocalArgs[sNewArgName] = NUMTYPE;
        }
    }

    mArguments[currentArg] = currentValue;
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
    if (!_currentProcedure)
        return;

    if (inliningMode)
    {
        createLocalInlineVars(sVarList);
        return;
    }

    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Get the number of declared variables
    size_t nLocalVarMapSize = countVarListElements(sVarList);

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        std::string currentDef = getNextArgument(sVarList, true);
        mu::value_type currentVal = 0;

        std::string sSymbol = currentDef.substr(0, currentDef.find('='));

        if (!checkSymbolName(sSymbol))
        {
            _debugger.gatherInformations(this, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            _debugger.throwException(SyntaxError(SyntaxError::INVALID_SYM_NAME, sVarList, SyntaxError::invalid_position, sSymbol));
        }

        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (currentDef.find('=') != string::npos)
        {
            std::string sVarValue = currentDef.substr(currentDef.find('=')+1);

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    _debugger.gatherInformations(this, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sVarList, SyntaxError::invalid_position));
                }

                try
                {
                    FlowCtrl::ProcedureInterfaceRetVal nReturn = _currentProcedure->procedureInterface(sVarValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                    if (nReturn == FlowCtrl::INTERFACE_ERROR)
                        throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sVarList, SyntaxError::invalid_position);
                    else if (nReturn == FlowCtrl::INTERFACE_EMPTY)
                        sVarValue = "false";
                }
                catch (...)
                {
                    _debugger.gatherInformations(this, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
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
                currentDef.erase(currentDef.find('='));
                currentVal = _parserRef->Eval();
            }
            catch (...)
            {
                _debugger.gatherInformations(this, sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                _debugger.showError(current_exception());
                throw;
            }
        }

        StripSpaces(currentDef);
        std::string currentVar = createMangledVarName(currentDef);

        mLocalVars[currentDef] = std::make_pair(currentVar, new mu::value_type);
        *mLocalVars[currentDef].second = currentVal;

        _parserRef->DefineVar(currentVar, mLocalVars[currentDef].second);
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
    if (!_currentProcedure)
        return;

    if (inliningMode)
    {
        createLocalInlineStrings(sStringList);
        return;
    }

    NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

    // Get the number of declared variables
    size_t nLocalStrMapSize = countVarListElements(sStringList);

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        std::string currentDef = getNextArgument(sStringList, true);
        std::string sVarValue;

        std::string sSymbol = currentDef.substr(0, currentDef.find('='));

        if (!checkSymbolName(sSymbol))
        {
            _debugger.gatherInformations(this, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            _debugger.throwException(SyntaxError(SyntaxError::INVALID_SYM_NAME, sStringList, SyntaxError::invalid_position, sSymbol));
        }
        // Fill in the value of the variable by either
        // using the default or the explicit passed value
        if (currentDef.find('=') != string::npos)
        {
            sVarValue = currentDef.substr(currentDef.find('=')+1);

            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    _debugger.gatherInformations(this, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.throwException(SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sStringList, SyntaxError::invalid_position));
                }

                try
                {
                    FlowCtrl::ProcedureInterfaceRetVal nReturn = _currentProcedure->procedureInterface(sVarValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                    if (nReturn == FlowCtrl::INTERFACE_ERROR)
                        throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sStringList, SyntaxError::invalid_position);
                    else if (nReturn == FlowCtrl::INTERFACE_EMPTY)
                        sVarValue = "false";
                }
                catch (...)
                {
                    _debugger.gatherInformations(this, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    _debugger.showError(current_exception());
                    throw;
                }
            }

            sVarValue = resolveLocalStrings(sVarValue, i);

            if (_dataRef->containsTablesOrClusters(sVarValue))
                getDataElements(sVarValue, *_parserRef, *_dataRef, *_optionRef);

            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sVarValue))
            {
                string sTemp;
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sVarValue, sTemp, true);
            }

            currentDef.erase(currentDef.find('='));
        }

        StripSpaces(currentDef);
        std::string currentVar = createMangledVarName(currentDef);

        try
        {
            NumeReKernel::getInstance()->getStringParser().setStringValue(currentVar, sVarValue);
        }
        catch (...)
        {
            _debugger.gatherInformations(this, sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            _debugger.showError(current_exception());
            throw;
        }

        mLocalStrings[currentDef] = std::make_pair(currentVar, sVarValue);
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
    if (!_currentProcedure)
        return;

    if (inliningMode)
        return;

    // Get the number of declared variables
    size_t nLocalTableSize = countVarListElements(sTableList);
    std::string sCurrentValue;

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalTableSize; i++)
    {
        std::string currentDef = getNextArgument(sTableList, true);

        std::string sSymbol = currentDef.substr(0, currentDef.find('='));

        if (!checkSymbolName(sSymbol))
        {
            NumeReKernel::getInstance()->getDebugger().gatherInformations(this, sTableList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            NumeReKernel::getInstance()->getDebugger().throwException(SyntaxError(SyntaxError::INVALID_SYM_NAME, sTableList, SyntaxError::invalid_position, sSymbol));
        }

        if (currentDef.find('=') != string::npos)
            sCurrentValue = currentDef.substr(currentDef.find('=')+1);

        if (currentDef.find('(') != string::npos)
            currentDef.erase(currentDef.find('('));

        StripSpaces(currentDef);
        StripSpaces(sCurrentValue);
        std::string currentVar = createMangledVarName(currentDef);

        try
        {
            _dataRef->addTable(currentVar, *_optionRef);

            if (sCurrentValue.length())
            {
                sCurrentValue = resolveLocalTables(sCurrentValue, i);

                if (sCurrentValue.find('$') != string::npos && sCurrentValue.find('(', sCurrentValue.find('$')+1))
                {
                    if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                    {
                        throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sTableList, SyntaxError::invalid_position);
                    }

                    sCurrentValue.insert(0, currentVar + "() = ");

                    FlowCtrl::ProcedureInterfaceRetVal nReturn = _currentProcedure->procedureInterface(sCurrentValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                    if (nReturn == FlowCtrl::INTERFACE_ERROR)
                        throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sTableList, SyntaxError::invalid_position);
                    else if (nReturn == FlowCtrl::INTERFACE_EMPTY)
                        sCurrentValue = "false";
                    else
                    {
                        if (sCurrentValue.substr(0, currentVar.length()+5) == currentVar + "() = ")
                            currentVar.erase(0, currentVar.length()+5);
                        else
                        {
                            mLocalTables[currentDef] = currentVar;
                            continue;
                        }
                    }
                }

                if (isCompleteTable(sCurrentValue, _dataRef))
                    _dataRef->copyTable(sCurrentValue.substr(0, sCurrentValue.find('(')), currentVar);
                else
                {
                    if (_dataRef->containsTablesOrClusters(sCurrentValue))
                        getDataElements(sCurrentValue, *_parserRef, *_dataRef, *_optionRef, false);

                    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentValue))
                    {
                        std::string sTable = currentVar + "(:,1)";
                        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCurrentValue, sTable, true);
                    }
                    else
                    {
                        mu::value_type* v = nullptr;
                        int nResults;
                        _parserRef->SetExpr(sCurrentValue);
                        v = _parserRef->Eval(nResults);
                        Indices _idx;
                        _idx.row = VectorIndex(0, VectorIndex::OPEN_END);
                        _idx.col = VectorIndex(0);
                        _dataRef->writeToTable(_idx, currentVar, v, nResults);
                    }
                }
            }
        }
        catch (...)
        {
            NumeReKernel::getInstance()->getDebugger().gatherInformations(this, sTableList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            throw;
        }

        mLocalTables[currentDef] = currentVar;
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
    if (!_currentProcedure)
        return;

    if (inliningMode)
        return;

    // Get the number of declared variables
    size_t nLocalClusterSize = countVarListElements(sClusterList);
    std::string sCurrentValue;

    // Decode the variable list
    for (unsigned int i = 0; i < nLocalClusterSize; i++)
    {
        std::string currentDef = getNextArgument(sClusterList, true);

        std::string sSymbol = currentDef.substr(0, currentDef.find('='));

        if (!checkSymbolName(sSymbol))
        {
            NumeReKernel::getInstance()->getDebugger().gatherInformations(this, sClusterList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            NumeReKernel::getInstance()->getDebugger().throwException(SyntaxError(SyntaxError::INVALID_SYM_NAME, sClusterList, SyntaxError::invalid_position, sSymbol));
        }

        if (currentDef.find('=') != string::npos)
            sCurrentValue = currentDef.substr(currentDef.find('=')+1);

        if (currentDef.find('{') != string::npos)
            currentDef.erase(currentDef.find('{'));

        StripSpaces(sCurrentValue);
        StripSpaces(currentDef);
        std::string currentVar = createMangledVarName(currentDef);

        try
        {
            NumeRe::Cluster& cluster = _dataRef->newCluster(currentVar);

            if (sCurrentValue.length())
            {
                sCurrentValue = resolveLocalClusters(sCurrentValue, i);

                if (sCurrentValue.find('$') != string::npos && sCurrentValue.find('(', sCurrentValue.find('$')+1))
                {
                    if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                    {
                        throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sClusterList, SyntaxError::invalid_position);
                    }

                    FlowCtrl::ProcedureInterfaceRetVal nReturn = _currentProcedure->procedureInterface(sCurrentValue, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);

                    if (nReturn == FlowCtrl::INTERFACE_ERROR)
                        throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sClusterList, SyntaxError::invalid_position);
                    else if (nReturn == FlowCtrl::INTERFACE_EMPTY)
                        sCurrentValue = "false";
                }

                if (isCompleteCluster(sCurrentValue, _dataRef))
                    cluster = _dataRef->getCluster(sCurrentValue.substr(0, sCurrentValue.find('{')));
                else
                {
                    if (_dataRef->containsTablesOrClusters(sCurrentValue))
                        getDataElements(sCurrentValue, *_parserRef, *_dataRef, *_optionRef, false);

                    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCurrentValue))
                    {
                        string sCluster = currentVar + "{}";
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
            }
        }
        catch (...)
        {
            NumeReKernel::getInstance()->getDebugger().gatherInformations(this, sClusterList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            throw;
        }

        mLocalClusters[currentDef] = currentVar;
    }
}


/////////////////////////////////////////////////
/// \brief Creates a special cluster containing
/// the test statistics.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string ProcedureVarFactory::createTestStatsCluster()
{
    const std::string currentDef = "TESTINFO";
    std::string currentVar = createMangledVarName(currentDef);
    NumeRe::Cluster& testCluster = _dataRef->newCluster(currentVar);

    testCluster.setString(0, "tests");
    testCluster.setDouble(1, 0);
    testCluster.setString(2, "successes");
    testCluster.setDouble(3, 0);
    testCluster.setString(4, "fails");
    testCluster.setDouble(5, 0);

    mLocalClusters[currentDef] = currentVar;

    return currentVar;
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

    for (const auto& iter : mArguments)
    {
        unsigned int nPos = 0;
        size_t nArgumentBaseLength = iter.first.length();

        if (iter.first.back() == '(' || iter.first.back() == '{')
            nArgumentBaseLength--;

        while ((nPos = sProcedureCommandLine.find(iter.first.substr(0, nArgumentBaseLength), nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~',nPos-1)] != '#')
                || (iter.first.back() != '(' && sProcedureCommandLine[nPos+nArgumentBaseLength] == '(')
                || (iter.first.back() != '{' && sProcedureCommandLine[nPos+nArgumentBaseLength] == '{'))
            {
                nPos += iter.first.length();
                continue;
            }

            if (checkDelimiter(sProcedureCommandLine.substr(nPos-1, nArgumentBaseLength+2), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true)
                    || isToCmd(sProcedureCommandLine, nPos)))
            {
                if ((iter.second.front() == '{' || iter.second.back() == ')') && iter.first.back() == '{')
                    sProcedureCommandLine.replace(nPos, getMatchingParenthesis(sProcedureCommandLine.substr(nPos))+1, iter.second);
                else
                    sProcedureCommandLine.replace(nPos, nArgumentBaseLength, iter.second);

                nPos += iter.second.length();
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

    for (const auto& iter : mLocalVars)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(iter.first, nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos+iter.first.length()] == '(')
            {
                nPos += iter.first.length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, iter.first.length() + 1 + nPos - nDelimCheck))
                && (!isInQuotes(sProcedureCommandLine, nPos, true)
                    || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, iter.first.length(), iter.second.first);
                nPos += iter.second.first.length();
            }
            else
                nPos += iter.first.length();
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

    for (const auto& iter : mLocalStrings)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(iter.first, nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos+iter.first.length()] == '(')
            {
                nPos += iter.first.length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, iter.first.length() + 1 + nPos - nDelimCheck), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true) || isToCmd(sProcedureCommandLine, nPos)))
            {
                if (inliningMode)
                    replaceStringMethod(sProcedureCommandLine, nPos, iter.first.length(), iter.second.first);
                else
                    sProcedureCommandLine.replace(nPos, iter.first.length(), iter.second.first);

                nPos += iter.second.first.length();
            }
            else
                nPos += iter.first.length();
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

    for (const auto& iter : mLocalTables)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(iter.first + "(", nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos-1] == '$')
            {
                nPos += iter.first.length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, iter.first.length() + 1 + nPos - nDelimCheck), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true) || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, iter.first.length(), iter.second);
                nPos += iter.second.length();
            }
            else
                nPos += iter.first.length();
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

    for (const auto& iter : mLocalClusters)
    {
        size_t nPos = 0;
        size_t nDelimCheck = 0;

        while ((nPos = sProcedureCommandLine.find(iter.first + "{", nPos)) != string::npos)
        {
            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#'))
            {
                nPos += iter.first.length();
                continue;
            }

            nDelimCheck = nPos-1;

            if ((sProcedureCommandLine[nDelimCheck] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nDelimCheck)] == '#'))
                nDelimCheck = sProcedureCommandLine.find_last_not_of('~', nDelimCheck);

            if (checkDelimiter(sProcedureCommandLine.substr(nDelimCheck, iter.first.length() + 1 + nPos - nDelimCheck), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true) || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, iter.first.length(), iter.second);
                nPos += iter.second.length();
            }
            else
                nPos += iter.first.length();
        }
    }

    return sProcedureCommandLine;
}


