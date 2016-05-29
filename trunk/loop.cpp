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
#include "loop.hpp"

extern value_type vAns;
extern bool bSupressAnswer;

Loop::Loop()
{
    sCmd = 0;
    vVarArray = 0;
    sVarArray = 0;
    //_bytecode = 0;
    //nValidByteCode = 0;
    nJumpTable = 0;
    nJumpTableLength = 0;
    nLoopSavety = -1;
    //dReturnValue = 0.0;
    //ReturnVal.dNumVal = NAN;
    //ReturnVal.sStringVal = "";
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
    dVarAdress = 0;
    bUseLoopParsingMode = false;
    bLockedPauseMode = false;
    nthRecursion = 0;
}

Loop::Loop(int _nDefaultLength)
{
    Loop();
    nDefaultLength = _nDefaultLength;
}

Loop::~Loop()
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

void Loop::generateCommandArray()
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
        //cerr << "|-> DEBUG: Eintraege kopiert!" << endl;
        for (int i = 0; i < nDefaultLength; i++)
        {
            delete[] sCmd[i];
        }
        delete[] sCmd;
        sCmd = 0;
        //cerr << "|-> DEBUG: Speicher freigegeben!" << endl;

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
        //cerr << "|-> DEBUG: Eintraege zurueck kopiert!" << endl;

        for (int i = 0; i < nDefaultLength; i++)
        {
            delete[] sTemp[i];
        }
        delete[] sTemp;
        sTemp = 0;
        //cerr << "|-> DEBUG: Temp. Speicher freigegeben!" << endl;

        nDefaultLength += nDefaultLength;
        //cerr << "|-> DEBUG: Max. Laenge verdoppelt!" << endl;
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

int Loop::for_loop(Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, int nth_Cmd, int nth_loop)
{
    int nVarAdress = 0;
    int nInc = 1;
    int nLoopCount = 0;
    string sForHead = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('=')+1);
    string sVar = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find(' ')+1);
    string sLine = "";
    string sCache = "";
    bPrintedStatus = false;
    sVar = sVar.substr(0,sVar.find('='));
    StripSpaces(sVar);
    bool bFault = false;
    int nNum = 0;
    value_type* v = 0;

    /*if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: Schleife " << nth_loop << endl;
        cerr << "|-> DEBUG: sForHead = " << sForHead << endl;
    }*/
    for (int i = 0; i < nVarArray; i++)
    {
        if (sVarArray[i] == sVar)
        {
            nVarAdress = i;
            break;
        }
    }
    /*if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: sVar = " << sVar << endl;
        cerr << "|-> DEBUG: nVarAdress = " << nVarAdress << endl;
        cerr << "|-> DEBUG: sVarArray[nVarAdress] = " << sVarArray[nVarAdress] << endl;
    }*/

//////////
    // --> Datafile- und Cache-Konstrukte abfangen <--
    if ((sForHead.find("data(") != string::npos || _data.containsCacheElements(sForHead))
        && (!containsStrings(sForHead) && !_data.containsStringVars(sForHead)))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        if (sForHead.find("data(") != string::npos && _data.isValid() && !isInQuotes(sForHead, sForHead.find("data(")))
            parser_ReplaceEntities(sForHead, "data(", _data, _parser, _option);
        else if (sForHead.find("data(") != string::npos && !_data.isValid() && !isInQuotes(sForHead, sForHead.find("data(")))
        {
            mu::console() << _T("|-> FEHLER: Keine Daten verfuegbar!") << endl;
            bFault = true;
        }

        if (_data.containsCacheElements(sForHead) && _data.isValidCache())
        {
            _data.setCacheStatus(true);
            for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
            {
                if (sForHead.find(iter->first+"(") != string::npos && !isInQuotes(sForHead, sForHead.find(iter->first+"(")))
                {
                    parser_ReplaceEntities(sForHead, iter->first+"(", _data, _parser, _option);
                }
            }
            _data.setCacheStatus(false);
        }
        else if (_data.containsCacheElements(sForHead) && !_data.isValidCache())
        {
            mu::console() << _T("|-> FEHLER: Keine Daten verfuegbar!") << endl;
            bFault = true;
        }

        if (bFault)
        {
            return -1;
        }
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }
    // --> Ggf. Vektor-Eingaben abfangen <--
    if (sForHead.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
        parser_VectorToExpr(sForHead, _option);
    if (sForHead.find("$") != string::npos)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode();
            _parser.LockPause();
        }
        int nReturn = procedureInterface(sForHead, _parser, _functions, _data, _out, _pData, _script, _option, nLoop+nWhile+nIf, nth_Cmd);
        if (nReturn == -1)
            return -1;
        else if (nReturn == -2)
            sForHead = "false";
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sForHead = " << sForHead << endl;
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode(false);
            _parser.LockPause(false);
        }
    }
    // --> String-Auswertung einbinden <--
    if (containsStrings(sForHead) || _data.containsStringVars(sForHead))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        int nReturn = parser_StringParser(sForHead, sCache, _data, _parser, _option);
        if (nReturn)
        {
            if (nReturn == 1)
            {
                StripSpaces(sForHead);
                if (containsStrings(sForHead))
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherLoopBasedInformations(sForHead, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                    throw CANNOT_EVAL_FOR;
                }
            }
        }
        else
        {
            if (_option.getUseDebugger())
                _option._debug.gatherLoopBasedInformations(sForHead, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
            throw STRING_ERROR;
        }
        replaceLocalVars(sForHead);
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }


    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parser.SetIndex(nth_Cmd); // i + (nCmd+1)*j
    if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && sForHead+" " == _parser.GetExpr())
    {
        if (_option.getbDebug())
            cerr << "|-> DEBUG: Parser-Expr = " << _parser.GetExpr() << endl;
        v = _parser.Eval(nNum);
    }
    else if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr().length())
    {
        if (_option.getbDebug())
            cerr << "|-> DEBUG: Parser-Expr = " << _parser.GetExpr() << endl;
        _parser.DeclareAsInvalid();
        _parser.SetExpr(sForHead);
        v = _parser.Eval(nNum);
    }
    else
    {
        _parser.SetExpr(sForHead);
        v = _parser.Eval(nNum);
    }
    //int nNum = _parser.GetNumResults();
    //value_type* v = _parser.Eval(nNum);
    for (int i = 0; i < 2; i++)
    {
        vVarArray[nVarAdress][i+1] = (int)v[i];
        if (_option.getbDebug())
            cerr << "|-> DEBUG: vVarArray[][" << i+1 << "] = " << vVarArray[nVarAdress][i+1] << endl;
    }

    if (vVarArray[nVarAdress][2] < vVarArray[nVarAdress][1])
        nInc *= -1;

    if (bSilent && !nth_loop && !bMask)
    {
        mu::console() << _T("|FOR> Werte aus ... 0 %");
        bPrintedStatus = true;
    }
    for (int __i = (int)vVarArray[nVarAdress][1]; (nInc)*__i <= nInc*(int)vVarArray[nVarAdress][2]; __i+=nInc)
    {
        vVarArray[nVarAdress][0] = __i;
        if (nLoopCount >= nLoopSavety && nLoopSavety > 0)
            return -1;
        nLoopCount++;
        for (int __j = nth_Cmd; __j < nCmd; __j++)
        {
            if (__j == nJumpTable[nth_Cmd][0])
                break;
            if (__j != nth_Cmd)
            {
                if (sCmd[__j][0].length())
                {
                    if (sCmd[__j][0].substr(0,3) == "for")
                    {
                        __j = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __j, nth_loop+1);
                        if (__j == -1 || __j == -2 || bReturnSignal)
                            return __j;
                    }
                    else if (sCmd[__j][0].substr(0,6) == "endfor")
                    {
                        if ((nInc)*__i < nInc*(int)vVarArray[nVarAdress][2])
                            break;
                        else
                        {
                            if (bPrintedStatus && !nth_loop)
                                cerr << "\r|FOR> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... 100 %: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                            else if (!nth_loop)
                            {
                                cerr << "|FOR> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... 100 %: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                bPrintedStatus = true;
                            }
                            return __j;
                        }
                    }
                    else if (sCmd[__j][0].substr(0,5) == "while")
                    {
                        __j = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __j, nth_loop+1);
                        if (__j == -1 || __j == -2 || bReturnSignal)
                            return __j;
                    }
                    else if (sCmd[__j][0].find(">>if") != string::npos)
                    {
                        __j = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __j, nth_loop+1);
                        if (__j == -1 || __j == -2 || bReturnSignal)
                            return __j;
                        if (bContinueSignal)
                        {
                            bContinueSignal = false;
                            break;
                        }
                        if (bBreakSignal)
                        {
                            bBreakSignal = false;
                            /*int nFor = 0;
                            int n_While = 0;
                            for (int n = __j; n < nCmd; n++)
                            {
                                if (sCmd[n][0].substr(0,3) == "for")
                                    nFor++;
                                if (sCmd[n][0].substr(0,5) == "while")
                                    n_While++;
                                if (sCmd[n][0].substr(0,6) == "endfor" && !nFor && !n_While)
                                    return n;
                                else if (sCmd[n][0].substr(0,8) == "endwhile" && n_While)
                                    n_While--;
                                if (sCmd[n][0].substr(0,6) == "endfor" && nFor)
                                    nFor--;
                            }*/
                            return nJumpTable[nth_Cmd][0];
                        }
                    }
                }
            }
            if (!sCmd[__j][1].length())
                continue;

            if (sCmd[__j][1] == "continue")
            {
                break;
            }
            if (sCmd[__j][1] == "break")
            {
                /*int nFor = 0;
                int n_While = 0;
                for (int n = __j; n < nCmd; n++)
                {
                    if (sCmd[n][0].substr(0,3) == "for")
                        nFor++;
                    if (sCmd[n][0].substr(0,5) == "while")
                        n_While++;
                    if (sCmd[n][0].substr(0,6) == "endfor" && !nFor && !n_While)
                        return n;
                    else if (sCmd[n][0].substr(0,8) == "endwhile" && n_While)
                        n_While--;
                    if (sCmd[n][0].substr(0,6) == "endfor" && nFor)
                        nFor--;
                }*/
                return nJumpTable[nth_Cmd][0];
            }


            if (bUseLoopParsingMode)
                _parser.SetIndex(__j + nCmd+1);
            try
            {
                if (calc(sCmd[__j][1], __j, _parser, _functions, _data, _option, _out, _pData, _script, "FOR") == -1)
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                    return -1;
                }
                if (bReturnSignal)
                    return -2;
            }
            catch(...)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }
        __i = (int)vVarArray[nVarAdress][0];

        if (!nth_loop)
        {
            if (bSilent
                && abs(int(vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1])) < 99999
                && !bMask
                && abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 20))
                    > abs((int)((vVarArray[nVarAdress][0]-1-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 20)))
            {
                mu::console() << _T("\r|FOR> Werte aus ... ") << abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 20)) * 5 << _T(" %");
                bPrintedStatus = true;
            }
            else if (bSilent
                && abs(int(vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) >= 99999)
                && !bMask
                && abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 100))
                    > abs((int)((vVarArray[nVarAdress][0]-1-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 100)))
            {
                mu::console() << _T("\r|FOR> Werte aus ... ") << abs((int)((vVarArray[nVarAdress][0]-vVarArray[nVarAdress][1]) / (vVarArray[nVarAdress][2]-vVarArray[nVarAdress][1]) * 100)) << _T(" %");
                bPrintedStatus = true;
            }
        }
    }
    return nJumpTable[nth_Cmd][0];
}

