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


// Implementation der FlowCtrl-Klasse
#include "flowctrl.hpp"
#include "../ui/error.hpp"
#include "../../kernel.hpp"
#include "../maths/parser_functions.hpp"
#include "../built-in.hpp"
#include "../utils/tools.hpp"
#include "../plotting/plotting.hpp"

// Definition of special return values
enum
{
    FLOWCTRL_ERROR = -1,
    FLOWCTRL_RETURN = -2,
    FLOWCTRL_BREAK = -3,
    FLOWCTRL_CONTINUE = -4,
    FLOWCTRL_LEAVE = -5,
    FLOWCTRL_NO_CMD = -6,
    FLOWCTRL_OK = 1
};

// Definition of standard values of the jump table
#define NO_FLOW_COMMAND -1
#define BLOCK_END 0
#define BLOCK_MIDDLE 1
#define PROCEDURE_INTERFACE 2
#define JUMP_TABLE_ELEMENTS 3 // The maximal number of elements
extern value_type vAns;

using namespace std;


/////////////////////////////////////////////////
/// \brief A simple helper function to extract
/// the header expression of control flow
/// statements.
///
/// \param sExpr const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string extractHeaderExpression(const std::string& sExpr)
{
    size_t p = sExpr.find('(');
    return sExpr.substr(p + 1, sExpr.rfind(')') - p - 1);
}


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
FlowCtrl::FlowCtrl()
{
    _parserRef = nullptr;
    _dataRef = nullptr;
    _outRef = nullptr;
    _optionRef = nullptr;
    _functionRef = nullptr;
    _pDataRef = nullptr;
    _scriptRef = nullptr;

    nLoopSafety = -1;

    for (size_t i = 0; i < FC_COUNT; i++)
        nFlowCtrlStatements[i] = 0;

    nCurrentCommand = 0;
    nDebuggerCode = 0;
    blockNames.clear();
    sLoopPlotCompose = "";
    bSilent = true;
    bMask = false;
    bLoopSupressAnswer = false;

    bPrintedStatus = false;

    bUseLoopParsingMode = false;
    bLockedPauseMode = false;

    bFunctionsReplaced = false;
    nthRecursion = 0;
    bEvaluatingFlowControlStatements = false;

    nReturnType = 1;
    bReturnSignal = false;
}


/////////////////////////////////////////////////
/// \brief Destructor. Cleanes the memory, if
/// necessary.
/////////////////////////////////////////////////
FlowCtrl::~FlowCtrl()
{
}


/////////////////////////////////////////////////
/// \brief Returns the current block depth while
/// reading a flow control statement to memory.
///
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::getCurrentBlockDepth() const
{
    int nCount = 0;

    for (size_t i = 0; i < FC_COUNT; i++)
    {
        nCount += nFlowCtrlStatements[i];
    }

    return nCount;
}


/////////////////////////////////////////////////
/// \brief Converts the block Ids to human
/// readable strings.
///
/// \param blockId FlowCtrl::FlowCtrlBlock
/// \return std::string
///
/////////////////////////////////////////////////
std::string FlowCtrl::getBlockName(FlowCtrl::FlowCtrlBlock blockId)
{
    switch (blockId)
    {
        case FCB_NONE:
            return "";
        case FCB_FOR:
            return "FOR";
        case FCB_WHL:
            return "WHL";
        case FCB_IF:
            return "IF";
        case FCB_ELIF:
            return "ELIF";
        case FCB_ELSE:
            return "ELSE";
        case FCB_SWCH:
            return "SWCH";
        case FCB_CASE:
            return "CASE";
        case FCB_DEF:
            return "DEF";
        case FCB_TRY:
            return "TRY";
        case FCB_CTCH:
            return "CTCH";
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief This member function realizes the FOR
/// control flow statement. The return value is
/// either an error value or the end of the
/// current flow control statement.
///
/// \param nth_Cmd int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::for_loop(int nth_Cmd, int nth_loop)
{
    int nVarAdress = 0;
    int nInc = 1;
    int nLoopCount = 0;
    int nFirstVal;
    int nLastVal;
    bPrintedStatus = false;

    int nNum = 0;
    value_type* v = 0;

    if (!vCmdArray[nth_Cmd].sFlowCtrlHeader.length())
    {
        vCmdArray[nth_Cmd].sFlowCtrlHeader = vCmdArray[nth_Cmd].sCommand.substr(vCmdArray[nth_Cmd].sCommand.find('=') + 1);
        StringView sVar(vCmdArray[nth_Cmd].sCommand, vCmdArray[nth_Cmd].sCommand.find(' ') + 1);
        sVar.remove_from(sVar.find('='));
        sVar.strip();

        if (sVar.starts_with("|>"))
        {
            vCmdArray[nth_Cmd].sFlowCtrlHeader.insert(0, "|>");
            sVar.trim_front(2);
        }

        sVar.strip();

        // Get the variable address of the loop
        // index
        for (size_t i = 0; i < sVarArray.size(); i++)
        {
            if (sVarArray[i] == sVar)
            {
                vCmdArray[nth_Cmd].nVarIndex = i;
                break;
            }
        }
    }

    std::string sHead = vCmdArray[nth_Cmd].sFlowCtrlHeader;
    nVarAdress = vCmdArray[nth_Cmd].nVarIndex;

    // Evaluate the header of the for loop
    v = evalHeader(nNum, sHead, true, nth_Cmd, "for");

    // Store the left and right boundary of the
    // loop index
    nFirstVal = intCast(v[0]);
    nLastVal = intCast(v[1]);

    // Depending on the order of the boundaries, we
    // have to consider the incrementation variable
    if (nLastVal < nFirstVal)
        nInc *= -1;

    // Print to the terminal, if needed
    if (bSilent && !nth_loop && !bMask)
    {
        NumeReKernel::printPreFmt("|FOR> " + _lang.get("COMMON_EVALUATING") + " ... 0 %");
        bPrintedStatus = true;
    }

    // Evaluate the whole for loop. The outer loop does the
    // loop index management (the actual "for" command), the
    // inner loop runs through the contained command lines
    for (int __i = nFirstVal; (nInc)*__i <= nInc * nLastVal; __i += nInc)
    {
        vVarArray[nVarAdress] = __i;

        // Ensure that the loop is aborted, if the
        // maximal number of repetitions has been
        // performed
        if (nLoopSafety > 0)
        {
            if (nLoopCount >= nLoopSafety)
                return FLOWCTRL_ERROR;

            nLoopCount++;
        }

        // This for loop handles the contained commands
        for (int __j = nth_Cmd+1; __j < nJumpTable[nth_Cmd][BLOCK_END]; __j++)
        {
            nCurrentCommand = __j;

            // If this is not the first line of the command block
            // try to find control flow statements in the first column
            if (vCmdArray[__j].fcFn)
            {
                // Evaluate the flow control commands
                int nReturn = (this->*vCmdArray[__j].fcFn)(__j, nth_loop+1);

                // Handle the return value
                if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN || nReturn == FLOWCTRL_LEAVE)
                    return nReturn;
                else if (nReturn == FLOWCTRL_BREAK)
                    return nJumpTable[nth_Cmd][BLOCK_END];
                else if (nReturn == FLOWCTRL_CONTINUE)
                    break;
                else if (nReturn != FLOWCTRL_NO_CMD)
                    __j = nReturn;

                continue;
            }

            // Handle the "continue" and "break" flow
            // control statements
            if (nCalcType[__j] & CALCTYPE_CONTINUECMD)
                break;
            else if (nCalcType[__j] & CALCTYPE_BREAKCMD)
                return nJumpTable[nth_Cmd][BLOCK_END];
            else if (__i == nFirstVal && !nCalcType[__j])
            {
                std::string sCommand = findCommand(vCmdArray[__j].sCommand).sString;

                // Evaluate the commands, store the bytecode
                if (sCommand == "continue")
                {
                    nCalcType[__j] = CALCTYPE_CONTINUECMD;

                    // "continue" is a break of the inner loop
                    break;
                }

                if (sCommand == "break")
                {
                    nCalcType[__j] = CALCTYPE_BREAKCMD;

                    // "break" requires to leave the current
                    // for loop. Therefore it is realized as
                    // return statement, returning the end index
                    // of the current block
                    return nJumpTable[nth_Cmd][BLOCK_END];
                }

                if (sCommand == "leave")
                {
                    // "leave" requires to leave the complete
                    // flow control block
                    return FLOWCTRL_LEAVE;
                }
            }

            // Increment the parser index, if the loop parsing
            // mode was activated
            if (bUseLoopParsingMode)
                _parserRef->SetIndex(__j);

            try
            {
                // Evaluate the command line with the calc function
                if (calc(vCmdArray[__j].sCommand, __j) == FLOWCTRL_ERROR)
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand,
                                                                                           getCurrentLineNumber(),
                                                                                           mVarMap, vVarArray, sVarArray);
                    getErrorInformationForDebugger();
                    return FLOWCTRL_ERROR;
                }

                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand,
                                                                                       getCurrentLineNumber(),
                                                                                       mVarMap, vVarArray, sVarArray);
                getErrorInformationForDebugger();
                NumeReKernel::getInstance()->getDebugger().showError(current_exception());
                nCalcType[__j] = CALCTYPE_NONE;
                catchExceptionForTest(current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());
            }
        }

        // The variable value might have been changed
        // snychronize the index
        __i = intCast(vVarArray[nVarAdress]);

        // Print the status to the terminal, if it is required
        if (!nth_loop && !bMask && bSilent)
        {
            if (abs(nLastVal - nFirstVal) < 99999
                    && abs(intCast((__i - nFirstVal) / double(nLastVal - nFirstVal) * 20.0))
                    > abs(intCast((__i - 1.0 - nFirstVal) / double(nLastVal - nFirstVal) * 20.0)))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... "
                                          + toString(abs(intCast((__i - nFirstVal) / double(nLastVal - nFirstVal) * 20.0)) * 5)
                                          + " %");
                bPrintedStatus = true;
            }
            else if (abs(nLastVal - nFirstVal) >= 99999
                     && abs(intCast((__i - nFirstVal) / double(nLastVal - nFirstVal) * 100.0))
                     > abs(intCast((__i - 1.0 - nFirstVal) / double(nLastVal - nFirstVal) * 100.0)))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... "
                                          + toString(abs(intCast((__i - nFirstVal) / double(nLastVal - nFirstVal) * 100.0)))
                                          + " %");
                bPrintedStatus = true;
            }
        }
    }

    return nJumpTable[nth_Cmd][BLOCK_END];
}


/////////////////////////////////////////////////
/// \brief Simple enum to make the variable types
/// of the range based for loop more readable.
/////////////////////////////////////////////////
enum RangeForVarType
{
    DECLARE_NEW_VAR = -1,
    DETECT_VAR_TYPE,
    NUMVAR,
    STRINGVAR,
    CLUSTERVAR
};


/////////////////////////////////////////////////
/// \brief This member function realizes the FOR
/// control flow statement for range based
/// indices. The return value is either an error
/// value or the end of the current flow control
/// statement.
///
/// \param nth_Cmd int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::range_based_for_loop(int nth_Cmd, int nth_loop)
{
    int nVarAdress = 0;
    int nLoopCount = 0;
    bPrintedStatus = false;
    RangeForVarType varType = DETECT_VAR_TYPE;

    if (!vCmdArray[nth_Cmd].sFlowCtrlHeader.length())
    {
        vCmdArray[nth_Cmd].sFlowCtrlHeader = vCmdArray[nth_Cmd].sCommand.substr(vCmdArray[nth_Cmd].sCommand.find("->") + 2);
        StringView sVar(vCmdArray[nth_Cmd].sCommand, vCmdArray[nth_Cmd].sCommand.find(' ') + 1);
        sVar.remove_from(sVar.find("->")+2);
        sVar.strip();

        if (sVar.starts_with("|>"))
        {
            vCmdArray[nth_Cmd].sFlowCtrlHeader.insert(0, "|>");
            sVar.trim_front(2);
        }

        sVar.strip();

        // Get the variable address of the loop
        // index
        for (size_t i = 0; i < sVarArray.size(); i++)
        {
            if (sVarArray[i] == sVar)
            {
                vCmdArray[nth_Cmd].nVarIndex = i;
                sVarArray[i].erase(sVarArray[i].find("->"));
                StripSpaces(sVarArray[i]);

                // The variable has not been declared yet
                varType = DECLARE_NEW_VAR;
                break;
            }
        }
    }

    std::string sHead = vCmdArray[nth_Cmd].sFlowCtrlHeader;
    nVarAdress = vCmdArray[nth_Cmd].nVarIndex;

    if (nVarAdress < 0)
        return FLOWCTRL_ERROR;

    NumeRe::StringParser& _stringParser = NumeReKernel::getInstance()->getStringParser();

    // Evaluate the header of the for loop and
    // assign the result to the iterator
    NumeRe::Cluster range = evalRangeBasedHeader(sHead, nth_Cmd, "for");

    // Shall we declare a new iterator variable? Otherwise try
    // to detect the type of the already created iterator variable
    if (varType == DECLARE_NEW_VAR)
    {
        if (sVarArray[nVarAdress].front() == '{')
        {
            EndlessVector<std::string> vIters = getAllArguments(sVarArray[nVarAdress].substr(1, sVarArray[nVarAdress].length()-2));

            // Create a temporary cluster for all iterators and remove
            // the trailing closing brace to change that to individual
            // indices for every iterator
            std::string sTempCluster = _dataRef->createTemporaryCluster(vIters.front());
            sTempCluster.pop_back();

            // Create single calls for every single iterator
            for (size_t i = 0; i < vIters.size(); i++)
            {
                mVarMap[vIters[i]] = sTempCluster + toString(i+1) + "}";
                replaceLocalVars(vIters[i], mVarMap[vIters[i]], nth_Cmd, nJumpTable[nth_Cmd][BLOCK_END]);

                // Add or replace the iterators in the list of iterators
                if (i)
                    sVarArray.push_back(mVarMap[vIters[i]]);
                else
                    sVarArray[nVarAdress] = mVarMap[vIters[0]];
            }

            // Note the needed iterator stepping
            vCmdArray[nth_Cmd].nRFStepping = vIters.size()-1;
            varType = CLUSTERVAR;
        }
        else if (range.isDouble())
        {
            // Create a local variable
            mVarMap[sVarArray[nVarAdress]] = "_~LOOP_" + sVarArray[nVarAdress] + "_" + toString(nthRecursion);
            replaceLocalVars(sVarArray[nVarAdress], mVarMap[sVarArray[nVarAdress]], nth_Cmd, nJumpTable[nth_Cmd][BLOCK_END]);
            sVarArray[nVarAdress] = mVarMap[sVarArray[nVarAdress]];
            _parserRef->DefineVar(sVarArray[nVarAdress], &vVarArray[nVarAdress]);
            varType = NUMVAR;
        }
        else if (range.isString())
        {
            // Create a local string variable
            mVarMap[sVarArray[nVarAdress]] = "_~LOOP_" + sVarArray[nVarAdress] + "_" + toString(nthRecursion);
            replaceLocalVars(sVarArray[nVarAdress], mVarMap[sVarArray[nVarAdress]], nth_Cmd, nJumpTable[nth_Cmd][BLOCK_END]);
            sVarArray[nVarAdress] = mVarMap[sVarArray[nVarAdress]];
            _stringParser.setStringValue(sVarArray[nVarAdress], "");
            varType = STRINGVAR;
        }
        else
        {
            // Create a cluster in all other cases as this is the
            // only variant type available
            mVarMap[sVarArray[nVarAdress]] = _dataRef->createTemporaryCluster(sVarArray[nVarAdress]);
            replaceLocalVars(sVarArray[nVarAdress], mVarMap[sVarArray[nVarAdress]], nth_Cmd, nJumpTable[nth_Cmd][BLOCK_END]);
            sVarArray[nVarAdress] = mVarMap[sVarArray[nVarAdress]];
            varType = CLUSTERVAR;
        }
    }
    else
    {
        // Find the type of the current iterator variable
        if (sVarArray[nVarAdress].find('{') != std::string::npos)
            varType = CLUSTERVAR;
        else if (_stringParser.isStringVar(sVarArray[nVarAdress]))
            varType = STRINGVAR;
        else
            varType = NUMVAR;
    }

    // Print to the terminal, if needed
    if (bSilent && !nth_loop && !bMask)
    {
        NumeReKernel::printPreFmt("|FOR> " + _lang.get("COMMON_EVALUATING") + " ... 0 %");
        bPrintedStatus = true;
    }

    // Evaluate the whole for loop. The outer loop does the
    // loop index management (the actual "for" command), the
    // inner loop runs through the contained command lines
    for (size_t i = 0; i < range.size(); i++)
    {
        // Set the value in the correct variable type
        switch (varType)
        {
            case NUMVAR:
                vVarArray[nVarAdress] = range.getDouble(i);
                break;
            case STRINGVAR:
                _stringParser.setStringValue(sVarArray[nVarAdress], range.getInternalString(i));
                break;
            default:
            {
                NumeRe::Cluster& iterCluster = _dataRef->getCluster(sVarArray[nVarAdress]);

                for (size_t n = 0; n <= vCmdArray[nth_Cmd].nRFStepping; n++)
                {
                    if (range.getType(i+n) == NumeRe::ClusterItem::ITEMTYPE_DOUBLE)
                        iterCluster.setDouble(n, range.getDouble(i+n));
                    else
                        iterCluster.setString(n, range.getInternalString(i+n));
                }

                i += vCmdArray[nth_Cmd].nRFStepping;
            }
        }

        // Ensure that the loop is aborted, if the
        // maximal number of repetitions has been
        // performed
        if (nLoopSafety > 0)
        {
            if (nLoopCount >= nLoopSafety)
                return FLOWCTRL_ERROR;

            nLoopCount++;
        }

        // This for loop handles the contained commands
        for (int __j = nth_Cmd+1; __j < nJumpTable[nth_Cmd][BLOCK_END]; __j++)
        {
            nCurrentCommand = __j;

            // If this is not the first line of the command block
            // try to find control flow statements in the first column
            if (vCmdArray[__j].fcFn)
            {
                // Evaluate the flow control commands
                int nReturn = (this->*vCmdArray[__j].fcFn)(__j, nth_loop+1);

                // Handle the return value
                if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN || nReturn == FLOWCTRL_LEAVE)
                    return nReturn;
                else if (nReturn == FLOWCTRL_BREAK)
                    return nJumpTable[nth_Cmd][BLOCK_END];
                else if (nReturn == FLOWCTRL_CONTINUE)
                    break;
                else if (nReturn != FLOWCTRL_NO_CMD)
                    __j = nReturn;

                continue;
            }

            // Handle the "continue" and "break" flow
            // control statements
            if (nCalcType[__j] & CALCTYPE_CONTINUECMD)
                break;
            else if (nCalcType[__j] & CALCTYPE_BREAKCMD)
                return nJumpTable[nth_Cmd][BLOCK_END];
            else if (!nCalcType[__j])
            {
                std::string sCommand = findCommand(vCmdArray[__j].sCommand).sString;

                // Evaluate the commands, store the bytecode
                if (sCommand == "continue")
                {
                    nCalcType[__j] = CALCTYPE_CONTINUECMD;

                    // "continue" is a break of the inner loop
                    break;
                }

                if (sCommand == "break")
                {
                    nCalcType[__j] = CALCTYPE_BREAKCMD;

                    // "break" requires to leave the current
                    // for loop. Therefore it is realized as
                    // return statement, returning the end index
                    // of the current block
                    return nJumpTable[nth_Cmd][BLOCK_END];
                }

                if (sCommand == "leave")
                {
                    // "leave" requires to leave the complete
                    // flow control block
                    return FLOWCTRL_LEAVE;
                }
            }

            // Increment the parser index, if the loop parsing
            // mode was activated
            if (bUseLoopParsingMode)
                _parserRef->SetIndex(__j);

            try
            {
                // Evaluate the command line with the calc function
                if (calc(vCmdArray[__j].sCommand, __j) == FLOWCTRL_ERROR)
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand,
                                                                                           getCurrentLineNumber(),
                                                                                           mVarMap, vVarArray, sVarArray);
                    getErrorInformationForDebugger();
                    return FLOWCTRL_ERROR;
                }

                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand,
                                                                                       getCurrentLineNumber(),
                                                                                       mVarMap, vVarArray, sVarArray);
                getErrorInformationForDebugger();
                NumeReKernel::getInstance()->getDebugger().showError(current_exception());
                nCalcType[__j] = CALCTYPE_NONE;
                catchExceptionForTest(current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());
            }
        }

        // Print the status to the terminal, if it is required
        if (!nth_loop && !bMask && bSilent)
        {
            if (range.size() < 99999
                    && intCast(i / (double)range.size() * 20.0) > intCast((i-1) / (double)range.size() * 20.0))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... "
                                          + toString(intCast(i / (double)range.size() * 20.0) * 5)
                                          + " %");
                bPrintedStatus = true;
            }
            else if (range.size() >= 99999
                     && intCast(i / (double)range.size() * 100.0) > intCast((i-1) / (double)range.size() * 100.0))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... "
                                          + toString(intCast(i / (double)range.size() * 100.0))
                                          + " %");
                bPrintedStatus = true;
            }
        }
    }

    return nJumpTable[nth_Cmd][BLOCK_END];
}


