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
#include "../../kernel.hpp"

// Definition of special return values
#define FLOWCTRL_ERROR -1
#define FLOWCTRL_RETURN -2
#define FLOWCTRL_BREAK -3
#define FLOWCTRL_CONTINUE -4
#define FLOWCTRL_NO_CMD -5
#define FLOWCTRL_OK 1

// Definition of standard values of the jump table
#define NO_FLOW_COMMAND -1
#define BLOCK_END 0
#define ELSE_START 1
#define PROCEDURE_INTERFACE 2
extern value_type vAns;

// Prototype of the toString function,
// avoiding inclusion of "tools"
string toString(double, int);


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
FlowCtrl::FlowCtrl()
{
    vVarArray = nullptr;
    sVarArray = nullptr;
    nJumpTable = nullptr;
    nCalcType = nullptr;
    dVarAdress = nullptr;

    _parserRef = nullptr;
    _dataRef = nullptr;
    _outRef = nullptr;
    _optionRef = nullptr;
    _functionRef = nullptr;
    _pDataRef = nullptr;
    _scriptRef = nullptr;

    nJumpTableLength = 0;
    nLoopSavety = -1;
    nReturnType = 1;
    nLoop = 0;
    nIf = 0;
    nSwitch = 0;
    nWhile = 0;
    nDefaultLength = 10;
    nVarArray = 0;
    nCurrentCommand = 0;
    nDebuggerCode = 0;
    sLoopNames = "";
    sLoopPlotCompose = "";
    bSilent = true;
    bMask = false;
    bPrintedStatus = false;
    bBreakSignal = false;
    bContinueSignal = false;
    bReturnSignal = false;
    sVarName = "";
    bUseLoopParsingMode = false;
    bLockedPauseMode = false;
    bFunctionsReplaced = false;
    nthRecursion = 0;
    bLoopSupressAnswer = false;
    bEvaluatingFlowControlStatements = false;
}


/////////////////////////////////////////////////
/// \brief Generalized constructor. Delegates to
/// the default constructor.
///
/// \param _nDefaultLength int
///
/////////////////////////////////////////////////
FlowCtrl::FlowCtrl(int _nDefaultLength) : FlowCtrl()
{
    nDefaultLength = _nDefaultLength;
}