int Loop::while_loop(Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, int nth_Cmd, int nth_loop)
{
    string sWhile_Condition = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('(')+1, sCmd[nth_Cmd][0].rfind(')')-sCmd[nth_Cmd][0].find('(')-1);
    string sWhile_Condition_Back = sWhile_Condition;
    string sLine = "";
    string sCache = "";
    bPrintedStatus = false;
    bool bFault = false;
    int nResults = 0;
    int nLoopCount = 0;
    value_type* v = 0;

    // --> Datafile- und Cache-Konstrukte abfangen <--
    //StripSpaces(sWhile_Condition);
    //cerr << sWhile_Condition << endl;
    if (sWhile_Condition.find("data(") != string::npos || _data.containsCacheElements(sWhile_Condition))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        if (sWhile_Condition.find("data()") != string::npos)
        {
            if (_data.isValid())
                sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find("data()")) + "true" + sWhile_Condition.substr(sWhile_Condition.find("data()")+6);
            else
                sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find("data()")) + "false" + sWhile_Condition.substr(sWhile_Condition.find("data()")+6);
        }
        if (!containsStrings(sWhile_Condition) && !_data.containsStringVars(sWhile_Condition))
        {
            if (sWhile_Condition.find("data(") != string::npos && _data.isValid() && !isInQuotes(sWhile_Condition, sWhile_Condition.find("data(")))
                parser_ReplaceEntities(sWhile_Condition, "data(", _data, _parser, _option);
            else if (sWhile_Condition.find("data(") != string::npos && !_data.isValid() && !isInQuotes(sWhile_Condition, sWhile_Condition.find("data(")))
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherLoopBasedInformations(sWhile_Condition_Back, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                throw NO_DATA_AVAILABLE;
            }
        }
        if (_data.containsCacheElements(sWhile_Condition))
        {

            _data.setCacheStatus(true);
            for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
            {
                if (sWhile_Condition.find(iter->first+"()") != string::npos)
                {
                    if (_data.getCacheCols(iter->first,false))
                        sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find(iter->first+"()")) + "true" + sWhile_Condition.substr(sWhile_Condition.find(iter->first+"()")+7);
                    else
                        sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find(iter->first+"()")) + "false" + sWhile_Condition.substr(sWhile_Condition.find(iter->first+"()")+7);
                }
                if (!containsStrings(sWhile_Condition) && !_data.containsStringVars(sWhile_Condition))
                {
                    if (sWhile_Condition.find(iter->first+"(") != string::npos && _data.getCacheCols(iter->first,false) && !isInQuotes(sWhile_Condition, sWhile_Condition.find(iter->first+"(")))
                        parser_ReplaceEntities(sWhile_Condition, iter->first+"(", _data, _parser, _option);
                    else if (sWhile_Condition.find(iter->first+"(") != string::npos && !isInQuotes(sWhile_Condition, sWhile_Condition.find(iter->first+"(")))
                    {
                        if (_option.getUseDebugger())
                            _option._debug.gatherLoopBasedInformations(sWhile_Condition, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                        throw NO_CACHED_DATA;
                    }
                }
            }
            _data.setCacheStatus(false);
        }


        if (bFault)
        {
            return -1;
        }
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }
    // --> Ggf. Vektor-Eingaben abfangen <--
    if (sWhile_Condition.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
        parser_VectorToExpr(sWhile_Condition, _option);

    // --> Prozeduren einbinden <--
    if (sWhile_Condition.find('$') != string::npos)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode();
            _parser.LockPause();
        }
        int nReturn = procedureInterface(sWhile_Condition, _parser, _functions, _data, _out, _pData, _script, _option, nLoop+nWhile+nIf, nth_Cmd);
        if (nReturn == -1)
            return -1;
        else if (nReturn == -2)
            sWhile_Condition = "false";
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sWhile_ = " << sWhile_Condition << endl;
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode(false);
            _parser.LockPause(false);
        }
    }

    // --> String-Auswertung einbinden <--
    if (containsStrings(sWhile_Condition) || _data.containsStringVars(sWhile_Condition))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        int nReturn = parser_StringParser(sWhile_Condition, sCache, _data, _parser, _option);
        if (nReturn)
        {
            if (nReturn == 1)
            {
                StripSpaces(sWhile_Condition);
                if (sWhile_Condition != "\"\"" && sWhile_Condition.length() > 2)
                    sWhile_Condition = "true";
                else
                    sWhile_Condition = "false";
            }
        }
        else
        {
            if (_option.getUseDebugger())
                _option._debug.gatherLoopBasedInformations(sWhile_Condition_Back, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
            throw STRING_ERROR;
        }
        replaceLocalVars(sWhile_Condition);
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }

    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parser.SetIndex(nth_Cmd);
    if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr() == sWhile_Condition+" ")
        _parser.Eval(nResults);
    else if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr().length())
    {
        _parser.DeclareAsInvalid();
        _parser.SetExpr(sWhile_Condition);
        _parser.Eval(nResults);
    }
    else
    {
        _parser.SetExpr(sWhile_Condition);
        _parser.Eval(nResults);
    }
    if (bSilent && !bMask && !nth_loop)
    {
        cerr << "|WHL> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... ";
        bPrintedStatus = true;
    }
    while (true)
    {
        if (bUseLoopParsingMode && !bLockedPauseMode)
            _parser.SetIndex(nth_Cmd);
        if (!bUseLoopParsingMode || bLockedPauseMode || !_parser.IsValidByteCode() || _parser.GetExpr() != sWhile_Condition+" ")
        {
            if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr().length())
                _parser.DeclareAsInvalid();
            _parser.SetExpr(sWhile_Condition);
        }
        if (nLoopCount >= nLoopSavety && nLoopSavety > 1)
            return -1;
        nLoopCount++;
        v = _parser.Eval(nResults);
        if (!(bool)v[0] || isnan(v[0]) || isinf(v[0]))
        {
            return nJumpTable[nth_Cmd][0];
        }
        for (int __j = nth_Cmd; __j < nCmd; __j++)
        {
            if (__j == nJumpTable[nth_Cmd][0])
                break;
            if (__j != nth_Cmd)
            {
                if (sCmd[__j][0].length())
                {
                    if (sCmd[__j][0].substr(0,3) == "for")
                    {
                        __j = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __j, nth_loop+1);
                        if (__j == -1 || __j == -2 || bReturnSignal)
                            return __j;
                    }
                    else if (sCmd[__j][0].substr(0,5) == "while")
                    {
                        __j = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __j, nth_loop+1);
                        if (__j == -1 || __j == -2 || bReturnSignal)
                            return __j;
                    }
                    else if (sCmd[__j][0].substr(0,8) == "endwhile")
                    {
                        break;
                    }
                    else if (sCmd[__j][0].find(">>if") != string::npos)
                    {
                        __j = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __j, nth_loop+1);
                        if (__j == -1 || __j == -2 || bReturnSignal)
                            return __j;
                        if (bContinueSignal)
                        {
                            bContinueSignal = false;
                            break;
                        }
                        if (bBreakSignal)
                        {
                            bBreakSignal = false;
                            /*int nFor = 0;
                            int n_While = 0;
                            for (int n = __j; n < nCmd; n++)
                            {
                                if (sCmd[n][0].substr(0,3) == "for")
                                    nFor++;
                                if (sCmd[n][0].substr(0,5) == "while")
                                    n_While++;
                                if (sCmd[n][0].substr(0,8) == "endwhile" && !nFor && !n_While)
                                    return n;
                                else if (sCmd[n][0].substr(0,8) == "endwhile" && n_While)
                                    n_While--;
                                if (sCmd[n][0].substr(0,6) == "endfor" && nFor)
                                    nFor--;
                            }*/
                            return nJumpTable[nth_Cmd][0];
                        }
                    }
                }
            }
            if (!sCmd[__j][1].length())
                continue;

            if (sCmd[__j][1] == "continue")
            {
                break;
            }
            if (sCmd[__j][1] == "break")
            {
                /*int nFor = 0;
                int n_While = 0;
                for (int n = __j; n < nCmd; n++)
                {
                    if (sCmd[n][0].substr(0,3) == "for")
                        nFor++;
                    if (sCmd[n][0].substr(0,5) == "while")
                        n_While++;
                    if (sCmd[n][0].substr(0,8) == "endwhile" && !nFor && !n_While)
                        return n;
                    else if (sCmd[n][0].substr(0,8) == "endwhile" && n_While)
                        n_While--;
                    if (sCmd[n][0].substr(0,6) == "endfor" && nFor)
                        nFor--;
                }*/
                return nJumpTable[nth_Cmd][0];
            }

            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parser.SetIndex(__j + nCmd+1);

            try
            {
                if (calc(sCmd[__j][1], __j, _parser, _functions, _data, _option, _out, _pData, _script, "WHL") == -1)
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                    return -1;
                }
                if (bReturnSignal)
                    return -2;
            }
            catch(...)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherLoopBasedInformations(sCmd[__j][1], nCmd-__j, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }

        if (!nth_loop)
        {
            if (bSilent && !bMask)
            {
                cerr << "\r|WHL> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... ";
                bPrintedStatus = true;
            }
        }

        if (sWhile_Condition != sWhile_Condition_Back || _data.containsCacheElements(sWhile_Condition_Back) || sWhile_Condition_Back.find("data(") != string::npos)
        {
            sWhile_Condition = sWhile_Condition_Back;
            if (sWhile_Condition.find("data(") != string::npos || _data.containsCacheElements(sWhile_Condition))
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode();
                if (sWhile_Condition.find("data()") != string::npos)
                {
                    if (_data.isValid())
                        sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find("data()")) + "1" + sWhile_Condition.substr(sWhile_Condition.find("data()")+6);
                    else
                        sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find("data()")) + "0" + sWhile_Condition.substr(sWhile_Condition.find("data()")+6);
                }
                if (!containsStrings(sWhile_Condition) && !_data.containsStringVars(sWhile_Condition))
                {
                    if (sWhile_Condition.find("data(") != string::npos && _data.isValid() && !isInQuotes(sWhile_Condition, sWhile_Condition.find("data(")))
                        parser_ReplaceEntities(sWhile_Condition, "data(", _data, _parser, _option);
                    else if (sWhile_Condition.find("data(") != string::npos && !_data.isValid() && !isInQuotes(sWhile_Condition, sWhile_Condition.find("data(")))
                    {
                        if (_option.getUseDebugger())
                            _option._debug.gatherLoopBasedInformations(sWhile_Condition_Back, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                        throw NO_DATA_AVAILABLE;
                    }
                }
                if (_data.containsCacheElements(sWhile_Condition))
                {

                    _data.setCacheStatus(true);
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (sWhile_Condition.find(iter->first+"()") != string::npos)
                        {
                            if (_data.getCacheCols(iter->first,false))
                                sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find(iter->first+"()")) + "true" + sWhile_Condition.substr(sWhile_Condition.find(iter->first+"()")+(iter->first).length()+2);
                            else
                                sWhile_Condition = sWhile_Condition.substr(0,sWhile_Condition.find(iter->first+"()")) + "false" + sWhile_Condition.substr(sWhile_Condition.find(iter->first+"()")+(iter->first).length()+2);
                        }
                        if (!containsStrings(sWhile_Condition) && !_data.containsStringVars(sWhile_Condition))
                        {
                            if (sWhile_Condition.find(iter->first+"(") != string::npos && _data.getCacheCols(iter->first,false) && !isInQuotes(sWhile_Condition, sWhile_Condition.find(iter->first+"(")))
                                parser_ReplaceEntities(sWhile_Condition, iter->first+"(", _data, _parser, _option);
                            else if (sWhile_Condition.find(iter->first+"(") != string::npos && !isInQuotes(sWhile_Condition, sWhile_Condition.find(iter->first+"(")))
                            {
                                if (_option.getUseDebugger())
                                    _option._debug.gatherLoopBasedInformations(sWhile_Condition_Back, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                                throw NO_CACHED_DATA;
                            }
                        }
                    }
                    _data.setCacheStatus(false);
                }

                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode(false);
            }
            //cerr << sWhile_Condition << endl;
            // --> Ggf. Vektor-Eingaben abfangen <--
            if (sWhile_Condition.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
                parser_VectorToExpr(sWhile_Condition, _option);
            // --> Prozeduren einbinden <--
            if (sWhile_Condition.find('$') != string::npos)
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                {
                    _parser.PauseLoopMode();
                    _parser.LockPause();
                }
                int nReturn = procedureInterface(sWhile_Condition, _parser, _functions, _data, _out, _pData, _script, _option, nLoop+nWhile+nIf, nth_Cmd);
                if (nReturn == -1)
                    return -1;
                else if (nReturn == -2)
                    sWhile_Condition = "false";
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: sWhile_Condition = " << sWhile_Condition << endl;
                if (!bLockedPauseMode && bUseLoopParsingMode)
                {
                    _parser.PauseLoopMode(false);
                    _parser.LockPause(false);
                }
            }

            // --> String-Auswertung einbinden <--
            if (containsStrings(sWhile_Condition) || _data.containsStringVars(sWhile_Condition))
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode();
                int nReturn = parser_StringParser(sWhile_Condition, sCache, _data, _parser, _option);
                if (nReturn)
                {
                    if (nReturn == 1)
                    {
                        StripSpaces(sWhile_Condition);
                        if (sWhile_Condition != "\"\"" && sWhile_Condition.length() > 2)
                            sWhile_Condition = "true";
                        else
                            sWhile_Condition = "false";
                    }
                }
                else
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherLoopBasedInformations(sWhile_Condition_Back, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                    throw STRING_ERROR;
                }
                replaceLocalVars(sWhile_Condition);
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode(false);
            }
        }
    }
    return nCmd;
}

