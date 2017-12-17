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


// Implementation der LOOP-Klasse
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
//extern bool bSupressAnswer;

FlowCtrl::FlowCtrl()
{
    sCmd = nullptr;
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
    nCmd = 0;
    nLoop = 0;
    nIf = 0;
    nWhile = 0;
    nDefaultLength = 10;
    nVarArray = 0;
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
}

FlowCtrl::FlowCtrl(int _nDefaultLength)
{
    FlowCtrl();
    nDefaultLength = _nDefaultLength;
}

FlowCtrl::~FlowCtrl()
{
    if (sCmd)
    {
        for(int i = 0; i < nDefaultLength; i++)
            delete[] sCmd[i];
        delete[] sCmd;
        sCmd = 0;
    }

    if (vVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
        {
            delete[] vVarArray[i];
        }
        delete[] vVarArray;
        vVarArray = 0;
    }
}

void FlowCtrl::generateCommandArray()
{
    if (sCmd)
    {
        string** sTemp = new string*[nDefaultLength];
        for (int i = 0; i < nDefaultLength; i++)
        {
            sTemp[i] = new string[2];
            for (int j = 0; j < 2; j++)
            {
                sTemp[i][j] = sCmd[i][j];
            }
        }
        for (int i = 0; i < nDefaultLength; i++)
        {
            delete[] sCmd[i];
        }
        delete[] sCmd;
        sCmd = 0;

        sCmd = new string*[2*nDefaultLength];
        for (int i = 0; i < 2*nDefaultLength; i++)
        {
            sCmd[i] = new string[2];
            for (int j = 0; j < 2; j++)
            {
                if (i < nDefaultLength)
                    sCmd[i][j] = sTemp[i][j];
                else
                    sCmd[i][j] = "";
            }
        }

        for (int i = 0; i < nDefaultLength; i++)
        {
            delete[] sTemp[i];
        }
        delete[] sTemp;
        sTemp = 0;

        nDefaultLength += nDefaultLength;
    }
    else
    {
        sCmd = new string*[nDefaultLength];
        for (int i = 0; i < nDefaultLength; i++)
        {
            sCmd[i] = new string[2];
            for (int j = 0; j < 2; j++)
            {
                sCmd[i][j] = "";
            }
        }
    }
    return;
}

int FlowCtrl::for_loop(int nth_Cmd, int nth_loop)
{
    int nVarAdress = 0;
    int nInc = 1;
    int nLoopCount = 0;
    string sForHead = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('=')+1);
    string sVar = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find(' ')+1);
    string sLine = "";
    bPrintedStatus = false;
    sVar = sVar.substr(0,sVar.find('='));
    StripSpaces(sVar);
    int nNum = 0;
    value_type* v = 0;

    for (int i = 0; i < nVarArray; i++)
    {
        if (sVarArray[i] == sVar)
        {
            nVarAdress = i;
            break;
        }
    }

    v = evalHeader(nNum, sForHead, true, nth_Cmd);

    for (int i = 0; i < 2; i++)
    {
        vVarArray[nVarAdress][i+1] = (int)v[i];
    }

    if (vVarArray[nVarAdress][2] < vVarArray[nVarAdress][1])
        nInc *= -1;

    if (bSilent && !nth_loop && !bMask)
    {
        NumeReKernel::printPreFmt("|FOR> "+_lang.get("COMMON_EVALUATING")+" ... 0 %");
        bPrintedStatus = true;
    }
    for (int __i = (int)vVarArray[nVarAdress][1]; (nInc)*__i <= nInc*(int)vVarArray[nVarAdress][2]; __i+=nInc)
    {
        vVarArray[nVarAdress][0] = __i;
        if (nLoopCount >= nLoopSavety && nLoopSavety > 0)
            return -1;
        nLoopCount++;
        for (int __j = nth_Cmd; __j < nJumpTable[nth_Cmd][BLOCK_END]; __j++)
        {
            if (__j != nth_Cmd)
            {
                if (sCmd[__j][0].length())
                {
                    int nReturn = evalLoopFlowCommands(__j, nth_loop);

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
                }
            }
            if (!sCmd[__j][1].length())
                continue;

            if (!nCalcType[__j])
            {
                string sCommand = findCommand(sCmd[__j][1]).sString;
                if (sCommand == "continue")
                {
                    nCalcType[__j] = CALCTYPE_CONTINUECMD;
                    break;
                }
                if (sCommand == "break")
                {
                    nCalcType[__j] = CALCTYPE_BREAKCMD;
                    return nJumpTable[nth_Cmd][BLOCK_END];
                }
            }
            else if (nCalcType[__j] & CALCTYPE_CONTINUECMD)
                break;
            else if (nCalcType[__j] & CALCTYPE_BREAKCMD)
                return nJumpTable[nth_Cmd][BLOCK_END];

            if (bUseLoopParsingMode)
                _parserRef->SetIndex(__j + nCmd+1);
            try
            {
                if (calc(sCmd[__j][1], __j, "FOR") == FLOWCTRL_ERROR)
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                    return FLOWCTRL_ERROR;
                }
                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }
        __i = (int)vVarArray[nVarAdress][0];

        if (!nth_loop && !bMask && bSilent)
        {
            if (abs(int(vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1])) < 99999
                && abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 20))
                    > abs((int)((vVarArray[nVarAdress][0]-1-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 20)))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... " + toString(abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 20)) * 5) + " %");
                bPrintedStatus = true;
            }
            else if (abs(int(vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) >= 99999)
                && abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 100))
                    > abs((int)((vVarArray[nVarAdress][0]-1-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 100)))
            {
                NumeReKernel::printPreFmt("\r|FOR> " + _lang.get("COMMON_EVALUATING") + " ... " + toString(abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 100))) + " %");
                bPrintedStatus = true;
            }
        }
    }
    return nJumpTable[nth_Cmd][BLOCK_END];
}

int FlowCtrl::while_loop(int nth_Cmd, int nth_loop)
{
    string sWhile_Condition = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('(')+1, sCmd[nth_Cmd][0].rfind(')')-sCmd[nth_Cmd][0].find('(')-1);
    string sWhile_Condition_Back = sWhile_Condition;
    string sLine = "";
    bPrintedStatus = false;
    //int nResults = 0;
    int nLoopCount = 0;
    value_type* v = 0;
    int nNum = 0;


    if (bSilent && !bMask && !nth_loop)
    {
        NumeReKernel::printPreFmt("|WHL> " + toSystemCodePage(_lang.get("COMMON_EVALUATING")) + " ... ");
        bPrintedStatus = true;
    }
    while (true)
    {
        v = evalHeader(nNum, sWhile_Condition, false, nth_Cmd);
        /*if (bUseLoopParsingMode && !bLockedPauseMode)
            _parserRef->SetIndex(nth_Cmd);
        if (!bUseLoopParsingMode || bLockedPauseMode || !_parserRef->IsValidByteCode() || _parserRef->GetExpr() != sWhile_Condition+" ")
        {
            if (bUseLoopParsingMode && !bLockedPauseMode && _parserRef->IsValidByteCode() && _parserRef->GetExpr().length())
                _parserRef->DeclareAsInvalid();
            _parserRef->SetExpr(sWhile_Condition);
        }*/
        if (nLoopCount >= nLoopSavety && nLoopSavety > 1)
            return -1;
        nLoopCount++;
        //v = _parserRef->Eval(nResults);*/
        if (!(bool)v[0] || isnan(v[0]) || isinf(v[0]))
        {
            return nJumpTable[nth_Cmd][BLOCK_END];
        }
        for (int __j = nth_Cmd; __j < nJumpTable[nth_Cmd][BLOCK_END]; __j++)
        {
            if (__j != nth_Cmd)
            {
                if (sCmd[__j][0].length())
                {
                    int nReturn = evalLoopFlowCommands(__j, nth_loop);

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
                }
            }
            if (!sCmd[__j][1].length())
                continue;
            if (!nCalcType[__j])
            {
                string sCommand = findCommand(sCmd[__j][1]).sString;
                if (sCommand == "continue")
                {
                    nCalcType[__j] = CALCTYPE_CONTINUECMD;
                    break;
                }
                if (sCommand == "break")
                {
                    nCalcType[__j] = CALCTYPE_BREAKCMD;
                    return nJumpTable[nth_Cmd][BLOCK_END];
                }
            }
            else if (nCalcType[__j] & CALCTYPE_CONTINUECMD)
                break;
            else if (nCalcType[__j] & CALCTYPE_BREAKCMD)
                return nJumpTable[nth_Cmd][BLOCK_END];

            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parserRef->SetIndex(__j + nCmd+1);

            try
            {
                if (calc(sCmd[__j][1], __j, "WHL") == FLOWCTRL_ERROR)
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                    return FLOWCTRL_ERROR;
                }
                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch(...)
            {
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }

        /*if (!nth_loop)
        {
            if (bSilent && !bMask)
            {
                NumeReKernel::printPreFmt("\r|WHL> " + toSystemCodePage(_lang.get("COMMON_EVALUATING")) + " ... ");
                bPrintedStatus = true;
            }
        }*/


        if (sWhile_Condition != sWhile_Condition_Back || _dataRef->containsCacheElements(sWhile_Condition_Back) || sWhile_Condition_Back.find("data(") != string::npos)
        {
            sWhile_Condition = sWhile_Condition_Back;
            v = evalHeader(nNum, sWhile_Condition, false, nth_Cmd);
        }
    }
    return nCmd;
}