/////////////////////////////////////////////////
/// \brief Destructor. Cleanes the memory, if
/// necessary.
/////////////////////////////////////////////////
FlowCtrl::~FlowCtrl()
{
    // Clean the variables array
    if (vVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
        {
            delete[] vVarArray[i];
        }

        delete[] vVarArray;
        vVarArray = nullptr;
    }
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
    string sForHead = vCmdArray[nth_Cmd].sCommand.substr(vCmdArray[nth_Cmd].sCommand.find('=') + 1);
    string sVar = vCmdArray[nth_Cmd].sCommand.substr(vCmdArray[nth_Cmd].sCommand.find(' ') + 1);
    string sLine = "";
    bPrintedStatus = false;
    sVar = sVar.substr(0, sVar.find('='));
    StripSpaces(sVar);
    int nNum = 0;
    value_type* v = 0;

    // Get the variable address of the loop
    // index
    for (int i = 0; i < nVarArray; i++)
    {
        if (sVarArray[i] == sVar)
        {
            nVarAdress = i;
            break;
        }
    }

    // Evaluate the header of the for loop
    v = evalHeader(nNum, sForHead, true, nth_Cmd);

    // Store the left and right boundary of the
    // loop index
    for (int i = 0; i < 2; i++)
    {
        vVarArray[nVarAdress][i + 1] = (int)v[i];
    }

    // Depending on the order of the boundaries, we
    // have to consider the incrementation variable
    if (vVarArray[nVarAdress][2] < vVarArray[nVarAdress][1])
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
    for (int __i = (int)vVarArray[nVarAdress][1]; (nInc)*__i <= nInc * (int)vVarArray[nVarAdress][2]; __i += nInc)
    {
        vVarArray[nVarAdress][0] = __i;

        // Ensure that the loop is aborted, if the
        // maximal number of repetitions has been
        // performed
        if (nLoopSavety > 0)
        {
            if (nLoopCount >= nLoopSavety)
                return FLOWCTRL_ERROR;

            nLoopCount++;
        }

        // This for loop handles the contained commands
        for (int __j = nth_Cmd+1; __j < nJumpTable[nth_Cmd][BLOCK_END]; __j++)
        {
            nCurrentCommand = __j;

            if (__j != nth_Cmd)
            {
                // If this is not the first line of the command block
                // try to find control flow statements in the first column
                if (vCmdArray[__j].bFlowCtrlStatement)
                {
                    // Evaluate the flow control commands
                    int nReturn = evalLoopFlowCommands(__j, nth_loop);

                    // Handle the return value
                    if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                        return nReturn;
                    else if (nReturn == FLOWCTRL_BREAK)
                    {
                        bBreakSignal = false;
                        return nJumpTable[nth_Cmd][BLOCK_END];
                    }
                    else if (nReturn == FLOWCTRL_CONTINUE)
                    {
                        bContinueSignal = false;
                        break;
                    }
                    else if (nReturn != FLOWCTRL_NO_CMD)
                        __j = nReturn;

                    continue;
                }
            }

            // Handle the "continue" and "break" flow
            // control statements
            if (!nCalcType[__j])
            {
                string sCommand = findCommand(vCmdArray[__j].sCommand).sString;

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
            }
            else if (nCalcType[__j] & CALCTYPE_CONTINUECMD)
                break;
            else if (nCalcType[__j] & CALCTYPE_BREAKCMD)
                return nJumpTable[nth_Cmd][BLOCK_END];

            // Increment the parser index, if the loop parsing
            // mode was activated
            if (bUseLoopParsingMode)
                _parserRef->SetIndex(__j);

            try
            {
                // Evaluate the command line with the calc function
                if (calc(vCmdArray[__j].sCommand, __j, "FOR") == FLOWCTRL_ERROR)
                {
                    if (_optionRef->getUseDebugger())
                    {
                        NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                        getErrorInformationForDebugger();
                    }

                    return FLOWCTRL_ERROR;
                }

                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                if (_optionRef->getUseDebugger())
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                    getErrorInformationForDebugger();
                }

                NumeReKernel::getInstance()->getDebugger().showError(current_exception());

                throw;
            }
        }

        // The variable value might have been changed
        // snychronize the index
        __i = (int)vVarArray[nVarAdress][0];

        // Print the status to the terminal, if it is required
        if (!nth_loop && !bMask && bSilent)
        {
            if (abs(int(vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1])) < 99999
                    && abs((int)((vVarArray[nVarAdress][0] - vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1]) * 20))
                    > abs((int)((vVarArray[nVarAdress][0] - 1 - vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1]) * 20)))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... " + toString(abs((int)((vVarArray[nVarAdress][0] - vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1]) * 20)) * 5) + " %");
                bPrintedStatus = true;
            }
            else if (abs(int(vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1]) >= 99999)
                     && abs((int)((vVarArray[nVarAdress][0] - vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1]) * 100))
                     > abs((int)((vVarArray[nVarAdress][0] - 1 - vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1]) * 100)))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... " + toString(abs((int)((vVarArray[nVarAdress][0] - vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2] - vVarArray[nVarAdress][1]) * 100))) + " %");
                bPrintedStatus = true;
            }
        }
    }

    return nJumpTable[nth_Cmd][BLOCK_END];
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
    string sWhile_Condition = vCmdArray[nth_Cmd].sCommand.substr(vCmdArray[nth_Cmd].sCommand.find('(') + 1, vCmdArray[nth_Cmd].sCommand.rfind(')') - vCmdArray[nth_Cmd].sCommand.find('(') - 1);
    string sWhile_Condition_Back = sWhile_Condition;
    string sLine = "";
    bPrintedStatus = false;
    int nLoopCount = 0;
    value_type* v = 0;
    int nNum = 0;

    // Show a working indicator, if it's necessary
    if (bSilent && !bMask && !nth_loop)
    {
        NumeReKernel::printPreFmt("|WHL> " + toSystemCodePage(_lang.get("COMMON_EVALUATING")) + " ... ");
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
        v = evalHeader(nNum, sWhile_Condition, false, nth_Cmd);

        // Ensure that the loop is aborted, if the
        // maximal number of repetitions has been
        // performed
        if (nLoopSavety > 0)
        {
            if (nLoopCount >= nLoopSavety)
                return FLOWCTRL_ERROR;
            nLoopCount++;
        }

        // Check, whether the header condition is true.
        // NaN and INF are no "true" values. Return the
        // end of the current block otherwise
        if (!(bool)v[0] || isnan(v[0]) || isinf(v[0]))
        {
            return nJumpTable[nth_Cmd][BLOCK_END];
        }

        // This for loop handles the contained commands
        for (int __j = nth_Cmd+1; __j < nJumpTable[nth_Cmd][BLOCK_END]; __j++)
        {
            nCurrentCommand = __j;

            if (__j != nth_Cmd)
            {
                // If this is not the first line of the command block
                // try to find control flow statements in the first column
                if (vCmdArray[__j].bFlowCtrlStatement)
                {
                    // Evaluate the flow control commands
                    int nReturn = evalLoopFlowCommands(__j, nth_loop);

                    // Handle the return value
                    if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                        return nReturn;
                    else if (nReturn == FLOWCTRL_BREAK)
                    {
                        bBreakSignal = false;
                        return nJumpTable[nth_Cmd][BLOCK_END];
                    }
                    else if (nReturn == FLOWCTRL_CONTINUE)
                    {
                        bContinueSignal = false;
                        break;
                    }
                    else if (nReturn != FLOWCTRL_NO_CMD)
                        __j = nReturn;

                    continue;
                }
            }

            // Handle the "continue" and "break" flow
            // control statements
            if (!nCalcType[__j])
            {
                string sCommand = findCommand(vCmdArray[__j].sCommand).sString;

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
            }
            else if (nCalcType[__j] & CALCTYPE_CONTINUECMD)
                break;
            else if (nCalcType[__j] & CALCTYPE_BREAKCMD)
                return nJumpTable[nth_Cmd][BLOCK_END];

            // Increment the parser index, if the loop parsing
            // mode was activated
            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parserRef->SetIndex(__j);

            try
            {
                // Evaluate the command line with the calc function
                if (calc(vCmdArray[__j].sCommand, __j, "WHL") == FLOWCTRL_ERROR)
                {
                    if (_optionRef->getUseDebugger())
                    {
                        NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                        getErrorInformationForDebugger();
                    }

                    return FLOWCTRL_ERROR;
                }

                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                if (_optionRef->getUseDebugger())
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__j].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                    getErrorInformationForDebugger();
                }

                NumeReKernel::getInstance()->getDebugger().showError(current_exception());
                throw;
            }
        }

        // If the while condition has changed during the evaluation
        // re-use the original condition to ensure that the result of
        // the condition is true
        if (sWhile_Condition != sWhile_Condition_Back || _dataRef->containsTablesOrClusters(sWhile_Condition_Back) || sWhile_Condition_Back.find("data(") != string::npos)
        {
            sWhile_Condition = sWhile_Condition_Back;
            //v = evalHeader(nNum, sWhile_Condition, false, nth_Cmd);
        }
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
    string sIf_Condition;
    int nElse = nJumpTable[nth_Cmd][ELSE_START]; // Position of next else/elseif
    int nEndif = nJumpTable[nth_Cmd][BLOCK_END];
    string sLine = "";
    bPrintedStatus = false;
    value_type* v;
    int nNum = 0;

    // As long as there are "if" or further "elseif"s
    // statements are found, otherwise jump to the else
    // statement
    while (vCmdArray[nth_Cmd].sCommand.find(">>if") != string::npos || vCmdArray[nth_Cmd].sCommand.find(">>elseif") != string::npos)
    {
        sIf_Condition = vCmdArray[nth_Cmd].sCommand.substr(vCmdArray[nth_Cmd].sCommand.find('(') + 1, vCmdArray[nth_Cmd].sCommand.rfind(')') - vCmdArray[nth_Cmd].sCommand.find('(') - 1);
        nElse = nJumpTable[nth_Cmd][ELSE_START];
        nEndif = nJumpTable[nth_Cmd][BLOCK_END];
        sLine = "";
        bPrintedStatus = false;

        // Evaluate the header of the current elseif case
        v = evalHeader(nNum, sIf_Condition, false, nth_Cmd);

        // If the condition is true, enter the if-case
        if ((bool)v[0] && !isnan(v[0]) && !isinf(v[0]))
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

                if (__i != nth_Cmd)
                {
                    // If this is not the first line of the command block
                    // try to find control flow statements in the first column
                    if (vCmdArray[__i].bFlowCtrlStatement)
                    {
                        // Evaluate the flow control commands
                        int nReturn = evalForkFlowCommands(__i, nth_loop);

                        // Handle the return value
                        if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                            return nReturn;
                        else if (nReturn == FLOWCTRL_BREAK || nReturn == FLOWCTRL_CONTINUE)
                        {
                            return nEndif;
                        }
                        else if (nReturn != FLOWCTRL_NO_CMD)
                            __i = nReturn;

                        continue;
                    }
                }

                // Handle the "continue" and "break" flow
                // control statements
                if (!nCalcType[__i])
                {
                    string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

                    // "continue" and "break" are both exiting the current
                    // condition, but they are yielding different signals
                    if (sCommand == "continue")
                    {
                        nCalcType[__i] = CALCTYPE_CONTINUECMD;
                        bContinueSignal = true;
                        return nEndif;
                    }

                    if (sCommand == "break")
                    {
                        nCalcType[__i] = CALCTYPE_BREAKCMD;
                        bBreakSignal = true;
                        return nEndif;
                    }
                }
                else if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
                {
                    bContinueSignal = true;
                    return nEndif;
                }
                else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
                {
                    bBreakSignal = true;
                    return nEndif;
                }

                // Increment the parser index, if the loop parsing
                // mode was activated
                if (bUseLoopParsingMode && !bLockedPauseMode)
                    _parserRef->SetIndex(__i);

                try
                {
                    // Evaluate the command line with the calc function
                    if (calc(vCmdArray[__i].sCommand, __i, "IF") == FLOWCTRL_ERROR)
                    {
                        if (_optionRef->getUseDebugger())
                        {
                            NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                            getErrorInformationForDebugger();
                        }

                        return FLOWCTRL_ERROR;
                    }

                    if (bReturnSignal)
                        return FLOWCTRL_RETURN;
                }
                catch (...)
                {
                    if (_optionRef->getUseDebugger())
                    {
                        NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                        getErrorInformationForDebugger();
                    }

                    NumeReKernel::getInstance()->getDebugger().showError(current_exception());
                    throw;
                }
            }

            return nEndif;
        }
        else
        {
            // Try to find the next elseif/else
            // statement of the current block
            if (nElse == -1)
                return nEndif;
            else
                nth_Cmd = nElse;
        }
    }

    // This is the else-case, if it is available
    for (int __i = nth_Cmd+1; __i < (int)vCmdArray.size(); __i++)
    {
        nCurrentCommand = __i;

        // If we're reaching the line with the
        // corresponding endif statement, we'll
        // return with this line as index
        if (__i >= nEndif)
            return nEndif;

        if (__i != nth_Cmd)
        {
            // If this is not the first line of the command block
            // try to find control flow statements in the first column
            if (vCmdArray[__i].bFlowCtrlStatement)
            {
                // Evaluate the flow control commands
                int nReturn = evalForkFlowCommands(__i, nth_loop);

                // Handle the return value
                if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                    return nReturn;
                else if (nReturn == FLOWCTRL_BREAK || nReturn == FLOWCTRL_CONTINUE)
                {
                    return nEndif;
                }
                else if (nReturn != FLOWCTRL_NO_CMD)
                    __i = nReturn;

                continue;
            }
        }

        if (!nCalcType[__i])
        {
            string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

            // "continue" and "break" are both exiting the current
            // condition, but they are yielding different signals
            if (sCommand == "continue")
            {
                nCalcType[__i] = CALCTYPE_CONTINUECMD;
                bContinueSignal = true;
                return nEndif;
            }

            if (sCommand == "break")
            {
                nCalcType[__i] = CALCTYPE_BREAKCMD;
                bBreakSignal = true;
                return nEndif;
            }
        }
        else if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
        {
            bContinueSignal = true;
            return nEndif;
        }
        else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
        {
            bBreakSignal = true;
            return nEndif;
        }

        if (bUseLoopParsingMode && !bLockedPauseMode)
            _parserRef->SetIndex(__i);

        try
        {
            // Evaluate the command line with the calc function
            if (calc(vCmdArray[__i].sCommand, __i, "IF") == FLOWCTRL_ERROR)
            {
                if (_optionRef->getUseDebugger())
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                    getErrorInformationForDebugger();
                }

                return FLOWCTRL_ERROR;
            }

            if (bReturnSignal)
                return FLOWCTRL_RETURN;
        }
        catch (...)
        {
            if (_optionRef->getUseDebugger())
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                getErrorInformationForDebugger();
            }

            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            throw;
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
    string sSwitch_Condition = vCmdArray[nth_Cmd].sCommand.substr(vCmdArray[nth_Cmd].sCommand.find('(') + 1, vCmdArray[nth_Cmd].sCommand.rfind(')') - vCmdArray[nth_Cmd].sCommand.find('(') - 1);
    int nNextCase = nJumpTable[nth_Cmd][ELSE_START]; // Position of next case/default
    int nSwitchEnd = nJumpTable[nth_Cmd][BLOCK_END];
    string sLine = "";
    bPrintedStatus = false;
    value_type* v;
    int nNum = 0;

    // Evaluate the header of the current switch statement and its cases
    v = evalHeader(nNum, sSwitch_Condition, false, nth_Cmd);

    // Search for the correct first(!) case
    for (int i = 0; i < nNum; i++)
    {
        if (!v[i])
            nNextCase = nJumpTable[nNextCase][ELSE_START];
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

        if (__i != nth_Cmd)
        {
            // If this is not the first line of the command block
            // try to find control flow statements in the first column
            if (vCmdArray[__i].bFlowCtrlStatement)
            {
                // Evaluate the flow control commands
                int nReturn = evalForkFlowCommands(__i, nth_loop);

                // Handle the return value
                if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                    return nReturn;
                else if (nReturn == FLOWCTRL_BREAK || nReturn == FLOWCTRL_CONTINUE)
                {
                    // We don't propagate the break signal
                    bBreakSignal = false;
                    return nSwitchEnd;
                }
                else if (nReturn != FLOWCTRL_NO_CMD)
                    __i = nReturn;

                continue;
            }
        }

        // Handle the "continue" and "break" flow
        // control statements
        if (!nCalcType[__i])
        {
            string sCommand = findCommand(vCmdArray[__i].sCommand).sString;

            // "continue" and "break" are both exiting the current
            // condition, but they are yielding different signals
            if (sCommand == "continue")
            {
                nCalcType[__i] = CALCTYPE_CONTINUECMD;
                bContinueSignal = true;
                return nSwitchEnd;
            }

            if (sCommand == "break")
            {
                // We don't propagate the break signal in this case
                nCalcType[__i] = CALCTYPE_BREAKCMD;
                return nSwitchEnd;
            }
        }
        else if (nCalcType[__i] & CALCTYPE_CONTINUECMD)
        {
            bContinueSignal = true;
            return nSwitchEnd;
        }
        else if (nCalcType[__i] & CALCTYPE_BREAKCMD)
        {
            // We don't propagate the break signal in this case
            return nSwitchEnd;
        }

        // Increment the parser index, if the loop parsing
        // mode was activated
        if (bUseLoopParsingMode && !bLockedPauseMode)
            _parserRef->SetIndex(__i);

        try
        {
            // Evaluate the command line with the calc function
            if (calc(vCmdArray[__i].sCommand, __i, "SWCH") == FLOWCTRL_ERROR)
            {
                if (_optionRef->getUseDebugger())
                {
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                    getErrorInformationForDebugger();
                }

                return FLOWCTRL_ERROR;
            }

            if (bReturnSignal)
                return FLOWCTRL_RETURN;
        }
        catch (...)
        {
            if (_optionRef->getUseDebugger())
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(vCmdArray[__i].sCommand, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                getErrorInformationForDebugger();
            }

            NumeReKernel::getInstance()->getDebugger().showError(current_exception());
            throw;
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
/// \param sHeadExpression string&
/// \param bIsForHead bool
/// \param nth_Cmd int
/// \return value_type*
///
/////////////////////////////////////////////////
value_type* FlowCtrl::evalHeader(int& nNum, string& sHeadExpression, bool bIsForHead, int nth_Cmd)
{
    value_type* v = nullptr;
    string sCache = "";

    // Replace the function definitions, if not already done
    if (!bFunctionsReplaced)
    {
        if (!_functionRef->call(sHeadExpression))
        {
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sHeadExpression, SyntaxError::invalid_position);
        }
    }

    // Catch and evaluate all data and cache calls
    if ((sHeadExpression.find("data(") != string::npos || _dataRef->containsTablesOrClusters(sHeadExpression)) && !NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();

        // Handle calls to "data()"
        if (sHeadExpression.find("data(") != string::npos && !isInQuotes(sHeadExpression, sHeadExpression.find("data(")))
            replaceDataEntities(sHeadExpression, "data(", *_dataRef, *_parserRef, *_optionRef, true);

        // Handle calls to an arbitrary "CACHE()"
        if (_dataRef->containsTablesOrClusters(sHeadExpression))
        {
            for (auto iter = _dataRef->mCachesMap.begin(); iter != _dataRef->mCachesMap.end(); ++iter)
            {
                if (sHeadExpression.find(iter->first + "(") != string::npos && !isInQuotes(sHeadExpression, sHeadExpression.find(iter->first + "(")))
                {
                    replaceDataEntities(sHeadExpression, iter->first + "(", *_dataRef, *_parserRef, *_optionRef, true);
                }
            }

            for (auto iter = _dataRef->getClusterMap().begin(); iter != _dataRef->getClusterMap().end(); ++iter)
            {
                if (sHeadExpression.find(iter->first + "{") != string::npos && !isInQuotes(sHeadExpression, sHeadExpression.find(iter->first + "{")))
                {
                    replaceDataEntities(sHeadExpression, iter->first + "{", *_dataRef, *_parserRef, *_optionRef, true);
                }
            }

        }

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }

    // Call procedures, if necessary
    if (sHeadExpression.find("$") != string::npos)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }

        // Call the procedure interface function
        int nReturn = procedureInterface(sHeadExpression, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nLoop + nWhile + nIf, nth_Cmd);

        // Handle the return value
        if (nReturn == -1)
            throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sHeadExpression, SyntaxError::invalid_position);
        else if (nReturn == -2)
            sHeadExpression = "false";

        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode(false);
            _parserRef->LockPause(false);
        }
    }

    // Evaluate strings
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();

        // Call the string parser
        auto retVal = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sHeadExpression, sCache, true);

        // Evaluate the return value
        if (retVal == NumeRe::StringParser::STRING_SUCCESS)
        {
            StripSpaces(sHeadExpression);

            if (bIsForHead && NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
            {
                if (_optionRef->getUseDebugger())
                    NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(sHeadExpression, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);

                NumeReKernel::getInstance()->getDebugger().throwException(SyntaxError(SyntaxError::CANNOT_EVAL_FOR, sHeadExpression, SyntaxError::invalid_position));
            }
            else if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sHeadExpression))
            {
                StripSpaces(sHeadExpression);

                if (sHeadExpression != "\"\"" && sHeadExpression.length() > 2)
                    sHeadExpression = "true";
                else
                    sHeadExpression = "false";
            }
        }

        // It's possible that the user might have done
        // something weird with string operations transformed
        // into a regular expression. Replace the local
        // variables here
        replaceLocalVars(sHeadExpression);

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }

    // Update the parser index, if the loop parsing
    // mode was activated
    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parserRef->SetIndex(nth_Cmd);

    // Evalute the already prepared equation
    if (bUseLoopParsingMode && !bLockedPauseMode && _parserRef->IsValidByteCode() && _parserRef->IsAlreadyParsed(sHeadExpression))
    {
        v = _parserRef->Eval(nNum);
    }
    else
    {
        _parserRef->SetExpr(sHeadExpression);
        v = _parserRef->Eval(nNum);
    }

    // Return the evaluation result
    return v;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// evaluation of flow control statements from