/////////////////////////////////////////////////
/// \brief Simple helper to check multiple
/// conditional values for their logical
/// trueness.
///
/// \param v mu::value_type*
/// \param nNum int
/// \return bool
///
/////////////////////////////////////////////////
static bool isTrue(mu::value_type* v, int nNum)
{
    for (int i = 0; i < nNum; i++)
    {
        if (v[i] == 0.0 || mu::isnan(v[i]))
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function realizes the
/// WHILE control flow statement. The return
/// value is either an error value or the end of
/// the current flow control statement.
///
/// \param nth_Cmd int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::while_loop(int nth_Cmd, int nth_loop)
{
    if (!vCmdArray[nth_Cmd].sFlowCtrlHeader.length())
        vCmdArray[nth_Cmd].sFlowCtrlHeader = extractHeaderExpression(vCmdArray[nth_Cmd].sCommand);

    std::string sWhile_Condition = vCmdArray[nth_Cmd].sFlowCtrlHeader;
    std::string sWhile_Condition_Back = sWhile_Condition;
    bPrintedStatus = false;
    int nLoopCount = 0;
    value_type* v = 0;
    int nNum = 0;

    // Show a working indicator, if it's necessary
    if (bSilent && !bMask && !nth_loop)
    {
        NumeReKernel::printPreFmt("|WHL> " + _lang.get("COMMON_EVALUATING") + " ... ");
        bPrintedStatus = true;
    }

    // The outer while loop realizes the "while" control
    // flow statement and handles the checking of the
    // while condition. The inner for loop runs through
    // the contained commands
    while (true)
    {
        // Evaluate the header (has to be done in every
        // run)
        v = evalHeader(nNum, sWhile_Condition, false, nth_Cmd, "while");

        // Ensure that the loop is aborted, if the
        // maximal number of repetitions has been
        // performed
        if (nLoopSafety > 0)
        {
            if (nLoopCount >= nLoopSafety)
                return FLOWCTRL_ERROR;

            nLoopCount++;
        }

        // Check, whether the header condition is true.
        // NaN and INF are no "true" values. Return the
        // end of the current block otherwise
        if (!isTrue(v, nNum))
            return nJumpTable[nth_Cmd][BLOCK_END];

        // This for loop handles the contained commands
        for (int __j = nth_Cmd+1; __j < nJumpTable[nth_Cmd][BLOCK_END]; __j++)
        {
            nCurrentCommand = __j;

            // If this is not the first line of the command block
            // try to find control flow statements in the first column
            if (vCmdArray[__j].fcFn)
            {
                // Evaluate the flow control commands
                int nReturn =  (this->*vCmdArray[__j].fcFn)(__j, nth_loop+1);

                // Handle the return value
                if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN || nReturn == FLOWCTRL_LEAVE)
                    return nReturn;
                else if (nReturn == FLOWCTRL_BREAK)
                    return nJumpTable[nth_Cmd][BLOCK_END];
                else if (nReturn == FLOWCTRL_CONTINUE)
                    break;
                else if (nReturn != FLOWCTRL_NO_CMD)
                    __j = nReturn;

                continue;
            }

            // Handle the "continue" and "break" flow
            // control statements
            if (nCalcType[__j] & CALCTYPE_CONTINUECMD)
                break;
            else if (nCalcType[__j] & CALCTYPE_BREAKCMD)
                return nJumpTable[nth_Cmd][BLOCK_END];
            else if (!nCalcType[__j])
            {
                std::string sCommand = findCommand(vCmdArray[__j].sCommand).sString;

                // Evaluate the commands, store the bytecode
                if (sCommand == "continue")
                {
                    nCalcType[__j] = CALCTYPE_CONTINUECMD;

                    // "continue" is a break of the inner loop
                    break;
                }

                if (sCommand == "break")
                {
                    nCalcType[__j] = CALCTYPE_BREAKCMD;

                    // "break" requires to leave the current
                    // for loop. Therefore it is realized as
                    // return statement, returning the end index
                    // of the current block
                    return nJumpTable[nth_Cmd][BLOCK_END];
                }

                if (sCommand == "leave")
                {
                    // "leave" requires to leave the complete
                    // flow control block
                    return FLOWCTRL_LEAVE;
                }
            }

            // Increment the parser index, if the loop parsing
            // mode was activated
            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parserRef->SetIndex(__j);

            try
            {
                // Evaluate the command line with the calc function
                if (calc(vCmdArray[__j].sCommand, __j) == FLOWCTRL_ERROR)
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand,
                                                                                           getCurrentLineNumber(),
                                                                                           mVarMap, vVarArray, sVarArray);
                    getErrorInformationForDebugger();
                    return FLOWCTRL_ERROR;
                }

                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand,
                                                                                       getCurrentLineNumber(),
                                                                                       mVarMap, vVarArray, sVarArray);
                getErrorInformationForDebugger();
                NumeReKernel::getInstance()->getDebugger().showError(current_exception());
                nCalcType[__j] = CALCTYPE_NONE;
                catchExceptionForTest(current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());
            }
        }

        // If the while condition has changed during the evaluation
        // re-use the original condition to ensure that the result of
        // the condition is true
        if (sWhile_Condition != sWhile_Condition_Back || _dataRef->containsTablesOrClusters(sWhile_Condition_Back))
            sWhile_Condition = sWhile_Condition_Back;
    }

    return vCmdArray.size();
}


/////////////////////////////////////////////////
/// \brief This member function realizes the
/// IF-ELSE control flow statement. The return
/// value is either an error value or the end of
/// the current flow control statement.
///
/// \param nth_Cmd int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::if_fork(int nth_Cmd, int nth_loop)
{
    int nElse = nJumpTable[nth_Cmd][BLOCK_MIDDLE]; // Position of next else/elseif
    int nEndif = nJumpTable[nth_Cmd][BLOCK_END];
    bPrintedStatus = false;
    value_type* v;
    int nNum = 0;
    std::string sHeadCommand = "if";

    // The first command is always an "if", therefore always
    // enter this section. The condition at the end will check
    // whether a further iteration is needed to find the next
    // "elseif" or directly leave the loop to continue with the
    // "else" case.
    do
    {
        if (!vCmdArray[nth_Cmd].sFlowCtrlHeader.length())
            vCmdArray[nth_Cmd].sFlowCtrlHeader = extractHeaderExpression(vCmdArray[nth_Cmd].sCommand);

        nElse = nJumpTable[nth_Cmd][BLOCK_MIDDLE];
        nEndif = nJumpTable[nth_Cmd][BLOCK_END];
        bPrintedStatus = false;
        std::string sHead = vCmdArray[nth_Cmd].sFlowCtrlHeader;

        // Evaluate the header of the current elseif case
        v = evalHeader(nNum, sHead, false, nth_Cmd, sHeadCommand);

        // If the condition is true, enter the if-case
        if (isTrue(v, nNum))
        {
            // The inner loop goes through the contained
            // commands
            for (int __i = nth_Cmd+1; __i < (int)vCmdArray.size(); __i++)
            {
                nCurrentCommand = __i;

                // If we reach the line of the next else/
                // elseif or the corresponding end of the
                // current block, return with the last line
                // of this block
                if (__i == nElse || __i >= nEndif)
                    return nEndif;

                // If this is not the first line of the command block
                // try to find control flow statements in the first column
                if (vCmdArray[__i].bFlowCtrlStatement)
                {
                    // Evaluate the flow control commands
                    int nReturn = evalForkFlowCommands(__i, nth_loop);

                    // Handle the return value
                    if (nReturn == FLOWCTRL_ERROR
                        || nReturn == FLOWCTRL_RETURN
                        || nReturn == FLOWCTRL_BREAK
                        || nReturn == FLOWCTRL_LEAVE
                        || nReturn == FLOWCTRL_CONTINUE)
                        return nReturn;
                    else if (nReturn != FLOWCTRL_NO_CMD)
                        __i = nReturn;

                    continue;
                }

                // Handle the "continue" and "break" flow
                // control statements
                if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
                    return FLOWCTRL_CONTINUE;
                else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
                    return FLOWCTRL_BREAK;
                else if (!nCalcType[__i])
                {
                    std::string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

                    // "continue" and "break" are both exiting the current
                    // condition, but they are yielding different signals
                    if (sCommand == "continue")
                    {
                        nCalcType[__i] = CALCTYPE_CONTINUECMD;
                        return FLOWCTRL_CONTINUE;
                    }

                    if (sCommand == "break")
                    {
                        nCalcType[__i] = CALCTYPE_BREAKCMD;
                        return FLOWCTRL_BREAK;
                    }

                    if (sCommand == "leave")
                    {
                        // "leave" requires to leave the complete
                        // flow control block
                        return FLOWCTRL_LEAVE;
                    }
                }

                // Increment the parser index, if the loop parsing
                // mode was activated
                if (bUseLoopParsingMode && !bLockedPauseMode)
                    _parserRef->SetIndex(__i);

                try
                {
                    // Evaluate the command line with the calc function
                    if (calc(vCmdArray[__i].sCommand, __i) == FLOWCTRL_ERROR)
                    {
                        NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                               getCurrentLineNumber(),
                                                                                               mVarMap, vVarArray, sVarArray);
                        getErrorInformationForDebugger();
                    }

                    if (bReturnSignal)
                        return FLOWCTRL_RETURN;
                }
                catch (...)
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                           getCurrentLineNumber(),
                                                                                           mVarMap, vVarArray, sVarArray);
                    getErrorInformationForDebugger();
                    NumeReKernel::getInstance()->getDebugger().showError(current_exception());
                    nCalcType[__i] = CALCTYPE_NONE;
                    catchExceptionForTest(current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());
                }
            }

            return nEndif;
        }
        else
        {
            // Try to find the next elseif/else
            // statement of the current block
            if (nElse == NO_FLOW_COMMAND)
                return nEndif;
            else
                nth_Cmd = nElse;

            sHeadCommand = "elseif";
        }
    } while (vCmdArray[nth_Cmd].sCommand.find(">>elseif") != std::string::npos);

    // This is the else-case, if it is available. Will enter this
    // section, if the loop condition fails.
    for (int __i = nth_Cmd+1; __i < (int)vCmdArray.size(); __i++)
    {
        nCurrentCommand = __i;

        // If we're reaching the line with the
        // corresponding endif statement, we'll
        // return with this line as index
        if (__i >= nEndif)
            return nEndif;

        // If this is not the first line of the command block
        // try to find control flow statements in the first column
        if (vCmdArray[__i].bFlowCtrlStatement)
        {
            // Evaluate the flow control commands
            int nReturn = evalForkFlowCommands(__i, nth_loop);

            // Handle the return value
            if (nReturn == FLOWCTRL_ERROR
                || nReturn == FLOWCTRL_RETURN
                || nReturn == FLOWCTRL_BREAK
                || nReturn == FLOWCTRL_LEAVE
                || nReturn == FLOWCTRL_CONTINUE)
                return nReturn;
            else if (nReturn != FLOWCTRL_NO_CMD)
                __i = nReturn;

            continue;
        }

        if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
            return FLOWCTRL_CONTINUE;
        else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
            return FLOWCTRL_BREAK;
        else if (!nCalcType[__i])
        {
            std::string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

            // "continue" and "break" are both exiting the current
            // condition, but they are yielding different signals
            if (sCommand == "continue")
            {
                nCalcType[__i] = CALCTYPE_CONTINUECMD;
                return FLOWCTRL_CONTINUE;
            }

            if (sCommand == "break")
            {
                nCalcType[__i] = CALCTYPE_BREAKCMD;
                return FLOWCTRL_BREAK;
            }

            if (sCommand == "leave")
            {
                // "leave" requires to leave the complete
                // flow control block
                return FLOWCTRL_LEAVE;
            }
        }

        if (bUseLoopParsingMode && !bLockedPauseMode)
            _parserRef->SetIndex(__i);

        try
        {
            // Evaluate the command line with the calc function
            if (calc(vCmdArray[__i].sCommand, __i) == FLOWCTRL_ERROR)
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                       getCurrentLineNumber(),
                                                                                       mVarMap, vVarArray, sVarArray);
                getErrorInformationForDebugger();
                return FLOWCTRL_ERROR;
            }

            if (bReturnSignal)
                return FLOWCTRL_RETURN;
        }
        catch (...)
        {
            NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                   getCurrentLineNumber(),
                                                                                   mVarMap, vVarArray, sVarArray);
            getErrorInformationForDebugger();
            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            nCalcType[__i] = CALCTYPE_NONE;
            catchExceptionForTest(current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());
        }
    }

    return vCmdArray.size();
}