int Loop::if_fork(Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, int nth_Cmd, int nth_loop)
{
    string sIf_Condition = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('(')+1, sCmd[nth_Cmd][0].rfind(')')-sCmd[nth_Cmd][0].find('(')-1);
    /*string sNth_if = sCmd[nth_Cmd][0].substr(0,sCmd[nth_Cmd][0].find(">>"));
    unsigned int nElseLength = (sNth_if+">>else").length();
    unsigned int nEndifLength = (sNth_if+">>endif").length();*/
    int nElse = nJumpTable[nth_Cmd][1];
    int nEndif = nJumpTable[nth_Cmd][0];
    string sLine = "";
    string sCache = "";
    bPrintedStatus = false;

    // --> Datafile- und Cache-Konstrukte abfangen <--
    //StripSpaces(sIf_Condition);
    //cerr << sIf_Condition << endl;
    if (sIf_Condition.find("data(") != string::npos || _data.containsCacheElements(sIf_Condition))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        if (sIf_Condition.find("data()") != string::npos)
        {
            if (_data.isValid())
                sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find("data()")) + "true" + sIf_Condition.substr(sIf_Condition.find("data()")+6);
            else
                sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find("data()")) + "false" + sIf_Condition.substr(sIf_Condition.find("data()")+6);
        }
        if (sIf_Condition.find("data(") != string::npos && !containsStrings(sIf_Condition) && !_data.containsStringVars(sIf_Condition))
        {
            if (_data.isValid() && !isInQuotes(sIf_Condition, sIf_Condition.find("data(")))
                parser_ReplaceEntities(sIf_Condition, "data(", _data, _parser, _option);
            else if (!_data.isValid() && !isInQuotes(sIf_Condition, sIf_Condition.find("data(")))
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherLoopBasedInformations(sIf_Condition, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                throw NO_DATA_AVAILABLE;
            }
        }
        if (_data.containsCacheElements(sIf_Condition))
        {
            _data.setCacheStatus(true);
            for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
            {
                if (sIf_Condition.find(iter->first+"()") != string::npos)
                {
                    if (_data.getCacheCols(iter->first,false))
                        sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find(iter->first+"()")) + "true" + sIf_Condition.substr(sIf_Condition.find(iter->first+"()")+(iter->first).length()+2);
                    else
                        sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find(iter->first+"()")) + "false" + sIf_Condition.substr(sIf_Condition.find(iter->first+"()")+(iter->first).length()+2);
                }
                if (!containsStrings(sIf_Condition) && !_data.containsStringVars(sIf_Condition))
                {
                    if (sIf_Condition.find(iter->first+"(") != string::npos && _data.getCacheCols(iter->first,false) && !isInQuotes(sIf_Condition, sIf_Condition.find(iter->first+"(")))
                        parser_ReplaceEntities(sIf_Condition, iter->first+"(", _data, _parser, _option);
                    else if (sIf_Condition.find(iter->first+"(") != string::npos && !isInQuotes(sIf_Condition, sIf_Condition.find(iter->first+"(")))
                    {
                        if (_option.getUseDebugger())
                            _option._debug.gatherLoopBasedInformations(sIf_Condition, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                        throw NO_CACHED_DATA;
                    }
                }
            }
            _data.setCacheStatus(false);
        }
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }
    // --> Ggf. Vektor-Eingaben abfangen <--
    if (sIf_Condition.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
        parser_VectorToExpr(sIf_Condition, _option);

    // --> Prozeduren einbinden <--
    if (sIf_Condition.find('$') != string::npos)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode();
            _parser.LockPause();
        }
        int nReturn = procedureInterface(sIf_Condition, _parser, _functions, _data, _out, _pData, _script, _option, nLoop+nWhile+nIf, nth_Cmd);
        if (nReturn == -1)
            return -1;
        else if (nReturn == -2)
            sIf_Condition = "false";
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sIf_Condition = " << sIf_Condition << endl;
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode(false);
            _parser.LockPause(false);
        }
    }

    // --> String-Auswertung einbinden <--
    if (containsStrings(sIf_Condition) || _data.containsStringVars(sIf_Condition))
    {
        if (!bLockedPauseMode)
            _parser.PauseLoopMode();
        int nReturn = parser_StringParser(sIf_Condition, sCache, _data, _parser, _option, true);
        if (nReturn)
        {
            if (nReturn == 1)
            {
                StripSpaces(sIf_Condition);
                if (sIf_Condition != "\"\"" && sIf_Condition.length() > 2)
                    sIf_Condition = "true";
                else
                    sIf_Condition = "false";
            }
        }
        else
        {
            if (_option.getUseDebugger())
                _option._debug.gatherLoopBasedInformations(sIf_Condition, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
            throw STRING_ERROR;
        }
        replaceLocalVars(sIf_Condition);
        if (!bLockedPauseMode)
            _parser.PauseLoopMode(false);
    }
    int nResults = 0;
    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parser.SetIndex(nth_Cmd);
    if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr() == sIf_Condition+" ")
        _parser.Eval(nResults);
    else if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr().length())
    {
        _parser.DeclareAsInvalid();
        _parser.SetExpr(sIf_Condition);
        _parser.Eval(nResults);
    }
    else
    {
        _parser.SetExpr(sIf_Condition);
        _parser.Eval(nResults);
    }
    /*if (bSilent && nth_loop == -1)
    {
        mu::console() << _T("|IF-> Werte aus ... ");
        bPrintedStatus = true;
    }*/

    //int nResults = 0;
    value_type* v = _parser.Eval(nResults);
    //v = _parser.Eval(nResults);
    //cerr << _parser.GetExpr() << endl;
    //cerr << v[0] << endl;
    if ((bool)v[0] && !isnan(v[0]) && !isinf(v[0]))
    {
        for (int __i = nth_Cmd; __i < nCmd; __i++)
        {
            if (nElse != -1 && __i == nElse)
                return nEndif;
            else if (nEndif == __i)
                return nEndif;
            if (__i != nth_Cmd)
            {
                if (sCmd[__i][0].length())
                {
                    if (sCmd[__i][0].substr(0,3) == "for")
                    {
                        if (nth_loop <= -1)
                        {
                            bPrintedStatus = false;
                            __i = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i);
                            if (!bReturnSignal && !bMask && __i != -1)
                            {
                                if (bSilent)
                                    cerr << "\r|FOR> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... 100 %: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                else
                                    cerr << "|FOR> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                            }
                        }
                        else
                            __i = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop+1);
                        if (__i == -1 || __i == -2 || bReturnSignal)
                            return __i;
                    }
                    else if (sCmd[__i][0].substr(0,5) == "while")
                    {
                        if (nth_loop <= -1)
                        {
                            bPrintedStatus = false;
                            __i = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i);
                            if (!bReturnSignal && !bMask && __i != -1)
                            {
                                if (bSilent)
                                    cerr << "\r|WHL> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ...: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                else
                                    cerr << "|WHL> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                            }
                        }
                        else
                            __i = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop+1);
                        if (__i == -1 || __i == -2 || bReturnSignal)
                            return __i;
                    }
                    else if (sCmd[__i][0].find(">>if") != string::npos)
                    {
                        if (nth_loop <= -1)
                            __i = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __i, -2);
                        else
                            __i = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop);
                        if (__i == -1 || __i == -2 || bReturnSignal)
                            return __i;
                        if (bBreakSignal || bContinueSignal)
                        {
                            return nEndif;
                        }
                    }
                }
            }

            if (!sCmd[__i][1].length())
                continue;

            if (sCmd[__i][1] == "continue" || sCmd[__i][1] == "break")
            {
                if (sCmd[__i][1] == "continue")
                    bContinueSignal = true;
                else
                    bBreakSignal = true;
                return nEndif;
            }

            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parser.SetIndex(__i+nCmd+1);
            try
            {
                if (calc(sCmd[__i][1], __i, _parser, _functions, _data, _option, _out, _pData, _script, "IF") == -1)
                    return -1;
                if (bReturnSignal)
                    return -2;
            }
            catch(...)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }
    }
    else
    {
        if (_option.getbDebug())
            cerr << "|-> DEBUG: ELSE-Zweig!" << endl;
        if (nElse == -1) // No else && no elseif
            return nEndif;
        else
            nth_Cmd = nElse;
        while (sCmd[nth_Cmd][0].find(">>elseif") != string::npos) // elseif
        {
            sIf_Condition = sCmd[nth_Cmd][0].substr(sCmd[nth_Cmd][0].find('(')+1, sCmd[nth_Cmd][0].rfind(')')-sCmd[nth_Cmd][0].find('(')-1);
            /*string sNth_if = sCmd[nth_Cmd][0].substr(0,sCmd[nth_Cmd][0].find(">>"));
            unsigned int nElseLength = (sNth_if+">>else").length();
            unsigned int nEndifLength = (sNth_if+">>endif").length();*/
            nElse = nJumpTable[nth_Cmd][1];
            nEndif = nJumpTable[nth_Cmd][0];
            sLine = "";
            sCache = "";
            bPrintedStatus = false;
            //bFault = false;

            // --> Datafile- und Cache-Konstrukte abfangen <--
            //StripSpaces(sIf_Condition);
            //cerr << sIf_Condition << endl;
            if (sIf_Condition.find("data(") != string::npos || _data.containsCacheElements(sIf_Condition))
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode();
                if (sIf_Condition.find("data()") != string::npos)
                {
                    if (_data.isValid())
                        sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find("data()")) + "true" + sIf_Condition.substr(sIf_Condition.find("data()")+6);
                    else
                        sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find("data()")) + "false" + sIf_Condition.substr(sIf_Condition.find("data()")+6);
                }
                if (sIf_Condition.find("data(") != string::npos && !containsStrings(sIf_Condition) && !_data.containsStringVars(sIf_Condition))
                {
                    if (_data.isValid() && !isInQuotes(sIf_Condition, sIf_Condition.find("data(")))
                        parser_ReplaceEntities(sIf_Condition, "data(", _data, _parser, _option);
                    else if (!_data.isValid() && !isInQuotes(sIf_Condition, sIf_Condition.find("data(")))
                    {
                        if (_option.getUseDebugger())
                            _option._debug.gatherLoopBasedInformations(sIf_Condition, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                        throw NO_DATA_AVAILABLE;
                    }
                }
                if (_data.containsCacheElements(sIf_Condition))
                {
                    _data.setCacheStatus(true);
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (sIf_Condition.find(iter->first+"()") != string::npos)
                        {
                            if (_data.getCacheCols(iter->first,false))
                                sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find(iter->first+"()")) + "true" + sIf_Condition.substr(sIf_Condition.find(iter->first+"()")+(iter->first).length()+2);
                            else
                                sIf_Condition = sIf_Condition.substr(0,sIf_Condition.find(iter->first+"()")) + "false" + sIf_Condition.substr(sIf_Condition.find(iter->first+"()")+(iter->first).length()+2);
                        }
                        if (!containsStrings(sIf_Condition) && !_data.containsStringVars(sIf_Condition))
                        {
                            if (sIf_Condition.find(iter->first+"(") != string::npos && _data.getCacheCols(iter->first,false) && !isInQuotes(sIf_Condition, sIf_Condition.find(iter->first+"(")))
                                parser_ReplaceEntities(sIf_Condition, iter->first+"(", _data, _parser, _option);
                            else if (sIf_Condition.find(iter->first+"(") != string::npos && !isInQuotes(sIf_Condition, sIf_Condition.find(iter->first+"(")))
                            {
                                if (_option.getUseDebugger())
                                    _option._debug.gatherLoopBasedInformations(sIf_Condition, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                                throw NO_CACHED_DATA;
                            }
                        }
                    }
                    _data.setCacheStatus(false);
                }
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode(false);
            }
            // --> Ggf. Vektor-Eingaben abfangen <--
            if (sIf_Condition.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
                parser_VectorToExpr(sIf_Condition, _option);

            // --> Prozeduren einbinden <--
            if (sIf_Condition.find('$') != string::npos)
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                {
                    _parser.PauseLoopMode();
                    _parser.LockPause();
                }
                int nReturn = procedureInterface(sIf_Condition, _parser, _functions, _data, _out, _pData, _script, _option, nLoop+nWhile+nIf, nth_Cmd);
                if (nReturn == -1)
                    return -1;
                else if (nReturn == -2)
                    sIf_Condition = "false";
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: sIf_Condition = " << sIf_Condition << endl;
                if (!bLockedPauseMode && bUseLoopParsingMode)
                {
                    _parser.PauseLoopMode(false);
                    _parser.LockPause(false);
                }
            }

            // --> String-Auswertung einbinden <--
            if (containsStrings(sIf_Condition) || _data.containsStringVars(sIf_Condition))
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode();
                int nReturn = parser_StringParser(sIf_Condition, sCache, _data, _parser, _option, true);
                if (nReturn)
                {
                    if (nReturn == 1)
                    {
                        StripSpaces(sIf_Condition);
                        if (sIf_Condition != "\"\"" && sIf_Condition.length() > 2)
                            sIf_Condition = "true";
                        else
                            sIf_Condition = "false";
                    }
                }
                else
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherLoopBasedInformations(sIf_Condition, nth_Cmd, mVarMap, vVarArray, sVarArray, nVarArray);
                    throw STRING_ERROR;
                }
                replaceLocalVars(sIf_Condition);
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode(false);
            }

            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parser.SetIndex(nth_Cmd);
            if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr() == sIf_Condition+" ")
                _parser.Eval(nResults);
            else if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr().length())
            {
                _parser.DeclareAsInvalid();
                _parser.SetExpr(sIf_Condition);
                _parser.Eval(nResults);
            }
            else
            {
                _parser.SetExpr(sIf_Condition);
                _parser.Eval(nResults);
            }
            /*if (bSilent && nth_loop == -1)
            {
                mu::console() << _T("|IF-> Werte aus ... ");
                bPrintedStatus = true;
            }*/
            v = _parser.Eval(nResults);
            if ((bool)v[0] && !isnan(v[0]) && !isinf(v[0]))
            {
                for (int __i = nth_Cmd; __i < nCmd; __i++)
                {
                    if (nElse != -1 && __i == nElse)
                        return nEndif;
                    else if (nEndif == __i)
                        return nEndif;
                    if (__i != nth_Cmd)
                    {
                        if (sCmd[__i][0].length())
                        {
                            if (sCmd[__i][0].substr(0,3) == "for")
                            {
                                if (nth_loop <= -1)
                                {
                                    bPrintedStatus = false;
                                    __i = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i);
                                    if (!bReturnSignal && !bMask && __i != -1)
                                    {
                                        if (bSilent)
                                            cerr << "\r|FOR> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... 100 %: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                        else
                                            cerr << "|FOR> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                    }
                                }
                                else
                                    __i = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop+1);
                                if (__i == -1 || __i == -2 || bReturnSignal)
                                    return __i;
                            }
                            else if (sCmd[__i][0].substr(0,5) == "while")
                            {
                                if (nth_loop <= -1)
                                {
                                    bPrintedStatus = false;
                                    __i = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i);
                                    if (!bReturnSignal && !bMask && __i != -1)
                                    {
                                        if (bSilent)
                                            cerr << "\r|WHL> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ...: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                        else
                                            cerr << "|WHL> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                    }
                                }
                                else
                                    __i = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop+1);
                                if (__i == -1 || __i == -2 || bReturnSignal)
                                    return __i;
                            }
                            else if (sCmd[__i][0].find(">>if") != string::npos)
                            {
                                if (nth_loop <= -1)
                                    __i = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __i, -2);
                                else
                                    __i = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop);
                                if (__i == -1 || __i == -2 || bReturnSignal)
                                    return __i;
                                if (bBreakSignal || bContinueSignal)
                                {
                                    /*int nIfCount = 0;
                                    for (int n = __i; n < nCmd; n++)
                                    {
                                        if (sCmd[n][0].find(">>if") != string::npos)
                                            nIfCount++;
                                        if (sCmd[n][0].find(">>endif") != string::npos && !nIfCount)
                                            return n;
                                        else if (sCmd[n][0].find(">>endif") != string::npos && nIfCount)
                                            nIfCount--;
                                    }*/
                                    return nEndif;
                                }
                            }
                            /*else if ((sCmd[__i][0].substr(0, nElseLength) == sNth_if + ">>else"
                                || sCmd[__i][0].substr(0, nEndifLength) == sNth_if + ">>endif"))
                            {
                                if (sCmd[__i][0].substr(0, nEndifLength) == sNth_if + ">>endif")
                                    return __i;
                                else
                                {
                                    while (__i < nCmd)
                                    {
                                        __i++;
                                        if (sCmd[__i][0].length())
                                        {
                                            if (sCmd[__i][0].substr(0, nEndifLength) == sNth_if + ">>endif")
                                                return __i;
                                        }
                                    }
                                    return -1;
                                }
                            }*/
                        }
                    }

                    if (!sCmd[__i][1].length())
                        continue;

                    if (sCmd[__i][1] == "continue" || sCmd[__i][1] == "break")
                    {
                        if (sCmd[__i][1] == "continue")
                            bContinueSignal = true;
                        else
                            bBreakSignal = true;

                        /*int nIfCount = 0;
                        for (int n = __i; n < nCmd; n++)
                        {
                            if (sCmd[n][0].find(">>if") != string::npos)
                                nIfCount++;
                            if (sCmd[n][0].find(">>endif") != string::npos && !nIfCount)
                                return n;
                            else if (sCmd[n][0].find(">>endif") != string::npos && nIfCount)
                                nIfCount--;
                        }*/
                        return nEndif;
                    }

                    if (bUseLoopParsingMode && !bLockedPauseMode)
                        _parser.SetIndex(__i+nCmd+1);

                    try
                    {
                        if (calc(sCmd[__i][1], __i, _parser, _functions, _data, _option, _out, _pData, _script, "IF") == -1)
                        {
                            if (_option.getUseDebugger())
                                _option._debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                            return -1;
                        }
                        if (bReturnSignal)
                            return -2;
                    }
                    catch (...)
                    {
                        if (_option.getUseDebugger())
                            _option._debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                        throw;
                    }
                }
            }
            else
            {
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: ELSE-Zweig!" << endl;
                if (nElse == -1) // No else && no elseif
                    return nEndif;
                else
                    nth_Cmd = nElse;
            }
        }
        /*while (nth_Cmd < nCmd)
        {
            nth_Cmd++;
            if (sCmd[nth_Cmd][0].length())
            {
                if (sCmd[nth_Cmd][0].substr(0, nElseLength) == sNth_if + ">>else")
                    break;
                else if (sCmd[nth_Cmd][0].substr(0, nEndifLength) == sNth_if + ">>endif")
                    return nth_Cmd;
            }
        }*/
        if (_option.getbDebug())
            cerr << "|-> DEBUG: nth_Cmd = " << nth_Cmd << " / nCmd = " << nCmd << endl;
        /*if (nth_Cmd+2 == nCmd)
            return -1;*/

        for (int __i = nth_Cmd; __i < nCmd; __i++)
        {
            if (__i == nEndif)
                return nEndif;
            if (__i != nth_Cmd)
            {
                if (sCmd[__i][0].length())
                {
                    if (sCmd[__i][0].substr(0,3) == "for")
                    {
                        if (nth_loop <= -1)
                        {
                            //mu::console() << endl;
                            bPrintedStatus = false;
                            __i = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i);
                            if (!bReturnSignal && !bMask && __i != -1)
                            {
                                if (bSilent)
                                    cerr << "\r|FOR> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... 100 %: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                else
                                    cerr << "|FOR> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                            }
                        }
                        else
                            __i = for_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop+1);
                        if (__i == -1 || __i == -2 || bReturnSignal)
                            return __i;
                    }
                    else if (sCmd[__i][0].substr(0,5) == "while")
                    {
                        if (nth_loop <= -1)
                        {
                            bPrintedStatus = false;
                            __i = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i);
                            if (!bReturnSignal && !bMask && __i != -1)
                            {
                                if (bSilent)
                                    cerr << "\r|WHL> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ...: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                                else
                                    cerr << "|WHL> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                            }
                        }
                        else
                            __i = while_loop(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop+1);
                        if (__i == -1 || __i == -2 || bReturnSignal)
                            return __i;
                    }
                    else if (sCmd[__i][0].find(">>if") != string::npos)
                    {
                        if (nth_loop <= -1)
                            __i = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __i, -2);
                        else
                            __i = if_fork(_parser, _functions, _data, _option, _out, _pData, _script, __i, nth_loop);
                        if (__i == -1 || __i == -2 || bReturnSignal)
                            return __i;
                        if (bBreakSignal || bContinueSignal)
                        {
                            /*int nIfCount = 0;
                            for (int n = __i; n < nCmd; n++)
                            {
                                if (sCmd[n][0].find(">>if") != string::npos)
                                    nIfCount++;
                                if (sCmd[n][0].find(">>endif") != string::npos && !nIfCount)
                                    return n;
                                else if (sCmd[n][0].find(">>endif") != string::npos && nIfCount)
                                    nIfCount--;
                            }*/
                            return nEndif;
                        }
                    }
                    /*else if (sCmd[__i][0].substr(0, nEndifLength) == sNth_if + ">>endif")
                    {
                        return __i;
                    }*/
                }
            }

            if (!sCmd[__i][1].length())
                continue;

            if (sCmd[__i][1] == "continue" || sCmd[__i][1] == "break")
            {
                if (sCmd[__i][1] == "continue")
                    bContinueSignal = true;
                else
                    bBreakSignal = true;

                /*int nIfCount = 0;
                for (int n = __i; n < nCmd; n++)
                {
                    if (sCmd[n][0].find(">>if") != string::npos)
                        nIfCount++;
                    if (sCmd[n][0].find(">>endif") != string::npos && !nIfCount)
                        return n;
                    else if (sCmd[n][0].find(">>endif") != string::npos && nIfCount)
                        nIfCount--;
                }*/
                return nEndif;
            }

            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parser.SetIndex(__i + nCmd+1);

            try
            {
                if (calc(sCmd[__i][1], __i, _parser, _functions, _data, _option, _out, _pData, _script, "IF") == -1)
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                    return -1;
                }
                if (bReturnSignal)
                    return -2;
            }
            catch(...)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherLoopBasedInformations(sCmd[__i][1], nCmd-__i, mVarMap, vVarArray, sVarArray, nVarArray);
                throw;
            }
        }
    }
    return nCmd;
}