int FlowCtrl::if_fork(int nth_Cmd, int nth_loop)
{
    string sIf_Condition = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('(')+1, sCmd[nth_Cmd][0].rfind(')')-sCmd[nth_Cmd][0].find('(')-1);
    int nElse = nJumpTable[nth_Cmd][ELSE_START];
    int nEndif = nJumpTable[nth_Cmd][BLOCK_END];
    string sLine = "";
    bPrintedStatus = false;
    value_type* v;
    int nNum = 0;

    v = evalHeader(nNum, sIf_Condition, false, nth_Cmd);

    if ((bool)v[0] && !isnan(v[0]) && !isinf(v[0]))
    {
        for (int __i = nth_Cmd; __i < nCmd; __i++)
        {
            if (__i == nElse || __i == nEndif)
                return nEndif;
            if (__i != nth_Cmd)
            {
                if (sCmd[__i][0].length())
                {
                    int nReturn = evalForkFlowCommands(__i, nth_loop);

                    if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                        return nReturn;
                    else if (nReturn == FLOWCTRL_BREAK || nReturn == FLOWCTRL_CONTINUE)
                    {
                        return nEndif;
                    }
                    else if (nReturn != FLOWCTRL_NO_CMD)
                        __i = nReturn;
                }
            }

            if (!sCmd[__i][1].length())
                continue;

            if (!nCalcType[__i])
            {
                string sCommand = findCommand(sCmd[__i][1]).sString;
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
                _parserRef->SetIndex(__i+nCmd+1);
            try
            {
                if (calc(sCmd[__i][1], __i, "IF") == FLOWCTRL_ERROR)
                    return FLOWCTRL_ERROR;
                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }
    }
    else
    {
        if (nElse == -1) // No else && no elseif
            return nEndif;
        else
            nth_Cmd = nElse;
        while (sCmd[nth_Cmd][0].find(">>elseif") != string::npos) // elseif
        {
            sIf_Condition = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('(')+1, sCmd[nth_Cmd][0].rfind(')')-sCmd[nth_Cmd][0].find('(')-1);
            nElse = nJumpTable[nth_Cmd][ELSE_START];
            nEndif = nJumpTable[nth_Cmd][BLOCK_END];
            sLine = "";
            bPrintedStatus = false;

            v = evalHeader(nNum, sIf_Condition, false, nth_Cmd);

            if ((bool)v[0] && !isnan(v[0]) && !isinf(v[0]))
            {
                for (int __i = nth_Cmd; __i < nCmd; __i++)
                {
                    if (__i == nElse || __i == nEndif)
                        return nEndif;
                    if (__i != nth_Cmd)
                    {
                        if (sCmd[__i][0].length())
                        {
                            int nReturn = evalForkFlowCommands(__i, nth_loop);

                            if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                                return nReturn;
                            else if (nReturn == FLOWCTRL_BREAK || nReturn == FLOWCTRL_CONTINUE)
                            {
                                return nEndif;
                            }
                            else if (nReturn != FLOWCTRL_NO_CMD)
                                __i = nReturn;
                        }
                    }

                    if (!sCmd[__i][1].length())
                        continue;

                    if (!nCalcType[__i])
                    {
                        string sCommand = findCommand(sCmd[__i][1]).sString;
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
                        _parserRef->SetIndex(__i+nCmd+1);

                    try
                    {
                        if (calc(sCmd[__i][1], __i, "IF") == FLOWCTRL_ERROR)
                        {
                            if (_optionRef->getUseDebugger())
                                _optionRef->_debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                            return FLOWCTRL_ERROR;
                        }
                        if (bReturnSignal)
                            return FLOWCTRL_RETURN;
                    }
                    catch (...)
                    {
                        if (_optionRef->getUseDebugger())
                            _optionRef->_debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                        throw;
                    }
                }
                return nEndif;
            }
            else
            {
                if (nElse == -1) // No else && no elseif
                    return nEndif;
                else
                    nth_Cmd = nElse;
            }
        }

        for (int __i = nth_Cmd; __i < nCmd; __i++)
        {
            if (__i == nEndif)
                return nEndif;
            if (__i != nth_Cmd)
            {
                if (sCmd[__i][0].length())
                {
                    int nReturn = evalForkFlowCommands(__i, nth_loop);

                    if (nReturn == FLOWCTRL_ERROR || nReturn == FLOWCTRL_RETURN)
                        return nReturn;
                    else if (nReturn == FLOWCTRL_BREAK || nReturn == FLOWCTRL_CONTINUE)
                    {
                        return nEndif;
                    }
                    else if (nReturn != FLOWCTRL_NO_CMD)
                        __i = nReturn;
                }
            }

            if (!sCmd[__i][1].length())
                continue;

            if (!nCalcType[__i])
            {
                string sCommand = findCommand(sCmd[__i][1]).sString;
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
                _parserRef->SetIndex(__i + nCmd+1);

            try
            {
                if (calc(sCmd[__i][1], __i, "IF") == FLOWCTRL_ERROR)
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                    return FLOWCTRL_ERROR;
                }
                if (bReturnSignal)
                    return FLOWCTRL_RETURN;
            }
            catch (...)
            {
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }
    }
    return nCmd;
}


value_type* FlowCtrl::evalHeader(int& nNum, string sHeadExpression, bool bIsForHead, int nth_Cmd)
{
    value_type* v = nullptr;
    string sCache = "";

    if (!bFunctionsReplaced)
    {
        if (!_functionRef->call(sHeadExpression, *_optionRef))
        {
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sHeadExpression, SyntaxError::invalid_position);
        }
    }
    // --> Datafile- und Cache-Konstrukte abfangen <--
    if ((sHeadExpression.find("data(") != string::npos || _dataRef->containsCacheElements(sHeadExpression))
        && (!containsStrings(sHeadExpression) && !_dataRef->containsStringVars(sHeadExpression)))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();
        if (sHeadExpression.find("data(") != string::npos && _dataRef->isValid() && !isInQuotes(sHeadExpression, sHeadExpression.find("data(")))
            parser_ReplaceEntities(sHeadExpression, "data(", *_dataRef, *_parserRef, *_optionRef, true);
        else if (sHeadExpression.find("data(") != string::npos && !_dataRef->isValid() && !isInQuotes(sHeadExpression, sHeadExpression.find("data(")))
        {
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sHeadExpression, SyntaxError::invalid_position);
        }

        if (_dataRef->containsCacheElements(sHeadExpression) && _dataRef->isValidCache())
        {
            _dataRef->setCacheStatus(true);
            for (auto iter = _dataRef->mCachesMap.begin(); iter != _dataRef->mCachesMap.end(); ++iter)
            {
                if (sHeadExpression.find(iter->first+"(") != string::npos && !isInQuotes(sHeadExpression, sHeadExpression.find(iter->first+"(")))
                {
                    parser_ReplaceEntities(sHeadExpression, iter->first+"(", *_dataRef, *_parserRef, *_optionRef, true);
                }
            }
            _dataRef->setCacheStatus(false);
        }
        else if (_dataRef->containsCacheElements(sHeadExpression) && !_dataRef->isValidCache())
        {
            throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sHeadExpression, SyntaxError::invalid_position);
        }

        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }
    // --> Ggf. Vektor-Eingaben abfangen <--
    if (sHeadExpression.find("{") != string::npos && (containsStrings(sHeadExpression) || _dataRef->containsStringVars(sHeadExpression)))
        parser_VectorToExpr(sHeadExpression, *_optionRef);
    if (sHeadExpression.find("$") != string::npos)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }
        int nReturn = procedureInterface(sHeadExpression, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nLoop+nWhile+nIf, nth_Cmd);
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
    // --> String-Auswertung einbinden <--
    if (containsStrings(sHeadExpression) || _dataRef->containsStringVars(sHeadExpression))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();
        int nReturn = parser_StringParser(sHeadExpression, sCache, *_dataRef, *_parserRef, *_optionRef, true);
        if (nReturn)
        {
            if (nReturn == 1)
            {
                StripSpaces(sHeadExpression);
                if (bIsForHead && containsStrings(sHeadExpression))
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherLoopBasedInformations(sHeadExpression, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                    throw SyntaxError(SyntaxError::CANNOT_EVAL_FOR, sHeadExpression, SyntaxError::invalid_position);
                }
                else if (containsStrings(sHeadExpression))
                {
                    StripSpaces(sHeadExpression);
                    if (sHeadExpression != "\"\"" && sHeadExpression.length() > 2)
                        sHeadExpression = "true";
                    else
                        sHeadExpression = "false";
                }
            }
        }
        else
        {
            if (_optionRef->getUseDebugger())
                _optionRef->_debug.gatherLoopBasedInformations(sHeadExpression, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
            throw SyntaxError(SyntaxError::STRING_ERROR, sHeadExpression, SyntaxError::invalid_position);
        }
        replaceLocalVars(sHeadExpression);
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }


    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parserRef->SetIndex(nth_Cmd); // i + (nCmd+1)*j
    if (bUseLoopParsingMode && !bLockedPauseMode && _parserRef->IsValidByteCode() && sHeadExpression+" " == _parserRef->GetExpr())
    {
        v = _parserRef->Eval(nNum);
    }
    else if (bUseLoopParsingMode && !bLockedPauseMode && _parserRef->IsValidByteCode() && _parserRef->GetExpr().length())
    {
        _parserRef->DeclareAsInvalid();
        _parserRef->SetExpr(sHeadExpression);
        v = _parserRef->Eval(nNum);
    }
    else
    {
        _parserRef->SetExpr(sHeadExpression);
        v = _parserRef->Eval(nNum);
    }
    return v;
}