/////////////////////////////////////////////////
/// \brief This member function realizes the
/// SWITCH-CASE control flow statement. The
/// return value is either an error value or the
/// end of the current flow control statement.
///
/// \param nth_Cmd int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::switch_fork(int nth_Cmd, int nth_loop)
{
    if (!vCmdArray[nth_Cmd].sFlowCtrlHeader.length())
        vCmdArray[nth_Cmd].sFlowCtrlHeader = extractHeaderExpression(vCmdArray[nth_Cmd].sCommand);

    std::string sSwitch_Condition = vCmdArray[nth_Cmd].sFlowCtrlHeader;
    int nNextCase = nJumpTable[nth_Cmd][BLOCK_MIDDLE]; // Position of next case/default
    int nSwitchEnd = nJumpTable[nth_Cmd][BLOCK_END];
    bPrintedStatus = false;
    value_type* v;
    int nNum = 0;

    // Evaluate the header of the current switch statement and its cases
    v = evalHeader(nNum, sSwitch_Condition, false, nth_Cmd, "switch");

    // Search for the correct first(!) case
    for (int i = 0; i < nNum; i++)
    {
        if (v[i] == 0.0)
            nNextCase = nJumpTable[nNextCase][BLOCK_MIDDLE];
        else
            break;

        if (nNextCase < 0)
            return nSwitchEnd;
    }

    // Set the start index of the current switch case
    if (nNextCase == -1)
        return nSwitchEnd;
    else
        nth_Cmd = nNextCase;

    // The inner loop goes through the contained
    // commands
    for (int __i = nth_Cmd+1; __i < (int)vCmdArray.size(); __i++)
    {
        nCurrentCommand = __i;
        // If we reach the end of the
        // current block, return with the last line
        // of this block
        if (__i >= nSwitchEnd)
            return nSwitchEnd;

        // If this is not the first line of the command block
        // try to find control flow statements in the first column
        if (vCmdArray[__i].bFlowCtrlStatement)
        {
            // Evaluate the flow control commands
            int nReturn = evalForkFlowCommands(__i, nth_loop);

            // Handle the return value
            if (nReturn == FLOWCTRL_ERROR
                || nReturn == FLOWCTRL_RETURN
                || nReturn == FLOWCTRL_LEAVE
                || nReturn == FLOWCTRL_CONTINUE)
                return nReturn;
            else if (nReturn == FLOWCTRL_BREAK)
            {
                // We don't propagate the break signal
                return nSwitchEnd;
            }
            else if (nReturn != FLOWCTRL_NO_CMD)
                __i = nReturn;

            continue;
        }

        // Handle the "continue" and "break" flow
        // control statements
        if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
            return FLOWCTRL_CONTINUE;
        else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
        {
            // We don't propagate the break signal in this case
            return nSwitchEnd;
        }
        else if (!nCalcType[__i])
        {
            std::string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

            // "continue" and "break" are both exiting the current
            // condition, but they are yielding different signals
            if (sCommand == "continue")
            {
                nCalcType[__i] = CALCTYPE_CONTINUECMD;
                return FLOWCTRL_CONTINUE;
            }

            if (sCommand == "break")
            {
                // We don't propagate the break signal in this case
                nCalcType[__i] = CALCTYPE_BREAKCMD;
                return nSwitchEnd;
            }

            if (sCommand == "leave")
            {
                // "leave" requires to leave the complete
                // flow control block
                return FLOWCTRL_LEAVE;
            }
        }

        // Increment the parser index, if the loop parsing
        // mode was activated
        if (bUseLoopParsingMode && !bLockedPauseMode)
            _parserRef->SetIndex(__i);

        try
        {
            // Evaluate the command line with the calc function
            if (calc(vCmdArray[__i].sCommand, __i) == FLOWCTRL_ERROR)
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                       getCurrentLineNumber(),
                                                                                       mVarMap, vVarArray, sVarArray);
                getErrorInformationForDebugger();
                return FLOWCTRL_ERROR;
            }

            if (bReturnSignal)
                return FLOWCTRL_RETURN;
        }
        catch (...)
        {
            NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                   getCurrentLineNumber(),
                                                                                   mVarMap, vVarArray, sVarArray);
            getErrorInformationForDebugger();
            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            nCalcType[__i] = CALCTYPE_NONE;
            catchExceptionForTest(current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());
        }
    }

    return vCmdArray.size();
}


/////////////////////////////////////////////////
/// \brief Implements a try-catch block.
///
/// \param nth_Cmd int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::try_catch(int nth_Cmd, int nth_loop)
{
    int nNextCatch = nJumpTable[nth_Cmd][BLOCK_MIDDLE]; // Position of next case/default
    int nTryEnd = nJumpTable[nth_Cmd][BLOCK_END];
    bPrintedStatus = false;

    // The inner loop goes through the contained
    // commands
    for (int __i = nth_Cmd+1; __i < (int)vCmdArray.size(); __i++)
    {
        nCurrentCommand = __i;
        // If we reach the end of the
        // current block, return with the last line
        // of this block
        if (__i >= nTryEnd || (nNextCatch >= 0 && __i >= nNextCatch))
            return nTryEnd;

        try
        {
            // If this is not the first line of the command block
            // try to find control flow statements in the first column
            if (vCmdArray[__i].bFlowCtrlStatement)
            {
                // Evaluate the flow control commands
                int nReturn = evalForkFlowCommands(__i, nth_loop);

                // Handle the return value
                if (nReturn == FLOWCTRL_ERROR
                    || nReturn == FLOWCTRL_RETURN
                    || nReturn == FLOWCTRL_BREAK
                    || nReturn == FLOWCTRL_LEAVE
                    || nReturn == FLOWCTRL_CONTINUE)
                    return nReturn;
                else if (nReturn != FLOWCTRL_NO_CMD)
                    __i = nReturn;

                continue;
            }

            // Handle the "continue" and "break" flow
            // control statements
            if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
                return FLOWCTRL_CONTINUE;
            else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
                return FLOWCTRL_BREAK;
            else if (!nCalcType[__i])
            {
                std::string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

                // "continue" and "break" are both exiting the current
                // condition, but they are yielding different signals
                if (sCommand == "continue")
                {
                    nCalcType[__i] = CALCTYPE_CONTINUECMD;
                    return FLOWCTRL_CONTINUE;
                }

                if (sCommand == "break")
                {
                    nCalcType[__i] = CALCTYPE_BREAKCMD;
                    return FLOWCTRL_BREAK;
                }

                if (sCommand == "leave")
                {
                    // "leave" requires to leave the complete
                    // flow control block
                    return FLOWCTRL_LEAVE;
                }
            }

            // Increment the parser index, if the loop parsing
            // mode was activated
            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parserRef->SetIndex(__i);

            // Evaluate the command line with the calc function
            if (calc(vCmdArray[__i].sCommand, __i) == FLOWCTRL_ERROR)
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                       getCurrentLineNumber(),
                                                                                       mVarMap, vVarArray, sVarArray);
                getErrorInformationForDebugger();
                return FLOWCTRL_ERROR;
            }

            if (bReturnSignal)
                return FLOWCTRL_RETURN;
        }
        catch (...)
        {
            NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                   getCurrentLineNumber(),
                                                                                   mVarMap, vVarArray, sVarArray);
            getErrorInformationForDebugger();
            NumeReKernel::getInstance()->getDebugger().showError(std::current_exception());
            nCalcType[__i] = CALCTYPE_NONE;

            ErrorType type = getErrorType(std::current_exception());

            // We won't catch every possible error
            if (type == TYPE_ABORT || type == TYPE_INTERNALERROR || type == TYPE_CRITICALERROR)
                catchExceptionForTest(std::current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());

            // Search for the correct first(!) case
            while (nNextCatch >= 0)
            {
                std::string sCatchExpression = vCmdArray[nNextCatch].sCommand;
                sCatchExpression.erase(0, sCatchExpression.find(">>catch")+7);

                if (sCatchExpression.find_first_not_of(" :") == std::string::npos
                    || sCatchExpression.find(" " + errorTypeToString(type)) != std::string::npos)
                    break;
                else
                    nNextCatch = nJumpTable[nNextCatch][BLOCK_MIDDLE];
            }

            // Set the start index of the current switch case
            if (nNextCatch < 0)
                catchExceptionForTest(std::current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());

            nth_Cmd = nNextCatch;
            NumeReKernel::getInstance()->getDebugger().finalizeCatched();
            nNextCatch = nJumpTable[nNextCatch][BLOCK_MIDDLE];

            // Execute handler code
            // The inner loop goes through the contained
            // commands
            for (int __i = nth_Cmd+1; __i < (int)vCmdArray.size(); __i++)
            {
                nCurrentCommand = __i;
                // If we reach the end of the
                // current block, return with the last line
                // of this block
                if (__i >= nTryEnd || (nNextCatch >= 0 && __i >= nNextCatch))
                    return nTryEnd;

                if (__i != nth_Cmd)
                {
                    // If this is not the first line of the command block
                    // try to find control flow statements in the first column
                    if (vCmdArray[__i].bFlowCtrlStatement)
                    {
                        // Evaluate the flow control commands
                        int nReturn = evalForkFlowCommands(__i, nth_loop);

                        // Handle the return value
                        if (nReturn == FLOWCTRL_ERROR
                            || nReturn == FLOWCTRL_RETURN
                            || nReturn == FLOWCTRL_BREAK
                            || nReturn == FLOWCTRL_LEAVE
                            || nReturn == FLOWCTRL_CONTINUE)
                            return nReturn;
                        else if (nReturn != FLOWCTRL_NO_CMD)
                            __i = nReturn;

                        continue;
                    }
                }

                // Handle the "continue" and "break" flow
                // control statements
                if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
                    return FLOWCTRL_CONTINUE;
                else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
                    return FLOWCTRL_BREAK;
                else if (!nCalcType[__i])
                {
                    std::string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

                    // "continue" and "break" are both exiting the current
                    // condition, but they are yielding different signals
                    if (sCommand == "continue")
                    {
                        nCalcType[__i] = CALCTYPE_CONTINUECMD;
                        return FLOWCTRL_CONTINUE;
                    }

                    if (sCommand == "break")
                    {
                        nCalcType[__i] = CALCTYPE_BREAKCMD;
                        return FLOWCTRL_BREAK;
                    }

                    if (sCommand == "leave")
                    {
                        // "leave" requires to leave the complete
                        // flow control block
                        return FLOWCTRL_LEAVE;
                    }
                }

                // Increment the parser index, if the loop parsing
                // mode was activated
                if (bUseLoopParsingMode && !bLockedPauseMode)
                    _parserRef->SetIndex(__i);

                try
                {
                    if (findCommand(vCmdArray[__i].sCommand).sString == "rethrow")
                        rethrow_exception(current_exception());

                    // Evaluate the command line with the calc function
                    if (calc(vCmdArray[__i].sCommand, __i) == FLOWCTRL_ERROR)
                    {
                        NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                               getCurrentLineNumber(),
                                                                                               mVarMap, vVarArray, sVarArray);
                        getErrorInformationForDebugger();
                        return FLOWCTRL_ERROR;
                    }

                    if (bReturnSignal)
                        return FLOWCTRL_RETURN;
                }
                catch (...)
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand,
                                                                                           getCurrentLineNumber(),
                                                                                           mVarMap, vVarArray, sVarArray);
                    getErrorInformationForDebugger();
                    NumeReKernel::getInstance()->getDebugger().showError(current_exception());
                    nCalcType[__i] = CALCTYPE_NONE;
                    catchExceptionForTest(current_exception(), NumeReKernel::bSupressAnswer, getCurrentLineNumber());
                }
            }
        }
    }

    return vCmdArray.size();
}


/////////////////////////////////////////////////
/// \brief This member function abstracts the
/// evaluation of all flow control headers. It
/// will return an array of evaluated return
/// values, although only the first (one/two) are
/// used here.
///
/// \param nNum int&
/// \param sHeadExpression std::string&
/// \param bIsForHead bool
/// \param nth_Cmd int
/// \param sHeadCommand const std::string&
/// \return value_type*
///
/////////////////////////////////////////////////
value_type* FlowCtrl::evalHeader(int& nNum, std::string& sHeadExpression, bool bIsForHead, int nth_Cmd, const std::string& sHeadCommand)
{
    int nCurrentCalcType = nCalcType[nth_Cmd];
    std::string sCache;
    nCurrentCommand = nth_Cmd;

    // Eval the debugger breakpoint first
    if (nCurrentCalcType & CALCTYPE_DEBUGBREAKPOINT
        || sHeadExpression.substr(sHeadExpression.find_first_not_of(' '), 2) == "|>"
        || nDebuggerCode == NumeReKernel::DEBUGGER_STEP)
    {
        Breakpoint bp(true);
        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

        if (sHeadExpression.substr(sHeadExpression.find_first_not_of(' '), 2) == "|>")
        {
            nCalcType[nth_Cmd] |= CALCTYPE_DEBUGBREAKPOINT;
            sHeadExpression.erase(sHeadExpression.find_first_not_of(' '), 2);
            StripSpaces(sHeadExpression);
        }

        if (_debugger.getBreakpointManager().isBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber()))
            bp = _debugger.getBreakpointManager().getBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber());

        if (bp.m_isConditional)
        {
            Procedure* proc = _debugger.getCurrentProcedure();

            if (proc)
                bp.m_condition = proc->resolveVariables(bp.m_condition);

            replaceLocalVars(bp.m_condition);
        }

        if (_optionRef->useDebugger()
            && nDebuggerCode != NumeReKernel::DEBUGGER_LEAVE
            && nDebuggerCode != NumeReKernel::DEBUGGER_STEPOVER
            && bp.isActive(!bLockedPauseMode && bUseLoopParsingMode))
        {
            _debugger.gatherLoopBasedInformations(sHeadCommand + " (" + sHeadExpression + ")", getCurrentLineNumber(), mVarMap, vVarArray, sVarArray);
            nDebuggerCode = evalDebuggerBreakPoint(*_parserRef, *_optionRef);
        }
    }

    // Check, whether the user tried to abort the
    // current evaluation
    if (NumeReKernel::GetAsyncCancelState())
    {
        if (bPrintedStatus)
            NumeReKernel::printPreFmt(" " + _lang.get("COMMON_CANCEL"));

        throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
    }

    // Update the parser index, if the loop parsing
    // mode was activated
    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parserRef->SetIndex(nth_Cmd);

    // Replace the function definitions, if not already done
    if (!bFunctionsReplaced)
    {
        if (!_functionRef->call(sHeadExpression))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sHeadExpression, SyntaxError::invalid_position);
    }

    // If the expression is numerical-only, evaluate it here
    if (nCurrentCalcType & CALCTYPE_NUMERICAL)
    {
        mu::value_type* v;

        // As long as bytecode parsing is not globally available,
        // this condition has to stay at this place
        if (!(bUseLoopParsingMode && !bLockedPauseMode)
            && !_parserRef->IsAlreadyParsed(sHeadExpression))
            _parserRef->SetExpr(sHeadExpression);

        // Evaluate all remaining equations in the stack
        do
        {
            v = _parserRef->Eval(nNum);
        } while (_parserRef->IsNotLastStackItem());

        return v;
    }

    // Include procedure and plugin calls
    if (nJumpTable[nth_Cmd][PROCEDURE_INTERFACE])
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }

        // Call the procedure interface function
        ProcedureInterfaceRetVal nReturn = procedureInterface(sHeadExpression, *_parserRef,
                                                              *_functionRef, *_dataRef, *_outRef,
                                                              *_pDataRef, *_scriptRef, *_optionRef, nth_Cmd);

        // Handle the return value
        if (nReturn == INTERFACE_ERROR)
            throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sHeadExpression, SyntaxError::invalid_position);
        else if (nReturn == INTERFACE_EMPTY)
            sHeadExpression = "false";

        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode(false);
            _parserRef->LockPause(false);
        }

        if (nReturn == INTERFACE_EMPTY || nReturn == INTERFACE_VALUE)
            nJumpTable[nth_Cmd][PROCEDURE_INTERFACE] = 1;
        else
            nJumpTable[nth_Cmd][PROCEDURE_INTERFACE] = 0;
    }

    // Catch and evaluate all data and cache calls
    if (nCurrentCalcType & CALCTYPE_DATAACCESS || !nCurrentCalcType)
    {
        if ((nCurrentCalcType && !(nCurrentCalcType & CALCTYPE_STRING))
            || (_dataRef->containsTablesOrClusters(sHeadExpression)
                && !NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression)))
        {
            if (!nCurrentCalcType)
                nCalcType[nth_Cmd] |= CALCTYPE_DATAACCESS;

            if (!_parserRef->HasCachedAccess()
                && _parserRef->CanCacheAccess()
                && !_parserRef->GetCachedEquation().length())
                _parserRef->SetCompiling(true);

            sCache = getDataElements(sHeadExpression, *_parserRef, *_dataRef, *_optionRef);

            // Ad-hoc bytecode adaption
#warning NOTE (numere#1#08/21/21): Might need some adaption, if bytecode issues are experienced
            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
                nCurrentCalcType |= CALCTYPE_STRING;

            if (_parserRef->IsCompiling()
                && _parserRef->CanCacheAccess())
            {
                _parserRef->CacheCurrentEquation(sHeadExpression);
                _parserRef->CacheCurrentTarget(sCache);
            }

            _parserRef->SetCompiling(false);
        }
    }

    // Evaluate std::strings
    if (nCurrentCalcType & CALCTYPE_STRING || !nCurrentCalcType)
    {
        if (nCurrentCalcType || NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
        {
            if (!nCurrentCalcType)
                nCalcType[nth_Cmd] |= CALCTYPE_STRING;

            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode();

            // Call the std::string parser
            auto retVal = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sHeadExpression, sCache, true, false, true);

            // Evaluate the return value
            if (retVal != NumeRe::StringParser::STRING_NUMERICAL)
            {
                StripSpaces(sHeadExpression);

                if (bIsForHead)
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(sHeadExpression,
                                                                                           getCurrentLineNumber(),
                                                                                           mVarMap, vVarArray, sVarArray);

                    NumeReKernel::getInstance()->getDebugger().throwException(SyntaxError(SyntaxError::CANNOT_EVAL_FOR,
                                                                                          sHeadExpression,
                                                                                          SyntaxError::invalid_position));
                }
                else
                {
                    StripSpaces(sHeadExpression);

                    if (sHeadExpression != "\"\"")
                        sHeadExpression = "true";
                    else
                        sHeadExpression = "false";
                }
            }

            // It's possible that the user might have done
            // something weird with std::string operations transformed
            // into a regular expression. Replace the local
            // variables here
            replaceLocalVars(sHeadExpression);

            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode(false);
        }
    }

    // Evalute the already prepared equation
    if (!_parserRef->IsAlreadyParsed(sHeadExpression))
        _parserRef->SetExpr(sHeadExpression);

    if (!nCalcType[nth_Cmd] && !nJumpTable[nth_Cmd][PROCEDURE_INTERFACE])
        nCalcType[nth_Cmd] |= CALCTYPE_NUMERICAL;

    return _parserRef->Eval(nNum);
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// evaluation of the range-based flow control
/// headers. It will return a cluster of the
/// evaluated return values.
///
/// \param sHeadExpression std::string&
/// \param nth_Cmd int
/// \param sHeadCommand const std::string&
/// \return NumeRe::Cluster
///
/////////////////////////////////////////////////
NumeRe::Cluster FlowCtrl::evalRangeBasedHeader(std::string& sHeadExpression, int nth_Cmd, const std::string& sHeadCommand)
{
    NumeRe::Cluster result;
    mu::value_type* v;
    int nNum;
    int nCurrentCalcType = nCalcType[nth_Cmd];
    std::string sCache;
    nCurrentCommand = nth_Cmd;

    // Eval the debugger breakpoint first
    if (nCurrentCalcType & CALCTYPE_DEBUGBREAKPOINT
        || sHeadExpression.substr(sHeadExpression.find_first_not_of(' '), 2) == "|>"
        || nDebuggerCode == NumeReKernel::DEBUGGER_STEP)
    {
        Breakpoint bp(true);
        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

        if (sHeadExpression.substr(sHeadExpression.find_first_not_of(' '), 2) == "|>")
        {
            nCalcType[nth_Cmd] |= CALCTYPE_DEBUGBREAKPOINT;
            sHeadExpression.erase(sHeadExpression.find_first_not_of(' '), 2);
            StripSpaces(sHeadExpression);
        }

        if (_debugger.getBreakpointManager().isBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber()))
            bp = _debugger.getBreakpointManager().getBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber());

        if (bp.m_isConditional)
        {
            Procedure* proc = _debugger.getCurrentProcedure();

            if (proc)
                bp.m_condition = proc->resolveVariables(bp.m_condition);

            replaceLocalVars(bp.m_condition);
        }

        if (_optionRef->useDebugger()
            && nDebuggerCode != NumeReKernel::DEBUGGER_LEAVE
            && nDebuggerCode != NumeReKernel::DEBUGGER_STEPOVER
            && bp.isActive(!bLockedPauseMode && bUseLoopParsingMode))
        {
            _debugger.gatherLoopBasedInformations(sHeadCommand + " (" + sHeadExpression + ")", getCurrentLineNumber(),
                                                  mVarMap, vVarArray, sVarArray);
            nDebuggerCode = evalDebuggerBreakPoint(*_parserRef, *_optionRef);
        }
    }

    // Check, whether the user tried to abort the
    // current evaluation
    if (NumeReKernel::GetAsyncCancelState())
    {
        if (bPrintedStatus)
            NumeReKernel::printPreFmt(" " + _lang.get("COMMON_CANCEL"));

        throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
    }

    // Update the parser index, if the loop parsing
    // mode was activated
    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parserRef->SetIndex(nth_Cmd);

    // Replace the function definitions, if not already done
    if (!bFunctionsReplaced)
    {
        if (!_functionRef->call(sHeadExpression))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sHeadExpression, SyntaxError::invalid_position);
    }

    // If the expression is numerical-only, evaluate it here
    if (nCurrentCalcType & CALCTYPE_NUMERICAL)
    {
        // As long as bytecode parsing is not globally available,
        // this condition has to stay at this place
        if (!(bUseLoopParsingMode && !bLockedPauseMode)
            && !_parserRef->IsAlreadyParsed(sHeadExpression))
            _parserRef->SetExpr(sHeadExpression);

        // Evaluate all remaining equations in the stack
        do
        {
            v = _parserRef->Eval(nNum);
        } while (_parserRef->IsNotLastStackItem());

        result.setDoubleArray(nNum, v);
        return result;
    }

    // Include procedure and plugin calls
    if (nJumpTable[nth_Cmd][PROCEDURE_INTERFACE])
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }

        // Call the procedure interface function
        ProcedureInterfaceRetVal nReturn = procedureInterface(sHeadExpression, *_parserRef,
                                                              *_functionRef, *_dataRef, *_outRef,
                                                              *_pDataRef, *_scriptRef, *_optionRef, nth_Cmd);

        // Handle the return value
        if (nReturn == INTERFACE_ERROR)
            throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sHeadExpression, SyntaxError::invalid_position);
        else if (nReturn == INTERFACE_EMPTY)
            sHeadExpression = "false";

        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode(false);
            _parserRef->LockPause(false);
        }

        if (nReturn == INTERFACE_EMPTY || nReturn == INTERFACE_VALUE)
            nJumpTable[nth_Cmd][PROCEDURE_INTERFACE] = 1;
        else
            nJumpTable[nth_Cmd][PROCEDURE_INTERFACE] = 0;
    }

    // Catch and evaluate all data and cache calls
    if (nCurrentCalcType & CALCTYPE_DATAACCESS || !nCurrentCalcType)
    {
        if ((nCurrentCalcType && !(nCurrentCalcType & CALCTYPE_STRING))
            || (_dataRef->containsTablesOrClusters(sHeadExpression)
                && !NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression)))
        {
            if (!nCurrentCalcType)
                nCalcType[nth_Cmd] |= CALCTYPE_DATAACCESS;

            if (!_parserRef->HasCachedAccess()
                && _parserRef->CanCacheAccess()
                && !_parserRef->GetCachedEquation().length())
                _parserRef->SetCompiling(true);

            sCache = getDataElements(sHeadExpression, *_parserRef, *_dataRef, *_optionRef);

            // Ad-hoc bytecode adaption
#warning NOTE (numere#1#08/21/21): Might need some adaption, if bytecode issues are experienced
            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
                nCurrentCalcType |= CALCTYPE_STRING;

            if (_parserRef->IsCompiling()
                && _parserRef->CanCacheAccess())
            {
                _parserRef->CacheCurrentEquation(sHeadExpression);
                _parserRef->CacheCurrentTarget(sCache);
            }

            _parserRef->SetCompiling(false);
        }
    }

    // Evaluate std::strings
    if (nCurrentCalcType & CALCTYPE_STRING || !nCurrentCalcType)
    {
        if (nCurrentCalcType || NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
        {
            if (!nCurrentCalcType)
                nCalcType[nth_Cmd] |= CALCTYPE_STRING;

            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode();

            // Call the std::string parser
            auto retVal = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sHeadExpression, sCache, true, false, true);

            // Evaluate the return value
            if (retVal != NumeRe::StringParser::STRING_NUMERICAL)
            {
                result = NumeReKernel::getInstance()->getAns();

                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parserRef->PauseLoopMode(false);

                return result;
            }

            // It's possible that the user might have done
            // something weird with std::string operations transformed
            // into a regular expression. Replace the local
            // variables here
            replaceLocalVars(sHeadExpression);

            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode(false);
        }
    }

    // Evalute the already prepared equation
    if (!_parserRef->IsAlreadyParsed(sHeadExpression))
        _parserRef->SetExpr(sHeadExpression);

    if (!nCalcType[nth_Cmd] && !nJumpTable[nth_Cmd][PROCEDURE_INTERFACE])
        nCalcType[nth_Cmd] |= CALCTYPE_NUMERICAL;

    v = _parserRef->Eval(nNum);
    result.setDoubleArray(nNum, v);
    return result;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// evaluation of flow control statements from