void Loop::setCommand(string& __sCmd, Parser& _parser, Datafile& _data, Define& _functions, Settings& _option, Output& _out, PlotData& _pData, Script& _script)
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
        reset(_parser);
        cerr << "|-> " << toSystemCodePage(_lang.get("LOOP_SETCOMMAND_ABORT")) << endl;
        return;
    }

    if (!sCmd)
        generateCommandArray();

    if (!_functions.call(__sCmd, _option))
    {
        reset(_parser);
        throw FUNCTION_ERROR;
    }


    StripSpaces(__sCmd);

    if (__sCmd.substr(0,3) == "for"
        || __sCmd.substr(0,6) == "endfor"
        || __sCmd.substr(0,3) == "if "
        || __sCmd.substr(0,3) == "if("
        || __sCmd.substr(0,5) == "endif"
        || __sCmd.substr(0,4) == "else"
        || __sCmd.substr(0,5) == "while"
        || __sCmd.substr(0,8) == "endwhile")
    {
        if (!validateParenthesisNumber(__sCmd))
        {
            reset(_parser);
            throw UNMATCHED_PARENTHESIS;
        }
        if (__sCmd.substr(0,3) == "for")
        {
            sLoopNames += ";FOR";
            nLoop++;
            // --> Pruefen wir, ob auch Grenzen eingegeben wurden <--
            if (__sCmd.find('=') == string::npos || __sCmd.find(':') == string::npos || __sCmd.find('(') == string::npos)
            {
                cerr << LineBreak("|-> " + toSystemCodePage(_lang.get("LOOP_SUPPLY_BORDERS_AND_VAR")) + ":", _option) << endl;
                string sTemp = "";
                do
                {
                    cerr << "|\n|<- ";
                    getline(cin, sTemp);
                    StripSpaces(sTemp);

                    if (sTemp.find('=') == string::npos || sTemp.find('|') == string::npos || sTemp.find('(') == string::npos)
                        sTemp = "";
                }
                while (!sTemp.length());
                __sCmd = "for " + sTemp;
            }
            // --> Schneller: ':' durch ',' ersetzen, da der Parser dann seine eigenen Mulit-Expr. einsetzen kann <--
            /*for (unsigned int i = 3; i < __sCmd.length(); i++)
            {
                if (__sCmd[i] != ' ')
                {
                    if (__sCmd[i] == '(' && __sCmd.back() == ')')
                    {
                        __sCmd[i] = ' ';
                        __sCmd.pop_back();
                    }
                    break;
                }
            }*/
            unsigned int nPos = __sCmd.find(':');
            if (__sCmd.find('(', __sCmd.find('(')+1) != string::npos && __sCmd.find('(', __sCmd.find('(')+1) < nPos)
            {
                nPos = getMatchingParenthesis(__sCmd.substr(__sCmd.find('(', __sCmd.find('(')+1)));
                nPos = __sCmd.find(':', nPos);
            }

            if (nPos == string::npos)
            {
                reset(_parser);
                throw CANNOT_EVAL_FOR;
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
                cerr << LineBreak("|-> " + toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option) << endl;

                string sTemp = "";
                while (sTemp.find('(') == string::npos)
                {
                    cerr << "|" << endl << "|<- while ";
                    getline(cin, sTemp);
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
                cerr << LineBreak("|-> " + toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option) << endl;

                string sTemp = "";
                while (sTemp.find('(') == string::npos)
                {
                    cerr << "|" << endl << "|<- if ";
                    getline(cin, sTemp);
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
                        cerr << LineBreak("|-> " + toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option) << endl;

                        string sTemp = "";
                        while (sTemp.find('(') == string::npos)
                        {
                            cerr << "|" << endl << "|<- elseif ";
                            getline(cin, sTemp);
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

        if (_option.getbDebug())
        {
            cerr << "|-> DEBUG: nIf = " << nIf << endl;
            cerr << "|-> DEBUG: sCmd[" << nCmd << "][0] = " << __sCmd << endl;
        }
    }
    else if (__sCmd.substr(0,4) == "repl")
    {
        int nLine = 0;
        int nInput = 0;
        string sNewCommand = "";
        if (__sCmd.find('"') == string::npos)
        {
            cerr << "|-> " << toSystemCodePage(_lang.get("LOOP_MISSING_COMMAND")) << endl;
            return;
        }
        sNewCommand = __sCmd.substr(__sCmd.find('"')+1);
        sNewCommand = sNewCommand.substr(0,sNewCommand.rfind('"'));
        StripSpaces(sNewCommand);
        if (!sNewCommand.length())
        {
            cerr << "|-> " << toSystemCodePage(_lang.get("LOOP_MISSING_COMMAND")) << endl;
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
                cerr << "|-> " << toSystemCodePage(_lang.get("LOOP_LINE_NOT_EXISTENT")) << endl;
                return;
            }
        }
        else if (nInput-1 > nLine)
        {
            cerr << "|-> " << toSystemCodePage(_lang.get("LOOP_LINE_NOT_EXISTENT")) << endl;
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
                        cerr << LineBreak("|-> " + toSystemCodePage(_lang.get("LOOP_SUPPLY_BORDERS_AND_VAR")) + ":", _option) << endl;
                        string sTemp = "";
                        do
                        {
                            cerr << "|\n|<- ";
                            getline(cin, sTemp);
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
                        cerr << LineBreak("|-> " + toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option) << endl;

                        string sTemp = "";
                        while (sTemp.find('(') == string::npos)
                        {
                            cerr << "|" << endl << "|<- if ";
                            getline(cin, sTemp);
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
                        cerr << LineBreak("|-> " + toSystemCodePage(_lang.get("LOOP_SUPPLY_FULFILLABLE_CONDITION")) + ":", _option) << endl;

                        string sTemp = "";
                        while (sTemp.find('(') == string::npos)
                        {
                            cerr << "|" << endl << "|<- while ";
                            getline(cin, sTemp);
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
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sCmd[" << nCmd << "][1] = " << __sCmd << endl;
        nCmd++;
    }

    if (nCmd+1 == nDefaultLength)
        generateCommandArray();
    if (nCmd)
    {
        if (!(nLoop+nIf+nWhile) && sCmd[nCmd][0].length())
            eval(_parser, _data, _functions, _option, _out, _pData, _script);
    }
    if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: nLoop = " << nLoop << endl;
        cerr << "|-> DEBUG: nIf = " << nIf << endl;
        cerr << "|-> DEBUG: nWhile = " << nWhile << endl;
        cerr << "|-> DEBUG: sLoopNames = " << sLoopNames << endl;
    }
    if (sAppendedExpression.length())
        setCommand(sAppendedExpression, _parser, _data, _functions, _option, _out, _pData, _script);
    return;
}

// --> Alle Variablendefinitionen und Deklarationen und den Error-Handler! <--
void Loop::eval(Parser& _parser, Datafile& _data, Define& _functions, Settings& _option, Output& _out, PlotData& _pData, Script& _script)
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
    //_bytecode = new ParserByteCode[nCmd+1];
    //nValidByteCode = new int[nCmd+1];

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
                if (_option.getbDebug())
                {
                    cerr << "|-> DEBUG: sVars = " << sVars << endl;
                    cerr << "|-> DEBUG: nVarArray = " << nVarArray << endl;
                }
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

    nJumpTableLength = nCmd+1;
    nJumpTable = new int*[nJumpTableLength];
    for (unsigned int i = 0; i < nJumpTableLength; i++)
    {
        nJumpTable[i] = new int[3];
        nJumpTable[i][0] = -1;
        nJumpTable[i][1] = -1;
        nJumpTable[i][2] = -1;
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
                            nJumpTable[i][0] = j;
                            if (!bUseLoopParsingMode && j-i > 1 && !bLockedPauseMode)
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
                            nJumpTable[i][0] = j;
                            if (!bUseLoopParsingMode && j-i > 1 && !bLockedPauseMode)
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
                    if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>else").length()) == sNth_If + ">>else" && nJumpTable[i][1] == -1)
                        nJumpTable[i][1] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>elseif").length()) == sNth_If + ">>elseif" && nJumpTable[i][1] == -1)
                        nJumpTable[i][1] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>endif").length()) == sNth_If + ">>endif")
                    {
                        nJumpTable[i][0] = j;
                        break;
                    }
                }
            }
            else if (sCmd[i][0].find(">>elseif") != string::npos)
            {
                string sNth_If = sCmd[i][0].substr(0, sCmd[i][0].find(">>"));
                for (int j = i+1; j <= nCmd; j++)
                {
                    if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>else").length()) == sNth_If + ">>else" && nJumpTable[i][1] == -1)
                        nJumpTable[i][1] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>elseif").length()) == sNth_If + ">>elseif" && nJumpTable[i][1] == -1)
                        nJumpTable[i][1] = j;
                    else if (sCmd[j][0].length() && sCmd[j][0].substr(0, (sNth_If + ">>endif").length()) == sNth_If + ">>endif")
                    {
                        nJumpTable[i][0] = j;
                        break;
                    }
                }
            }
        //cerr << nJumpTable[i][0] << "  " << nJumpTable[i][1] << endl;
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
        }
    }
    catch (...)
    {
        reset(_parser);
        throw;
    }

    //cerr << "Loop-Parse-Mode: " << bUseLoopParsingMode << endl;

    if (_option.getbDebug())
        cerr << "|-> DEBUG: Silent-Mode = " << bSilent << endl;

    if (bSilent)
        bSupressAnswer = true;

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
            mVarMap[sVarArray[i]] = "LOOP_"+sVarArray[i]+"_"+toString(nthRecursion);
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
                        if ((!j && checkDelimiter(" "+sCmd[i][0].substr(j,(iter->first).length()+1)))
                            || (j && checkDelimiter(sCmd[i][0].substr(j-1, (iter->first).length()+2))))
                        {
                            sCmd[i][0].replace(j,(iter->first).length(), iter->second);
                            j += (iter->second).length()-(iter->first).length();
                        }
                        //cerr << sCmd[i][0] << endl;
                    }
                }
                /*if (sCmd[i][0][sCmd[i][0].length()-1] == ' ')
                    sCmd[i][0].pop_back();*/
            }
            if (sCmd[i][1].length())
            {
                sCmd[i][1] += " ";
                //cerr << sCmd[i][1] << endl;
                for (unsigned int j = 0; j < sCmd[i][1].length(); j++)
                {
                    if (sCmd[i][1].substr(j,(iter->first).length()) == iter->first)
                    {
                        if ((!j && checkDelimiter(" "+sCmd[i][1].substr(j,(iter->first).length()+1)))
                            || (j && checkDelimiter(sCmd[i][1].substr(j-1, (iter->first).length()+2))))
                        {
                            sCmd[i][1].replace(j,(iter->first).length(), iter->second);
                            j += (iter->second).length()-(iter->first).length();
                        }
                        //cerr << sCmd[i][1] << endl;
                    }
                }
                // Könnte Schwierigkeiten machen
                if (sCmd[i][1].find("+=") != string::npos
                    || sCmd[i][1].find("-=") != string::npos
                    || sCmd[i][1].find("*=") != string::npos
                    || sCmd[i][1].find("/=") != string::npos
                    || sCmd[i][1].find("^=") != string::npos)
                {
                    bool bBreakPoint = (sCmd[i][1].substr(sCmd[i][1].find_first_not_of(" \t"),2) == "|>");
                    if (bBreakPoint)
                    {
                        sCmd[i][1].erase(sCmd[i][1].find_first_not_of(" \t"),2);
                        StripSpaces(sCmd[i][1]);
                    }
                    unsigned int nArgSepPos = 0;
                    for (unsigned int i = 0; i < sCmd[i][1].length(); i++)
                    {
                        if (isInQuotes(sCmd[i][1], i, false))
                            continue;
                        if (sCmd[i][1][i] == '(' || sCmd[i][1][i] == '{')
                            i += getMatchingParenthesis(sCmd[i][1].substr(i));
                        if (sCmd[i][1][i] == ',')
                            nArgSepPos = i;
                        if (sCmd[i][1].substr(i,2) == "+="
                            || sCmd[i][1].substr(i,2) == "-="
                            || sCmd[i][1].substr(i,2) == "*="
                            || sCmd[i][1].substr(i,2) == "/="
                            || sCmd[i][1].substr(i,2) == "^=")
                        {
                            if (sCmd[i][1].find(',', i) != string::npos)
                            {
                                for (unsigned int j = i; j < sCmd[i][1].length(); j++)
                                {
                                    if (sCmd[i][1][j] == '(')
                                        j += getMatchingParenthesis(sCmd[i][1].substr(j));
                                    if (sCmd[i][1][j] == ',' || j+1 == sCmd[i][1].length())
                                    {
                                        if (!nArgSepPos && j+1 != sCmd[i][1].length())
                                            sCmd[i][1] = sCmd[i][1].substr(0, i)
                                                + " = "
                                                + sCmd[i][1].substr(0, i)
                                                + sCmd[i][1][i]
                                                + "("
                                                + sCmd[i][1].substr(i+2, j-i-2)
                                                + ") "
                                                + sCmd[i][1].substr(j);
                                        else if (nArgSepPos && j+1 != sCmd[i][1].length())
                                            sCmd[i][1] = sCmd[i][1].substr(0, i)
                                                + " = "
                                                + sCmd[i][1].substr(nArgSepPos+1, i-nArgSepPos-1)
                                                + sCmd[i][1][i]
                                                + "("
                                                + sCmd[i][1].substr(i+2, j-i-2)
                                                + ") "
                                                + sCmd[i][1].substr(j);
                                        else if (!nArgSepPos && j+1 == sCmd[i][1].length())
                                            sCmd[i][1] = sCmd[i][1].substr(0, i)
                                                + " = "
                                                + sCmd[i][1].substr(0, i)
                                                + sCmd[i][1][i]
                                                + "("
                                                + sCmd[i][1].substr(i+2)
                                                + ") ";
                                        else
                                            sCmd[i][1] = sCmd[i][1].substr(0, i)
                                                + " = "
                                                + sCmd[i][1].substr(nArgSepPos+1, i-nArgSepPos-1)
                                                + sCmd[i][1][i]
                                                + "("
                                                + sCmd[i][1].substr(i+2)
                                                + ") ";

                                        for (unsigned int k = i; k < sCmd[i][1].length(); k++)
                                        {
                                            if (sCmd[i][1][k] == '(')
                                                k += getMatchingParenthesis(sCmd[i][1].substr(k));
                                            if (sCmd[i][1][k] == ',')
                                            {
                                                nArgSepPos = k;
                                                i = k;
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                if (!nArgSepPos)
                                    sCmd[i][1] = sCmd[i][1].substr(0, i)
                                        + " = "
                                        + sCmd[i][1].substr(0, i)
                                        + sCmd[i][1][i]
                                        + "("
                                        + sCmd[i][1].substr(i+2)
                                        + ")";
                                else
                                    sCmd[i][1] = sCmd[i][1].substr(0, i)
                                        + " = "
                                        + sCmd[i][1].substr(nArgSepPos+1, i-nArgSepPos-1)
                                        + sCmd[i][1][i]
                                        + "("
                                        + sCmd[i][1].substr(i+2)
                                        + ")";
                                break;
                            }
                        }
                    }
                    if (bBreakPoint)
                        sCmd[i][1].insert(0,"|> ");

                    if (_option.getbDebug())
                        cerr << "|-> DEBUG: sCmd[i][1] = " << sCmd[i][1] << endl;
                }


                StripSpaces(sCmd[i][1]);
                /*if (sCmd[i][1][sCmd[i][1].length()-1] == ' ')
                    sCmd[i][1].pop_back();*/
            }
        }
    }

    if (bUseLoopParsingMode && !bLockedPauseMode)
        _parser.ActivateLoopMode((unsigned int)(2*(nCmd+1)));

    try
    {
        if (sCmd[0][0].substr(0,3) == "for")
        {
            if (for_loop(_parser, _functions, _data, _option, _out, _pData, _script) == -1)
            {
                if (bSilent || bMask)
                    cerr << endl;
                throw CANNOT_EVAL_FOR;
            }
            else if (!bReturnSignal && !bMask)
            {
                if (bSilent)
                    cerr << "\r|FOR> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ... 100 %: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                else
                    cerr << "|FOR> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
            }
        }
        else if (sCmd[0][0].substr(0,5) == "while")
        {
            if (while_loop(_parser, _functions, _data, _option, _out, _pData, _script) == -1)
            {
                if (bSilent || bMask)
                    cerr << endl;
                throw CANNOT_EVAL_WHILE;
            }
            else if (!bReturnSignal && !bMask)
            {
                if (bSilent)
                    cerr << "\r|WHL> " << toSystemCodePage(_lang.get("COMMON_EVALUATING")) << " ...: " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
                else
                    cerr << "|WHL> " << toSystemCodePage(_lang.get("COMMON_SUCCESS")) << "." << endl;
            }
        }
        else
        {
            if (if_fork(_parser, _functions, _data, _option, _out, _pData, _script) == -1)
            {
                if (bSilent || bMask)
                    cerr << endl;
                throw CANNOT_EVAL_IF;
            }
        }
    }
    catch (...)
    {
        reset(_parser);
        if (bSupressAnswer)
            bSupressAnswer = false;
        if (bPrintedStatus)
            cerr << endl;
        throw;
    }
    reset(_parser);

    if (bSupressAnswer)
        bSupressAnswer = false;
    return;
}

void Loop::reset(Parser& _parser)
{
    if (sCmd)
    {
        for (int i = 0; i < nDefaultLength; i++)
        {
            delete[] sCmd[i];
        }
        delete[] sCmd;
        sCmd = 0;
    }

    if (vVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
            _parser.RemoveVar(sVarArray[i]);
    }
    if (mVarMap.size() && nVarArray)
    {
        for (int i = 0; i < nVarArray; i++)
        {
            if (mVarMap.find(sVarArray[i]) != mVarMap.end())
                mVarMap.erase(mVarMap.find(sVarArray[i]));
        }
        if (!mVarMap.size())
            _parser.mVarMapPntr = 0;
    }
    if (!vVars.empty())
    {
        varmap_type::const_iterator item = vVars.begin();
        for (; item != vVars.end(); ++item)
        {
            _parser.DefineVar(item->first, item->second);
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
        vVarArray = 0;
        sVarArray = 0;
    }

    /*if (_bytecode)
    {
        delete[] _bytecode;
        _bytecode = 0;
    }
    if (nValidByteCode)
    {
        delete[] nValidByteCode;
        nValidByteCode = 0;
    }*/

    if (nJumpTable)
    {
        for (unsigned int i = 0; i < nJumpTableLength; i++)
            delete[] nJumpTable[i];
        delete[] nJumpTable;
        nJumpTable = 0;
        nJumpTableLength = 0;
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
        _parser.DeactivateLoopMode();
        bUseLoopParsingMode = false;
    }
    bLockedPauseMode = false;
    _parser.ClearVectorVars();
    return;
}

int Loop::calc(string sLine, int nthCmd, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, string sBlock)
{
    string sLine_Temp = sLine;
    string sCache = "";
    string si_pos[2];
    string sj_pos[2];
    value_type* v = 0;
    int nNum = 0;

//    static string sVarName = "";
//    static double* dVarAdress = 0;

    //cerr << sLine << endl;
    bool bMultLinCol[2] = {false, false};
    int i_pos[2];
    int j_pos[2];
    int nProcedureCmd = 0;
    bool bWriteToCache = false;
    if (sLine.substr(sLine.find_first_not_of(' '),2) == "|>")
    {
        sLine.erase(sLine.find_first_not_of(' '),2);
        sLine_Temp.erase(sLine_Temp.find_first_not_of(' '),2);
        StripSpaces(sLine);
        StripSpaces(sLine_Temp);
        if (_option.getUseDebugger())
        {
            _option._debug.gatherLoopBasedInformations(sLine, nCmd-nthCmd, mVarMap, vVarArray, sVarArray, nVarArray);
            evalDebuggerBreakPoint(_option, _data.getStringVars());
        }
    }

    if (findCommand(sLine).sString == "throw" || sLine == "throw")
    {
        if (_option.getbDebug())
            cerr << "|-> DEBUG: throw found!" << endl;
        if (sLine.length() > 6 && (containsStrings(sLine) || _data.containsStringVars(sLine)))
        {
            if (_data.containsStringVars(sLine))
                _data.getStringValues(sLine);
            getStringArgument(sLine, sErrorToken);
            sErrorToken += " -nq";
            parser_StringParser(sErrorToken, sCache, _data, _parser, _option, true);
        }
        throw LOOP_THROW;
    }
    if (findCommand(sLine).sString == "return")
    {
        if (_option.getbDebug())
            cerr << "|-> DEBUG: return found: " << sLine << endl;
        if (sLine.find("void", sLine.find("return")+6) != string::npos)
        {
            string sReturnValue = sLine.substr(sLine.find("return")+6);
            StripSpaces(sReturnValue);
            if (sReturnValue == "void")
            {
                bReturnSignal = true;
                nReturnType = 0;
                return -2;
            }
        }
        sLine = sLine.substr(sLine.find("return") + 6);
        StripSpaces(sLine);
        if (_option.getbDebug())
            cerr << "|-> DEBUG: returning sLine = " << sLine << endl;
        if (!sLine.length())
        {
            ReturnVal.vNumVal.push_back(1.0);
            bReturnSignal = true;
            return -2;
        }
        bReturnSignal = true;
    }
    if (GetAsyncKeyState(VK_ESCAPE))
    {
        if (bPrintedStatus)
            cerr << " ABBRUCH!";
        throw PROCESS_ABORTED_BY_USER;
        ReturnVal.vNumVal.push_back(NAN);
        bReturnSignal = true;
        return -2;
    }

    if (_parser.IsValidByteCode() && _parser.GetExpr() == sLine + " " && !bLockedPauseMode && bUseLoopParsingMode)
    {
        //_parser.Eval();
        if (!bSilent)
        {
            /* --> Der Benutzer will also die Ergebnisse sehen. Es gibt die Moeglichkeit,
             *     dass der Parser mehrere Ausdruecke je Zeile auswertet. Dazu muessen die
             *     Ausdruecke durch Kommata getrennt sein. Damit der Parser bemerkt, dass er
             *     mehrere Ausdruecke auszuwerten hat, muss man die Auswerte-Funktion des
             *     Parsers einmal aufgerufen werden <--
             */
            v = _parser.Eval(nNum);
            cerr << "|" << sBlock << "> " << sLine_Temp << " => ";
            // --> Zahl der Ausdruecke erhalten <--
            //int nNum = _parser.GetNumResults();
            // --> Ist die Zahl der Ausdruecke groesser als 1? <--
            if (nNum > 1)
            {
                // --> Gib die nNum Ergebnisse an das Array, das durch Pointer v bestimmt wird <--
                //value_type *v = _parser.Eval(nNum);

                // --> Gebe die nNum Ergebnisse durch Kommata getrennt aus <--
                cerr << std::setprecision(_option.getPrecision()) << "[";
                vAns = v[0];
                for (int i=0; i<nNum; ++i)
                {
                    cerr << v[i];
                    if (i < nNum-1)
                        cerr << ", ";
                }
                cerr << "]" << endl;
            }
            else
            {
                // --> Zahl der Ausdruecke gleich 1? Dann einfach ausgeben <--
                //vAns = _parser.Eval();
                vAns = v[0];
                cerr << vAns << endl;
            }
        }
        else
        {
            /* --> Hier ist bSilent = TRUE: d.h., werte die Ausdruecke im Stillen aus,
             *     ohne dass der Benutzer irgendetwas davon mitbekommt <--
             */
            _parser.Eval();
        }
        return 1;
    }

    if ((findCommand(sLine).sString == "compose"
            || findCommand(sLine).sString == "endcompose"
            || sLoopPlotCompose.length())
        && findCommand(sLine).sString != "quit")
    {
        if (!sLoopPlotCompose.length() && findCommand(sLine).sString == "compose")
        {
            sLoopPlotCompose = "plotcompose ";
            return 1;
        }
        else if (findCommand(sLine).sString == "abort")
        {
            sLoopPlotCompose = "";
            //cerr << "|-> Deklaration abgebrochen." << endl;
            return 1;
        }
        else if (findCommand(sLine).sString != "endcompose")
        {
            string sCommand = findCommand(sLine).sString;
            if (sCommand.substr(0,4) == "plot"
                || sCommand.substr(0,4) == "grad"
                || sCommand.substr(0,4) == "dens"
                || sCommand.substr(0,4) == "vect"
                || sCommand.substr(0,4) == "cont"
                || sCommand.substr(0,4) == "surf"
                || sCommand.substr(0,4) == "mesh")
                sLoopPlotCompose += sLine + " <<COMPOSE>> ";
            return 1;
        }
        else
        {
            sLine = sLoopPlotCompose;
            sLoopPlotCompose = "";
        }
    }


    if (sLine.find("to_cmd(") != string::npos)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        unsigned int nPos = 0;
        while (sLine.find("to_cmd(", nPos) != string::npos)
        {
            nPos = sLine.find("to_cmd(", nPos) + 6;
            if (isInQuotes(sLine, nPos))
                continue;
            unsigned int nParPos = getMatchingParenthesis(sLine.substr(nPos));
            if (nParPos == string::npos)
                throw UNMATCHED_PARENTHESIS;
            string sCmdString = sLine.substr(nPos+1, nParPos-1);
            StripSpaces(sCmdString);
            if (containsStrings(sCmdString) || _data.containsStringVars(sCmdString))
            {
                sCmdString += " -nq";
                parser_StringParser(sCmdString, sCache, _data, _parser, _option, true);
                sCache = "";
            }
            sLine = sLine.substr(0, nPos-6) + sCmdString + sLine.substr(nPos + nParPos+1);
            nPos -= 5;
        }
        replaceLocalVars(sLine);
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }

    if (findCommand(sLine).sString == "progress" && sLine.length() > 9)
    {
        value_type* vVals = 0;
        string sExpr;
        string sArgument;
        int nArgument;
        if (containsStrings(sLine) || _data.containsStringVars(sLine))
        {
            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parser.PauseLoopMode();
            sLine = BI_evalParamString(sLine, _parser, _data, _option, _functions);
            if (bUseLoopParsingMode && !bLockedPauseMode)
                _parser.PauseLoopMode(false);
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
        }
        else
        {
            sArgument = "std";
            sExpr = "1,100";
        }
        while (sLine.length() && (sLine[sLine.length()-1] == ' ' || sLine[sLine.length()-1] == '-'))
            sLine.pop_back();
        if (!sLine.length())
            return 1;
        if (_parser.GetExpr() != sLine.substr(findCommand(sLine).nPos+8)+","+sExpr+" ")
        {
            if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr().length())
                _parser.DeclareAsInvalid();
            _parser.SetExpr(sLine.substr(findCommand(sLine).nPos+8)+","+sExpr);
        }
        vVals = _parser.Eval(nArgument);
        make_progressBar((int)vVals[0], (int)vVals[1], (int)vVals[2], sArgument);
        return 1;
    }

    // --> Prompt <--
    if (sLine.find("??") != string::npos)
    {
        //nValidByteCode[nthCmd] = 0;
        if (bPrintedStatus)
            mu::console() << endl;
        sLine = parser_Prompt(sLine);
        bPrintedStatus = false;
        if (_option.getbDebug())
            mu::console() << _T("|-> DEBUG: sLine = ") << sLine << endl;
    }

    // --> Prozeduren einbinden <--
    if (nJumpTable[nthCmd][2])
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode();
            _parser.LockPause();
        }
        int nReturn = procedureInterface(sLine, _parser, _functions, _data, _out, _pData, _script, _option, nLoop+nWhile+nIf, nthCmd);
        if (!bLockedPauseMode && bUseLoopParsingMode)
        {
            _parser.PauseLoopMode(false);
            _parser.LockPause(false);
        }
        if (nReturn == -2 || nReturn == 2)
            nJumpTable[nthCmd][2] = 1;
        else
            nJumpTable[nthCmd][2] = 0;
        if (nReturn == -1)
            return -1;
        else if (nReturn == -2)
            return 1;
        if (_option.getbDebug())
            cerr << "|-> DEBUG: sLine = " << sLine << endl;
    }

    // --> Procedure-CMDs einbinden <--
    nProcedureCmd = procedureCmdInterface(sLine);
    if (nProcedureCmd)
    {
        if (nProcedureCmd == 1)
            return 1;
    }
    else
        return -1;
    if (findCommand(sLine).sString == "explicit")
    {
        sLine.erase(findCommand(sLine).nPos,8);
        StripSpaces(sLine);
    }
    if (!bLockedPauseMode && bUseLoopParsingMode)
        _parser.PauseLoopMode();
    switch (BI_CheckKeyword(sLine, _data, _out, _option, _parser, _functions, _pData, _script, true))
    {
        case  0: break;
        case  1:
            SetConsTitle(_data, _option);
            return 1;
        case -1: return 1;
        case  2: return 1;
    }
    if (!bLockedPauseMode && bUseLoopParsingMode)
        _parser.PauseLoopMode(false);
    // --> Prompt <--
    if (sLine.find("??") != string::npos)
    {
        //nValidByteCode[nthCmd] = 0;
        if (bPrintedStatus)
            mu::console() << endl;
        sLine = parser_Prompt(sLine);
        bPrintedStatus = false;
        if (_option.getbDebug())
            mu::console() << _T("|-> DEBUG: sLine = ") << sLine << endl;
    }


    evalRecursiveExpressions(sLine);

    // --> Datafile/Cache! <--
    if (!containsStrings(sLine)
        && !_data.containsStringVars(sLine)
        && (sLine.find("data(") != string::npos || _data.containsCacheElements(sLine)))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        sCache = parser_GetDataElement(sLine, _parser, _data, _option);
        if (sCache.length() && sCache.find('#') == string::npos)
            bWriteToCache = true;
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }

    // --> String-Parser <--
    if (containsStrings(sLine) || _data.containsStringVars(sLine))
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        int nReturn = parser_StringParser(sLine, sCache, _data, _parser, _option, bSilent);
        if (nReturn)
        {
            if (nReturn == 1)
            {
                if (!bLockedPauseMode && bUseLoopParsingMode)
                    _parser.PauseLoopMode(false);
                if (bReturnSignal)
                {
                    ReturnVal.vStringVal.push_back(sLine);
                    return -2;
                }
                return 1;
            }
        }
        else
        {
            throw STRING_ERROR;
        }
        replaceLocalVars(sLine);
        if (sCache.length() && _data.containsCacheElements(sCache) && !bWriteToCache)
            bWriteToCache = true;
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }
    if (bWriteToCache)
    {
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode();
        //cerr << bWriteToCache << endl;
        StripSpaces(sCache);
        while (sCache[0] == '(')
            sCache.erase(0,1);
        si_pos[0] = sCache.substr(sCache.find('('));
        parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);

        if (si_pos[0].find(':') == string::npos && sj_pos[0].find(':') == string::npos)
        {
            StripSpaces(si_pos[0]);
            StripSpaces(sj_pos[0]);
            if (!si_pos[0].length() || !sj_pos[0].length())
            {
                throw INVALID_INDEX;
            }
            _parser.SetExpr(si_pos[0] + "," + sj_pos[0]);
            _parser.Eval();
            value_type* v = 0;
            int nResults = _parser.GetNumResults();
            v = _parser.Eval(nResults);
            i_pos[0] = (int)v[0]-1;
            if (i_pos[0] < 0)
                i_pos[0] = 0;
            i_pos[1] = i_pos[0];
            j_pos[0] = (int)v[1]-1;
            if (j_pos[0] < 0)
                j_pos[0] = 0;
            j_pos[1] = j_pos[0];
        }
        else
        {
            if (si_pos[0].find(":") != string::npos)
            {
                si_pos[1] = si_pos[0].substr(si_pos[0].find(":")+1);
                si_pos[0] = si_pos[0].substr(0, si_pos[0].find(":"));
                bMultLinCol[0] = true;
            }
            if (sj_pos[0].find(":") != string::npos)
            {
                sj_pos[1] = sj_pos[0].substr(sj_pos[0].find(":")+1);
                sj_pos[0] = sj_pos[0].substr(0, sj_pos[0].find(":"));
                bMultLinCol[1] = true;
            }
            if (bMultLinCol[0] && bMultLinCol[1])
            {
                throw NO_MATRIX;
            }
            if (parser_ExprNotEmpty(si_pos[0]))
            {
                _parser.SetExpr(si_pos[0]);
                i_pos[0] = (int)_parser.Eval();
                i_pos[0]--;
            }
            else
                i_pos[0] = 0;

            if (i_pos[0] < 0)
                i_pos[0] = 0;

            if (parser_ExprNotEmpty(sj_pos[0]))
            {
                _parser.SetExpr(sj_pos[0]);
                j_pos[0] = (int)_parser.Eval();
                j_pos[0]--;
            }
            else
                j_pos[0] = 0;

            if (j_pos[0] < 0)
                j_pos[0] = 0;

            if (parser_ExprNotEmpty(si_pos[1]) && bMultLinCol[0])
            {
                _parser.SetExpr(si_pos[1]);
                i_pos[1] = (int)_parser.Eval();
                i_pos[1]--;
                parser_CheckIndices(i_pos[0], i_pos[1]);
            }
            else if (bMultLinCol[0])
                si_pos[1] = "inf";
            else
                i_pos[1] = i_pos[0];

            if (parser_ExprNotEmpty(sj_pos[1]) && bMultLinCol[1])
            {
                _parser.SetExpr(sj_pos[1]);
                j_pos[1] = (int)_parser.Eval();
                j_pos[1]--;
                parser_CheckIndices(j_pos[0], j_pos[1]);
            }
            else if (bMultLinCol[1])
                sj_pos[1] = "inf";
            else
                j_pos[1] = j_pos[0];
        }
        /*cerr << i_pos[0] << " "
             << i_pos[1] << " "
             << j_pos[0] << " "
             << j_pos[1] << endl;
        cerr << si_pos[0] << " "
             << si_pos[1] << " "
             << sj_pos[0] << " "
             << sj_pos[1] << endl;*/
        if (!bLockedPauseMode && bUseLoopParsingMode)
            _parser.PauseLoopMode(false);
    }
    // --> Vector-Ausdruck <--
    if (sLine.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
    {
        parser_VectorToExpr(sLine, _option);
    }

    /* --> Hier setzt wieder der x=x+1-Workaround an: eine Erlaeuterung findet sich in der
     *     Funktion parser_Calc(Parser&, Settings&) <--
     */
    while (sLine.find(":=") != string::npos)
    {
        sLine.erase(sLine.find(":="), 1);
    }
    // --> Uebliche Auswertung <--

    //cerr << "\"" << _parser.GetExpr() << "\"/\"" << sLine+" " << "\"" << endl;
    if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: ByteCodeState = " << _parser.IsValidByteCode() << " / ParserExpr = " << _parser.GetExpr() << endl;
        //cerr << "|-> DEBUG: LockedPauseMode = " << bLockedPauseMode << endl;
    }
    if (_parser.GetExpr() != sLine+" " /*|| nValidByteCode[nthCmd] != 1*/)
    {
        if (bUseLoopParsingMode && !bLockedPauseMode && _parser.IsValidByteCode() && _parser.GetExpr().length())
            _parser.DeclareAsInvalid();
        _parser.SetExpr(sLine);
    }