int FlowCtrl::evalLoopFlowCommands(int __j, int nth_loop)
{
    if (nJumpTable[__j][BLOCK_END] == NO_FLOW_COMMAND)
        return FLOWCTRL_NO_CMD;

    if (sCmd[__j][0].substr(0,3) == "for")
    {
        return for_loop(__j, nth_loop+1);
    }
    else if (sCmd[__j][0].substr(0,5) == "while")
    {
        return while_loop(__j, nth_loop+1);
    }
    else if (sCmd[__j][0].find(">>if") != string::npos)
    {
        __j = if_fork(__j, nth_loop+1);
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
    return FLOWCTRL_NO_CMD;
}

int FlowCtrl::evalForkFlowCommands(int __i, int nth_loop)
{
    if (nJumpTable[__i][BLOCK_END] == NO_FLOW_COMMAND)
        return FLOWCTRL_NO_CMD;

    if (sCmd[__i][0].substr(0,3) == "for")
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
            return for_loop(__i, nth_loop+1);
    }
    else if (sCmd[__i][0].substr(0,5) == "while")
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
            return while_loop(__i, nth_loop+1);
    }
    else if (sCmd[__i][0].find(">>if") != string::npos)
    {
        __i = if_fork(__i, nth_loop+1);
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
    return FLOWCTRL_NO_CMD;
}