/// the viewpoint of an if-else control flow.
///
/// \param __i int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::evalForkFlowCommands(int __i, int nth_loop)
{
    // Do nothing,if here's no flow command
    if (nJumpTable[__i][BLOCK_END] == NO_FLOW_COMMAND || !vCmdArray[__i].fcFn)
        return FLOWCTRL_NO_CMD;

    // Evaluate the possible flow control statements.
    // In this case, we have to consider the possiblity
    // that we need to display a status
    if (nth_loop <= -1)
    {
        if (vCmdArray[__i].sCommand.starts_with("for"))
        {
            bPrintedStatus = false;
            __i = (this->*vCmdArray[__i].fcFn)(__i, nth_loop+1);

            if (!bReturnSignal && !bMask && __i != -1)
            {
                if (bSilent)
                    NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... 100 %: "
                                              + _lang.get("COMMON_SUCCESS") + ".\n");
                else
                    NumeReKernel::printPreFmt("|FOR> " + _lang.get("COMMON_SUCCESS") + ".\n");
            }

            return __i;
        }
        else if (vCmdArray[__i].sCommand.starts_with("while"))
        {
            bPrintedStatus = false;
            __i = (this->*vCmdArray[__i].fcFn)(__i, nth_loop+1);

            if (!bReturnSignal && !bMask && __i != -1)
            {
                if (bSilent)
                    NumeReKernel::printPreFmt("\r|WHL> " + _lang.get("COMMON_EVALUATING") + " ...: "
                                              + _lang.get("COMMON_SUCCESS") + ".\n");
                else
                    NumeReKernel::printPreFmt("|WHL> " + _lang.get("COMMON_SUCCESS") + ".\n");
            }

            return __i;
        }
    }

    return (this->*vCmdArray[__i].fcFn)(__i, nth_loop+1);
}


/////////////////////////////////////////////////
/// \brief This member function is used to set a
/// command line from the outside into the flow
/// control statement class. The internal flow
/// control command buffer grows as needed.
///
/// \param __sCmd MutableStringView
/// \param nCurrentLine int
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::addToControlFlowBlock(MutableStringView __sCmd, int nCurrentLine)
{
    __sCmd.strip();
    bool bDebuggingBreakPoint = __sCmd.starts_with("|>");
    MutableStringView sAppendedExpression;

    // Remove the breakpoint syntax
    if (bDebuggingBreakPoint)
    {
        __sCmd.trim_front(2);
        __sCmd.strip();
    }

    if (bReturnSignal)
    {
        bReturnSignal = false;
        nReturnType = 1;
    }

    // If one passes the "abort" command, then NumeRe will
    // clean the internal buffer and return to the terminal
    if (__sCmd == "abort")
    {
        reset();
        NumeReKernel::print(_lang.get("LOOP_SETCOMMAND_ABORT"));
        return;
    }

    // Find the command of the current command line
    // if there's any
    std::string command = findCommand(__sCmd).sString;

    if (isAnyFlowCtrlStatement(command))
    {
        // Ensure that the parentheses are
        // matching each other
        if (!validateParenthesisNumber(__sCmd))
        {
            reset();
            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, __sCmd.get_viewed_string(),
                              __sCmd.find_first_of("({[]})")+__sCmd.get_offset());
        }

        FlowCtrlFunction fn = nullptr;

        if (command == "for")
        {
            blockNames.push_back(FCB_FOR);
            nFlowCtrlStatements[FC_FOR]++;

            bool isRangeBased = __sCmd.find("->") != std::string::npos;

            // Check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, true))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_FOR, __sCmd.get_viewed_string(),
                                  __sCmd.find("for")+__sCmd.get_offset());
            }

            // Replace the colon operator with a comma, which is faster
            // because it can be parsed internally
            size_t nQuotes = 0;
            size_t nPos = std::string::npos;
            size_t nTermPos = getMatchingParenthesis(__sCmd);

            for (size_t i = __sCmd.find('(')+1; i < nTermPos; i++)
            {
                if (__sCmd[i] == '"' && __sCmd[i-1] != '\\')
                    nQuotes++;

                if (!(nQuotes % 2))
                {
                    if (__sCmd[i] == '(' || __sCmd[i] == '{' || __sCmd[i] == '[')
                        i += getMatchingParenthesis(__sCmd.subview(i));

                    if (__sCmd[i] == ':')
                    {
                        nPos = i;
                        break;
                    }
                }
            }

            if (nPos == std::string::npos && isRangeBased)
                fn = FlowCtrl::range_based_for_loop;
            else
            {
                if (nPos != std::string::npos)
                    __sCmd[nPos] = ',';

                replaceAll(__sCmd, "->", "=");
                fn = FlowCtrl::for_loop;
            }
        }
        else if (command == "while")
        {
            blockNames.push_back(FCB_WHL);
            nFlowCtrlStatements[FC_WHILE]++;

            // check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, false))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_WHILE, __sCmd.get_viewed_string(),
                                  __sCmd.find("while")+__sCmd.get_offset());
            }

            fn = FlowCtrl::while_loop;
        }
        else if (command == "if")
        {
            blockNames.push_back(FCB_IF);
            nFlowCtrlStatements[FC_IF]++;

            // check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, false))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_IF, __sCmd.get_viewed_string(),
                                  __sCmd.find("if")+__sCmd.get_offset());
            }

            __sCmd.insert(0, toString(nFlowCtrlStatements[FC_IF]) + ">>");
            fn = FlowCtrl::if_fork;
        }
        else if (command == "else" || command == "elseif")
        {
            if (nFlowCtrlStatements[FC_IF] && (getCurrentBlock() == FCB_IF || getCurrentBlock() == FCB_ELIF))
            {
                if (command == "elseif")
                {
                    blockNames.back() = FCB_ELIF;

                    // check the flow control argument for completeness
                    if (!checkFlowControlArgument(__sCmd, false))
                    {
                        reset();
                        throw SyntaxError(SyntaxError::CANNOT_EVAL_IF, __sCmd.get_viewed_string(),
                                          __sCmd.find("elseif")+__sCmd.get_offset());
                    }
                }
                else
                    blockNames.back() = FCB_ELSE;

                __sCmd.insert(0, toString(nFlowCtrlStatements[FC_IF]) + ">>");
            }
            else
                return;
        }
        else if (command == "endif")
        {
            if (nFlowCtrlStatements[FC_IF] && (getCurrentBlock() == FCB_IF || getCurrentBlock() == FCB_ELIF || getCurrentBlock() == FCB_ELSE))
            {
                blockNames.pop_back();
                __sCmd.insert(0, toString(nFlowCtrlStatements[FC_IF]) + ">>");
                nFlowCtrlStatements[FC_IF]--;
            }
            else
                return;
        }
        else if (command == "switch")
        {
            blockNames.push_back(FCB_SWCH);
            nFlowCtrlStatements[FC_SWITCH]++;

            // check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, false))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_SWITCH, __sCmd.get_viewed_string(),
                                  __sCmd.find("switch")+__sCmd.get_offset());
            }

            __sCmd.insert(0, toString(nFlowCtrlStatements[FC_SWITCH]) + ">>");
            fn = FlowCtrl::switch_fork;
        }
        else if (command == "case" || command == "default")
        {
            if (nFlowCtrlStatements[FC_SWITCH] && (getCurrentBlock() == FCB_SWCH || getCurrentBlock() == FCB_CASE))
            {
                // check the case definition for completeness
                if (!checkCaseValue(__sCmd))
                {
                    reset();
                    throw SyntaxError(SyntaxError::CANNOT_EVAL_SWITCH, __sCmd.get_viewed_string(),
                                      __sCmd.find_first_of("cd")+__sCmd.get_offset());
                }

                // Set the corresponding loop names
                if (command == "default")
                    blockNames.back() = FCB_DEF;
                else
                    blockNames.back() = FCB_CASE;

                __sCmd.insert(0, toString(nFlowCtrlStatements[FC_SWITCH]) + ">>");
            }
            else
                return;
        }
        else if (command == "endswitch")
        {
            if (nFlowCtrlStatements[FC_SWITCH] && (getCurrentBlock() == FCB_SWCH
                                                   || getCurrentBlock() == FCB_CASE
                                                   || getCurrentBlock() == FCB_DEF))
            {
                blockNames.pop_back();
                __sCmd.insert(0, toString(nFlowCtrlStatements[FC_SWITCH]) + ">>");
                nFlowCtrlStatements[FC_SWITCH]--;
            }
            else
                return;
        }
        else if (command == "try")
        {
            blockNames.push_back(FCB_TRY);
            nFlowCtrlStatements[FC_TRY]++;

            __sCmd.insert(0, toString(nFlowCtrlStatements[FC_TRY]) + ">>");
            fn = FlowCtrl::try_catch;
        }
        else if (command == "catch")
        {
            if (nFlowCtrlStatements[FC_TRY] && (getCurrentBlock() == FCB_TRY || getCurrentBlock() == FCB_CTCH))
            {
                // check the case definition for completeness
                if (!checkCaseValue(__sCmd))
                {
                    reset();
                    throw SyntaxError(SyntaxError::CANNOT_EVAL_TRY, __sCmd.get_viewed_string(),
                                      __sCmd.find("catch")+__sCmd.get_offset());
                }

                // Set the corresponding loop names
                blockNames.back() = FCB_CTCH;

                __sCmd.insert(0, toString(nFlowCtrlStatements[FC_TRY]) + ">>");
            }
            else
                return;
        }
        else if (command == "endtry")
        {
            if (nFlowCtrlStatements[FC_TRY] && (getCurrentBlock() == FCB_TRY || getCurrentBlock() == FCB_CTCH))
            {
                blockNames.pop_back();
                __sCmd.insert(0, toString(nFlowCtrlStatements[FC_TRY]) + ">>");
                nFlowCtrlStatements[FC_TRY]--;
            }
            else
                return;
        }
        else if (command == "endfor")
        {
            if (nFlowCtrlStatements[FC_FOR] && getCurrentBlock() == FCB_FOR)
            {
                blockNames.pop_back();
                nFlowCtrlStatements[FC_FOR]--;
            }
            else
                return;
        }
        else if (command == "endwhile")
        {
            if (nFlowCtrlStatements[FC_WHILE] && getCurrentBlock() == FCB_WHL)
            {
                blockNames.pop_back();
                nFlowCtrlStatements[FC_WHILE]--;
            }
            else
                return;
        }
        else
            return;

        // If there's something after the current flow
        // control statement (i.e. a continued command
        // line), then cache this part here
        if (__sCmd.find('(') != std::string::npos
            && (command == "for"
                || command == "if"
                || command == "elseif"
                || command == "switch"
                || command == "while"))
        {
            if (bDebuggingBreakPoint)
                __sCmd.insert(__sCmd.find('(')+1, "|>");

            sAppendedExpression = __sCmd.subview(getMatchingParenthesis(__sCmd) + 1);
            __sCmd.remove_from(getMatchingParenthesis(__sCmd) + 1);
        }
        else if (command == "case" || command == "default" || command == "catch")
        {
            if (__sCmd.find(':', 4) != std::string::npos
                && __sCmd.find_first_not_of(": ", __sCmd.find(':', 4)) != std::string::npos)
            {
                sAppendedExpression = __sCmd.subview(__sCmd.find(':', 4)+1);
                __sCmd.remove_from(__sCmd.find(':', 4)+1);
            }
        }
        else if (__sCmd.find(' ', 4) != std::string::npos
            && __sCmd.find_first_not_of(' ', __sCmd.find(' ', 4)) != std::string::npos
            && __sCmd[__sCmd.find_first_not_of(' ', __sCmd.find(' ', 4))] != '-')
        {
            sAppendedExpression = __sCmd.subview(__sCmd.find(' ', 4));
            __sCmd.remove_from(__sCmd.find(' ', 4));
        }

        __sCmd.strip();

        if (sAppendedExpression.find_first_not_of(";") == std::string::npos)
            sAppendedExpression = nullptr;

        // Store the current flow control statement
        vCmdArray.push_back(FlowCtrlCommand(__sCmd.to_string(), nCurrentLine, true, fn));
    }
    else
    {
        // If there's a flow control statement somewhere
        // in the current std::string, then store this part in
        // a temporary cache
        size_t nQuotes = 0;

        for (size_t n = 0; n < __sCmd.length(); n++)
        {
            if (__sCmd[n] == '"' && (!n || __sCmd[n-1] != '\\'))
                nQuotes++;

            if (__sCmd[n] == ' ' && !(nQuotes % 2))
            {
                if (__sCmd.match(" for ", n)
                    || __sCmd.match(" for(", n)
                    || __sCmd.match(" endfor", n)
                    || __sCmd.match(" if ", n)
                    || __sCmd.match(" if(", n)
                    || __sCmd.match(" else", n)
                    || __sCmd.match(" elseif ", n)
                    || __sCmd.match(" elseif(", n)
                    || __sCmd.match(" endif", n)
                    || __sCmd.match(" switch ", n)
                    || __sCmd.match(" switch(", n)
                    || __sCmd.match(" case ", n)
                    || __sCmd.match(" default ", n)
                    || __sCmd.match(" default:", n)
                    || __sCmd.match(" endswitch", n)
                    || __sCmd.match(" try", n)
                    || __sCmd.match(" catch ", n)
                    || __sCmd.match(" catch:", n)
                    || __sCmd.match(" endtry", n)
                    || __sCmd.match(" while ", n)
                    || __sCmd.match(" while(", n)
                    || __sCmd.match(" endwhile", n))
                {
                    sAppendedExpression = __sCmd.subview(n + 1);
                    __sCmd.remove_from(n);
                    break;
                }
            }
        }

        // Store the current command line
        vCmdArray.push_back(FlowCtrlCommand((bDebuggingBreakPoint ? "|> " : "") + __sCmd.to_string(), nCurrentLine));
    }

    // If there's a command available and all flow
    // control statements have been closed, evaluate
    // the complete block
    if (vCmdArray.size())
    {
        if (!getCurrentBlockDepth() && vCmdArray.back().bFlowCtrlStatement)
            eval();
    }

    // If there's something left in the current cache,
    // call this function recursively
    if (sAppendedExpression.length())
        addToControlFlowBlock(sAppendedExpression, nCurrentLine);

    return;
}