/// the viewpoint of a loop control flow.
///
/// \param __j int
/// \param nth_loop int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::evalLoopFlowCommands(int __j, int nth_loop)
{
    // Do nothing, if here's no flow command
    if (nJumpTable[__j][BLOCK_END] == NO_FLOW_COMMAND)
        return FLOWCTRL_NO_CMD;

    // Evaluate the possible flow control statements
    if (vCmdArray[__j].sCommand.substr(0, 3) == "for")
    {
        return for_loop(__j, nth_loop + 1);
    }
    else if (vCmdArray[__j].sCommand.substr(0, 5) == "while")
    {
        return while_loop(__j, nth_loop + 1);
    }
    else if (vCmdArray[__j].sCommand.find(">>if") != string::npos)
    {
        __j = if_fork(__j, nth_loop + 1);

        if (__j == FLOWCTRL_ERROR || __j == FLOWCTRL_RETURN || bReturnSignal)
            return __j;

        if (bContinueSignal)
        {
            return FLOWCTRL_CONTINUE;
        }

        if (bBreakSignal)
        {
            return FLOWCTRL_BREAK;
        }

        return __j;
    }
    else if (vCmdArray[__j].sCommand.find(">>switch") != string::npos)
    {
        __j = switch_fork(__j, nth_loop + 1);

        if (__j == FLOWCTRL_ERROR || __j == FLOWCTRL_RETURN || bReturnSignal)
            return __j;

        if (bContinueSignal)
        {
            return FLOWCTRL_CONTINUE;
        }

        return __j;
    }

    return FLOWCTRL_NO_CMD;
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
    if (nJumpTable[__i][BLOCK_END] == NO_FLOW_COMMAND)
        return FLOWCTRL_NO_CMD;

    // Evaluate the possible flow control statements.
    // In this case, we have to consider the possiblity
    // that we need to display a status
    if (vCmdArray[__i].sCommand.substr(0, 3) == "for")
    {
        if (nth_loop <= -1)
        {
            bPrintedStatus = false;
            __i = for_loop(__i);

            if (!bReturnSignal && !bMask && __i != -1)
            {
                if (bSilent)
                    NumeReKernel::printPreFmt("\r|FOR> " + toSystemCodePage(_lang.get("COMMON_EVALUATING")) + " ... 100 %: " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
                else
                    NumeReKernel::printPreFmt("|FOR> " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
            }

            return __i;
        }
        else
            return for_loop(__i, nth_loop + 1);
    }
    else if (vCmdArray[__i].sCommand.substr(0, 5) == "while")
    {
        if (nth_loop <= -1)
        {
            bPrintedStatus = false;
            __i = while_loop(__i);

            if (!bReturnSignal && !bMask && __i != -1)
            {
                if (bSilent)
                    NumeReKernel::printPreFmt("\r|WHL> " + toSystemCodePage(_lang.get("COMMON_EVALUATING")) + " ...: " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
                else
                    NumeReKernel::printPreFmt("|WHL> " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
            }

            return __i;
        }
        else
            return while_loop(__i, nth_loop + 1);
    }
    else if (vCmdArray[__i].sCommand.find(">>if") != string::npos)
    {
        __i = if_fork(__i, nth_loop + 1);

        if (__i == FLOWCTRL_ERROR || __i == FLOWCTRL_RETURN || bReturnSignal)
            return __i;

        if (bContinueSignal)
        {
            return FLOWCTRL_CONTINUE;
        }

        if (bBreakSignal)
        {
            return FLOWCTRL_BREAK;
        }

        return __i;
    }
    else if (vCmdArray[__i].sCommand.find(">>switch") != string::npos)
    {
        __i = switch_fork(__i, nth_loop + 1);

        if (__i == FLOWCTRL_ERROR || __i == FLOWCTRL_RETURN || bReturnSignal)
            return __i;

        if (bContinueSignal)
        {
            return FLOWCTRL_CONTINUE;
        }

        return __i;
    }

    return FLOWCTRL_NO_CMD;
}


/////////////////////////////////////////////////
/// \brief This member function is used to set a
/// command line from the outside into the flow
/// control statement class. The internal flow
/// control command buffer grows as needed.
///
/// \param __sCmd string&
/// \param nCurrentLine int
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::setCommand(string& __sCmd, int nCurrentLine)
{
    bool bDebuggingBreakPoint = (__sCmd.substr(__sCmd.find_first_not_of(' '), 2) == "|>");
    string sAppendedExpression = "";

    // Remove the breakpoint syntax
    if (bDebuggingBreakPoint)
    {
        __sCmd.erase(__sCmd.find_first_not_of(' '), 2);
        StripSpaces(__sCmd);
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
        NumeReKernel::print(toSystemCodePage(_lang.get("LOOP_SETCOMMAND_ABORT")));
        return;
    }

    // Remove unnecessary whitespaces
    StripSpaces(__sCmd);

    // Find the command of the current command line
    // if there's any
    string command =  findCommand(__sCmd).sString;

    if (command == "for"
        || command == "endfor"
        || command == "if"
        || command == "endif"
        || command == "else"
        || command == "elseif"
        || command == "switch"
        || command == "endswitch"
        || command == "case"
        || command == "default"
        || command == "while"
        || command == "endwhile")
    {
        // Ensure that the parentheses are
        // matching each other
        if (!validateParenthesisNumber(__sCmd))
        {
            reset();
            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, __sCmd, __sCmd.find('('));
        }

        if (command == "for")
        {
            sLoopNames += ";FOR";
            nLoop++;

            // Check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, true))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_FOR, __sCmd, SyntaxError::invalid_position);
            }

            // Replace the colon operator with a comma, which is faster
            // because it can be parsed internally
            unsigned int nPos = __sCmd.find(':');

            if (__sCmd.find('(', __sCmd.find('(') + 1) != string::npos && __sCmd.find('(', __sCmd.find('(') + 1) < nPos)
            {
                nPos = getMatchingParenthesis(__sCmd.substr(__sCmd.find('(', __sCmd.find('(') + 1)));
                nPos = __sCmd.find(':', nPos);
            }

            if (nPos == string::npos)
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_FOR, __sCmd, SyntaxError::invalid_position);
            }

            __sCmd.replace(nPos, 1, ",");
        }
        else if (command == "while")
        {
            sLoopNames += ";WHL";
            nWhile++;

            // check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, false))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_WHILE, __sCmd, SyntaxError::invalid_position);
            }
        }
        else if (command == "if")
        {
            sLoopNames += ";IF";
            nIf++;

            // check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, false))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_IF, __sCmd, SyntaxError::invalid_position);
            }

            __sCmd = toString(nIf) + ">>" + __sCmd;
        }
        else if (command == "else" || command == "elseif")
        {
            if (nIf && (getCurrentBlock() == "IF" || getCurrentBlock() == "ELIF"))
            {
                if (command == "elseif")
                {
                    sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';')) + ";ELIF";

                    // check the flow control argument for completeness
                    if (!checkFlowControlArgument(__sCmd, false))
                    {
                        reset();
                        throw SyntaxError(SyntaxError::CANNOT_EVAL_IF, __sCmd, SyntaxError::invalid_position);
                    }
                }
                else
                {
                    sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';')) + ";ELSE";
                }

                __sCmd = toString(nIf) + ">>" + __sCmd;
            }
            else
                return;
        }
        else if (command == "endif")
        {
            if (nIf && (getCurrentBlock() == "IF" || getCurrentBlock() == "ELIF" || getCurrentBlock() == "ELSE"))
            {
                sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';'));
                __sCmd = toString(nIf) + ">>" + __sCmd;
                nIf--;
            }
            else
                return;
        }
        else if (command == "switch")
        {
            sLoopNames += ";SWCH";
            nSwitch++;

            // check the flow control argument for completeness
            if (!checkFlowControlArgument(__sCmd, false))
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_SWITCH, __sCmd, SyntaxError::invalid_position);
            }

            __sCmd = toString(nSwitch) + ">>" + __sCmd;
        }
        else if (command == "case" || command == "default")
        {
            if (nSwitch && (getCurrentBlock() == "SWCH" || getCurrentBlock() == "CASE"))
            {
                // check the case definition for completeness
                if (!checkCaseValue(__sCmd))
                {
                    reset();
                    throw SyntaxError(SyntaxError::CANNOT_EVAL_SWITCH, __sCmd, SyntaxError::invalid_position);
                }

                // Set the corresponding loop names
                if (command == "default")
                {
                    sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';')) + ";DEF";
                }
                else
                {
                    sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';')) + ";CASE";
                }

                __sCmd = toString(nSwitch) + ">>" + __sCmd;
            }
            else
                return;
        }
        else if (command == "endswitch")
        {
            if (nSwitch && (getCurrentBlock() == "SWCH" || getCurrentBlock() == "CASE" || getCurrentBlock() == "DEF"))
            {
                sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';'));
                __sCmd = toString(nSwitch) + ">>" + __sCmd;
                nSwitch--;
            }
            else
                return;
        }
        else if (command == "endfor")
        {
            if (nLoop && getCurrentBlock() == "FOR")
            {
                sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';'));
                nLoop--;
            }
            else
                return;
        }
        else if (command == "endwhile")
        {
            if (nWhile && getCurrentBlock() == "WHL")
            {
                sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';'));
                nWhile--;
            }
            else
                return;
        }
        else
            return;

        // If there's something after the current flow
        // control statement (i.e. a continued command
        // line), then cache this part here
        if (__sCmd.find('(') != string::npos
            && (command == "for"
                || command == "if"
                || command == "elseif"
                || command == "switch"
                || command == "while"))
        {
            sAppendedExpression = __sCmd.substr(getMatchingParenthesis(__sCmd) + 1);
            __sCmd.erase(getMatchingParenthesis(__sCmd) + 1);
        }
        else if (command == "case" || command == "default")
        {
            if (__sCmd.find(':', 4) != string::npos
                && __sCmd.find_first_not_of(": ", __sCmd.find(':', 4)) != string::npos)
            {
                sAppendedExpression = __sCmd.substr(__sCmd.find(':', 4)+1);
                __sCmd.erase(__sCmd.find(':', 4)+1);
            }
        }
        else if (__sCmd.find(' ', 4) != string::npos
            && __sCmd.find_first_not_of(' ', __sCmd.find(' ', 4)) != string::npos
            && __sCmd[__sCmd.find_first_not_of(' ', __sCmd.find(' ', 4))] != '-')
        {
            sAppendedExpression = __sCmd.substr(__sCmd.find(' ', 4));
            __sCmd.erase(__sCmd.find(' ', 4));
        }

        StripSpaces(sAppendedExpression);
        StripSpaces(__sCmd);

        // Store the current flow control statement
        vCmdArray.push_back(FlowCtrlCommand(__sCmd, nCurrentLine, true));
    }
    else
    {
        // If there's a flow control statement somewhere
        // in the current string, then store this part in
        // a temporary cache
        if (__sCmd.find(" for ") != string::npos
            || __sCmd.find(" for(") != string::npos
            || __sCmd.find(" endfor") != string::npos
            || __sCmd.find(" if ") != string::npos
            || __sCmd.find(" if(") != string::npos
            || __sCmd.find(" else") != string::npos
            || __sCmd.find(" elseif ") != string::npos
            || __sCmd.find(" elseif(") != string::npos
            || __sCmd.find(" endif") != string::npos
            || __sCmd.find(" switch ") != string::npos
            || __sCmd.find(" switch(") != string::npos
            || __sCmd.find(" case ") != string::npos
            || __sCmd.find(" default ") != string::npos
            || __sCmd.find(" default:") != string::npos
            || __sCmd.find(" endswitch") != string::npos
            || __sCmd.find(" while ") != string::npos
            || __sCmd.find(" while(") != string::npos
            || __sCmd.find(" endwhile") != string::npos)
        {
            for (unsigned int n = 0; n < __sCmd.length(); n++)
            {
                if (__sCmd[n] == ' ' && !isInQuotes(__sCmd, n))
                {
                    if (__sCmd.substr(n, 5) == " for "
                        || __sCmd.substr(n, 5) == " for("
                        || __sCmd.substr(n, 7) == " endfor"
                        || __sCmd.substr(n, 4) == " if "
                        || __sCmd.substr(n, 4) == " if("
                        || __sCmd.substr(n, 5) == " else"
                        || __sCmd.substr(n, 8) == " elseif "
                        || __sCmd.substr(n, 8) == " elseif("
                        || __sCmd.substr(n, 6) == " endif"
                        || __sCmd.substr(n, 8) == " switch "
                        || __sCmd.substr(n, 8) == " switch("
                        || __sCmd.substr(n, 6) == " case "
                        || __sCmd.substr(n, 9) == " default "
                        || __sCmd.substr(n, 9) == " default:"
                        || __sCmd.substr(n, 10) == " endswitch"
                        || __sCmd.substr(n, 7) == " while "
                        || __sCmd.substr(n, 7) == " while("
                        || __sCmd.substr(n, 9) == " endwhile")
                    {
                        sAppendedExpression = __sCmd.substr(n + 1);
                        __sCmd.erase(n);
                        break;
                    }
                }
            }
        }

        // Add the breakpoint
        if (bDebuggingBreakPoint)
            __sCmd.insert(0, "|> ");

        // Store the current command line
        vCmdArray.push_back(FlowCtrlCommand(__sCmd, nCurrentLine));
    }

    // If there's a command available and all flow
    // control statements have been closed, evaluate
    // the complete block
    if (vCmdArray.size())
    {
        if (!(nLoop + nIf + nWhile + nSwitch) && vCmdArray.back().bFlowCtrlStatement)
            eval();
    }

    // If there's something left in the current cache,
    // call this function recursively
    if (sAppendedExpression.length())
        setCommand(sAppendedExpression, nCurrentLine);

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
    bBreakSignal = false;
    bContinueSignal = false;
    ReturnVal.clear();
    string sVars = ";";
    sVarName = "";
    dVarAdress = nullptr;
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
    _dataRef = &NumeReKernel::getInstance()->getData();
    _outRef = &NumeReKernel::getInstance()->getOutput();
    _optionRef = &NumeReKernel::getInstance()->getSettings();
    _functionRef = &NumeReKernel::getInstance()->getDefinitions();
    _pDataRef = &NumeReKernel::getInstance()->getPlottingData();
    _scriptRef = &NumeReKernel::getInstance()->getScript();

    if (_parserRef->IsLockedPause())
        bLockedPauseMode = true;

    // Evaluate the user options
    if (_optionRef->getUseMaskAsDefault())
        bMask = true;

    // Read the flow control statements only and
    // extract the index variables and the flow
    // control flags
    sVars = extractFlagsAndIndexVariables();

    // If already suppressed from somewhere else
    if (!_optionRef->getSystemPrintStatus())
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
        checkParsingModeAndExpandDefinitions();
    }
    catch (...)
    {
        reset();
        NumeReKernel::bSupressAnswer = bSupressAnswer_back;
        throw;
    }

    // Prepare the bytecode storage
    nCalcType = new int[vCmdArray.size()];

    for (size_t i = 0; i < vCmdArray.size(); i++)
        nCalcType[i] = CALCTYPE_NONE;

    // Prepare the jump table
    nJumpTableLength = vCmdArray.size();
    nJumpTable = new int*[nJumpTableLength];

    for (size_t i = 0; i < nJumpTableLength; i++)
    {
        nJumpTable[i] = new int[3];
        nJumpTable[i][BLOCK_END] = NO_FLOW_COMMAND;
        nJumpTable[i][ELSE_START] = NO_FLOW_COMMAND;
        nJumpTable[i][PROCEDURE_INTERFACE] = NO_FLOW_COMMAND;
    }

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
    prepareLocalVarsAndReplace(sVars);

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
        if (vCmdArray[0].sCommand.substr(0, 3) == "for")
        {
            if (for_loop() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_FOR, "", SyntaxError::invalid_position);
            }
            else if (!bReturnSignal && !bMask)
            {
                if (bSilent)
                    NumeReKernel::printPreFmt("\r|FOR> " + toSystemCodePage(_lang.get("COMMON_EVALUATING")) + " ... 100 %: " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
                else
                    NumeReKernel::printPreFmt("|FOR> " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
            }
        }
        else if (vCmdArray[0].sCommand.substr(0, 5) == "while")
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
                    NumeReKernel::printPreFmt("\r|WHL> " + toSystemCodePage(_lang.get("COMMON_EVALUATING")) + " ...: " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
                else
                    NumeReKernel::printPreFmt("|WHL> " + toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
            }
        }
        else if (vCmdArray[0].sCommand.find(">>if") != string::npos)
        {
            if (if_fork() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_IF, "", SyntaxError::invalid_position);
            }
        }
        else
        {
            if (switch_fork() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");

                throw SyntaxError(SyntaxError::CANNOT_EVAL_SWITCH, "", SyntaxError::invalid_position);
            }
        }
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

    if (vVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
            _parserRef->RemoveVar(sVarArray[i]);
    }

    if (mVarMap.size() && nVarArray && sVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
        {
            if (mVarMap.find(sVarArray[i]) != mVarMap.end())
                mVarMap.erase(mVarMap.find(sVarArray[i]));
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

            if (vVarArray)
            {
                for (int i = 0; i < nVarArray; i++)
                {
                    if (item->first == sVarArray[i])
                        *item->second = vVarArray[i][0];
                }
            }
        }

        vVars.clear();
    }

    if (vVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
        {
            delete[] vVarArray[i];
        }

        delete[] vVarArray;
        delete[] sVarArray;
        vVarArray = nullptr;
        sVarArray = nullptr;
    }

    if (nJumpTable)
    {
        for (unsigned int i = 0; i < nJumpTableLength; i++)
            delete[] nJumpTable[i];

        delete[] nJumpTable;
        nJumpTable = nullptr;
        nJumpTableLength = 0;
    }

    if (nCalcType)
    {
        delete[] nCalcType;
        nCalcType = nullptr;
    }

    if (nDebuggerCode == NumeReKernel::DEBUGGER_STEPOVER)
        nDebuggerCode = NumeReKernel::DEBUGGER_STEP;

    nVarArray = 0;
    nLoop = 0;
    nLoopSavety = -1;
    nIf = 0;
    nSwitch = 0;
    nWhile = 0;
    nDefaultLength = 10;
    bSilent = true;
    bBreakSignal = false;
    bContinueSignal = false;
    sLoopNames = "";
    sLoopPlotCompose = "";
    dVarAdress = 0;
    sVarName = "";
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
    if (_parserRef)
        _parserRef->ClearVectorVars();

    // Remove all temporary clusters defined for
    // inlined procedures
    if (_dataRef)
        _dataRef->removeTemporaryClusters();

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
/// work and calculates the numerical and string
/// results for the current command line. It will
/// use the previously determined bytecode
/// whereever possible.
///
/// \param sLine string
/// \param nthCmd int
/// \param sBlock string
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::calc(string sLine, int nthCmd, string sBlock)
{
    string sLine_Temp = sLine;
    string sCache = "";
    string sCommand;

    value_type* v = 0;
    int nNum = 0;
    Indices _idx;
    bool bCompiling = false;
    bool bWriteToCache = false;
    bool bWriteToCluster = false;

    // Get the current bytecode for this command
    int nCurrentCalcType = nCalcType[nthCmd];

    // Handle the suppression semicolon
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_SUPPRESSANSWER)
    {
        if (nCurrentCalcType & CALCTYPE_SUPPRESSANSWER)
        {
            sLine.erase(sLine.rfind(';'));
            bLoopSupressAnswer = true;
        }
        else if (sLine.find_last_not_of(" \t") != string::npos && sLine[sLine.find_last_not_of(" \t")] == ';')
        {
            sLine.erase(sLine.rfind(';'));
            bLoopSupressAnswer = true;
            nCalcType[nthCmd] |= CALCTYPE_SUPPRESSANSWER;
        }
        else
            bLoopSupressAnswer = false;
    }
    else
        bLoopSupressAnswer = false;

    // Eval the debugger breakpoint first
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_DEBUGBREAKPOINT || nDebuggerCode == NumeReKernel::DEBUGGER_STEP)
    {
        if (sLine.substr(sLine.find_first_not_of(' '), 2) == "|>" || nDebuggerCode == NumeReKernel::DEBUGGER_STEP)
        {
            if (!nCurrentCalcType && sLine.substr(sLine.find_first_not_of(' '), 2) == "|>")
                nCalcType[nthCmd] |= CALCTYPE_DEBUGBREAKPOINT;

            if (sLine.substr(sLine.find_first_not_of(' '), 2) == "|>")
            {
                sLine.erase(sLine.find_first_not_of(' '), 2);
                sLine_Temp.erase(sLine_Temp.find_first_not_of(' '), 2);
                StripSpaces(sLine);
                StripSpaces(sLine_Temp);
            }

            if (_optionRef->getUseDebugger() && nDebuggerCode != NumeReKernel::DEBUGGER_LEAVE && nDebuggerCode != NumeReKernel::DEBUGGER_STEPOVER)
            {
                NumeReKernel::getInstance()->getDebugger().gatherLoopBasedInformations(sLine, getCurrentLineNumber(), mVarMap, vVarArray, sVarArray, nVarArray);
                nDebuggerCode = evalDebuggerBreakPoint(*_parserRef, *_optionRef);
            }
        }
    }

    if (!nCurrentCalcType
        || !bFunctionsReplaced
        || nCurrentCalcType & (CALCTYPE_COMMAND | CALCTYPE_DEFINITION | CALCTYPE_PROGRESS | CALCTYPE_COMPOSE | CALCTYPE_RETURNCOMMAND | CALCTYPE_THROWCOMMAND | CALCTYPE_EXPLICIT))
        sCommand = findCommand(sLine).sString;

    // Replace the custom defined functions, if it wasn't already done
    if (!nCurrentCalcType || !(nCurrentCalcType & CALCTYPE_DEFINITION) || !bFunctionsReplaced)
    {
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

        if (!nCurrentCalcType && (sCommand == "define" || sCommand == "redef" || sCommand == "redefine" || sCommand == "undefine" || sCommand == "undef" || sCommand == "ifndef" || sCommand == "ifndefined"))
        {
            nCalcType[nthCmd] |= CALCTYPE_COMMAND | CALCTYPE_DEFINITION;
        }

        if (!nCurrentCalcType && bFunctionsReplaced && nCalcType[nthCmd] & CALCTYPE_DEFINITION)
            nCalcType[nthCmd] |= CALCTYPE_RECURSIVEEXPRESSION;
    }

    // Handle the throw command
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_THROWCOMMAND)
    {
        if (sCommand == "throw" || sLine == "throw")
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

            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_THROWCOMMAND;

            throw SyntaxError(SyntaxError::LOOP_THROW, sLine, SyntaxError::invalid_position, sErrorToken);
        }
    }

    // Handle the return command
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_RETURNCOMMAND)
    {
        if (sCommand == "return")
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_RETURNCOMMAND;

            if (sLine.find("void", sLine.find("return") + 6) != string::npos)
            {
                string sReturnValue = sLine.substr(sLine.find("return") + 6);
                StripSpaces(sReturnValue);

                if (sReturnValue == "void")
                {
                    bReturnSignal = true;
                    nReturnType = 0;
                    return FLOWCTRL_RETURN;
                }
            }

            sLine = sLine.substr(sLine.find("return") + 6);
            StripSpaces(sLine);

            if (!sLine.length())
            {
                ReturnVal.vNumVal.push_back(1.0);
                bReturnSignal = true;
                return FLOWCTRL_RETURN;
            }

            bReturnSignal = true;
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

    // Is it a numerical expression, which was already
    // parsed? Evaluate it here
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_NUMERICAL)
    {
        if (_parserRef->IsValidByteCode() == 1 && _parserRef->IsAlreadyParsed(sLine) && !bLockedPauseMode && bUseLoopParsingMode)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_NUMERICAL;

            v = _parserRef->Eval(nNum);
            vAns = v[0];
            NumeReKernel::getInstance()->getAns().clear();
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
    }

    // Does this contain a plot composition? Combine the
    // needed lines here. This is not necessary, if the lines
    // are read from a procedure, which will provide the compositon
    // in a single line
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_COMPOSE || sLoopPlotCompose.length())
    {
        if ((sCommand == "compose"
            || sCommand == "endcompose"
            || sLoopPlotCompose.length())
            && sCommand != "quit")
        {
            if (!nCurrentCalcType)
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
                if (sCommand.substr(0, 4) == "plot"
                    || sCommand.substr(0, 4) == "grad"
                    || sCommand.substr(0, 4) == "dens"
                    || sCommand.substr(0, 4) == "vect"
                    || sCommand.substr(0, 4) == "cont"
                    || sCommand.substr(0, 4) == "surf"
                    || sCommand.substr(0, 4) == "mesh")
                    sLoopPlotCompose += sLine + " <<COMPOSE>> ";

                return FLOWCTRL_OK;
            }
            else
            {
                sLine = sLoopPlotCompose;
                sLoopPlotCompose = "";
            }
        }
    }

    // Handle the "to_cmd()" function here
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_TOCOMMAND)
    {
        if (sLine.find("to_cmd(") != string::npos)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_TOCOMMAND | CALCTYPE_COMMAND | CALCTYPE_DATAACCESS | CALCTYPE_EXPLICIT | CALCTYPE_PROGRESS | CALCTYPE_RECURSIVEEXPRESSION;

            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode();

            unsigned int nPos = 0;

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
    }

    // Display a progress bar, if it is desired
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_PROGRESS)
    {
        if (sCommand == "progress" && sLine.length() > 9)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_PROGRESS;

            value_type* vVals = 0;
            string sExpr;
            string sArgument;
            int nArgument;

            if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine))
            {
                if (bUseLoopParsingMode && !bLockedPauseMode)
                    _parserRef->PauseLoopMode();

                sLine = evaluateParameterValues(sLine);

                if (bUseLoopParsingMode && !bLockedPauseMode)
                    _parserRef->PauseLoopMode(false);
            }

            if (sLine.find("-set") != string::npos || sLine.find("--") != string::npos)
            {
                if (sLine.find("-set") != string::npos)
                    sArgument = sLine.substr(sLine.find("-set"));
                else
                    sArgument = sLine.substr(sLine.find("--"));

                sLine.erase(sLine.find(sArgument));

                if (findParameter(sArgument, "first", '='))
                {
                    sExpr = getArgAtPos(sArgument, findParameter(sArgument, "first", '=') + 5) + ",";
                }
                else
                    sExpr = "1,";

                if (findParameter(sArgument, "last", '='))
                {
                    sExpr += getArgAtPos(sArgument, findParameter(sArgument, "last", '=') + 4);
                }
                else
                    sExpr += "100";

                if (findParameter(sArgument, "type", '='))
                {
                    sArgument = getArgAtPos(sArgument, findParameter(sArgument, "type", '=') + 4);

                    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sArgument))
                    {
                        if (sArgument.front() != '"')
                            sArgument = "\"" + sArgument + "\" -nq";

                        if (bUseLoopParsingMode && !bLockedPauseMode)
                            _parserRef->PauseLoopMode();

                        string sDummy;
                        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sArgument, sDummy, true);

                        if (bUseLoopParsingMode && !bLockedPauseMode)
                            _parserRef->PauseLoopMode(false);
                    }
                }
                else
                    sArgument = "std";
            }
            else
            {
                sArgument = "std";
                sExpr = "1,100";
            }

            while (sLine.length() && (sLine[sLine.length() - 1] == ' ' || sLine[sLine.length() - 1] == '-'))
                sLine.pop_back();

            if (!sLine.length())
                return FLOWCTRL_OK;

            if (!_parserRef->IsAlreadyParsed(sLine.substr(findCommand(sLine).nPos + 8) + "," + sExpr))
            {
                _parserRef->SetExpr(sLine.substr(findCommand(sLine).nPos + 8) + "," + sExpr);
            }

            vVals = _parserRef->Eval(nArgument);
            make_progressBar((int)vVals[0], (int)vVals[1], (int)vVals[2], sArgument);
            return FLOWCTRL_OK;
        }
    }

    // Display the prompt to the user
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_PROMPT)
    {
        // --> Prompt <--
        if (sLine.find("??") != string::npos)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_PROMPT | CALCTYPE_PROCEDURECMDINTERFACE | CALCTYPE_COMMAND | CALCTYPE_DATAACCESS | CALCTYPE_STRING | CALCTYPE_RECURSIVEEXPRESSION;

            if (bPrintedStatus)
                NumeReKernel::printPreFmt("\n");

            sLine = promptForUserInput(sLine);
            bPrintedStatus = false;
        }
    }

    // Include procedure and plugin calls
    if (nJumpTable[nthCmd][PROCEDURE_INTERFACE])
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }

        int nReturn = procedureInterface(sLine, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nLoop + nWhile + nIf, nthCmd);

        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode(false);
            _parserRef->LockPause(false);
        }

        if (nReturn == -2 || nReturn == 2)
            nJumpTable[nthCmd][PROCEDURE_INTERFACE] = 1;
        else
            nJumpTable[nthCmd][PROCEDURE_INTERFACE] = 0;

        if (nReturn == -1)
            return FLOWCTRL_ERROR;
        else if (nReturn == -2)
            return FLOWCTRL_OK;
    }

    // Handle the procedure commands like "namespace" here
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_PROCEDURECMDINTERFACE)
    {
        int nProcedureCmd = procedureCmdInterface(sLine);

        if (nProcedureCmd)
        {
            if (nProcedureCmd == 1)
            {
                if (!nCurrentCalcType)
                    nCalcType[nthCmd] |= CALCTYPE_PROCEDURECMDINTERFACE;

                return FLOWCTRL_OK;
            }
        }
        else
            return FLOWCTRL_ERROR;
    }

    // Remove the "explicit" command here
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_EXPLICIT)
    {
        if (sCommand == "explicit")
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_EXPLICIT;

            sLine.erase(findCommand(sLine).nPos, 8);
            StripSpaces(sLine);
        }
    }

    // Evaluate the command, if this is a command
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_COMMAND)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();
        {
            bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;
            string sPreCommandLine = sLine;
            NumeReKernel::bSupressAnswer = bLoopSupressAnswer;

            switch (commandHandler(sLine))
            {
                case NO_COMMAND:
                    if (!nCurrentCalcType)
                    {
                        StripSpaces(sPreCommandLine);
                        string sCurrentLine = sLine;
                        StripSpaces(sCurrentLine);

                        if (sPreCommandLine != sCurrentLine)
                            nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                    }

                    break;
                case COMMAND_PROCESSED:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;

                    if (!nCurrentCalcType)
                        nCalcType[nthCmd] |= CALCTYPE_COMMAND;

                    return FLOWCTRL_OK;
                case NUMERE_QUIT:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;

                    if (!nCurrentCalcType)
                        nCalcType[nthCmd] |= CALCTYPE_COMMAND;

                    return FLOWCTRL_OK;
                case COMMAND_HAS_RETURNVALUE:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;

                    if (!nCurrentCalcType)
                        nCalcType[nthCmd] |= CALCTYPE_COMMAND;

                    break;
            }

            NumeReKernel::bSupressAnswer = bSupressAnswer_back;
        }

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }

    // Expand recursive expressions, if not already done
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_RECURSIVEEXPRESSION)
        evalRecursiveExpressions(sLine);

    // Get the data from the used data object
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_DATAACCESS)
    {
        // --> Datafile/Cache! <--
        if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine)
            && (sLine.find("data(") != string::npos || _dataRef->containsTablesOrClusters(sLine)))
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_DATAACCESS;

            if (!_parserRef->HasCachedAccess() && _parserRef->CanCacheAccess())
            {
                bCompiling = true;
                _parserRef->SetCompiling(true);
            }

            sCache = getDataElements(sLine, *_parserRef, *_dataRef, *_optionRef);

            if (sCache.length() && sCache.find('#') == string::npos)
                bWriteToCache = true;

            if (_parserRef->IsCompiling())
            {
                _parserRef->CacheCurrentEquation(sLine);
                _parserRef->CacheCurrentTarget(sCache);
            }

            _parserRef->SetCompiling(false);
        }
        else if (isClusterCandidate(sLine, sCache))
        {
            bWriteToCache = true;

            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_DATAACCESS;
        }
    }

    // Evaluate string expressions
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_STRING)
    {
        // --> String-Parser <--
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sLine))
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_STRING | CALCTYPE_DATAACCESS;

            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode();

            auto retVal = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sLine, sCache, bLoopSupressAnswer);
            NumeReKernel::getInstance()->getStringParser().removeTempStringVectorVars();

            if (retVal == NumeRe::StringParser::STRING_SUCCESS)
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parserRef->PauseLoopMode(false);

                if (bReturnSignal)
                {
                    ReturnVal.vStringVal.push_back(sLine);
                    return FLOWCTRL_RETURN;
                }

                return FLOWCTRL_OK;
            }

            replaceLocalVars(sLine);

            if (sCache.length() && _dataRef->containsTablesOrClusters(sCache) && !bWriteToCache)
                bWriteToCache = true;

            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode(false);
        }
    }

    // Get the target indices of the target data object
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_DATAACCESS)
    {
        if (bWriteToCache)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_DATAACCESS;

            if (bCompiling)
            {
                if (sCache[sCache.find_first_of("({")] == '{')
                    bWriteToCluster = true;

                _parserRef->SetCompiling(true);
                _idx = getIndices(sCache, *_parserRef, *_dataRef, *_optionRef);

                if (!isValidIndexSet(_idx))
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "");

                if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                    throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

                sCache.erase(sCache.find_first_of("({"));
                StripSpaces(sCache);

                if (!bWriteToCluster)
                    _parserRef->CacheCurrentTarget(sCache + "(" + _idx.sCompiledAccessEquation + ")");
                else
                    _parserRef->CacheCurrentTarget(sCache + "{" + _idx.sCompiledAccessEquation + "}");

                _parserRef->SetCompiling(false);
            }
            else
            {
                _idx = getIndices(sCache, *_parserRef, *_dataRef, *_optionRef);

                if (sCache[sCache.find_first_of("({")] == '{')
                    bWriteToCluster = true;

                if (!isValidIndexSet(_idx))
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "");

                if (!bWriteToCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
                    throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");

                sCache.erase(sCache.find_first_of("({"));
                StripSpaces(sCache);
            }
        }
    }

    // Parse the numerical expression, if it is not
    // already available as bytecode
    if (!_parserRef->IsAlreadyParsed(sLine))
    {
        _parserRef->SetExpr(sLine);
    }

    // Calculate the result
    v = _parserRef->Eval(nNum);
    vAns = v[0];
    NumeReKernel::getInstance()->getAns().clear();
    NumeReKernel::getInstance()->getAns().setDoubleArray(nNum, v);

    if (!nCurrentCalcType && !(nCalcType[nthCmd] & CALCTYPE_DATAACCESS || nCalcType[nthCmd] & CALCTYPE_STRING))
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
        {
            NumeRe::Cluster& cluster = _dataRef->getCluster(sCache);
            cluster.assignResults(_idx, nNum, v);
            bWriteToCluster = false;
        }
        else
        {
            _dataRef->writeToTable(_idx, sCache, v, nNum);
            bWriteToCache = false;
        }
    }

    if (bReturnSignal)
    {
        for (int i = 0; i < nNum; i++)
            ReturnVal.vNumVal.push_back(v[i]);

        return FLOWCTRL_RETURN;
    }

    return FLOWCTRL_OK;
}