/*else if (nValidByteCode[nthCmd] == 1)
        _parser.SetByteCode(_bytecode[nthCmd]);*/
    v = _parser.Eval(nNum);
    vAns = v[0];
    //cerr << vAns << " " << nNum << endl;
    if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: ByteCodeState = " << _parser.IsValidByteCode() << " / ParserExpr = " << _parser.GetExpr() << endl;
    }
    /*if (nValidByteCode[nthCmd] == -1 && _parser.GetExpr() == sLine_Temp + " ")
    {
        nValidByteCode[nthCmd] = 2;
        _bytecode[nthCmd] = _parser.GetByteCode();
    }
    else if (nValidByteCode[nthCmd] == -1 && bWriteToCache && sCache + sLine == sLine_Temp)
    {
        nValidByteCode[nthCmd] = 1;
        _bytecode[nthCmd] = _parser.GetByteCode();
    }
    else
        nValidByteCode[nthCmd] = 0;*/
    if (!bSilent)
    {
        /* --> Der Benutzer will also die Ergebnisse sehen. Es gibt die Moeglichkeit,
         *     dass der Parser mehrere Ausdruecke je Zeile auswertet. Dazu muessen die
         *     Ausdruecke durch Kommata getrennt sein. Damit der Parser bemerkt, dass er
         *     mehrere Ausdruecke auszuwerten hat, muss man die Auswerte-Funktion des
         *     Parsers einmal aufgerufen werden <--
         */
        //_parser.Eval();
        cerr << "|" << sBlock << "> " << sLine_Temp << " => ";
        // --> Zahl der Ausdruecke erhalten <--
        //int nNum = _parser.GetNumResults();
        // --> Ist die Zahl der Ausdruecke groesser als 1? <--
        if (nNum > 1)
        {
            // --> Gib die nNum Ergebnisse an das Array, das durch Pointer v bestimmt wird <--
            //value_type *v = _parser.Eval(nNum);

            // --> Gebe die nNum Ergebnisse durch Kommata getrennt aus <--
            cerr << std::setprecision(_option.getPrecision()) << "{";
            //vAns = v[0];
            for (int i=0; i<nNum; ++i)
            {
                cerr << v[i];
                if (i < nNum-1)
                    cerr << ", ";
            }
            cerr << "}" << endl;
        }
        else
        {
            // --> Zahl der Ausdruecke gleich 1? Dann einfach ausgeben <--
            //vAns = _parser.Eval();
            //vAns = v[0];
            cerr << vAns << endl;
        }
    }
    if (bWriteToCache)
    {
        /*cerr << i_pos[0] << " "
             << i_pos[1] << " "
             << j_pos[0] << " "
             << j_pos[1] << endl;
        cerr << si_pos[0] << " "
             << si_pos[1] << " "
             << sj_pos[0] << " "
             << sj_pos[1] << endl;
        cerr << sCache << endl;
        cerr << nNum << endl;*/
        if (i_pos[0] == i_pos[1] && j_pos[0] == j_pos[1])
        {
            if (nNum == 1)
            {
                double dResult = vAns;
                if (_option.getbDebug())
                    mu::console() << _T("|-> DEBUG: i_pos[0] = ") << i_pos[0] <<  _T(", j_pos[0] = ") << j_pos[0] << endl;
                if (!_data.writeToCache(i_pos[0], j_pos[0], sCache.substr(0,sCache.find('(')), dResult))
                    return -1;
            }
            else
            {
                for (int i = i_pos[0]; i < i_pos[0] + nNum; i++)
                {
                    if (!_data.writeToCache(i, j_pos[0], sCache.substr(0,sCache.find('(')), (double)v[i-i_pos[0]]))
                        return -1;
                }
            }
        }
        else
        {
            if (si_pos[1] == "inf")
                i_pos[1] = i_pos[0] + nNum;
            if (sj_pos[1] == "inf")
                j_pos[1] = j_pos[0] + nNum;
            for (int i = i_pos[0]; i <= i_pos[1]; i++)
            {
                for (int j = j_pos[0]; j <= j_pos[1]; j++)
                {
                    if ((i-i_pos[0] == nNum && i_pos[0] != i_pos[1])
                        || (j-j_pos[0] == nNum && j_pos[0] != j_pos[1]))
                        break;
                    if (i_pos[0] != i_pos[1])
                    {
                        if (!_data.writeToCache(i,j,sCache.substr(0,sCache.find('(')), (double)v[i-i_pos[0]]))
                            return -1;
                    }
                    else if (!_data.writeToCache(i,j,sCache.substr(0,sCache.find('(')), (double)v[j-j_pos[0]]))
                        return -1;
                }
            }
        }
        bWriteToCache = false;
        for (int i = 0; i < 2; i++)
        {
            bMultLinCol[i] = false;
            si_pos[i] = "";
            sj_pos[i] = "";
            i_pos[i] = -1;
            j_pos[i] = -1;
        }
    }
    if (bReturnSignal)
    {
        for (int i = 0; i < nNum; i++)
            ReturnVal.vNumVal.push_back(v[i]);
        //ReturnVal.dNumVal = vAns;
        return -2;
    }
    return 1;
}

int Loop::procedureInterface(string& sLine, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_loop, int nth_command)
{
    cerr << "LOOP_procedureInterface" << endl;
    return 1;
}

int Loop::procedureCmdInterface(string& sLine)
{
    cerr << "LOOP_procedureCmdInterface" << endl;
    return 1;
}

bool Loop::isInline(const string& sProc)
{
    cerr << "LOOP_isInline" << endl;
    return true;
}

void Loop::replaceLocalVars(string& sLine)
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

void Loop::evalDebuggerBreakPoint(Settings& _option, const map<string,string>& sStringMap)
{
    return;
}