/////////////////////////////////////////////////
/// \brief This member function prepares the
/// command array by pre-evaluating all constant
/// stuff or function calls whereever possible.
///
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::eval()
{
    nReturnType = 1;
    ReturnVal.clear();
    bUseLoopParsingMode = false;
    bFunctionsReplaced = false;
    bEvaluatingFlowControlStatements = true;
    bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;

    if (!NumeReKernel::getInstance())
    {
        reset();
        return;
    }

    // Copy the references to the centeral objects
    _parserRef = &NumeReKernel::getInstance()->getParser();
    _dataRef = &NumeReKernel::getInstance()->getMemoryManager();
    _outRef = &NumeReKernel::getInstance()->getOutput();
    _optionRef = &NumeReKernel::getInstance()->getSettings();
    _functionRef = &NumeReKernel::getInstance()->getDefinitions();
    _pDataRef = &NumeReKernel::getInstance()->getPlottingData();
    _scriptRef = &NumeReKernel::getInstance()->getScript();

    if (_parserRef->IsLockedPause())
        bLockedPauseMode = true;

    if (!_parserRef->ActiveLoopMode() || !_parserRef->IsLockedPause())
        _parserRef->ClearVectorVars(true);

    // Evaluate the user options
    if (_optionRef->useMaskDefault())
        bMask = true;

    // Read the flow control statements only and
    // extract the index variables and the flow
    // control flags
    std::string sVars = extractFlagsAndIndexVariables();

    // If already suppressed from somewhere else
    if (!_optionRef->systemPrints())
        bMask = true;

    // If the loop parsing mode is active, ensure that only
    // inline procedures are used in this case. Otherwise
    // turn it off again. Additionally check for "to_cmd()"
    // function, which will also turn the loop parsing mode
    // off.
    // If no function definition commands are found in the
    // command block, replace the definitions with their
    // expanded form.
    try
    {
        prepareLocalVarsAndReplace(sVars);
        checkParsingModeAndExpandDefinitions();
    }
    catch (...)
    {
        reset();
        NumeReKernel::bSupressAnswer = bSupressAnswer_back;
        throw;
    }

    // Prepare the bytecode storage
    nCalcType.resize(vCmdArray.size(), CALCTYPE_NONE);

    // Prepare the jump table
    nJumpTable.resize(vCmdArray.size(), std::vector<int>(JUMP_TABLE_ELEMENTS, NO_FLOW_COMMAND));

    // Go again through the whole command set and fill
    // the jump table with the corresponding block ends
    // and pre-evaluate the recursive expressions.
    // Furthermore, determine, whether the loop parsing
    // mode is reasonable.
    fillJumpTableAndExpandRecursives();

    // Prepare the array for the local variables
    //
    // If a loop variable was defined before the
    // current loop, this one is used. All others
    // are create locally and therefore get
    // special names
    // prepareLocalVarsAndReplace(sVars);

    // Activate the loop mode, if it is not locked
    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parserRef->ActivateLoopMode(vCmdArray.size());

    // Start the evaluation of the outermost
    // flow control statement
    try
    {
        // Determine the flow control command, which
        // is in the first line and call the corresponding
        // function for evaluation
        if (vCmdArray[0].sCommand.starts_with("for"))
        {
            if ((this->*vCmdArray[0].fcFn)(0, 0) == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_FOR, "", SyntaxError::invalid_position);
            }
            else if (!bReturnSignal && !bMask)
            {
                if (bSilent)
                    NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... 100 %: " + _lang.get("COMMON_SUCCESS") + ".\n");
                else
                    NumeReKernel::printPreFmt("|FOR> " + _lang.get("COMMON_SUCCESS") + ".\n");
            }
        }
        else if (vCmdArray[0].sCommand.starts_with("while"))
        {
            if (while_loop() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_WHILE, "", SyntaxError::invalid_position);
            }
            else if (!bReturnSignal && !bMask)
            {
                if (bSilent)
                    NumeReKernel::printPreFmt("\r|WHL> " + _lang.get("COMMON_EVALUATING") + " ...: " + _lang.get("COMMON_SUCCESS") + ".\n");
                else
                    NumeReKernel::printPreFmt("|WHL> " + _lang.get("COMMON_SUCCESS") + ".\n");
            }
        }
        else if (vCmdArray[0].sCommand.find(">>if") != std::string::npos)
        {
            if (if_fork() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_IF, "", SyntaxError::invalid_position);
            }
        }
        else if (vCmdArray[0].sCommand.find(">>switch") != std::string::npos)
        {
            if (switch_fork() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_SWITCH, "", SyntaxError::invalid_position);
            }
        }
        else if (vCmdArray[0].sCommand.find(">>try") != std::string::npos)
        {
            if (try_catch() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_TRY, "", SyntaxError::invalid_position);
            }
        }
        else
            throw SyntaxError(SyntaxError::INVALID_FLOWCTRL_STATEMENT, vCmdArray[0].sCommand, SyntaxError::invalid_position);
    }
    catch (...)
    {
        reset();
        NumeReKernel::bSupressAnswer = bSupressAnswer_back;

        if (bLoopSupressAnswer)
            bLoopSupressAnswer = false;

        if (bPrintedStatus)
            NumeReKernel::printPreFmt("\n");

        throw;
    }

    // Clear memory
    reset();

    if (bLoopSupressAnswer)
        bLoopSupressAnswer = false;

    return;
}


/////////////////////////////////////////////////
/// \brief This function clears the memory of
/// this FlowCtrl object and sets everything back
/// to its original state.
///
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::reset()
{
    vCmdArray.clear();

    for (size_t i = 0; i < sVarArray.size(); i++)
    {
        if (sVarArray[i].find('{') == std::string::npos && sVarArray[i].find("->") == std::string::npos)
        {
            if (NumeReKernel::getInstance()->getStringParser().isStringVar(sVarArray[i]))
                NumeReKernel::getInstance()->getStringParser().removeStringVar(sVarArray[i]);
            else
                _parserRef->RemoveVar(sVarArray[i]);
        }
    }

    if (mVarMap.size())
    {
        for (size_t i = 0; i < sVarArray.size(); i++)
        {
            for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
            {
                if (iter->second == sVarArray[i])
                {
                    mVarMap.erase(iter);
                    break;
                }
            }
        }

        if (!mVarMap.size())
            _parserRef->mVarMapPntr = nullptr;
    }

    if (!vVars.empty())
    {
        varmap_type::const_iterator item = vVars.begin();

        for (; item != vVars.end(); ++item)
        {
            _parserRef->DefineVar(item->first, item->second);

            for (size_t i = 0; i < sVarArray.size(); i++)
            {
                if (item->first == sVarArray[i])
                    *item->second = vVarArray[i];
            }
        }

        vVars.clear();
    }

    vVarArray.clear();
    sVarArray.clear();
    nJumpTable.clear();
    nCalcType.clear();

    if (nDebuggerCode == NumeReKernel::DEBUGGER_STEPOVER)
        nDebuggerCode = NumeReKernel::DEBUGGER_STEP;

    nLoopSafety = -1;

    for (size_t i = 0; i < FC_COUNT; i++)
        nFlowCtrlStatements[i] = 0;

    bSilent = true;
    blockNames.clear();
    sLoopPlotCompose = "";
    bMask = false;
    nCurrentCommand = 0;
    bEvaluatingFlowControlStatements = false;

    if (bUseLoopParsingMode && !bLockedPauseMode)
    {
        _parserRef->DeactivateLoopMode();
        bUseLoopParsingMode = false;
    }

    bLockedPauseMode = false;
    bFunctionsReplaced = false;

    // Remove obsolete vector variables
    if (_parserRef && (!_parserRef->ActiveLoopMode() || !_parserRef->IsLockedPause()))
        _parserRef->ClearVectorVars(true);

    for (const auto& cst : inlineClusters)
        _dataRef->removeCluster(cst.substr(0, cst.find('{')));

    inlineClusters.clear();
    _parserRef = nullptr;
    _dataRef = nullptr;
    _outRef = nullptr;
    _optionRef = nullptr;
    _functionRef = nullptr;
    _pDataRef = nullptr;
    _scriptRef = nullptr;

    return;
}