/////////////////////////////////////////////////
/// \brief This member function is used to
/// replace variable occurences with their
/// (auto-determined) internal name.
///
/// \param sLine string&
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::replaceLocalVars(string& sLine)
{
    if (!mVarMap.size())
        return;

    for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
    {
        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if (sLine.substr(i, (iter->first).length()) == iter->first)
            {
                if ((i && checkDelimiter(sLine.substr(i - 1, (iter->first).length() + 2)))
                    || (!i && checkDelimiter(" " + sLine.substr(i, (iter->first).length() + 1))))
                {
                    sLine.replace(i, (iter->first).length(), iter->second);
                }
            }
        }
    }

    return;
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether
/// the passed flow control argument is valid or
/// not.
///
/// \param sFlowControlArgument const string&
/// \param isForLoop bool
/// \return bool
///
/////////////////////////////////////////////////
bool FlowCtrl::checkFlowControlArgument(const string& sFlowControlArgument, bool isForLoop)
{
    // The argument shall be enclosed in parentheses
    if (sFlowControlArgument.find('(') == string::npos)
        return false;

    string sArgument = sFlowControlArgument.substr(sFlowControlArgument.find('('));
    sArgument = sArgument.substr(1, getMatchingParenthesis(sArgument) - 1);

    // Ensure that the argument is not empty
    if (!isNotEmptyExpression(sArgument))
        return false;

    // That's everything for a non-for loop
    if (!isForLoop)
        return true;

    // If it is a for loop, then it has to fulfill
    // another requirement: the index and its interval
    if (sArgument.find('=') == string::npos
        || sArgument.find(':', sArgument.find('=') + 1) == string::npos)
        return false;

    // Everything seems to be OK
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function checks, whether
/// the entered case definition is valid or not.
///
/// \param sCaseDefinition const string&
/// \return bool
///
/////////////////////////////////////////////////
bool FlowCtrl::checkCaseValue(const string& sCaseDefinition)
{
    // Colon operator is missing
    if (sCaseDefinition.find(':') == string::npos)
        return false;

    // Check, whether there's a valid value between
    // "case" and the colon operator
    if (sCaseDefinition.substr(0, 5) == "case ")
    {
        // Extract the value
        string sValue = sCaseDefinition.substr(4);
        sValue.erase(sValue.find(':'));

        // Check, whether there are other characters
        // than the whitespace
        if (sValue.find_first_not_of(' ') == string::npos)
            return false;

        // Cut of the first expression (in the possible list)
        getNextArgument(sValue, true);

        // Check for more than one value (only one allowed)
        if (sValue.length() && sValue.find_first_not_of(' ') != string::npos)
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
/// \return string
///
/////////////////////////////////////////////////
string FlowCtrl::extractFlagsAndIndexVariables()
{
    string sVars = ";";
    string sVar;
    for (size_t i = 0; i < vCmdArray.size(); i++)
    {
        // No flow control statement
        if (!vCmdArray[i].bFlowCtrlStatement)
            continue;

        // Extract the index variables
        if (vCmdArray[i].sCommand.substr(0, 3) == "for")
        {
            sVar = vCmdArray[i].sCommand.substr(vCmdArray[i].sCommand.find('(') + 1);
            sVar = sVar.substr(0, sVar.find('='));
            StripSpaces(sVar);

            if (sVars.find(";" + sVar + ";") == string::npos)
            {
                sVars += sVar + ";";
                nVarArray++;
            }

            vCmdArray[i].sCommand[vCmdArray[i].sCommand.find('(')] = ' ';
            vCmdArray[i].sCommand.pop_back();
        }

        // Extract the flow control flags
        if (vCmdArray[i].sCommand.find("end") != string::npos && findParameter(vCmdArray[i].sCommand, "sv"))
            bSilent = false;

        if (vCmdArray[i].sCommand.find("end") != string::npos && findParameter(vCmdArray[i].sCommand, "mask"))
            bMask = true;

        if (vCmdArray[i].sCommand.find("end") != string::npos && findParameter(vCmdArray[i].sCommand, "sp"))
            bMask = false;

        if (vCmdArray[i].sCommand.find("end") != string::npos && findParameter(vCmdArray[i].sCommand, "lnumctrl"))
            nLoopSavety = 1000;

        if (vCmdArray[i].sCommand.find("end") != string::npos && findParameter(vCmdArray[i].sCommand, "lnumctrl", '='))
        {
            _parserRef->SetExpr(getArgAtPos(vCmdArray[i].sCommand, findParameter(vCmdArray[i].sCommand, "lnumctrl", '=') + 8));
            nLoopSavety = (int)_parserRef->Eval();

            if (nLoopSavety <= 0)
                nLoopSavety = 1000;
        }
    }
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
            if (vCmdArray[i].sCommand.substr(0, 3) == "for")
            {
                int nForCount = 0;

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, 6) == "endfor")
                    {
                        if (nForCount)
                            nForCount--;
                        else
                        {
                            nJumpTable[i][BLOCK_END] = j;
                            break;
                        }
                    }
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, 3) == "for")
                        nForCount++;
                }
            }
            else if (vCmdArray[i].sCommand.substr(0, 5) == "while")
            {
                int nWhileCount = 0;

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, 8) == "endwhile")
                    {
                        if (nWhileCount)
                            nWhileCount--;
                        else
                        {
                            nJumpTable[i][BLOCK_END] = j;
                            break;
                        }
                    }
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, 5) == "while")
                        nWhileCount++;
                }
            }
            else if (vCmdArray[i].sCommand.find(">>if") != string::npos)
            {
                string sNth_If = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_If + ">>else").length()) == sNth_If + ">>else" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_If + ">>elseif").length()) == sNth_If + ">>elseif" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_If + ">>endif").length()) == sNth_If + ">>endif")
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
            else if (vCmdArray[i].sCommand.find(">>elseif") != string::npos)
            {
                string sNth_If = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_If + ">>else").length()) == sNth_If + ">>else" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_If + ">>elseif").length()) == sNth_If + ">>elseif" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_If + ">>endif").length()) == sNth_If + ">>endif")
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
            else if (vCmdArray[i].sCommand.find(">>switch") != string::npos)
            {
                string sNth_Switch = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_Switch + ">>case").length()) == sNth_Switch + ">>case" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_Switch + ">>default").length()) == sNth_Switch + ">>default" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_Switch + ">>endswitch").length()) == sNth_Switch + ">>endswitch")
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
            else if (vCmdArray[i].sCommand.find(">>case") != string::npos)
            {
                string sNth_Switch = vCmdArray[i].sCommand.substr(0, vCmdArray[i].sCommand.find(">>"));

                for (size_t j = i + 1; j < vCmdArray.size(); j++)
                {
                    if (!vCmdArray[j].bFlowCtrlStatement)
                        continue;

                    if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_Switch + ">>case").length()) == sNth_Switch + ">>case" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_Switch + ">>default").length()) == sNth_Switch + ">>default" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_Switch + ">>endswitch").length()) == sNth_Switch + ">>endswitch")
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
                && (vCmdArray[i].sCommand.find("+=") != string::npos
                    || vCmdArray[i].sCommand.find("-=") != string::npos
                    || vCmdArray[i].sCommand.find("*=") != string::npos
                    || vCmdArray[i].sCommand.find("/=") != string::npos
                    || vCmdArray[i].sCommand.find("^=") != string::npos
                    || vCmdArray[i].sCommand.find("++") != string::npos
                    || vCmdArray[i].sCommand.find("--") != string::npos))
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
    string sSwitchArgument = vCmdArray[nSwitchStart].sCommand;
    vCmdArray[nSwitchStart].sCommand.erase(sSwitchArgument.find('('));
    sSwitchArgument.erase(0, sSwitchArgument.find('(')+1);
    sSwitchArgument.erase(sSwitchArgument.rfind(')'));

    string sArgument = "";

    // Extract the switch level
    string sNth_Switch = vCmdArray[nSwitchStart].sCommand.substr(0, vCmdArray[nSwitchStart].sCommand.find(">>"));

    // Search for all cases, which belong to the current
    // switch level
    for (size_t j = nSwitchStart + 1; j < vCmdArray.size(); j++)
    {
        // Extract the value of the found case and gather
        // it in the argument list
        if (vCmdArray[j].sCommand.length() && vCmdArray[j].sCommand.substr(0, (sNth_Switch + ">>case").length()) == sNth_Switch + ">>case")
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
        string sExpr = sSwitchArgument;
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
            if (vCmdArray[i].sCommand.substr(0, 3) == "for" || vCmdArray[i].sCommand.substr(0, 5) == "while")
            {
                if (!bUseLoopParsingMode && !bLockedPauseMode)
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
            if (vCmdArray[i].sCommand.find("to_cmd(") != string::npos)
            {
                bUseLoopParsingMode = false;
                break;
            }

            if (vCmdArray[i].sCommand.find('$') != string::npos)
            {
                int nInlining = 0;

                if (!(nInlining = isInline(vCmdArray[i].sCommand)))
                {
                    bUseLoopParsingMode = false;
                    break;
                }
                else if (!vCmdArray[i].bFlowCtrlStatement && nInlining != INLINING_GLOBALINRETURN)
                {
                    vector<string> vExpandedProcedure = expandInlineProcedures(vCmdArray[i].sCommand);
                    int nLine = vCmdArray[i].nInputLine;

                    for (size_t j = 0; j < vExpandedProcedure.size(); j++)
                    {
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
}


/////////////////////////////////////////////////
/// \brief This method prepares the local
/// variables including their names and replaces
/// them in the command lines in the current
/// command block.
///
/// \param sVars string&
/// \return void
///
/////////////////////////////////////////////////
void FlowCtrl::prepareLocalVarsAndReplace(string& sVars)
{
    // Prepare the array for the local variables
    vVarArray = new value_type*[nVarArray];
    sVarArray = new string[nVarArray];
    sVars = sVars.substr(1, sVars.length() - 1);

    // If a loop variable was defined before the
    // current loop, this one is used. All others
    // are create locally and therefore get
    // special names
    for (int i = 0; i < nVarArray; i++)
    {
        vVarArray[i] = new value_type[3];
        sVarArray[i] = sVars.substr(0, sVars.find(';'));

        if (i + 1 != nVarArray)
            sVars = sVars.substr(sVars.find(';') + 1);

        StripSpaces(sVarArray[i]);

        // Is it already defined?
        if (getPointerToVariable(sVarArray[i], *_parserRef))
            vVars[sVarArray[i]] = getPointerToVariable(sVarArray[i], *_parserRef);
        else
        {
            // Create a local variable otherwise
            mVarMap[sVarArray[i]] = "_~LOOP_" + sVarArray[i] + "_" + toString(nthRecursion);
            sVarArray[i] = mVarMap[sVarArray[i]];
        }

        _parserRef->DefineVar(sVarArray[i], &vVarArray[i][0]);
    }

    _parserRef->mVarMapPntr = &mVarMap;

    // Replace the local index variables in the
    // whole command set for this flow control
    // statement
    for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
    {
        for (size_t i = 0; i < vCmdArray.size(); i++)
        {
            // Replace it in the flow control
            // statements
            if (vCmdArray[i].sCommand.length())
            {
                vCmdArray[i].sCommand += " ";

                for (unsigned int j = 0; j < vCmdArray[i].sCommand.length(); j++)
                {
                    if (vCmdArray[i].sCommand.substr(j, (iter->first).length()) == iter->first)
                    {
                        if (((!j && checkDelimiter(" " + vCmdArray[i].sCommand.substr(j, (iter->first).length() + 1)))
                            || (j && checkDelimiter(vCmdArray[i].sCommand.substr(j - 1, (iter->first).length() + 2))))
                            && !isInQuotes(vCmdArray[i].sCommand, j, true))
                        {
                            vCmdArray[i].sCommand.replace(j, (iter->first).length(), iter->second);
                            j += (iter->second).length() - (iter->first).length();
                        }
                    }
                }

                StripSpaces(vCmdArray[i].sCommand);
            }

        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// current line number as enumerated during
/// passing the commands via "setCommand()".
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
/// \return string
///
/////////////////////////////////////////////////
string FlowCtrl::getCurrentCommand() const
{
    if (!vCmdArray.size())
        return "";

    return vCmdArray[nCurrentCommand].sCommand;
}


// VIRTUAL FUNCTION IMPLEMENTATIONS
/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param sLine string&
/// \param _parser Parser&
/// \param _functions Define&
/// \param _data Datafile&
/// \param _out Output&
/// \param _pData PlotData&
/// \param _script Script&
/// \param _option Settings&
/// \param nth_loop unsigned int
/// \param nth_command int
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::procedureInterface(string& sLine, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_loop, int nth_command)
{
    return 1;
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param sLine string&
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::procedureCmdInterface(string& sLine)
{
    return 1;
}


/////////////////////////////////////////////////
/// \brief Dummy implementation.
///
/// \param sProc const string&
/// \return int
///
/////////////////////////////////////////////////
int FlowCtrl::isInline(const string& sProc)
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
/// \param sLine string&
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> FlowCtrl::expandInlineProcedures(string& sLine)
{
    return vector<string>();
}