void FlowCtrl::setCommand(string& __sCmd, Parser& _parser, Datafile& _data, Define& _functions, Settings& _option, Output& _out, PlotData& _pData, Script& _script)
{
    bool bDebuggingBreakPoint = (__sCmd.substr(__sCmd.find_first_not_of(' '),2) == "|>");
    string sAppendedExpression = "";
    if (bDebuggingBreakPoint)
    {
        __sCmd.erase(__sCmd.find_first_not_of(' '),2);
        StripSpaces(__sCmd);
    }
    if (bReturnSignal)
    {
        bReturnSignal = false;
        //dReturnValue = 0.0;
        nReturnType = 1;
    }
    if (__sCmd == "abort")
    {
        reset();
        NumeReKernel::print(toSystemCodePage(_lang.get("LOOP_SETCOMMAND_ABORT")));
        return;
    }

    if (!sCmd)
        generateCommandArray();

    StripSpaces(__sCmd);

    string command =  findCommand(__sCmd).sString;

    if (command == "for"
        || command == "endfor"
        || command == "if"
        || command == "endif"
        || command == "else"
        || command == "elseif"
        || command == "while"
        || command == "endwhile")
    {
        if (!validateParenthesisNumber(__sCmd))
        {
            reset();
            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, __sCmd, __sCmd.find('('));
        }
        if (__sCmd.substr(0,3) == "for")
        {
            sLoopNames += ";FOR";
            nLoop++;
            // --> Pruefen wir, ob auch Grenzen eingegeben wurden <--
            if (__sCmd.find('=') == string::npos || __sCmd.find(':') == string::npos || __sCmd.find('(') == string::npos)
            {
                NumeReKernel::print(LineBreak(toSystemCodePage(_lang.get("LOOP_SUPPLY_BORDERS_AND_VAR")) + ":", _option));
                string sTemp = "";
                do
                {
                    NumeReKernel::printPreFmt("|\n|<- ");
                    NumeReKernel::getline(sTemp);
                    StripSpaces(sTemp);

                    if (sTemp.find('=') == string::npos || sTemp.find('|') == string::npos || sTemp.find('(') == string::npos)
                        sTemp = "";
                }
                while (!sTemp.length());
                __sCmd = "for " + sTemp;
            }
            // --> Schneller: ':' durch ',' ersetzen, da der Parser dann seine eigenen Mulit-Expr. einsetzen kann <--
            unsigned int nPos = __sCmd.find(':');
            if (__sCmd.find('(', __sCmd.find('(')+1) != string::npos && __sCmd.find('(', __sCmd.find('(')+1) < nPos)
            {
                nPos = getMatchingParenthesis(__sCmd.substr(__sCmd.find('(', __sCmd.find('(')+1)));
                nPos = __sCmd.find(':', nPos);
            }

            if (nPos == string::npos)
            {
                reset();
                throw SyntaxError(SyntaxError::CANNOT_EVAL_FOR, __sCmd, SyntaxError::invalid_position);
            }
            __sCmd.replace(nPos, 1, ",");
        }
        else if (__sCmd.substr(0,5) == "while")
        {
            sLoopNames += ";WHL";
            nWhile++;
            // --> Pruefen wir, ob auch eine Bedingung eingegeben wurde <--
            if (__sCmd.find('(') != string::npos)
            {
                unsigned int nPos_1 = __sCmd.find('(')+1;
                unsigned int nPos_2 = 0;
                if (__sCmd.find(')', nPos_1) != string::npos)
                {
                    nPos_2 = __sCmd.find(')', nPos_1);
                    if (!parser_ExprNotEmpty(__sCmd.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                        __sCmd = "while";
                }
                else
                    __sCmd = "while";
            }
            if (__sCmd.find('(') == string::npos)
            {
                NumeReKernel::print(LineBreak(toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option));

                string sTemp = "";
                while (sTemp.find('(') == string::npos)
                {
                    NumeReKernel::printPreFmt("|\n|<- while ");
                    NumeReKernel::getline(sTemp);
                    if (sTemp.find("while") != string::npos)
                        sTemp.erase(0,sTemp.find("while")+5);
                    if (sTemp.find('(') != string::npos)
                    {
                        unsigned int nPos_1 = sTemp.find('(')+1;
                        unsigned int nPos_2 = 0;
                        if (sTemp.find(')', nPos_1) != string::npos)
                        {
                            nPos_2 = sTemp.find(')', nPos_1);
                            if (!parser_ExprNotEmpty(sTemp.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                sTemp = "";
                        }
                        else
                            sTemp = "";
                    }
                }
                __sCmd += " " + sTemp;
            }
        }
        else if (__sCmd.substr(0,3) == "if " || __sCmd.substr(0,3) == "if(")
        {
            sLoopNames += ";IF";
            nIf++;
            // --> Pruefen wir, ob auch eine Bedingung eingegeben wurde <--
            if (__sCmd.find('(') != string::npos)
            {
                unsigned int nPos_1 = __sCmd.find('(')+1;
                unsigned int nPos_2 = 0;
                if (__sCmd.find(')', nPos_1) != string::npos)
                {
                    nPos_2 = __sCmd.find(')', nPos_1);
                    if (!parser_ExprNotEmpty(__sCmd.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                        __sCmd = "if";
                }
                else
                    __sCmd = "if";
            }
            if (__sCmd.find('(') == string::npos)
            {
                NumeReKernel::printPreFmt(LineBreak(toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option)+"\n");

                string sTemp = "";
                while (sTemp.find('(') == string::npos)
                {
                    NumeReKernel::printPreFmt("|\n|<- if ");
                    NumeReKernel::getline(sTemp);
                    if (sTemp.find("if") != string::npos)
                    {
                        sTemp.erase(0,sTemp.find("if")+2);
                    }
                    if (sTemp.find('(') != string::npos)
                    {
                        unsigned int nPos_1 = sTemp.find('(')+1;
                        unsigned int nPos_2 = 0;
                        if (sTemp.find(')', nPos_1) != string::npos)
                        {
                            nPos_2 = sTemp.find(')', nPos_1);
                            if (!parser_ExprNotEmpty(sTemp.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                sTemp = "";
                        }
                        else
                            sTemp = "";
                    }
                }
                __sCmd += " " + sTemp;
            }
            __sCmd = toString(nIf) + ">>" + __sCmd;
        }
        else if (__sCmd.substr(0,4) == "else")
        {
            if (nIf && (getCurrentBlock() == "IF" || getCurrentBlock() == "ELIF"))
            {
                if (__sCmd.substr(0,6) == "elseif")
                {
                    sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';')) + ";ELIF";
                    if (__sCmd.find('(') != string::npos)
                    {
                        unsigned int nPos_1 = __sCmd.find('(')+1;
                        unsigned int nPos_2 = 0;
                        if (__sCmd.find(')', nPos_1) != string::npos)
                        {
                            nPos_2 = __sCmd.find(')', nPos_1);
                            if (!parser_ExprNotEmpty(__sCmd.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                __sCmd = "elseif";
                        }
                        else
                            __sCmd = "elseif";
                    }
                    if (__sCmd.find('(') == string::npos)
                    {
                        NumeReKernel::printPreFmt(LineBreak(toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option)+"\n");

                        string sTemp = "";
                        while (sTemp.find('(') == string::npos)
                        {
                            NumeReKernel::printPreFmt("|\n|<- elseif ");
                            NumeReKernel::getline(sTemp);
                            if (sTemp.find("elseif") != string::npos)
                                sTemp.erase(0, sTemp.find("elseif")+6);
                            if (sTemp.find('(') != string::npos)
                            {
                                unsigned int nPos_1 = sTemp.find('(')+1;
                                unsigned int nPos_2 = 0;
                                if (sTemp.find(')', nPos_1) != string::npos)
                                {
                                    nPos_2 = sTemp.find(')', nPos_1);
                                    if (!parser_ExprNotEmpty(sTemp.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                        sTemp = "";
                                }
                                else
                                    sTemp = "";
                            }
                        }
                        __sCmd += " " + sTemp;
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
        else if (__sCmd.substr(0,5) == "endif")
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
        else if (__sCmd.substr(0,6) == "endfor")
        {
            if (nLoop && getCurrentBlock() == "FOR")
            {
                sLoopNames = sLoopNames.substr(0, sLoopNames.rfind(';'));
                nLoop--;
            }
            else
                return;
        }
        else if (__sCmd.substr(0,8) == "endwhile")
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

        if (__sCmd.find('(') != string::npos
            && (__sCmd.substr(0,3) == "for"
                || __sCmd.find(">>if") != string::npos
                || __sCmd.find(">>elseif") != string::npos
                || __sCmd.substr(0,5) == "while"))
        {
            sAppendedExpression = __sCmd.substr(getMatchingParenthesis(__sCmd)+1);
            __sCmd.erase(getMatchingParenthesis(__sCmd)+1);
        }
        else if (__sCmd.find(' ',4) != string::npos
            && __sCmd.find_first_not_of(' ', __sCmd.find(' ', 4)) != string::npos
            && __sCmd[__sCmd.find_first_not_of(' ', __sCmd.find(' ',4))] != '-')
        {
            sAppendedExpression = __sCmd.substr(__sCmd.find(' ',4));
            __sCmd.erase(__sCmd.find(' ',4));
        }
        StripSpaces(sAppendedExpression);
        StripSpaces(__sCmd);
        if (sCmd[nCmd][0].length())
            nCmd++;
        sCmd[nCmd][0] += __sCmd;

    }
    else if (__sCmd.substr(0,4) == "repl")
    {
        int nLine = 0;
        int nInput = 0;
        string sNewCommand = "";
        if (__sCmd.find('"') == string::npos)
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("LOOP_MISSING_COMMAND")));
            return;
        }
        sNewCommand = __sCmd.substr(__sCmd.find('"')+1);
        sNewCommand = sNewCommand.substr(0,sNewCommand.rfind('"'));
        StripSpaces(sNewCommand);
        if (!sNewCommand.length())
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("LOOP_MISSING_COMMAND")));
            return;
        }
        for (int i = 0; i < nCmd; i++)
        {
            if (sCmd[i][0].length())
                nLine++;
            if (sCmd[i][1].length())
                nLine++;
        }

        if (matchParams(__sCmd, "line", '='))
        {
            nInput = StrToInt(getArgAtPos(__sCmd, matchParams(__sCmd, "line", '=')+4));
        }
        if (matchParams(__sCmd, "l", '='))
        {
            nInput = StrToInt(getArgAtPos(__sCmd, matchParams(__sCmd, "l", '=')+1));
        }

        if (nInput < 0)
        {
            nLine += nInput+1;
            if (nLine < 0)
            {
                NumeReKernel::print(toSystemCodePage(_lang.get("LOOP_LINE_NOT_EXISTENT")));
                return;
            }
        }
        else if (nInput-1 > nLine)
        {
            NumeReKernel::print(toSystemCodePage(_lang.get("LOOP_LINE_NOT_EXISTENT")));
            return;
        }
        else
            nLine = nInput-1;



        for (int i = 0; i < nCmd; i++)
        {
            if (!nLine)
            {
                nLine = i;
                nInput = 0;
                break;
            }
            if (sCmd[i][0].length())
                nLine--;
            if (!nLine)
            {
                nLine = i;
                nInput = 0;
                break;
            }
            if (sCmd[i][1].length())
                nLine--;
            if (!nLine)
            {
                nLine = i;
                nInput = 1;
                break;
            }
        }
        if (nInput)
        {
            sCmd[nLine][1] = sNewCommand;
        }
        else
        {
            if (sNewCommand.substr(0,3) == "for"
                || sNewCommand.substr(0,2) == "if"
                || sNewCommand.substr(0,5) == "while")
            {
                if (sNewCommand.substr(0,3) == "for")
                {
                    //sLoopNames += ";FOR";
                    //nLoop++;
                    // --> Pruefen wir, ob auch Grenzen eingegeben wurden <--
                    if (sNewCommand.find('=') == string::npos)
                    {
                        NumeReKernel::print(LineBreak(toSystemCodePage(_lang.get("LOOP_SUPPLY_BORDERS_AND_VAR")) + ":", _option));
                        string sTemp = "";
                        do
                        {
                            NumeReKernel::printPreFmt("|\n|<- ");
                            NumeReKernel::getline(sTemp);
                            StripSpaces(sTemp);

                            if (sTemp.find('=') == string::npos)
                                sTemp = "";
                        }
                        while (!sTemp.length());
                        sNewCommand = "for " + sTemp;
                    }
                    // --> Schneller: ':' durch ',' ersetzen, da der Parser dann seine eigenen Mulit-Expr. einsetzen kann <--
                    for (unsigned int i = 3; i < __sCmd.length(); i++)
                    {
                        if (__sCmd[i] != ' ')
                        {
                            if (__sCmd[i] == '(' && __sCmd[__sCmd.length()-1] == ')')
                            {
                                __sCmd[i] = ' ';
                                __sCmd = __sCmd.substr(0,__sCmd.length()-1);
                            }
                            break;
                        }
                    }
                    unsigned int nPos = sNewCommand.find(':');
                    if (sNewCommand.find('(') != string::npos && sNewCommand.find('(') < nPos)
                    {
                        nPos = getMatchingParenthesis(sNewCommand.substr(sNewCommand.find('(')));
                        nPos = sNewCommand.find(':', nPos);
                    }
                    sNewCommand.replace(nPos, 1, ",");
                }
                else if (sNewCommand.substr(0,2) == "if")
                {
                    //sLoopNames += ";IF";
                    //nIf++;
                    // --> Pruefen wir, ob auch eine Bedingung eingegeben wurde <--
                    if (sNewCommand.find('(') != string::npos)
                    {
                        unsigned int nPos_1 = sNewCommand.find('(')+1;
                        unsigned int nPos_2 = 0;
                        if (sNewCommand.find(')', nPos_1) != string::npos)
                        {
                            nPos_2 = sNewCommand.find(')', nPos_1);
                            if (!parser_ExprNotEmpty(sNewCommand.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                sNewCommand = "if";
                        }
                        else
                            sNewCommand = "if";
                    }
                    if (sNewCommand.find('(') == string::npos)
                    {
                        NumeReKernel::print(LineBreak(toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option));

                        string sTemp = "";
                        while (sTemp.find('(') == string::npos)
                        {
                            NumeReKernel::printPreFmt("|\n|<- if ");
                            NumeReKernel::getline(sTemp);
                            if (sTemp.find("if") != string::npos)
                                sTemp.erase(0, sTemp.find("if")+2);
                            if (sTemp.find('(') != string::npos)
                            {
                                unsigned int nPos_1 = sTemp.find('(')+1;
                                unsigned int nPos_2 = 0;
                                if (sTemp.find(')', nPos_1) != string::npos)
                                {
                                    nPos_2 = sTemp.find(')', nPos_1);
                                    if (!parser_ExprNotEmpty(sTemp.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                        sTemp = "";
                                }
                                else
                                    sTemp = "";
                            }
                        }
                        sNewCommand += " " + sTemp;
                    }
                    sNewCommand = toString(nIf) + ">>" + sNewCommand;
                }
                else if (sNewCommand.substr(0,5) == "while")
                {
                    // --> Pruefen wir, ob auch eine Bedingung eingegeben wurde <--
                    if (sNewCommand.find('(') != string::npos)
                    {
                        unsigned int nPos_1 = sNewCommand.find('(')+1;
                        unsigned int nPos_2 = 0;
                        if (sNewCommand.find(')', nPos_1) != string::npos)
                        {
                            nPos_2 = sNewCommand.find(')', nPos_1);
                            if (!parser_ExprNotEmpty(sNewCommand.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                sNewCommand = "while";
                        }
                        else
                            sNewCommand = "while";
                    }
                    if (sNewCommand.find('(') == string::npos)
                    {
                        NumeReKernel::print(LineBreak(toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option));

                        string sTemp = "";
                        while (sTemp.find('(') == string::npos)
                        {
                            NumeReKernel::printPreFmt("|\n|<- while ");
                            NumeReKernel::getline(sTemp);
                            if (sTemp.find("while") != string::npos)
                                sTemp.erase(0, sTemp.find("while")+5);
                            if (sTemp.find('(') != string::npos)
                            {
                                unsigned int nPos_1 = sTemp.find('(')+1;
                                unsigned int nPos_2 = 0;
                                if (sTemp.find(')', nPos_1) != string::npos)
                                {
                                    nPos_2 = sTemp.find(')', nPos_1);
                                    if (!parser_ExprNotEmpty(sTemp.substr(nPos_1, nPos_2-nPos_1)) || nPos_1 == nPos_2)
                                        sTemp = "";
                                }
                                else
                                    sTemp = "";
                            }
                        }
                        sNewCommand += " " + sTemp;
                    }
                }
                else
                    return;

                sCmd[nLine][0] = sNewCommand;
            }
            else
                return;
        }
    }
    else
    {
        if (__sCmd.find(" for ") != string::npos
            || __sCmd.find(" for(") != string::npos
            || __sCmd.find(" endfor") != string::npos
            || __sCmd.find(" if ") != string::npos
            || __sCmd.find(" if(") != string::npos
            || __sCmd.find(" else") != string::npos
            || __sCmd.find(" elseif ") != string::npos
            || __sCmd.find(" elseif(") != string::npos
            || __sCmd.find(" endif") != string::npos
            || __sCmd.find(" while ") != string::npos
            || __sCmd.find(" while(") != string::npos
            || __sCmd.find(" endwhile") != string::npos)
        {
            for (unsigned int n = 0; n < __sCmd.length(); n++)
            {
                if (__sCmd[n] == ' ' && !isInQuotes(__sCmd, n))
                {
                    if (__sCmd.substr(n,5) == " for "
                        || __sCmd.substr(n,5) == " for("
                        || __sCmd.substr(n,7) == " endfor"
                        || __sCmd.substr(n,4) == " if "
                        || __sCmd.substr(n,4) == " if("
                        || __sCmd.substr(n,5) == " else"
                        || __sCmd.substr(n,8) == " elseif "
                        || __sCmd.substr(n,8) == " elseif("
                        || __sCmd.substr(n,6) == " endif"
                        || __sCmd.substr(n,7) == " while "
                        || __sCmd.substr(n,7) == " while("
                        || __sCmd.substr(n,9) == " endwhile")
                    {
                        sAppendedExpression = __sCmd.substr(n+1);
                        __sCmd.erase(n);
                        break;
                    }
                }
            }
        }
        if (bDebuggingBreakPoint)
            __sCmd.insert(0,"|> ");
        sCmd[nCmd][1] = __sCmd;
        nCmd++;
    }

    if (nCmd+1 == nDefaultLength)
        generateCommandArray();
    if (nCmd)
    {
        if (!(nLoop+nIf+nWhile) && sCmd[nCmd][0].length())
            eval(_parser, _data, _functions, _option, _out, _pData, _script);
    }

    if (sAppendedExpression.length())
        setCommand(sAppendedExpression, _parser, _data, _functions, _option, _out, _pData, _script);
    return;
}

// --> Alle Variablendefinitionen und Deklarationen und den Error-Handler! <--
void FlowCtrl::eval(Parser& _parser, Datafile& _data, Define& _functions, Settings& _option, Output& _out, PlotData& _pData, Script& _script)
{
    if (_parser.IsLockedPause())
        bLockedPauseMode = true;
    nReturnType = 1;
    bBreakSignal = false;
    bContinueSignal = false;
    ReturnVal.vNumVal.clear();
    ReturnVal.vStringVal.clear();
    string sVars = ";";
    string sVar = "";
    sVarName = "";
    dVarAdress = 0;
    bUseLoopParsingMode = false;
    bFunctionsReplaced = false;
    bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;

    _parserRef = &_parser;
    _dataRef = &_data;
    _outRef = &_out;
    _optionRef = &_option;
    _functionRef = &_functions;
    _pDataRef = &_pData;
    _scriptRef = &_script;


    for (int i = 0; i <= nCmd; i++)
    {
        //nValidByteCode[i] = -1;
        if (!sCmd[i][0].length())
            continue;
        if (sCmd[i][0].substr(0,3) == "for")
        {
            sVar = sCmd[i][0].substr(sCmd[i][0].find('(')+1);
            sVar = sVar.substr(0, sVar.find('='));
            StripSpaces(sVar);
            if (sVars.find(";" + sVar + ";") == string::npos)
            {
                sVars += sVar + ";";
                nVarArray++;
            }
            sCmd[i][0][sCmd[i][0].find('(')] = ' ';
            sCmd[i][0].pop_back();
        }
        if ((sCmd[i][0].substr(0,6) == "endfor" || sCmd[i][0].substr(0,8) == "endwhile" || sCmd[i][0].find(">>endif") != string::npos) && matchParams(sCmd[i][0], "sv"))
            bSilent = false;
        if ((sCmd[i][0].substr(0,6) == "endfor" || sCmd[i][0].substr(0,8) == "endwhile" || sCmd[i][0].find(">>endif") != string::npos) && matchParams(sCmd[i][0], "mask"))
            bMask = true;
        if ((sCmd[i][0].substr(0,6) == "endfor" || sCmd[i][0].substr(0,8) == "endwhile" || sCmd[i][0].find(">>endif") != string::npos) && matchParams(sCmd[i][0], "lnumctrl"))
            nLoopSavety = 1000;
        if ((sCmd[i][0].substr(0,6) == "endfor" || sCmd[i][0].substr(0,8) == "endwhile" || sCmd[i][0].find(">>endif") != string::npos) && matchParams(sCmd[i][0], "lnumctrl", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd[i][0], matchParams(sCmd[i][0], "lnumctrl",'=')+8));
            nLoopSavety = (int)_parser.Eval();
            if (nLoopSavety <= 0)
                nLoopSavety = 1000;
        }
    }
    if (!_option.getSystemPrintStatus())
        bMask = true;

    nCalcType = new int[nCmd+1];
    for (int i = 0; i < nCmd+1; i++)
        nCalcType[i] = CALCTYPE_NONE;

    nJumpTableLength = nCmd+1;
    nJumpTable = new int*[nJumpTableLength];
    for (unsigned int i = 0; i < nJumpTableLength; i++)
    {
        nJumpTable[i] = new int[3];
        nJumpTable[i][BLOCK_END] = NO_FLOW_COMMAND;
        nJumpTable[i][ELSE_START] = NO_FLOW_COMMAND;
        nJumpTable[i][PROCEDURE_INTERFACE] = NO_FLOW_COMMAND;
    }

    for (int i = 0; i <= nCmd; i++)
    {
        if (sCmd[i][0].length())
        {
            //cerr << sCmd[i][0] << endl;
            if (sCmd[i][0].substr(0,3) == "for")
            {
                int nForCount = 0;
                for (int j = i+1; j <= nCmd; j++)
                {
                    if (sCmd[j][0].length() && sCmd[j][0].substr(0,6) == "endfor")
                    {
                        if (nForCount)
                            nForCount--;
                        else
                        {
                            nJumpTable[i][BLOCK_END] = j;
                            if (!bUseLoopParsingMode /*&& j-i > 1*/ && !bLockedPauseMode)
                                bUseLoopParsingMode = true;
                            break;
                        }
                    }
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0,3) == "for")
                        nForCount++;
                }
            }
            else if (sCmd[i][0].substr(0,5) == "while")
            {
                int nWhileCount = 0;
                for (int j = i+1; j <= nCmd; j++)
                {
                    if (sCmd[j][0].length() && sCmd[j][0].substr(0,8) == "endwhile")
                    {
                        if (nWhileCount)
                            nWhileCount--;
                        else
                        {
                            nJumpTable[i][BLOCK_END] = j;
                            if (!bUseLoopParsingMode /*&& j-i > 1*/ && !bLockedPauseMode)
                                bUseLoopParsingMode = true;
                            break;
                        }
                    }
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0,5) == "while")
                        nWhileCount++;
                }
            }
            else if (sCmd[i][0].find(">>if") != string::npos)
            {
                string sNth_If = sCmd[i][0].substr(0, sCmd[i][0].find(">>"));
                for (int j = i+1; j <= nCmd; j++)
                {
                    if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>else").length()) == sNth_If + ">>else" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>elseif").length()) == sNth_If + ">>elseif" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>endif").length()) == sNth_If + ">>endif")
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
            else if (sCmd[i][0].find(">>elseif") != string::npos)
            {
                string sNth_If = sCmd[i][0].substr(0, sCmd[i][0].find(">>"));
                for (int j = i+1; j <= nCmd; j++)
                {
                    if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>else").length()) == sNth_If + ">>else" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>elseif").length()) == sNth_If + ">>elseif" && nJumpTable[i][ELSE_START] == NO_FLOW_COMMAND)
                        nJumpTable[i][ELSE_START] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>endif").length()) == sNth_If + ">>endif")
                    {
                        nJumpTable[i][BLOCK_END] = j;
                        break;
                    }
                }
            }
        }
        if (sCmd[i][1].length())
        {
            sCmd[i][1] += " ";
            // Könnte Schwierigkeiten machen
            if (findCommand(sCmd[i][1]).sString != "matop"
                && findCommand(sCmd[i][1]).sString != "mtrxop"
                && (sCmd[i][1].find("+=") != string::npos
                    || sCmd[i][1].find("-=") != string::npos
                    || sCmd[i][1].find("*=") != string::npos
                    || sCmd[i][1].find("/=") != string::npos
                    || sCmd[i][1].find("^=") != string::npos
                    || sCmd[i][1].find("++") != string::npos
                    || sCmd[i][1].find("--") != string::npos))
            {
                bool bBreakPoint = (sCmd[i][1].substr(sCmd[i][1].find_first_not_of(" \t"),2) == "|>");
                if (bBreakPoint)
                {
                    sCmd[i][1].erase(sCmd[i][1].find_first_not_of(" \t"),2);
                    StripSpaces(sCmd[i][1]);
                }
                evalRecursiveExpressions(sCmd[i][1]);
                if (bBreakPoint)
                    sCmd[i][1].insert(0,"|> ");
            }
            StripSpaces(sCmd[i][1]);
        }
    }
    try
    {
        if (bUseLoopParsingMode)
        {
            for (int i = 0; i <= nCmd; i++)
            {
                if (sCmd[i][0].find('$') != string::npos)
                {
                    if (!isInline(sCmd[i][0]))
                    {
                        bUseLoopParsingMode = false;
                        break;
                    }
                }
                if (sCmd[i][1].find('$') != string::npos)
                {
                    if (!isInline(sCmd[i][1]))
                    {
                        bUseLoopParsingMode = false;
                        break;
                    }
                }
                if (sCmd[i][0].find("to_cmd(") != string::npos)
                {
                    bUseLoopParsingMode = false;
                    break;
                }
                if (sCmd[i][1].find("to_cmd(") != string::npos)
                {
                    bUseLoopParsingMode = false;
                    break;
                }
            }
            bool bDefineCommands = false;
            for (int i = 0; i <= nCmd; i++)
            {
                //cerr << findCommand(sCmd[i][1]).sString << "|" << endl;
                if (findCommand(sCmd[i][1]).sString == "define"
                    || findCommand(sCmd[i][1]).sString == "taylor"
                    || findCommand(sCmd[i][1]).sString == "spline"
                    || findCommand(sCmd[i][1]).sString == "redefine"
                    || findCommand(sCmd[i][1]).sString == "redef"
                    || findCommand(sCmd[i][1]).sString == "undefine"
                    || findCommand(sCmd[i][1]).sString == "undef"
                    || findCommand(sCmd[i][1]).sString == "ifndefined"
                    || findCommand(sCmd[i][1]).sString == "ifndef")
                {
                    bDefineCommands = true;
                    break;
                }
            }
            if (!bDefineCommands)
            {
                for (int i = 0; i <= nCmd; i++)
                {
                    if (!_functions.call(sCmd[i][0], _option))
                    {
                        reset();
                        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd[i][0], SyntaxError::invalid_position);
                    }
                    if (!_functions.call(sCmd[i][1], _option))
                    {
                        reset();
                        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd[i][1], SyntaxError::invalid_position);
                    }
                    StripSpaces(sCmd[i][0]);
                }
                bFunctionsReplaced = true;
            }
        }
    }
    catch (...)
    {
        reset();
        NumeReKernel::bSupressAnswer = bSupressAnswer_back;
        throw;
    }

    if (bSilent)
        bLoopSupressAnswer = true;

    vVarArray = new value_type*[nVarArray];
    sVarArray = new string[nVarArray];
    sVars = sVars.substr(1,sVars.length()-1);

    for (int i = 0; i < nVarArray; i++)
    {
        vVarArray[i] = new value_type[3];
        sVarArray[i] = sVars.substr(0,sVars.find(';'));
        if (i+1 != nVarArray)
            sVars = sVars.substr(sVars.find(';')+1);
        StripSpaces(sVarArray[i]);
        if (parser_GetVarAdress(sVarArray[i], _parser))
            vVars[sVarArray[i]] = parser_GetVarAdress(sVarArray[i], _parser);
        else
        {
            mVarMap[sVarArray[i]] = "_~LOOP_"+sVarArray[i]+"_"+toString(nthRecursion);
            sVarArray[i] = mVarMap[sVarArray[i]];
        }
        _parser.DefineVar(sVarArray[i], &vVarArray[i][0]);
    }
    //cerr << "Pointer" << endl;
    _parser.mVarMapPntr = &mVarMap;
    for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
    {
        for (int i = 0; i <= nCmd; i++)
        {
            if (sCmd[i][0].length())
            {
                sCmd[i][0] += " ";
                for (unsigned int j = 0; j < sCmd[i][0].length(); j++)
                {
                    if (sCmd[i][0].substr(j,(iter->first).length()) == iter->first)
                    {
                        if (((!j && checkDelimiter(" "+sCmd[i][0].substr(j,(iter->first).length()+1)))
                                || (j && checkDelimiter(sCmd[i][0].substr(j-1, (iter->first).length()+2))))
                            && !isInQuotes(sCmd[i][0], j, true))
                        {
                            sCmd[i][0].replace(j,(iter->first).length(), iter->second);
                            j += (iter->second).length()-(iter->first).length();
                        }
                        //cerr << sCmd[i][0] << endl;
                    }
                }
            }
            if (sCmd[i][1].length())
            {
                sCmd[i][1] += " ";
                for (unsigned int j = 0; j < sCmd[i][1].length(); j++)
                {
                    if (sCmd[i][1].substr(j,(iter->first).length()) == iter->first)
                    {
                        if (((!j && checkDelimiter(" "+sCmd[i][1].substr(j,(iter->first).length()+1)))
                                || (j && checkDelimiter(sCmd[i][1].substr(j-1, (iter->first).length()+2))))
                            && !isInQuotes(sCmd[i][1], j, true))
                        {
                            sCmd[i][1].replace(j,(iter->first).length(), iter->second);
                            j += (iter->second).length()-(iter->first).length();
                        }
                    }
                }
                // Könnte Schwierigkeiten machen
                /*if (sCmd[i][1].find("+=") != string::npos
                    || sCmd[i][1].find("-=") != string::npos
                    || sCmd[i][1].find("*=") != string::npos
                    || sCmd[i][1].find("/=") != string::npos
                    || sCmd[i][1].find("^=") != string::npos
                    || sCmd[i][1].find("++") != string::npos
                    || sCmd[i][1].find("--") != string::npos)
                {
                    bool bBreakPoint = (sCmd[i][1].substr(sCmd[i][1].find_first_not_of(" \t"),2) == "|>");
                    if (bBreakPoint)
                    {
                        sCmd[i][1].erase(sCmd[i][1].find_first_not_of(" \t"),2);
                        StripSpaces(sCmd[i][1]);
                    }
                    evalRecursiveExpressions(sCmd[i][1]);
                    if (bBreakPoint)
                        sCmd[i][1].insert(0,"|> ");
                }*/
                StripSpaces(sCmd[i][1]);
            }
        }
    }

    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parser.ActivateLoopMode((unsigned int)(2*(nCmd+1)));

    try
    {
        if (sCmd[0][0].substr(0,3) == "for")
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
        else if (sCmd[0][0].substr(0,5) == "while")
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
        else
        {
            if (if_fork() == FLOWCTRL_ERROR)
            {
                if (bSilent || bMask)
                    NumeReKernel::printPreFmt("\n");
                throw SyntaxError(SyntaxError::CANNOT_EVAL_IF, "", SyntaxError::invalid_position);
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
    reset();

    if (bLoopSupressAnswer)
        bLoopSupressAnswer = false;
    return;
}

// function will be executed multiple times -> take care of the pointers!
void FlowCtrl::reset()
{
    if (sCmd)
    {
        for (int i = 0; i < nDefaultLength; i++)
        {
            delete[] sCmd[i];
        }
        delete[] sCmd;
        sCmd = nullptr;
    }

    if (vVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
            _parserRef->RemoveVar(sVarArray[i]);
    }
    if (mVarMap.size() && nVarArray)
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

    nVarArray = 0;
    nCmd = 0;
    nLoop = 0;
    nLoopSavety = -1;
    nIf = 0;
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
    if (bUseLoopParsingMode && !bLockedPauseMode)
    {
        _parserRef->DeactivateLoopMode();
        bUseLoopParsingMode = false;
    }
    bLockedPauseMode = false;
    bFunctionsReplaced = false;
    if (_parserRef)
        _parserRef->ClearVectorVars();

    _parserRef = nullptr;
    _dataRef = nullptr;
    _outRef = nullptr;
    _optionRef = nullptr;
    _functionRef = nullptr;
    _pDataRef = nullptr;
    _scriptRef = nullptr;

    return;
}

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

    int nCurrentCalcType = nCalcType[nthCmd];

    // Eval the debugger breakpoint first
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_DEBUGBREAKPOINT)
    {
        if (sLine.substr(sLine.find_first_not_of(' '),2) == "|>")
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_DEBUGBREAKPOINT;
            sLine.erase(sLine.find_first_not_of(' '),2);
            sLine_Temp.erase(sLine_Temp.find_first_not_of(' '),2);
            StripSpaces(sLine);
            StripSpaces(sLine_Temp);
            if (_optionRef->getUseDebugger())
            {
                _optionRef->_debug.gatherLoopBasedInformations(sLine, nCmd - nthCmd, mVarMap, vVarArray, sVarArray, nVarArray);
                evalDebuggerBreakPoint(*_parserRef, *_optionRef, _dataRef->getStringVars());
            }
        }
    }

    if (!nCurrentCalcType
        || !bFunctionsReplaced
        || nCurrentCalcType & CALCTYPE_COMMAND
        || nCurrentCalcType & CALCTYPE_DEFINITION
        || nCurrentCalcType & CALCTYPE_PROGRESS
        || nCurrentCalcType & CALCTYPE_COMPOSE
        || nCurrentCalcType & CALCTYPE_RETURNCOMMAND
        || nCurrentCalcType & CALCTYPE_THROWCOMMAND
        || nCurrentCalcType & CALCTYPE_EXPLICIT)
        sCommand = findCommand(sLine).sString;


    if (!nCurrentCalcType || !(nCurrentCalcType & CALCTYPE_DEFINITION) || !bFunctionsReplaced)
    {
        //cerr << sLine << endl;
        if (!bFunctionsReplaced
            && sCommand != "define"
            && sCommand != "redef"
            && sCommand != "redefine"
            && sCommand != "undefine"
            && sCommand != "undef"
            && sCommand != "ifndef"
            && sCommand != "ifndefined")
        {
            if (!_functionRef->call(sLine, *_optionRef))
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
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_THROWCOMMAND)
    {
        if (sCommand == "throw" || sLine == "throw")
        {
            string sErrorToken;
            if (sLine.length() > 6 && (containsStrings(sLine) || _dataRef->containsStringVars(sLine)))
            {
                if (_dataRef->containsStringVars(sLine))
                    _dataRef->getStringValues(sLine);
                getStringArgument(sLine, sErrorToken);
                sErrorToken += " -nq";
                parser_StringParser(sErrorToken, sCache, *_dataRef, *_parserRef, *_optionRef, true);
            }
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_THROWCOMMAND;
            throw SyntaxError(SyntaxError::LOOP_THROW, sLine, SyntaxError::invalid_position, sErrorToken);
        }
    }
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_RETURNCOMMAND)
    {
        if (sCommand == "return")
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_RETURNCOMMAND;
            if (sLine.find("void", sLine.find("return")+6) != string::npos)
            {
                string sReturnValue = sLine.substr(sLine.find("return")+6);
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
    if (NumeReKernel::GetAsyncCancelState())
    {
        if (bPrintedStatus)
            NumeReKernel::printPreFmt(" ABBRUCH!");

        throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
    }
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_NUMERICAL)
    {
        if (_parserRef->IsValidByteCode() == 1 && _parserRef->GetExpr() == sLine + " " && !bLockedPauseMode && bUseLoopParsingMode)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_NUMERICAL;
            if (!bSilent)
            {
                /* --> Der Benutzer will also die Ergebnisse sehen. Es gibt die Moeglichkeit,
                 *     dass der Parser mehrere Ausdruecke je Zeile auswertet. Dazu muessen die
                 *     Ausdruecke durch Kommata getrennt sein. Damit der Parser bemerkt, dass er
                 *     mehrere Ausdruecke auszuwerten hat, muss man die Auswerte-Funktion des
                 *     Parsers einmal aufgerufen werden <--
                 */
                v = _parserRef->Eval(nNum);
                string sAns = "|" + sBlock + "> " + sLine_Temp + " => ";
                // --> Ist die Zahl der Ausdruecke groesser als 1? <--
                if (nNum > 1)
                {
                    // --> Gebe die nNum Ergebnisse durch Kommata getrennt aus <--
                    sAns += "{";
                    vAns = v[0];
                    for (int i = 0; i < nNum; ++i)
                    {
                        sAns += toString(v[i], *_optionRef);
                        if (i < nNum-1)
                            sAns += ", ";
                    }
                    NumeReKernel::printPreFmt(sAns + "}\n");
                }
                else
                {
                    // --> Zahl der Ausdruecke gleich 1? Dann einfach ausgeben <--
                    vAns = v[0];
                    NumeReKernel::printPreFmt(sAns + toString(vAns, *_optionRef) + "\n");
                }
            }
            else
            {
                /* --> Hier ist bSilent = TRUE: d.h., werte die Ausdruecke im Stillen aus,
                 *     ohne dass der Benutzer irgendetwas davon mitbekommt <--
                 */
                v = _parserRef->Eval(nNum);
                if (nNum)
                    vAns = v[0];
            }
            return FLOWCTRL_OK;
        }
    }
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
                if (sCommand.substr(0,4) == "plot"
                    || sCommand.substr(0,4) == "grad"
                    || sCommand.substr(0,4) == "dens"
                    || sCommand.substr(0,4) == "vect"
                    || sCommand.substr(0,4) == "cont"
                    || sCommand.substr(0,4) == "surf"
                    || sCommand.substr(0,4) == "mesh")
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
                string sCmdString = sLine.substr(nPos+1, nParPos-1);
                StripSpaces(sCmdString);
                if (containsStrings(sCmdString) || _dataRef->containsStringVars(sCmdString))
                {
                    sCmdString += " -nq";
                    parser_StringParser(sCmdString, sCache, *_dataRef, *_parserRef, *_optionRef, true);
                    sCache = "";
                }
                sLine = sLine.substr(0, nPos-6) + sCmdString + sLine.substr(nPos + nParPos+1);
                nPos -= 5;
            }
            replaceLocalVars(sLine);
            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode(false);
            sCommand = findCommand(sLine).sString;
        }
    }
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
            if (containsStrings(sLine) || _dataRef->containsStringVars(sLine))
            {
                if (bUseLoopParsingMode && !bLockedPauseMode)
                    _parserRef->PauseLoopMode();
                sLine = BI_evalParamString(sLine, *_parserRef, *_dataRef, *_optionRef, *_functionRef);
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
                if (matchParams(sArgument, "first", '='))
                {
                    sExpr = getArgAtPos(sArgument, matchParams(sArgument, "first", '=')+5) + ",";
                }
                else
                    sExpr = "1,";
                if (matchParams(sArgument, "last", '='))
                {
                    sExpr += getArgAtPos(sArgument, matchParams(sArgument, "last", '=')+4);
                }
                else
                    sExpr += "100";
                if (matchParams(sArgument, "type", '='))
                {
                    sArgument = getArgAtPos(sArgument, matchParams(sArgument, "type", '=')+4);
                }
                else
                    sArgument = "std";
            }
            else
            {
                sArgument = "std";
                sExpr = "1,100";
            }
            while (sLine.length() && (sLine[sLine.length()-1] == ' ' || sLine[sLine.length()-1] == '-'))
                sLine.pop_back();
            if (!sLine.length())
                return FLOWCTRL_OK;
            if (_parserRef->GetExpr() != sLine.substr(findCommand(sLine).nPos+8)+","+sExpr+" ")
            {
                if (bUseLoopParsingMode && !bLockedPauseMode && _parserRef->IsValidByteCode() && _parserRef->GetExpr().length())
                    _parserRef->DeclareAsInvalid();
                _parserRef->SetExpr(sLine.substr(findCommand(sLine).nPos+8)+","+sExpr);
            }
            vVals = _parserRef->Eval(nArgument);
            make_progressBar((int)vVals[0], (int)vVals[1], (int)vVals[2], sArgument);
            return FLOWCTRL_OK;
        }
    }
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_PROMPT)
    {
        // --> Prompt <--
        if (sLine.find("??") != string::npos)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_PROMPT | CALCTYPE_PROCEDURECMDINTERFACE | CALCTYPE_COMMAND | CALCTYPE_DATAACCESS | CALCTYPE_STRING | CALCTYPE_RECURSIVEEXPRESSION;
            if (bPrintedStatus)
                NumeReKernel::printPreFmt("\n");
            sLine = parser_Prompt(sLine);
            bPrintedStatus = false;
        }
    }
    // --> Prozeduren einbinden <--
    if (nJumpTable[nthCmd][PROCEDURE_INTERFACE])
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parserRef->PauseLoopMode();
            _parserRef->LockPause();
        }
        int nReturn = procedureInterface(sLine, *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nLoop+nWhile+nIf, nthCmd);
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

    // --> Procedure-CMDs einbinden <--
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
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_EXPLICIT)
    {
        if (sCommand == "explicit")
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_EXPLICIT;
            sLine.erase(findCommand(sLine).nPos,8);
            StripSpaces(sLine);
        }
    }
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_COMMAND)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode();
        {
            bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;
            string sPreCommandLine = sLine;
            NumeReKernel::bSupressAnswer = bLoopSupressAnswer;
            switch (BI_CommandHandler(sLine, *_dataRef, *_outRef, *_optionRef, *_parserRef, *_functionRef, *_pDataRef, *_scriptRef, true))
            {
                case  0:
                    if (!nCurrentCalcType)
                    {
                        StripSpaces(sPreCommandLine);
                        string sCurrentLine = sLine;
                        StripSpaces(sCurrentLine);
                        if (sPreCommandLine != sCurrentLine)
                            nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                    }
                    break;
                case  1:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                    if (!nCurrentCalcType)
                        nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                    return FLOWCTRL_OK;
                case -1:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                    if (!nCurrentCalcType)
                        nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                    return FLOWCTRL_OK;
                case  2:
                    NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                    if (!nCurrentCalcType)
                        nCalcType[nthCmd] |= CALCTYPE_COMMAND;
                    return FLOWCTRL_OK;
            }
            NumeReKernel::bSupressAnswer = bSupressAnswer_back;
        }
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parserRef->PauseLoopMode(false);
    }
    // --> Prompt <--
    /*if (sLine.find("??") != string::npos)
    {
        if (bPrintedStatus)
            NumeReKernel::printPreFmt("\n");
        sLine = parser_Prompt(sLine);
        bPrintedStatus = false;
    }*/

    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_RECURSIVEEXPRESSION)
        evalRecursiveExpressions(sLine);

    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_DATAACCESS)
    {
        // --> Datafile/Cache! <--
        if (!containsStrings(sLine)
            && !_dataRef->containsStringVars(sLine)
            && (sLine.find("data(") != string::npos || _dataRef->containsCacheElements(sLine)))
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_DATAACCESS;
            if (!_parserRef->HasCachedAccess() && _parserRef->CanCacheAccess())
            {
                bCompiling = true;
                _parserRef->SetCompiling(true);
            }
            /*if (!bLockedPauseMode && bUseLoopParsingMode)
                _parser.PauseLoopMode();*/
            sCache = parser_GetDataElement(sLine, *_parserRef, *_dataRef, *_optionRef);
            if (sCache.length() && sCache.find('#') == string::npos)
                bWriteToCache = true;

            if (_parserRef->IsCompiling())
            {
                _parserRef->CacheCurrentEquation(sLine);
                _parserRef->CacheCurrentTarget(sCache);
            }
            _parserRef->SetCompiling(false);
            /*if (!bLockedPauseMode && bUseLoopParsingMode)
                _parser.PauseLoopMode(false);*/
        }
    }
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_STRING)
    {
        // --> String-Parser <--
        if (containsStrings(sLine) || _dataRef->containsStringVars(sLine))
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_STRING | CALCTYPE_DATAACCESS;
            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode();
            int nReturn = parser_StringParser(sLine, sCache, *_dataRef, *_parserRef, *_optionRef, bSilent);
            if (nReturn)
            {
                if (nReturn == 1)
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
            }
            else
            {
                throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
            }
            replaceLocalVars(sLine);
            if (sCache.length() && _dataRef->containsCacheElements(sCache) && !bWriteToCache)
                bWriteToCache = true;
            if (!bLockedPauseMode && bUseLoopParsingMode)
                _parserRef->PauseLoopMode(false);
        }
    }
    if (!nCurrentCalcType || nCurrentCalcType & CALCTYPE_DATAACCESS)
    {
        if (bWriteToCache)
        {
            if (!nCurrentCalcType)
                nCalcType[nthCmd] |= CALCTYPE_DATAACCESS;
            /*if (!bLockedPauseMode && bUseLoopParsingMode)
                _parser.PauseLoopMode();*/

            if (bCompiling)
            {
                _parserRef->SetCompiling(true);
                _idx = parser_getIndices(sCache, *_parserRef, *_dataRef, *_optionRef);
                if ((_idx.nI[0] < 0 || _idx.nJ[0] < 0) && !_idx.vI.size() && !_idx.vJ.size())
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "");
                if ((_idx.nI[1] == -2 && _idx.nJ[1] == -2))
                    throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");
                sCache.erase(sCache.find('('));
                StripSpaces(sCache);
                _parserRef->CacheCurrentTarget(sCache + "(" + _idx.sCompiledAccessEquation + ")");
                _parserRef->SetCompiling(false);
            }
            else
            {
                _idx = parser_getIndices(sCache, *_parserRef, *_dataRef, *_optionRef);
                if ((_idx.nI[0] < 0 || _idx.nJ[0] < 0) && !_idx.vI.size() && !_idx.vJ.size())
                    throw SyntaxError(SyntaxError::INVALID_INDEX, sCache, "");
                if ((_idx.nI[1] == -2 && _idx.nJ[1] == -2))
                    throw SyntaxError(SyntaxError::NO_MATRIX, sCache, "");
                sCache.erase(sCache.find('('));
                StripSpaces(sCache);
            }

            if (_idx.nI[1] == -1)
                _idx.nI[1] = _idx.nI[0];
            if (_idx.nJ[1] == -1)
                _idx.nJ[1] = _idx.nJ[0];

            /*if (!bLockedPauseMode && bUseLoopParsingMode)
                _parser.PauseLoopMode(false);*/
        }
    }
    // --> Vector-Ausdruck <--
    /*if (sLine.find("{") != string::npos && (containsStrings(sLine) || _dataRef->containsStringVars(sLine)))
    {
        parser_VectorToExpr(sLine, *_optionRef);
    }*/

    // --> Uebliche Auswertung <--
    if (_parserRef->GetExpr() != sLine+" ")
    {
        if (bUseLoopParsingMode && !bLockedPauseMode && _parserRef->IsValidByteCode() && _parserRef->GetExpr().length())
            _parserRef->DeclareAsInvalid();
        _parserRef->SetExpr(sLine);
    }

    v = _parserRef->Eval(nNum);
    vAns = v[0];

    if (!nCurrentCalcType && !(nCalcType[nthCmd] & CALCTYPE_DATAACCESS || nCalcType[nthCmd] & CALCTYPE_STRING))
        nCalcType[nthCmd] |= CALCTYPE_NUMERICAL;

    if (!bSilent)
    {
        /* --> Der Benutzer will also die Ergebnisse sehen. Es gibt die Moeglichkeit,
         *     dass der Parser mehrere Ausdruecke je Zeile auswertet. Dazu muessen die
         *     Ausdruecke durch Kommata getrennt sein. Damit der Parser bemerkt, dass er
         *     mehrere Ausdruecke auszuwerten hat, muss man die Auswerte-Funktion des
         *     Parsers einmal aufgerufen werden <--
         */
        // --> Ist die Zahl der Ausdruecke groesser als 1? <--
        string sAns = "|" + sBlock + "> " + sLine_Temp + " => ";
        if (nNum > 1)
        {
            // --> Gebe die nNum Ergebnisse durch Kommata getrennt aus <--
            sAns += "{";
            for (int i=0; i<nNum; ++i)
            {
                sAns += toString(v[i], *_optionRef);
                if (i < nNum-1)
                    sAns += ", ";
            }
            NumeReKernel::printPreFmt(sAns + "}\n");
        }
        else
        {
            // --> Zahl der Ausdruecke gleich 1? Dann einfach ausgeben <--
            NumeReKernel::printPreFmt(sAns + toString(vAns, *_optionRef) + "\n");
        }
    }
    if (bWriteToCache)
    {
        _dataRef->writeToCache(_idx, sCache, v, nNum);
        bWriteToCache = false;
    }
    if (bReturnSignal)
    {
        for (int i = 0; i < nNum; i++)
            ReturnVal.vNumVal.push_back(v[i]);
        return FLOWCTRL_RETURN;
    }
    return FLOWCTRL_OK;
}

int FlowCtrl::procedureInterface(string& sLine, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_loop, int nth_command)
{
    cerr << "LOOP_procedureInterface" << endl;
    return 1;
}

int FlowCtrl::procedureCmdInterface(string& sLine)
{
    cerr << "LOOP_procedureCmdInterface" << endl;
    return 1;
}

bool FlowCtrl::isInline(const string& sProc)
{
    cerr << "LOOP_isInline" << endl;
    return true;
}

void FlowCtrl::replaceLocalVars(string& sLine)
{
    if (!mVarMap.size())
        return;
    for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
    {
        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if (sLine.substr(i,(iter->first).length()) == iter->first)
            {
                if ((i && checkDelimiter(sLine.substr(i-1, (iter->first).length()+2)))
                    || (!i && checkDelimiter(" "+sLine.substr(i, (iter->first).length()+1))))
                {
                    sLine.replace(i, (iter->first).length(), iter->second);
                }
            }
        }
    }
    return;
}

void FlowCtrl::evalDebuggerBreakPoint(Parser& _parser, Settings& _option, const map<string,string>& sStringMap)
{
    return;
}