/////////////////////////////////////////////////
/// \brief This member function does the hard
/// work and compiles the numerical and std::string
/// results for the current command line to
/// provide the bytecode for the usual
/// calculation function.
///
/// \param sLine std::string
/// \param nthCmd int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::compile(std::string sLine, int nthCmd)
{
    std::string sCache;
    std::string sCommand;

    value_type* v = 0;
    int nNum = 0;
    Indices _idx;
    bool bCompiling = false;
    bool bWriteToCache = false;
    bool bWriteToCluster = false;
    bool returnCommand = false;
    _assertionHandler.reset();
    updateTestStats();

    // Eval the assertion command
    if (findCommand(sLine, "assert").sString == "assert")
    {
        nCalcType[nthCmd] |= CALCTYPE_ASSERT;
        _assertionHandler.enable(sLine);
        sLine.erase(findCommand(sLine, "assert").nPos, 6);
        StripSpaces(sLine);
        vCmdArray[nthCmd].sCommand.erase(findCommand(vCmdArray[nthCmd].sCommand, "assert").nPos, 6);
        StripSpaces(vCmdArray[nthCmd].sCommand);
    }


    // Eval the debugger breakpoint first
    if (sLine.substr(sLine.find_first_not_of(' '), 2) == "|>" || nDebuggerCode == NumeReKernel::DEBUGGER_STEP)
    {
        Breakpoint bp(true);
        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

        if (sLine.substr(sLine.find_first_not_of(' '), 2) == "|>")
        {
            nCalcType[nthCmd] |= CALCTYPE_DEBUGBREAKPOINT;
            sLine.erase(sLine.find("|>"), 2);

            if (_debugger.getBreakpointManager().isBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber()))
                bp = _debugger.getBreakpointManager().getBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber());

            if (bp.m_isConditional)
            {
                Procedure* proc = _debugger.getCurrentProcedure();

                if (proc)
                    bp.m_condition = proc->resolveVariables(bp.m_condition);

                replaceLocalVars(bp.m_condition);
            }

            StripSpaces(sLine);
            vCmdArray[nthCmd].sCommand.erase(vCmdArray[nthCmd].sCommand.find("|>"), 2);
            StripSpaces(vCmdArray[nthCmd].sCommand);
        }

        if (_optionRef->useDebugger()
            && nDebuggerCode != NumeReKernel::DEBUGGER_LEAVE
            && nDebuggerCode != NumeReKernel::DEBUGGER_STEPOVER
            && bp.isActive(!bLockedPauseMode && bUseLoopParsingMode))
        {
            _debugger.gatherLoopBasedInformations(sLine, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray);
            nDebuggerCode = evalDebuggerBreakPoint(*_parserRef, *_optionRef);
        }
    }

    // Handle the suppression semicolon
    if (sLine.find_last_not_of(" \t") != std::string::npos && sLine[sLine.find_last_not_of(" \t")] == ';')
    {
        sLine.erase(sLine.rfind(';'));
        bLoopSupressAnswer = true;
        nCalcType[nthCmd] |= CALCTYPE_SUPPRESSANSWER;
        vCmdArray[nthCmd].sCommand.erase(vCmdArray[nthCmd].sCommand.rfind(';'));
    }
    else
        bLoopSupressAnswer = false;

    sCommand = findCommand(sLine).sString;

    // Replace the custom defined functions, if it wasn't already done
    if (!bFunctionsReplaced
        && sCommand != "define"
        && sCommand != "redef"
        && sCommand != "redefine"
        && sCommand != "undefine"
        && sCommand != "undef"
        && sCommand != "ifndef"
        && sCommand != "ifndefined")
    {
        if (!_functionRef->call(sLine))
        {
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sLine, SyntaxError::invalid_position);
        }
    }

    if (sCommand == "define"
        || sCommand == "redef"
        || sCommand == "redefine"
        || sCommand == "undefine"
        || sCommand == "undef"
        || sCommand == "ifndef"
        || sCommand == "ifndefined")
    {
        nCalcType[nthCmd] |= CALCTYPE_COMMAND | CALCTYPE_DEFINITION;
    }

    if (bFunctionsReplaced && nCalcType[nthCmd] & CALCTYPE_DEFINITION)
        nCalcType[nthCmd] |= CALCTYPE_RECURSIVEEXPRESSION;

    // Handle the throw command
    if (sCommand == "throw" || sLine == "throw")
    {
        std::string sErrorToken;

        if (sLine.length() > 6 && NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine))
        {
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sLine))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sLine);

            sErrorToken = sLine.substr(findCommand(sLine).nPos+6);
            sErrorToken += " -nq";
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sErrorToken, sCache, true, false, true);
        }

        nCalcType[nthCmd] |= CALCTYPE_THROWCOMMAND;
        throw SyntaxError(SyntaxError::LOOP_THROW, sLine, SyntaxError::invalid_position, sErrorToken);
    }

    // Handle the return command
    if (sCommand == "return")
    {
        nCalcType[nthCmd] |= CALCTYPE_RETURNCOMMAND;

        std::string sReturnValue = sLine.substr(sLine.find("return") + 6);
        StripSpaces(sReturnValue);

        if (sReturnValue.back() == ';')
            sReturnValue.pop_back();

        StripSpaces(sReturnValue);

        if (sReturnValue == "void")
        {
            bReturnSignal = true;
            nReturnType = 0;
            return FLOWCTRL_RETURN;
        }
        else if (sReturnValue.find('(') != std::string::npos
                 && sReturnValue.substr(sReturnValue.find('(')) == "()"
                 && _dataRef->isTable(sReturnValue.substr(0, sReturnValue.find('('))))
        {
            ReturnVal.sReturnedTable = sReturnValue.substr(0, sReturnValue.find('('));
            bReturnSignal = true;
            return FLOWCTRL_RETURN;
        }
        else if (!sReturnValue.length())
        {
            ReturnVal.vNumVal.push_back(1.0);
            bReturnSignal = true;
            return FLOWCTRL_RETURN;
        }

        sLine = sReturnValue;
        returnCommand = true;
    }

    // Check, whether the user tried to abort the
    // current evaluation
    if (NumeReKernel::GetAsyncCancelState())
    {
        if (bPrintedStatus)
            NumeReKernel::printPreFmt(" " + _lang.get("COMMON_CANCEL"));

        throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
    }

    // Is it a numerical expression, which was already
    // parsed? Evaluate it here
    if (_parserRef->IsValidByteCode() == 1 && _parserRef->IsAlreadyParsed(sLine) && !bLockedPauseMode && bUseLoopParsingMode)
    {
        nCalcType[nthCmd] |= CALCTYPE_NUMERICAL;
        v = _parserRef->Eval(nNum);
        _assertionHandler.checkAssertion(v, nNum);

        vAns = v[0];
        NumeReKernel::getInstance()->getAns().setDoubleArray(nNum, v);

        if (!bLoopSupressAnswer)
        {
            /* --> Der Benutzer will also die Ergebnisse sehen. Es gibt die Moeglichkeit,
             *     dass der Parser mehrere Ausdruecke je Zeile auswertet. Dazu muessen die
             *     Ausdruecke durch Kommata getrennt sein. Damit der Parser bemerkt, dass er
             *     mehrere Ausdruecke auszuwerten hat, muss man die Auswerte-Funktion des
             *     Parsers einmal aufgerufen werden <--
             */
            NumeReKernel::print(NumeReKernel::formatResultOutput(nNum, v));
        }

        return FLOWCTRL_OK;
    }

    // Does this contain a plot composition? Combine the
    // needed lines here. This is not necessary, if the lines
    // are read from a procedure, which will provide the compositon
    // in a single line
    if ((sCommand == "compose"
        || sCommand == "endcompose"
        || sLoopPlotCompose.length())
        && sCommand != "quit")
    {
        nCalcType[nthCmd] |= CALCTYPE_COMPOSE;

        if (!sLoopPlotCompose.length() && sCommand == "compose")
        {
            sLoopPlotCompose = "plotcompose ";
            return FLOWCTRL_OK;
        }
        else if (sCommand == "abort")
        {
            sLoopPlotCompose = "";
            return FLOWCTRL_OK;
        }
        else if (sCommand != "endcompose")
        {
            if (Plot::isPlottingCommand(sCommand))
                sLoopPlotCompose += sLine + " <<COMPOSE>> ";

            return FLOWCTRL_OK;
        }
        else
        {
            sLine = sLoopPlotCompose;
            sLoopPlotCompose = "";
        }
    }

    // Handle the "to_cmd()" function here
    if (sLine.find("to_cmd(") != std::string::npos)
    {
        nCalcType[nthCmd] |= CALCTYPE_TOCOMMAND | CALCTYPE_COMMAND | CALCTYPE_DATAACCESS | CALCTYPE_EXPLICIT | CALCTYPE_PROGRESS | CALCTYPE_RECURSIVEEXPRESSION;

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();

        size_t nPos = 0;

        while (sLine.find("to_cmd(", nPos) != std::string::npos)
        {
            nPos = sLine.find("to_cmd(", nPos) + 6;

            if (isInQuotes(sLine, nPos))
                continue;

            size_t nParPos = getMatchingParenthesis(StringView(sLine, nPos));

            if (nParPos == std::string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nPos);

            std::string sCmdString = sLine.substr(nPos + 1, nParPos - 1);
            StripSpaces(sCmdString);

            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmdString))
            {
                sCmdString += " -nq";
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmdString, sCache, true);
                sCache = "";
            }

            sLine = sLine.substr(0, nPos - 6) + sCmdString + sLine.substr(nPos + nParPos + 1);
            nPos -= 5;
        }

        replaceLocalVars(sLine);

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);

        sCommand = findCommand(sLine).sString;
    }

    // Display the prompt to the user
    if (sLine.find("??") != std::string::npos)
    {
        nCalcType[nthCmd] |= CALCTYPE_PROMPT | CALCTYPE_PROCEDURECMDINTERFACE | CALCTYPE_COMMAND | CALCTYPE_DATAACCESS | CALCTYPE_STRING | CALCTYPE_RECURSIVEEXPRESSION;

        if (bPrintedStatus)
            NumeReKernel::printPreFmt("\n");

        sLine = promptForUserInput(sLine);
        bPrintedStatus = false;
    }

    // Include procedure and plugin calls
    if (nJumpTable[nthCmd][PROCEDURE_INTERFACE])
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }

        // Prepend the return statement
        if (returnCommand)
            sLine.insert(0, "return ");

        ProcedureInterfaceRetVal nReturn = procedureInterface(sLine, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nthCmd);

        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode(false);
            _parserRef->LockPause(false);
        }

        if (nReturn == INTERFACE_EMPTY || nReturn == INTERFACE_VALUE)
            nJumpTable[nthCmd][PROCEDURE_INTERFACE] = 1;
        else
            nJumpTable[nthCmd][PROCEDURE_INTERFACE] = 0;

        if (bReturnSignal)
            return FLOWCTRL_RETURN;
        else if (returnCommand)
            sLine.erase(0, 7); // Rempve the return statement

        if (nReturn == INTERFACE_ERROR)
            return FLOWCTRL_ERROR;
        else if (nReturn == INTERFACE_EMPTY)
            return FLOWCTRL_OK;
    }

    // Handle the procedure commands like "namespace" here
    int nProcedureCmd = procedureCmdInterface(sLine);

    if (nProcedureCmd)
    {
        if (nProcedureCmd == 1)
        {
            nCalcType[nthCmd] |= CALCTYPE_PROCEDURECMDINTERFACE;
            return FLOWCTRL_OK;
        }
    }
    else
        return FLOWCTRL_ERROR;

    // Remove the "explicit" command here
    if (sCommand == "explicit")
    {
        nCalcType[nthCmd] |= CALCTYPE_EXPLICIT;
        sLine.erase(findCommand(sLine).nPos, 8);
        StripSpaces(sLine);
    }

    // Evaluate the command, if this is a command
    if (!bLockedPauseMode && bUseLoopParsingMode)
        _parserRef->PauseLoopMode();

    {
        bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;
        std::string sPreCommandLine = sLine;
        NumeReKernel::bSupressAnswer = bLoopSupressAnswer;

        switch (commandHandler(sLine))
        {
            case NO_COMMAND:
                {
                    StripSpaces(sPreCommandLine);
                    std::string sCurrentLine = sLine;
                    StripSpaces(sCurrentLine);

                    if (sPreCommandLine != sCurrentLine)
                        nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                }

                break;
            case COMMAND_PROCESSED:
                NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                nCalcType[nthCmd] |= CALCTYPE_COMMAND;

                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parserRef->PauseLoopMode(false);

                return FLOWCTRL_OK;
            case NUMERE_QUIT:
                NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                return FLOWCTRL_OK;
            case COMMAND_HAS_RETURNVALUE:
                NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                break;
        }

        NumeReKernel::bSupressAnswer = bSupressAnswer_back;

        // It may be possible that some procedure occures at this
        // position. Handle them here
        if (sLine.find('$') != std::string::npos)
            procedureInterface(sLine, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nthCmd);
    }

    if (!bLockedPauseMode && bUseLoopParsingMode)
        _parserRef->PauseLoopMode(false);

    // Expand recursive expressions, if not already done
    evalRecursiveExpressions(sLine);

    bool stringParserAdHoc = false;

    // Get the data from the used data object
    if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine)
        && _dataRef->containsTablesOrClusters(sLine))
    {
        nCalcType[nthCmd] |= CALCTYPE_DATAACCESS;

        if (!_parserRef->HasCachedAccess() && _parserRef->CanCacheAccess() && !_parserRef->GetCachedEquation().length())
        {
            bCompiling = true;
            _parserRef->SetCompiling(true);
        }

        sCache = getDataElements(sLine, *_parserRef, *_dataRef, *_optionRef);

        if (sCache.length())
            bWriteToCache = true;

        // Ad-hoc bytecode adaption
#warning NOTE (numere#1#08/21/21): Might need some adaption, if bytecode issues are experienced
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine))
            stringParserAdHoc = true;

        if (_parserRef->IsCompiling() && _parserRef->CanCacheAccess())
        {
            _parserRef->CacheCurrentEquation(sLine);
            _parserRef->CacheCurrentTarget(sCache);
        }

        _parserRef->SetCompiling(false);
    }
    else if (isClusterCandidate(sLine, sCache))
    {
        bWriteToCache = true;
        nCalcType[nthCmd] |= CALCTYPE_DATAACCESS;
    }


    // Evaluate std::string expressions
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine)
        || (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCache)
            && !_dataRef->isTable(sCache))) // hack for performance Re:#178
    {
        // Do not add the byte code, if it was added ad-hoc
        if (!stringParserAdHoc)
            nCalcType[nthCmd] |= CALCTYPE_STRING | CALCTYPE_DATAACCESS;

        //if (!bLockedPauseMode && bUseLoopParsingMode)
        //    _parserRef->PauseLoopMode();

        auto retVal = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sLine, sCache, bLoopSupressAnswer, true, true);
        NumeReKernel::getInstance()->getStringParser().removeTempStringVectorVars();

        if (retVal == NumeRe::StringParser::STRING_SUCCESS)
        {
            //if (!bLockedPauseMode && bUseLoopParsingMode)
            //    _parserRef->PauseLoopMode(false);

            if (returnCommand)
            {
                ReturnVal.vStringVal = NumeReKernel::getInstance()->getAns().to_string();
                bReturnSignal = true;
                return FLOWCTRL_RETURN;
            }

            return FLOWCTRL_OK;
        }

        replaceLocalVars(sLine);

#warning NOTE (erik.haenel#3#): This is changed due to double writes in combination with c{nlen+1} = VAL
        //if (sCache.length() && _dataRef->containsTablesOrClusters(sCache) && !bWriteToCache)
        //    bWriteToCache = true;

        if (sCache.length())
        {
            bWriteToCache = false;
            sCache.clear();
        }


        //if (!bLockedPauseMode && bUseLoopParsingMode)
        //    _parserRef->PauseLoopMode(false);
    }

    // Get the target indices of the target data object
    if (bWriteToCache)
    {
        size_t pos;

        if (bCompiling)
        {
            _parserRef->SetCompiling(true);
            getIndices(sCache, _idx, *_parserRef, *_dataRef, *_optionRef, true);

            if (sCache[(pos = sCache.find_first_of("({"))] == '{')
                bWriteToCluster = true;

            if (!isValidIndexSet(_idx))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "", _idx.row.to_string() + ", " + _idx.col.to_string());

            if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

            sCache.erase(pos);
            StripSpaces(sCache);

            if (!bWriteToCluster)
                _parserRef->CacheCurrentTarget(sCache + "(" + _idx.sCompiledAccessEquation + ")");
            else
                _parserRef->CacheCurrentTarget(sCache + "{" + _idx.sCompiledAccessEquation + "}");

            _parserRef->SetCompiling(false);
        }
        else
        {
            getIndices(sCache, _idx, *_parserRef, *_dataRef, *_optionRef, true);
            //_idx.col.front() = 0;
            //_idx.row.front() = 0;

            if (sCache[(pos = sCache.find_first_of("({"))] == '{')
                bWriteToCluster = true;

            if (!isValidIndexSet(_idx))
                throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "", _idx.row.to_string() + ", " + _idx.col.to_string());

            if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

            sCache.erase(pos);
        }
    }

    // Parse the numerical expression, if it is not
    // already available as bytecode
    if (!_parserRef->IsAlreadyParsed(sLine))
        _parserRef->SetExpr(sLine);

    // Calculate the result
    v = _parserRef->Eval(nNum);
    _assertionHandler.checkAssertion(v, nNum);

    vAns = v[0];
    NumeReKernel::getInstance()->getAns().setDoubleArray(nNum, v);

    // Do only add the bytecode flag, if it does not depend on
    // previous operations
    if (!nJumpTable[nthCmd][PROCEDURE_INTERFACE]
        && !(nCalcType[nthCmd] & (CALCTYPE_DATAACCESS
                                  | CALCTYPE_STRING
                                  | CALCTYPE_TOCOMMAND
                                  | CALCTYPE_COMMAND
                                  | CALCTYPE_PROCEDURECMDINTERFACE
                                  | CALCTYPE_PROMPT)))
        nCalcType[nthCmd] |= CALCTYPE_NUMERICAL;

    if (!bLoopSupressAnswer)
    {
        /* --> Der Benutzer will also die Ergebnisse sehen. Es gibt die Moeglichkeit,
         *     dass der Parser mehrere Ausdruecke je Zeile auswertet. Dazu muessen die
         *     Ausdruecke durch Kommata getrennt sein. Damit der Parser bemerkt, dass er
         *     mehrere Ausdruecke auszuwerten hat, muss man die Auswerte-Funktion des
         *     Parsers einmal aufgerufen werden <--
         */
        NumeReKernel::print(NumeReKernel::formatResultOutput(nNum, v));
    }

    // Write the result to a table or a cluster
    // this was implied by the syntax of the command
    // line
    if (bWriteToCache)
    {
        // Is it a cluster?
        if (bWriteToCluster)
            _dataRef->getCluster(sCache).assignResults(_idx, nNum, v);
        else
            _dataRef->writeToTable(_idx, sCache, v, nNum);
    }

    if (returnCommand)
    {
        for (int i = 0; i < nNum; i++)
            ReturnVal.vNumVal.push_back(v[i]);

        bReturnSignal = true;
        return FLOWCTRL_RETURN;
    }

    return FLOWCTRL_OK;
}


/////////////////////////////////////////////////
/// \brief This member function does the hard
/// work and calculates the numerical and std::string
/// results for the current command line. It will
/// use the previously determined bytecode
/// whereever possible.
///
/// \param sLine std::string
/// \param nthCmd int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::calc(StringView sLine, int nthCmd)
{
    value_type* v = 0;
    int nNum = 0;
    NumeRe::Cluster& ans = NumeReKernel::getInstance()->getAns();

    // No great impact on calctime
    _assertionHandler.reset();
    updateTestStats();

    // Get the current bytecode for this command
    int nCurrentCalcType = nCalcType[nthCmd];

    // If the current line has no bytecode attached, then
    // change the function and calculate it. Using the determined
    // bytecode, we can omit many checks at the next
    // iteration
    if (!nCurrentCalcType)
        return compile(sLine.to_string(), nthCmd);

    bLoopSupressAnswer = nCurrentCalcType & CALCTYPE_SUPPRESSANSWER;
    //return FLOWCTRL_OK;

    std::string sBuffer;

    // Eval the assertion command
    if (nCurrentCalcType & CALCTYPE_ASSERT)
        _assertionHandler.enable("assert " + sLine);

    // Eval the debugger breakpoint first
    if (nCurrentCalcType & CALCTYPE_DEBUGBREAKPOINT || nDebuggerCode == NumeReKernel::DEBUGGER_STEP)
    {
        Breakpoint bp(true);
        NumeReDebugger& _debugger = NumeReKernel::getInstance()->getDebugger();

        if (nCurrentCalcType & CALCTYPE_DEBUGBREAKPOINT)
        {
            if (_debugger.getBreakpointManager().isBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber()))
                bp = _debugger.getBreakpointManager().getBreakpoint(_debugger.getExecutedModule(), getCurrentLineNumber());

            if (bp.m_isConditional)
            {
                Procedure* proc = _debugger.getCurrentProcedure();

                if (proc)
                    bp.m_condition = proc->resolveVariables(bp.m_condition);

                replaceLocalVars(bp.m_condition);
            }
        }

        if (_optionRef->useDebugger()
            && nDebuggerCode != NumeReKernel::DEBUGGER_LEAVE
            && nDebuggerCode != NumeReKernel::DEBUGGER_STEPOVER
            && bp.isActive(!bLockedPauseMode && bUseLoopParsingMode))
        {
            _debugger.gatherLoopBasedInformations(sLine.to_string(), getCurrentLineNumber(),
                                                  mVarMap, vVarArray, sVarArray);
            nDebuggerCode = evalDebuggerBreakPoint(*_parserRef, *_optionRef);
        }
    }

    // Replace the custom defined functions, if it wasn't already done
    if (!(nCurrentCalcType & CALCTYPE_DEFINITION) && !bFunctionsReplaced)
    {
        sBuffer = sLine.to_string();

        if (!_functionRef->call(sBuffer))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sBuffer, SyntaxError::invalid_position);

        sLine = StringView(sBuffer);
    }

    // Handle the throw command
    if (nCurrentCalcType & CALCTYPE_THROWCOMMAND)
    {
        std::string sErrorToken;
        sBuffer = sLine.to_string();

        if (sLine.length() > 6 && NumeReKernel::getInstance()->getStringParser().isStringExpression(sBuffer))
        {
            if (NumeReKernel::getInstance()->getStringParser().containsStringVars(sBuffer))
                NumeReKernel::getInstance()->getStringParser().getStringValues(sBuffer);

            sErrorToken = sBuffer.substr(findCommand(sBuffer).nPos+6);
            sErrorToken += " -nq";
            std::string sDummy;
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(sErrorToken, sDummy, true, false, true);
        }

        throw SyntaxError(SyntaxError::LOOP_THROW, sBuffer, SyntaxError::invalid_position, sErrorToken);
    }

    // Handle the return command
    if (nCurrentCalcType & CALCTYPE_RETURNCOMMAND)
    {

        std::string sReturnValue = sLine.subview(sLine.find("return") + 6).to_string();
        StripSpaces(sReturnValue);

        if (sReturnValue.back() == ';')
            sReturnValue.pop_back();

        StripSpaces(sReturnValue);

        if (sReturnValue == "void")
        {
            bReturnSignal = true;
            nReturnType = 0;
            return FLOWCTRL_RETURN;
        }
        else if (sReturnValue.find('(') != std::string::npos
                 && sReturnValue.substr(sReturnValue.find('(')) == "()"
                 && _dataRef->isTable(sReturnValue.substr(0, sReturnValue.find('('))))
        {
            ReturnVal.sReturnedTable = sReturnValue.substr(0, sReturnValue.find('('));
            bReturnSignal = true;
            return FLOWCTRL_RETURN;
        }
        else if (!sReturnValue.length())
        {
            ReturnVal.vNumVal.push_back(1.0);
            bReturnSignal = true;
            return FLOWCTRL_RETURN;
        }

        sBuffer = sReturnValue;
        sLine = StringView(sBuffer);
    }

    // Check, whether the user tried to abort the
    // current evaluation
    if (NumeReKernel::GetAsyncCancelState())
    {
        if (bPrintedStatus)
            NumeReKernel::printPreFmt(" " + _lang.get("COMMON_CANCEL"));

        throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
    }

    // Is it a numerical expression, which was already
    // parsed? Evaluate it here
    if (nCurrentCalcType & CALCTYPE_NUMERICAL)
    {
        // As long as bytecode parsing is not globally available,
        // this condition has to stay at this place
        if (!(bUseLoopParsingMode && !bLockedPauseMode) && !_parserRef->IsAlreadyParsed(sLine))
            _parserRef->SetExpr(sLine);

        // Evaluate all remaining equations in the stack
        do
        {
            v = _parserRef->Eval(nNum);
        } while (_parserRef->IsNotLastStackItem());

        // Check only the last expression
        _assertionHandler.checkAssertion(v, nNum);

        vAns = v[0];
        ans.setDoubleArray(nNum, v);

        if (!bLoopSupressAnswer)
            NumeReKernel::print(NumeReKernel::formatResultOutput(nNum, v));

        return FLOWCTRL_OK;
    }

    // Does this contain a plot composition? Combine the
    // needed lines here. This is not necessary, if the lines
    // are read from a procedure, which will provide the compositon
    // in a single line
    if (nCurrentCalcType & CALCTYPE_COMPOSE)// || sLoopPlotCompose.length())
    {
        std::string sCommand = findCommand(sLine).sString;

        if (sCommand == "compose"
            || sCommand == "endcompose"
            || sLoopPlotCompose.length())
        {
            if (!sLoopPlotCompose.length() && sCommand == "compose")
            {
                sLoopPlotCompose = "plotcompose ";
                return FLOWCTRL_OK;
            }
            else if (sCommand == "abort")
            {
                sLoopPlotCompose = "";
                return FLOWCTRL_OK;
            }
            else if (sCommand != "endcompose")
            {
                if (Plot::isPlottingCommand(sCommand))
                    sLoopPlotCompose += sLine.to_string() + " <<COMPOSE>> ";

                return FLOWCTRL_OK;
            }
            else
            {
                sBuffer = sLoopPlotCompose;
                sLine = StringView(sBuffer);
                sLoopPlotCompose = "";
            }
        }
    }

    // Handle the "to_cmd()" function here
    if (nCurrentCalcType & CALCTYPE_TOCOMMAND)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();

        size_t nPos = 0;
        sBuffer = sLine.to_string();

        while (sBuffer.find("to_cmd(", nPos) != std::string::npos)
        {
            nPos = sBuffer.find("to_cmd(", nPos) + 6;

            if (isInQuotes(sBuffer, nPos))
                continue;

            size_t nParPos = getMatchingParenthesis(StringView(sBuffer, nPos));

            if (nParPos == std::string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sBuffer, nPos);

            std::string sCmdString = sBuffer.substr(nPos + 1, nParPos - 1);
            StripSpaces(sCmdString);

            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmdString))
            {
                sCmdString += " -nq";
                std::string sDummy;
                NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmdString, sDummy, true);
            }

            sBuffer = sBuffer.substr(0, nPos - 6) + sCmdString + sBuffer.substr(nPos + nParPos + 1);
            nPos -= 5;
        }

        replaceLocalVars(sBuffer);
        sLine = StringView(sBuffer);

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }

    // Display the prompt to the user
    if (nCurrentCalcType & CALCTYPE_PROMPT)
    {
        if (bPrintedStatus)
            NumeReKernel::printPreFmt("\n");

        sBuffer = sLine.to_string();
        sBuffer = promptForUserInput(sBuffer);
        sLine = StringView(sBuffer);
        bPrintedStatus = false;
    }

    // Include procedure and plugin calls
    if (nJumpTable[nthCmd][PROCEDURE_INTERFACE])
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }

        sBuffer = sLine.to_string();

        if (nCurrentCalcType & CALCTYPE_RETURNCOMMAND)
            sBuffer.insert(0, "return ");

        ProcedureInterfaceRetVal nReturn = procedureInterface(sBuffer, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nthCmd);

        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode(false);
            _parserRef->LockPause(false);
        }

        if (nReturn == INTERFACE_EMPTY || nReturn == INTERFACE_VALUE)
            nJumpTable[nthCmd][PROCEDURE_INTERFACE] = 1;
        else
            nJumpTable[nthCmd][PROCEDURE_INTERFACE] = 0;

        if (bReturnSignal)
            return FLOWCTRL_RETURN;
        else if (nCurrentCalcType & CALCTYPE_RETURNCOMMAND)
            sBuffer.erase(0, 7);

        if (nReturn == INTERFACE_ERROR)
            return FLOWCTRL_ERROR;
        else if (nReturn == INTERFACE_EMPTY)
            return FLOWCTRL_OK;

        sLine = StringView(sBuffer);
    }

    // Handle the procedure commands like "namespace" here
    if (nCurrentCalcType & CALCTYPE_PROCEDURECMDINTERFACE)
    {
        int nProcedureCmd = procedureCmdInterface(sLine);

        if (nProcedureCmd)
        {
            if (nProcedureCmd == 1)
                return FLOWCTRL_OK;
        }
        else
            return FLOWCTRL_ERROR;
    }

    // Remove the "explicit" command here
    if (nCurrentCalcType & CALCTYPE_EXPLICIT)
    {
        if (findCommand(sLine).sString == "explicit")
        {
            sBuffer = sLine.to_string();
            sBuffer.erase(findCommand(sBuffer).nPos, 8);
            sLine = StringView(sBuffer);
            sLine.strip();
        }
    }

    // Evaluate the command, if this is a command
    if (nCurrentCalcType & CALCTYPE_COMMAND)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();

        {
            bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;
            sBuffer = sLine.to_string();
            NumeReKernel::bSupressAnswer = bLoopSupressAnswer;

            switch (commandHandler(sBuffer))
            {
                case NO_COMMAND:
                    break;
                case COMMAND_PROCESSED:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;

                    if (!bLockedPauseMode && bUseLoopParsingMode)
                        _parserRef->PauseLoopMode(false);

                    return FLOWCTRL_OK;
                case NUMERE_QUIT:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                    return FLOWCTRL_OK;
                case COMMAND_HAS_RETURNVALUE:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                    break;
            }

            NumeReKernel::bSupressAnswer = bSupressAnswer_back;

            // It may be possible that some procedure occures at this
            // position. Handle them here
            if (sBuffer.find('$') != std::string::npos)
                procedureInterface(sBuffer, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nthCmd);

            sLine = StringView(sBuffer);
        }

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }

    // Expand recursive expressions, if not already done
    if (nCurrentCalcType & CALCTYPE_RECURSIVEEXPRESSION)
    {
        sBuffer = sLine.to_string();
        evalRecursiveExpressions(sBuffer);
        sLine = StringView(sBuffer);
    }

    // Declare all further necessary variables
    std::string sDataObject;
    bool bCompiling = false;
    bool bWriteToCache = false;
    bool bWriteToCluster = false;

    // Get the data from the used data object
    if (nCurrentCalcType & CALCTYPE_DATAACCESS)
    {
        sBuffer = sLine.to_string();
        // --> Datafile/Cache! <--
        if (!(nCurrentCalcType & CALCTYPE_STRING)
            || (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sBuffer)
                && _dataRef->containsTablesOrClusters(sBuffer)))
        {
            if (!_parserRef->HasCachedAccess() && _parserRef->CanCacheAccess() && !_parserRef->GetCachedEquation().length())
            {
                bCompiling = true;
                _parserRef->SetCompiling(true);
            }

            sDataObject = getDataElements(sBuffer, *_parserRef, *_dataRef, *_optionRef);

            if (sDataObject.length())
                bWriteToCache = true;

            // Ad-hoc bytecode adaption
#warning NOTE (numere#1#08/21/21): Might need some adaption, if bytecode issues are experienced
            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sBuffer))
                nCurrentCalcType |= CALCTYPE_STRING;

            if (_parserRef->IsCompiling() && _parserRef->CanCacheAccess())
            {
                _parserRef->CacheCurrentEquation(sBuffer);
                _parserRef->CacheCurrentTarget(sDataObject);
            }

            _parserRef->SetCompiling(false);
        }
        else if (isClusterCandidate(sBuffer, sDataObject))
            bWriteToCache = true;

        sLine = StringView(sBuffer);
    }

    // Evaluate std::string expressions
    if (nCurrentCalcType & CALCTYPE_STRING)
    {
        //if (!bLockedPauseMode && bUseLoopParsingMode)
        //    _parserRef->PauseLoopMode();

        sBuffer = sLine.to_string();
        auto retVal = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sBuffer, sDataObject, bLoopSupressAnswer, true, true);
        NumeReKernel::getInstance()->getStringParser().removeTempStringVectorVars();

        if (retVal == NumeRe::StringParser::STRING_SUCCESS)
        {
            //if (!bLockedPauseMode && bUseLoopParsingMode)
            //    _parserRef->PauseLoopMode(false);

            if (nCurrentCalcType & CALCTYPE_RETURNCOMMAND)
            {
                bReturnSignal = true;
                ReturnVal.vStringVal = NumeReKernel::getInstance()->getAns().to_string();
                return FLOWCTRL_RETURN;
            }

            return FLOWCTRL_OK;
        }

        replaceLocalVars(sBuffer);

#warning NOTE (erik.haenel#3#): This is changed due to double writes in combination with c{nlen+1} = VAL
        //if (sDataObject.length() && _dataRef->containsTablesOrClusters(sDataObject) && !bWriteToCache)
        //    bWriteToCache = true;

        if (sDataObject.length())
        {
            bWriteToCache = false;
            sDataObject.clear();
        }

        //if (!bLockedPauseMode && bUseLoopParsingMode)
        //    _parserRef->PauseLoopMode(false);

        sLine = StringView(sBuffer);
    }

    // Prepare the indices for storing the data
    Indices _idx;

    // Get the target indices of the target data object
    if (nCurrentCalcType & CALCTYPE_DATAACCESS)
    {
        if (bWriteToCache)
        {
            size_t pos;

            if (bCompiling)
            {
                _parserRef->SetCompiling(true);
                getIndices(sDataObject, _idx, *_parserRef, *_dataRef, *_optionRef, true);

                if (sDataObject[(pos = sDataObject.find_first_of("({"))] == '{')
                    bWriteToCluster = true;

                if (!isValidIndexSet(_idx))
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sDataObject, "", _idx.row.to_string() + ", " + _idx.col.to_string());

                if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                    throw SyntaxError(SyntaxError::NO_MATRIX, sDataObject, "");

                sDataObject.erase(pos);
                StripSpaces(sDataObject);

                if (!bWriteToCluster)
                    _parserRef->CacheCurrentTarget(sDataObject + "(" + _idx.sCompiledAccessEquation + ")");
                else
                    _parserRef->CacheCurrentTarget(sDataObject + "{" + _idx.sCompiledAccessEquation + "}");

                _parserRef->SetCompiling(false);
            }
            else
            {
                getIndices(sDataObject, _idx, *_parserRef, *_dataRef, *_optionRef, true);
                //_idx.col.front() = 0;
                //_idx.row.front() = 0;

                if (sDataObject[(pos = sDataObject.find_first_of("({"))] == '{')
                    bWriteToCluster = true;

                if (!isValidIndexSet(_idx))
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sDataObject, "", _idx.row.to_string() + ", " + _idx.col.to_string());

                if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                    throw SyntaxError(SyntaxError::NO_MATRIX, sDataObject, "");

                sDataObject.erase(pos);
            }
        }
    }

    // Parse the numerical expression, if it is not
    // already available as bytecode
    if (!_parserRef->IsAlreadyParsed(sLine))
        _parserRef->SetExpr(sLine);

    // Calculate the result
    v = _parserRef->Eval(nNum);
    _assertionHandler.checkAssertion(v, nNum);

    vAns = v[0];
    ans.setDoubleArray(nNum, v);

    if (!bLoopSupressAnswer)
        NumeReKernel::print(NumeReKernel::formatResultOutput(nNum, v));

    // Write the result to a table or a cluster
    // this was implied by the syntax of the command
    // line
    if (bWriteToCache)
    {
        // Is it a cluster?
        if (bWriteToCluster)
            _dataRef->getCluster(sDataObject).assignResults(_idx, nNum, v);
        else
            _dataRef->writeToTable(_idx, sDataObject, v, nNum);
    }

    if (nCurrentCalcType & CALCTYPE_RETURNCOMMAND)
    {
        for (int i = 0; i < nNum; i++)
            ReturnVal.vNumVal.push_back(v[i]);

        bReturnSignal = true;
        return FLOWCTRL_RETURN;
    }

    return FLOWCTRL_OK;
}


/////////////////////////////////////////////////
/// \brief This member function is used to
/// replace variable occurences with their
/// (auto-determined) internal name.
///
/// \param sLine MutableStringView
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::replaceLocalVars(MutableStringView sLine)
{
    if (!mVarMap.size())
        return;

    for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
    {
        for (size_t i = 0; i < sLine.length(); i++)
        {
            if (sLine.match(iter->first, i))
            {
                if (sLine.is_delimited_sequence(i, iter->first.length()))
                    sLine.replace(i, (iter->first).length(), iter->second);
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Replaces all occurences of the
/// selected variable in the range of the
/// selected lines with the new name.
///
/// \param sOldVar const std::string&
/// \param sNewVar const std::string&
/// \param from size_t
/// \param to size_t
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::replaceLocalVars(const std::string& sOldVar, const std::string& sNewVar, size_t from, size_t to)
{
    for (size_t i = from; i < std::min(to, vCmdArray.size()); i++)
    {
        // Replace it in the flow control
        // statements
        if (vCmdArray[i].sCommand.length())
        {
            MutableStringView cmd(vCmdArray[i].sCommand);

            for (size_t j = 0; j < cmd.length(); j++)
            {
                if (cmd.match(sOldVar, j))
                {
                    if (cmd.is_delimited_sequence(j, sOldVar.length(), StringViewBase::STRING_DELIMITER)
                        && !isInQuotes(cmd, j, true))
                    {
                        cmd.replace(j, sOldVar.length(), sNewVar);
                        j += sNewVar.length() - sOldVar.length();
                    }
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether
/// the passed flow control argument is valid or
/// not.
///
/// \param sFlowControlArgument StringView
/// \param isForLoop bool
/// \return bool
///
/////////////////////////////////////////////////
bool FlowCtrl::checkFlowControlArgument(StringView sFlowControlArgument, bool isForLoop)
{
    // The argument shall be enclosed in parentheses
    if (sFlowControlArgument.find('(') == std::string::npos)
        return false;

    StringView sArgument = sFlowControlArgument.subview(sFlowControlArgument.find('('));
    sArgument = sArgument.subview(1, getMatchingParenthesis(sArgument) - 1);

    // Ensure that the argument is not empty
    if (!isNotEmptyExpression(sArgument))
        return false;

    // That's everything for a non-for loop
    if (!isForLoop)
        return true;

    // If it is a for loop, then it has to fulfill
    // another requirement: the index and its interval
    if (sArgument.find('=') == std::string::npos && sArgument.find("->") == std::string::npos)
        return false;

    // Everything seems to be OK
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether
/// the entered case definition is valid or not.
///
/// \param sCaseDefinition StringView
/// \return bool
///
/////////////////////////////////////////////////
bool FlowCtrl::checkCaseValue(StringView sCaseDefinition)
{
    // Colon operator is missing
    if (sCaseDefinition.find(':') == std::string::npos)
        return false;

    // Check, whether there's a valid value between
    // "case" and the colon operator
    if (sCaseDefinition.starts_with("case "))
    {
        // Extract the value
        StringView sValue = sCaseDefinition.subview(4);
        sValue.remove_from(sValue.find(':'));

        // Check, whether there are other characters
        // than the whitespace
        if (sValue.find_first_not_of(' ') == std::string::npos)
            return false;

        // Cut of the first expression (in the possible list)
        getNextViewedArgument(sValue);

        // Check for more than one value (only one allowed)
        if (sValue.length() && sValue.find_first_not_of(' ') != std::string::npos)
            return false;
    }
    // Everything seems to be OK
    return true;
}


/////////////////////////////////////////////////
/// \brief Read the flow control statements only
/// and extract the index variables and the flow
/// control flags.
///
/// \return std::string
///
/////////////////////////////////////////////////
string FlowCtrl::extractFlagsAndIndexVariables()
{
    std::string sVars = ";";
    int nVarArray = 0;

    for (size_t i = 0; i < vCmdArray.size(); i++)
    {
        // No flow control statement
        if (!vCmdArray[i].bFlowCtrlStatement)
            continue;

        // Extract the index variables
        if (vCmdArray[i].sCommand.starts_with("for"))
        {
            StringView sVar(vCmdArray[i].sCommand, vCmdArray[i].sCommand.find('(') + 1);

            if (sVar.find("->") != std::string::npos)
                sVar.remove_from(sVar.find("->")+2); // Has the arrow appended
            else
                sVar.remove_from(sVar.find('='));

            if (sVar.starts_with("|>"))
                sVar.trim_front(2);

            sVar.strip();

            if (sVars.find(";" + sVar + ";") == std::string::npos)
            {
                sVars += sVar + ";";
                nVarArray++;
            }

            vCmdArray[i].sCommand[vCmdArray[i].sCommand.find('(')] = ' ';
            vCmdArray[i].sCommand.pop_back();
        }

        // Extract the flow control flags
        if (vCmdArray[i].sCommand.find("end") != std::string::npos && findParameter(vCmdArray[i].sCommand, "sv"))
            bSilent = false;

        if (vCmdArray[i].sCommand.find("end") != std::string::npos && findParameter(vCmdArray[i].sCommand, "mask"))
            bMask = true;

        if (vCmdArray[i].sCommand.find("end") != std::string::npos && findParameter(vCmdArray[i].sCommand, "sp"))
            bMask = false;

        if (vCmdArray[i].sCommand.find("end") != std::string::npos && findParameter(vCmdArray[i].sCommand, "lnumctrl"))
            nLoopSafety = 1000;

        if (vCmdArray[i].sCommand.find("end") != std::string::npos && findParameter(vCmdArray[i].sCommand, "lnumctrl", '='))
        {
            _parserRef->SetExpr(getArgAtPos(vCmdArray[i].sCommand, findParameter(vCmdArray[i].sCommand, "lnumctrl", '=') + 8));
            nLoopSafety = intCast(_parserRef->Eval());

            if (nLoopSafety <= 0)
                nLoopSafety = 1000;
        }
    }

    sVarArray.resize(nVarArray);
    vVarArray.resize(nVarArray);

    return sVars;
}


/////////////////////////////////////////////////
/// \brief Go again through the whole command set
/// and fill the jump table with the
/// corresponding block ends and pre-evaluate the
/// recursive expressions. Furthermore, determine,
/// whether the loop parsing mode is reasonable.
///
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::fillJumpTableAndExpandRecursives()
{
    for (size_t i = 0; i < vCmdArray.size(); i++)
    {
        // Fill the jump table and determine, whether
        // the loop parsing mode is reasonable
        if (vCmdArray[i].bFlowCtrlStatement)
        {
            if (vCmdArray[i].sCommand.starts_with("for"))
            {
                int nForCount = 0;

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.starts_with("endfor"))
                    {
                        if (nForCount)
                            nForCount--;
                        else
                        {
                            nJumpTable[i][BLOCK_END] = j;
                            break;
                        }
                    }
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.starts_with("for"))
                        nForCount++;
                }
            }
            else if (vCmdArray[i].sCommand.starts_with("while"))
            {
                int nWhileCount = 0;

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.starts_with("endwhile"))
                    {
                        if (nWhileCount)
                            nWhileCount--;
                        else
                        {
                            nJumpTable[i][BLOCK_END] = j;
                            break;
                        }
                    }
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.starts_with("while"))
                        nWhileCount++;
                }
            }
            else if (vCmdArray[i].sCommand.find(">>if") != std::string::npos)
            {
                std::string sNth_If = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length()
                        && vCmdArray[j].sCommand.starts_with(sNth_If + ">>else")
                        && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_If + ">>elseif")
                             && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_If + ">>endif"))
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
            else if (vCmdArray[i].sCommand.find(">>elseif") != std::string::npos)
            {
                std::string sNth_If = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length()
                        && vCmdArray[j].sCommand.starts_with(sNth_If + ">>else")
                        && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_If + ">>elseif")
                             && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_If + ">>endif"))
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
            else if (vCmdArray[i].sCommand.find(">>switch") != std::string::npos)
            {
                std::string sNth_Switch = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length()
                        && vCmdArray[j].sCommand.starts_with(sNth_Switch + ">>case")
                        && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_Switch + ">>default")
                             && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_Switch + ">>endswitch"))
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }

                // For the switch case, all case values are gathered
                // as logical expression into one single expression,
                // which will be evaluated at once and the switch
                // will jump into the first non-zero case
                prepareSwitchExpression(i);
            }
            else if (vCmdArray[i].sCommand.find(">>case") != std::string::npos)
            {
                std::string sNth_Switch = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length()
                        && vCmdArray[j].sCommand.starts_with(sNth_Switch + ">>case")
                        && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_Switch + ">>default")
                             && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_Switch + ">>endswitch"))
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
            else if (vCmdArray[i].sCommand.find(">>try") != std::string::npos)
            {
                std::string sNth_Try = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length()
                        && vCmdArray[j].sCommand.starts_with(sNth_Try + ">>catch")
                        && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_Try + ">>endtry"))
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
            else if (vCmdArray[i].sCommand.find(">>catch") != std::string::npos)
            {
                std::string sNth_Try = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length()
                        && vCmdArray[j].sCommand.starts_with(sNth_Try + ">>catch")
                        && nJumpTable[i][BLOCK_MIDDLE] == NO_FLOW_COMMAND)
                        nJumpTable[i][BLOCK_MIDDLE] = j;
                    else if (vCmdArray[j].sCommand.length()
                             && vCmdArray[j].sCommand.starts_with(sNth_Try + ">>endtry"))
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }

            continue;
        }

        // Pre-evaluate all recursive expressions
        if (vCmdArray[i].sCommand.length())
        {
            vCmdArray[i].sCommand += " ";

            // Do not expand the recursive expression
            // if it is part of a matrix operation
            if (findCommand(vCmdArray[i].sCommand).sString != "matop"
                && findCommand(vCmdArray[i].sCommand).sString != "mtrxop"
                && (vCmdArray[i].sCommand.find("+=") != std::string::npos
                    || vCmdArray[i].sCommand.find("-=") != std::string::npos
                    || vCmdArray[i].sCommand.find("*=") != std::string::npos
                    || vCmdArray[i].sCommand.find("/=") != std::string::npos
                    || vCmdArray[i].sCommand.find("^=") != std::string::npos
                    || vCmdArray[i].sCommand.find("++") != std::string::npos
                    || vCmdArray[i].sCommand.find("--") != std::string::npos))
            {
                // Store the breakpoint to insert
                // it after expanding the recursive
                // espression
                bool bBreakPoint = (vCmdArray[i].sCommand.substr(vCmdArray[i].sCommand.find_first_not_of(" \t"), 2) == "|>");

                if (bBreakPoint)
                {
                    vCmdArray[i].sCommand.erase(vCmdArray[i].sCommand.find_first_not_of(" \t"), 2);
                    StripSpaces(vCmdArray[i].sCommand);
                }

                evalRecursiveExpressions(vCmdArray[i].sCommand);

                if (bBreakPoint)
                    vCmdArray[i].sCommand.insert(0, "|> ");
            }

            StripSpaces(vCmdArray[i].sCommand);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function will prepare the
/// single logical switch expression.
///
/// \param nSwitchStart int
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::prepareSwitchExpression(int nSwitchStart)
{
    // Extract the condition of the "switch"
    std::string sSwitchArgument = vCmdArray[nSwitchStart].sCommand;
    vCmdArray[nSwitchStart].sCommand.erase(sSwitchArgument.find('('));
    sSwitchArgument.erase(0, sSwitchArgument.find('(')+1);
    sSwitchArgument.erase(sSwitchArgument.rfind(')'));

    std::string sArgument = "";

    // Extract the switch level
    std::string sNth_Switch = vCmdArray[nSwitchStart].sCommand.substr(0, vCmdArray[nSwitchStart].sCommand.find(">>"));

    // Search for all cases, which belong to the current
    // switch level
    for (size_t j = nSwitchStart + 1; j < vCmdArray.size(); j++)
    {
        // Extract the value of the found case and gather
        // it in the argument list
        if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.starts_with(sNth_Switch + ">>case"))
        {
            if (sArgument.length())
                sArgument += ", ";

            sArgument += vCmdArray[j].sCommand.substr(vCmdArray[j].sCommand.find(' ', vCmdArray[j].sCommand.find(">>case"))+1);
            sArgument.erase(sArgument.rfind(':'));
        }
    }

    // If the argument list is not empty, transform it
    // into a vector and append it using the equality
    // operator to the switch condition
    if (sArgument.length())
    {
        std::string sExpr = sSwitchArgument;
        sSwitchArgument.clear();

        while (sArgument.length())
        {
            sSwitchArgument += sExpr + " == " + getNextArgument(sArgument, true) + ",";
        }

        sSwitchArgument.pop_back();
    }

    // Append the new switch condition to the "switch" command
    vCmdArray[nSwitchStart].sCommand += "(" + sSwitchArgument + ")";
}


/////////////////////////////////////////////////
/// \brief If the loop parsing mode is active,
/// ensure that only inline procedures are used
/// in this case. Otherwise turn it off again.
/// Additionally check for "to_cmd()" function,
/// which will also turn the loop parsing mode
/// off. If no function definition commands are
/// found in the command block, replace the
/// definitions with their expanded form.
///
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::checkParsingModeAndExpandDefinitions()
{
    const int INLINING_GLOBALINRETURN = 2;

    for (size_t i = 0; i < vCmdArray.size(); i++)
    {
        if (vCmdArray[i].bFlowCtrlStatement)
        {
            if (vCmdArray[i].sCommand.starts_with("for") || vCmdArray[i].sCommand.starts_with("while"))
            {
                if (!bUseLoopParsingMode)
                    bUseLoopParsingMode = true;

                break;
            }
        }
    }

    if (bUseLoopParsingMode)
    {
        // Check for inline procedures and "to_cmd()"
        for (size_t i = 0; i < vCmdArray.size(); i++)
        {
            if (vCmdArray[i].sCommand.find("to_cmd(") != std::string::npos)
                bUseLoopParsingMode = false;

            if (vCmdArray[i].sCommand.find('$') != std::string::npos)
            {
                int nInlining = 0;

                if (!(nInlining = isInline(vCmdArray[i].sCommand)))
                    bUseLoopParsingMode = false;
                else if (!vCmdArray[i].bFlowCtrlStatement && nInlining != INLINING_GLOBALINRETURN)
                {
                    std::vector<std::string> vExpandedProcedure = expandInlineProcedures(vCmdArray[i].sCommand);
                    int nLine = vCmdArray[i].nInputLine;

                    for (size_t j = 0; j < vExpandedProcedure.size(); j++)
                    {
                        //g_logger.info("Emplaced '" + vExpandedProcedure[j] + "'");
                        vCmdArray.emplace(vCmdArray.begin() + i + j, FlowCtrlCommand(vExpandedProcedure[j], nLine));
                    }

                    i += vExpandedProcedure.size();
                }
            }
        }

        bool bDefineCommands = false;

        // Search for function definition commands
        for (size_t i = 0; i < vCmdArray.size(); i++)
        {
            if (findCommand(vCmdArray[i].sCommand).sString == "define"
                || findCommand(vCmdArray[i].sCommand).sString == "taylor"
                || findCommand(vCmdArray[i].sCommand).sString == "spline"
                || findCommand(vCmdArray[i].sCommand).sString == "redefine"
                || findCommand(vCmdArray[i].sCommand).sString == "redef"
                || findCommand(vCmdArray[i].sCommand).sString == "undefine"
                || findCommand(vCmdArray[i].sCommand).sString == "undef"
                || findCommand(vCmdArray[i].sCommand).sString == "ifndefined"
                || findCommand(vCmdArray[i].sCommand).sString == "ifndef")
            {
                bDefineCommands = true;
                break;
            }
        }

        // No commands found? Then expand the defined
        // function. Otherwise deactivate the loop parsing
        // mode (temporary fix)
        if (!bDefineCommands)
        {
            for (size_t i = 0; i < vCmdArray.size(); i++)
            {
                if (!_functionRef->call(vCmdArray[i].sCommand))
                {
                    throw SyntaxError(SyntaxError::FUNCTION_ERROR, vCmdArray[i].sCommand, SyntaxError::invalid_position);
                }

                StripSpaces(vCmdArray[i].sCommand);
            }

            bFunctionsReplaced = true;
        }
        else
            bUseLoopParsingMode = false;
    }

    if (bLockedPauseMode && bUseLoopParsingMode)
        bUseLoopParsingMode = false;
}


/////////////////////////////////////////////////
/// \brief This method prepares the local
/// variables including their names and replaces
/// them in the command lines in the current
/// command block.
///
/// \param sVars std::string&
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::prepareLocalVarsAndReplace(std::string& sVars)
{
    sVars = sVars.substr(1, sVars.length() - 1);

    // If a loop variable was defined before the
    // current loop, this one is used. All others
    // are create locally and therefore get
    // special names
    for (size_t i = 0; i < sVarArray.size(); i++)
    {
        sVarArray[i] = sVars.substr(0, sVars.find(';'));
        bool isRangeBased = sVarArray[i].find("->") != std::string::npos;

        if (sVars.find(';') != std::string::npos)
            sVars = sVars.substr(sVars.find(';') + 1);

        StripSpaces(sVarArray[i]);

        // Is it already defined?
        if (sVarArray[i].starts_with("_~") && getPointerToVariable(sVarArray[i], *_parserRef))
            vVars[sVarArray[i]] = getPointerToVariable(sVarArray[i], *_parserRef);
        else
        {
            // Create a local variable otherwise
            if (!isRangeBased)
            {
                mVarMap[sVarArray[i]] = "_~LOOP_" + sVarArray[i] + "_" + toString(nthRecursion);
                sVarArray[i] = mVarMap[sVarArray[i]];
            }
        }

        if (!isRangeBased)
            _parserRef->DefineVar(sVarArray[i], &vVarArray[i]);
    }

    _parserRef->mVarMapPntr = &mVarMap;

    // Replace the local index variables in the
    // whole command set for this flow control
    // statement
    for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
    {
        replaceLocalVars(iter->first, iter->second);
    }
}


/////////////////////////////////////////////////
/// \brief Updates the test statistics with the
/// total test statistics.
///
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::updateTestStats()
{
    if (sTestClusterName.length())
    {
        NumeRe::Cluster& testcluster = NumeReKernel::getInstance()->getMemoryManager().getCluster(sTestClusterName);
        AssertionStats total = _assertionHandler.getStats();

        testcluster.setDouble(1, double(total.nCheckedAssertions - baseline.nCheckedAssertions));
        testcluster.setDouble(3, double(total.nCheckedAssertions - baseline.nCheckedAssertions - (total.nFailedAssertions - baseline.nFailedAssertions)));
        testcluster.setDouble(5, double(total.nFailedAssertions - baseline.nFailedAssertions));
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// current line number as enumerated during
/// passing the commands via
/// "addToControlFlowBlock()".
///
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::getCurrentLineNumber() const
{
    if (vCmdArray.size() > (size_t)nCurrentCommand)
        return vCmdArray[nCurrentCommand].nInputLine;

    return 0;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// current command line, which will or has been
/// evaluated in the current line.
///
/// \return std::string
///
/////////////////////////////////////////////////
string FlowCtrl::getCurrentCommand() const
{
    if (!vCmdArray.size())
        return "";

    return vCmdArray[nCurrentCommand].sCommand;
}


/////////////////////////////////////////////////
/// \brief This static member function returns
/// whether the passed command is a flow control
/// statement.
///
/// \param sCmd StringView
/// \return bool
///
/////////////////////////////////////////////////
bool FlowCtrl::isFlowCtrlStatement(StringView sCmd)
{
    return sCmd == "if" || sCmd == "for" || sCmd == "while" || sCmd == "switch" || sCmd == "try";
}


/////////////////////////////////////////////////
/// \brief This static member function returns
/// whether the passed command is any of the
/// known flow control statements (not only their
/// headers).
///
/// \param sCmd StringView
/// \return bool
///
/////////////////////////////////////////////////
bool FlowCtrl::isAnyFlowCtrlStatement(StringView sCmd)
{
    return sCmd == "for"
        || sCmd == "endfor"
        || sCmd == "if"
        || sCmd == "endif"
        || sCmd == "else"
        || sCmd == "elseif"
        || sCmd == "switch"
        || sCmd == "endswitch"
        || sCmd == "case"
        || sCmd == "default"
        || sCmd == "try"
        || sCmd == "catch"
        || sCmd == "endtry"
        || sCmd == "while"
        || sCmd == "endwhile";
}


// VIRTUAL FUNCTION IMPLEMENTATIONS
/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param sLine std::string&
/// \param _parser Parser&
/// \param _functions Define&
/// \param _data Datafile&
/// \param _out Output&
/// \param _pData PlotData&
/// \param _script Script&
/// \param _option Settings&
/// \param nth_command int
/// \return ProcedureInterfaceRetVal
///
/////////////////////////////////////////////////
FlowCtrl::ProcedureInterfaceRetVal FlowCtrl::procedureInterface(std::string& sLine, Parser& _parser, FunctionDefinitionManager& _functions, MemoryManager& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, int nth_command)
{
    return FlowCtrl::INTERFACE_NONE;
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param sLine StringView
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::procedureCmdInterface(StringView sLine)
{
    return 1;
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param sProc const std::string&
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::isInline(const std::string& sProc)
{
    return true;
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param _parser Parser&
/// \param _option Settings&
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::evalDebuggerBreakPoint(Parser& _parser, Settings& _option)
{
    return 0;
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::getErrorInformationForDebugger()
{
    return 0;
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param sLine std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
vector<std::string> FlowCtrl::expandInlineProcedures(std::string& sLine)
{
    return std::vector<std::string>();
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param e_ptr exception_ptr
/// \param bSupressAnswer_back bool
/// \param nLine int
/// \param cleanUp bool
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::catchExceptionForTest(exception_ptr e_ptr, bool bSupressAnswer_back, int nLine, bool cleanUp)
{
    return 0;
}

