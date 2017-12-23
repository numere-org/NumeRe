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

#define FLAG_EXPLICIT 1
#define FLAG_INLINE 2


Procedure::Procedure() : FlowCtrl(), Plugin()
{
    sProcNames = "";
    sCurrentProcedureName = "";
    sLastWrittenProcedureFile = "";
    sProcKeep = "";
    sProcPlotCompose = "";
    sNameSpace = "";
    sCallingNameSpace = "main";
    sThisNameSpace = "";
    bProcSupressAnswer = false;
    bWritingTofile = false;
    nCurrentLine = 0;
    nthBlock = 0;
    nFlags = 0;
    //nReturnType = 1;

    sVars = 0;
    dVars = 0;
    sStrings = 0;

    nVarSize = 0;
    nStrSize = 0;
}

Procedure::Procedure(const Procedure& _procedure) : FlowCtrl(), Plugin(_procedure)
{
    Procedure();
    sVars = 0;
    dVars = 0;
    sStrings = 0;

    nVarSize = 0;
    nStrSize = 0;
    sPath = _procedure.sPath;
    sWhere = _procedure.sWhere;
    sCallingNameSpace = _procedure.sCallingNameSpace;
    sProcNames = _procedure.sProcNames;
    for (unsigned int i = 0; i < 6; i++)
    {
        sTokens[i][0] = _procedure.sTokens[i][0];
        sTokens[i][1] = _procedure.sTokens[i][1];
    }
    //Procedure::Plugin() = _procedure;
}

Procedure::~Procedure()
{
    if (fProcedure.is_open())
        fProcedure.close();
}


Returnvalue Procedure::ProcCalc(string sLine, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script)
{
    string sCache = "";
    string sCurrentCommand = findCommand(sLine).sString;

    Indices _idx;


    if (!_parser.ActiveLoopMode() || (!_parser.IsLockedPause() && !(nFlags & FLAG_INLINE)))
        _parser.ClearVectorVars(true);

    bool bWriteToCache = false;
    Returnvalue thisReturnVal;

    int nNum = 0;
    value_type* v = 0;

    if (_script.wasLastCommand() && _option.getSystemPrintStatus())
        cerr << LineBreak("|-> "+_lang.get("PARSER_SCRIPT_FINIHED", _script.getScriptFileName()), _option) << endl;

    if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
    {
        throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
    }
    // --> Leerzeichen und Tabulatoren entfernen <--
    StripSpaces(sLine);
    //cerr << sLine << endl;
    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '\t')
            sLine[i] = ' ';
    }

    // --> Keine Laenge? Ignorieren! <--
    if (!sLine.length() || sLine[0] == '@')
    {
        thisReturnVal.vNumVal.push_back(NAN);
        return thisReturnVal;
    }
    // --> Kommando "global" entfernen <--
    if (sCurrentCommand == "global")
    {
        sLine = sLine.substr(findCommand(sLine).nPos+6);
        StripSpaces(sLine);
    }
    // --> Wenn die Laenge groesser als 2 ist, koennen '\' am Ende sein <--
    if (sLine.length() > 2)
    {
        if (sLine.substr(sLine.length()-2,2) == "\\\\")
        {
            // --> Ergaenze die Eingabe zu sProcKeep und beginne einen neuen Schleifendurchlauf <--
            sProcKeep += sLine.substr(0,sLine.length()-2);
            thisReturnVal.vNumVal.push_back(NAN);
            return thisReturnVal;
        }
    }

    /* --> Steht etwas in sProcKeep? Ergaenze die aktuelle Eingabe, weise dies
     *     wiederum an sLine zu und loesche den Inhalt von sProcKeep <--
     */
    if (sProcKeep.length())
    {
        sProcKeep += sLine;
        sLine = sProcKeep;
        sProcKeep = "";
    }

    if ((sCurrentCommand == "compose"
            || sCurrentCommand == "endcompose"
            || sProcPlotCompose.length())
        && sCurrentCommand != "quit")
    {
        if (!sProcPlotCompose.length() && sCurrentCommand == "compose")
        {
            sProcPlotCompose = "plotcompose ";
            if (matchParams(sLine, "multiplot", '='))
            {
                sProcPlotCompose += "-multiplot=" + getArgAtPos(sLine, matchParams(sLine, "multiplot", '=')+9) + " <<COMPOSE>> ";
            }
            thisReturnVal.vNumVal.push_back(NAN);
            return thisReturnVal;
        }
        else if (sCurrentCommand == "abort")
        {
            sProcPlotCompose = "";
            //cerr << "|-> Deklaration abgebrochen." << endl;
            thisReturnVal.vNumVal.push_back(NAN);
            return thisReturnVal;
        }
        else if (sCurrentCommand != "endcompose")
        {
            string sCommand = findCommand(sLine).sString;
            if (sCommand.substr(0,4) == "plot"
                || sCommand.substr(0,7) == "subplot"
                || sCommand.substr(0,4) == "grad"
                || sCommand.substr(0,4) == "dens"
                || sCommand.substr(0,4) == "vect"
                || sCommand.substr(0,4) == "cont"
                || sCommand.substr(0,4) == "surf"
                || sCommand.substr(0,4) == "mesh")
                sProcPlotCompose += sLine + " <<COMPOSE>> ";
            {
                thisReturnVal.vNumVal.push_back(NAN);
                return thisReturnVal;
            }
        }
        else
        {
            sLine = sProcPlotCompose;
            sProcPlotCompose = "";
        }
    }

    if (sLine.find("to_cmd(") != string::npos && !getLoop())
    {
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
        sCurrentCommand = findCommand(sLine).sString;
    }

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
        throw SyntaxError(SyntaxError::PROCEDURE_THROW, sLine, SyntaxError::invalid_position, sErrorToken);
    }
    // --> Ist das letzte Zeichen ein ';'? Dann weise bProcSupressAnswer TRUE zu <--
    while (sLine.back() == ';')
    {
        sLine.pop_back();
        StripSpaces(sLine);
        bProcSupressAnswer = true;
    }
    // --> Gibt es "??"? Dann rufe die Prompt-Funktion auf <--
    if (!getLoop() && sLine.find("??") != string::npos && sCurrentCommand != "help")
        sLine = parser_Prompt(sLine);

    /* --> Die Keyword-Suche soll nur funktionieren, wenn keine Schleife eingegeben wird, oder wenn eine
     *     eine Schleife eingegeben wird, dann nur in den wenigen Spezialfaellen, die zum Nachschlagen
     *     eines Keywords noetig sind ("list", "help", "find", etc.) <--
     */
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
        //bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;
        NumeReKernel::bSupressAnswer = bProcSupressAnswer;
        switch (BI_CommandHandler(sLine, _data, _out, _option, _parser, _functions, _pData, _script, true))
        {
            case  0: break; // Kein Keywort: Mit dem Parser auswerten
            case  1:        // Keywort: Naechster Schleifendurchlauf!
                thisReturnVal.vNumVal.push_back(NAN);
                //NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                return thisReturnVal;
            default:
                thisReturnVal.vNumVal.push_back(NAN);
                //NumeReKernel::bSupressAnswer = bSupressAnswer_back;
                return thisReturnVal;  // Keywort "mode"
        }
        //NumeReKernel::bSupressAnswer = bSupressAnswer_back;
    }


    // --> Wenn die call()-Methode FALSE zurueckgibt, ist etwas schief gelaufen! <--
    if (!getLoop() && sCurrentCommand != "for" && sCurrentCommand != "if" && sCurrentCommand != "while")
    {
        if (!_functions.call(sLine, _option))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, sLine, SyntaxError::invalid_position);
    }
    // --> Nochmals ueberzaehlige Leerzeichen entfernen <--
    StripSpaces(sLine);

    if (!getLoop())
    {
        bool bBreakPoint = (sLine.substr(sLine.find_first_not_of(" \t"),2) == "|>");
        if (bBreakPoint)
        {
            sLine.erase(sLine.find_first_not_of(" \t"),2);
            StripSpaces(sLine);
        }
        evalRecursiveExpressions(sLine);
        if (bBreakPoint)
            sLine.insert(0,"|> ");
    }

    if (_option.getbDebug())
        cerr << "|-> DEBUG: sLine = " << sLine << endl;

    // --> Befinden wir uns in einem Loop? Dann ist nLoop > -1! <--
    //cerr << getLoop() << endl;
    if (getLoop() || sCurrentCommand == "for" || sCurrentCommand == "if" || sCurrentCommand == "while")
    {
        // --> Die Zeile in den Ausdrucksspeicher schreiben, damit sie spaeter wiederholt aufgerufen werden kann <--
        setCommand(sLine, _parser, _data, _functions, _option, _out, _pData, _script);
        //cerr << getLoop() << endl;
        /* --> So lange wir im Loop sind und nicht endfor aufgerufen wurde, braucht die Zeile nicht an den Parser
         *     weitergegeben werden. Wir ignorieren daher den Rest dieser for(;;)-Schleife <--
         */
        thisReturnVal.vNumVal.push_back(NAN);
        return thisReturnVal;

    }

    // --> Gibt es "??" ggf. nochmal? Dann rufe die Prompt-Funktion auf <--
    if (sLine.find("??") != string::npos)
        sLine = parser_Prompt(sLine);

    // --> Gibt es "data(" oder "cache("? Dann rufe die GetDataElement-Methode auf <--
    if (!containsStrings(sLine)
        && !_data.containsStringVars(sLine)
        && (sLine.find("data(") != string::npos || _data.containsCacheElements(sLine)))
    {
        sCache = parser_GetDataElement(sLine, _parser, _data, _option);
        if (sCache.length() && sCache.find('#') == string::npos)
            bWriteToCache = true;
    }

    /** TODO: Diese Loesung kommt noch nicht mit "A ? x : y" zurecht!
      * --> Ggf. waere es eine Loesung, selbst eine Auswertung des Ternary's zu schreiben.
      */

    // --> Workaround fuer den x = x+1-Bug: In diesem Fall sollte die Eingabe x := x+1 lauten und wird hier weiterverarbeitet <--
    while (sLine.find(":=") != string::npos)
    {
        sLine.erase(sLine.find(":="), 1);
    }

    //cerr << sLine << endl;

    // --> String-Syntax ("String" oder #VAR)? String-Parser aufrufen und mit dem naechsten Schleifendurchlauf fortfahren <--
    if (containsStrings(sLine) || _data.containsStringVars(sLine))
    {
        int nReturn = parser_StringParser(sLine, sCache, _data, _parser, _option, bProcSupressAnswer);
        if (nReturn == 1)
        {
            thisReturnVal.vStringVal.push_back(sLine);
            return thisReturnVal;
        }
        else if (!nReturn)
        {
            throw SyntaxError(SyntaxError::STRING_ERROR, sLine, SyntaxError::invalid_position);
        }
        if (sCache.length() && _data.containsCacheElements(sCache) && !bWriteToCache)
            bWriteToCache = true;
        replaceLocalVars(sLine);
    }

    // --> Moeglicherweise erscheint nun "{{". Dies muss ersetzt werden <--
    if (sLine.find("{") != string::npos && (containsStrings(sLine) || _data.containsStringVars(sLine)))
    {
        parser_VectorToExpr(sLine, _option);
    }


    // --> Wenn die Ergebnisse in den Cache geschrieben werden sollen, bestimme hier die entsprechenden Koordinaten <--
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

    // --> Ausdruck an den Parser uebergeben und einmal auswerten <--
    if (sLine + " " != _parser.GetExpr())
        _parser.SetExpr(sLine);
    //cerr << std::setprecision(_option.getPrecision());
    v = _parser.Eval(nNum);

    // --> Jetzt weiss der Parser, wie viele Ergebnisse er berechnen muss <--
    if (nNum > 1)
    {
        for (int i = 0; i < nNum; ++i)
            thisReturnVal.vNumVal.push_back(v[i]);
        if (!bProcSupressAnswer)
        {
            //cerr << std::setprecision(_option.getPrecision());
            int nLineBreak = parser_LineBreak(_option);
            NumeReKernel::toggleTableStatus();
            NumeReKernel::printPreFmt("|-> ans = {");
            for (int i = 0; i < nNum; ++i)
            {
                NumeReKernel::printPreFmt(strfill(toString(v[i], _option), _option.getPrecision()+7));
                if (i < nNum-1)
                    NumeReKernel::printPreFmt(", ");
                if (nNum + 1 > nLineBreak && !((i+1) % nLineBreak) && i < nNum-1)
                    NumeReKernel::printPreFmt("...\n|          ");
            }
            NumeReKernel::toggleTableStatus();
            NumeReKernel::printPreFmt("}\n");
        }
    }
    else
    {
        thisReturnVal.vNumVal.push_back(v[0]);

        if (!bProcSupressAnswer)
            NumeReKernel::print("ans = " + toString(thisReturnVal.vNumVal[0], _option));
    }
    if (bWriteToCache)
        _data.writeToCache(_idx, sCache, v, nNum);
    if (!_parser.ActiveLoopMode() || (!_parser.IsLockedPause() && !(nFlags & FLAG_INLINE)))
        _parser.ClearVectorVars(true);
    return thisReturnVal;
}

bool Procedure::setProcName(const string& sProc, bool bInstallFileName)
{
    if (sProc.length())
    {
        string _sProc = sProc;
        //cerr << sProc << endl;
        //cerr << sProcNames << endl;

        if (sProcNames.length() && !bInstallFileName && _sProc.substr(0,9) == "thisfile~")
        {
            sCurrentProcedureName = sProcNames.substr(sProcNames.rfind(';')+1);
            sProcNames += ";" + sCurrentProcedureName;
            return true;
        }
        else if (sLastWrittenProcedureFile.length() && bInstallFileName && _sProc.substr(0,9) == "thisfile~")
        {
            sCurrentProcedureName = sLastWrittenProcedureFile.substr(0,sLastWrittenProcedureFile.find('|'));
            return true;
        }
        else if (_sProc.substr(0,9) == "thisfile~")
            return false;
        sCurrentProcedureName = FileSystem::ValidFileName(sProc, ".nprc");
        if (sCurrentProcedureName.find('~') != string::npos)
        {
            unsigned int nPos = sCurrentProcedureName.rfind('/');
            if (nPos < sCurrentProcedureName.rfind('\\') && sCurrentProcedureName.rfind('\\') != string::npos)
                nPos = sCurrentProcedureName.rfind('\\');
            for (unsigned int i = nPos; i < sCurrentProcedureName.length(); i++)
            {
                if (sCurrentProcedureName[i] == '~')
                {
                    if (sCurrentProcedureName.length() > 5 && i >= 4 && sCurrentProcedureName.substr(i-4,5) == "main~")
                        sCurrentProcedureName = sCurrentProcedureName.substr(0,i-4) + sCurrentProcedureName.substr(i+1);
                    else
                        sCurrentProcedureName[i] = '/';
                }
            }
        }
        //cerr << sCurrentProcedureName << endl;
        sProcNames += ";" + sCurrentProcedureName;
        return true;
    }
    else
        return false;
}

Returnvalue Procedure::execute(string sProc, string sVarList, Parser& _parser, Define& _functions, Datafile& _data, Settings& _option, Output& _out, PlotData& _pData, Script& _script, unsigned int nth_procedure)
{
    StripSpaces(sProc);
    _option._debug.pushStackItem(sProc + "(" + sVarList + ")");

    if (!setProcName(sProc))
        throw SyntaxError(SyntaxError::INVALID_PROCEDURE_NAME, "$" + sProc + "(" + sVarList + ")", SyntaxError::invalid_position);

    ifstream fProc_in;
    ifstream fInclude;

    string sProcCommandLine = "";
    string sCmdCache = "";
    bool bBlockComment = false;
    bool bReadingFromInclude = false;
    int nIncludeType = 0;
    Returnvalue _ReturnVal;

    sThisNameSpace = "";
    nCurrentLine = 0;
    nFlags = 0;
    nReturnType = 1;
    bReturnSignal = false;
    nthRecursion = nth_procedure;
    bool bSupressAnswer_back = NumeReKernel::bSupressAnswer;

    ProcedureElement* ProcElement;
    pair<int,string> currentLine;
    currentLine.first = -1;

    ProcedureVarFactory _varfactory(this, &_parser, &_data, &_option, &_functions, &_out, &_pData, &_script, sProc, nth_procedure);
    ProcElement = NumeReKernel::ProcLibrary.getProcedureContents(sCurrentProcedureName);

    // add spaces in front of and at the end of sVarList
    sVarList = " " + sVarList + " ";

    if (sProc.length() > 5 && sProc.substr(sProc.length()-5) == ".nprc")
        sProc = sProc.substr(0, sProc.rfind('.'));

    for (unsigned int i = sProc.length()-1; i >= 0; i--)
    {
        if (sProc[i] == '\\' || sProc[i] == '/' || sProc[i] == '~')
        {
            sThisNameSpace = sProc.substr(0,i);
            //NumeReKernel::print("NameSpace = " + sThisNameSpace);
            if (sThisNameSpace.find(':') == string::npos)
            {
                for (unsigned int j = 0; j < sThisNameSpace.length(); j++)
                {
                    if (sThisNameSpace[j] == '\\' || sThisNameSpace[j] == '/')
                        sThisNameSpace[j] = '~';
                }
            }
            //NumeReKernel::print("NameSpace = " + sThisNameSpace);
            break;
        }
        if (!i)
        {
            sThisNameSpace = "main";
            break;
        }
    }
    if (sThisNameSpace == "thisfile")
        sThisNameSpace = sCallingNameSpace;
    //cerr << sCallingNameSpace << endl;
    //cerr << sThisNameSpace << endl;

    if (sProc.find('~') != string::npos)
        sProc = sProc.substr(sProc.rfind('~')+1);
    if (sProc.find('/') != string::npos)
        sProc = sProc.substr(sProc.rfind('/')+1);
    if (sProc.find('\\') != string::npos)
        sProc = sProc.substr(sProc.rfind('\\')+1);

    //fProc_in.seekg(0);
    while (!ProcElement->isLastLine(nCurrentLine))
    {
        if (currentLine.first < 0)
        {
            currentLine = ProcElement->getFirstLine();
            nCurrentLine = currentLine.first;
            sProcCommandLine = currentLine.second;
        }
        else
        {
            if (!bReadingFromInclude)
            {
                currentLine = ProcElement->getNextLine(currentLine.first);
                nCurrentLine = currentLine.first;
                sProcCommandLine = currentLine.second;
            }
            else
            {
                bool bSkipNextLine = false;
                bool bAppendNextLine = false;
                while (!fInclude.eof())
                {
                    getline(fInclude, sProcCommandLine);
                    //cerr << sProcCommandLine << endl;
                    //nIncludeLine++;
                    StripSpaces(sProcCommandLine);
                    if (!sProcCommandLine.length())
                        continue;
                    if (sProcCommandLine.substr(0,2) == "##")
                        continue;
                    if (sProcCommandLine.substr(0,9) == "<install>"
                        || (findCommand(sProcCommandLine).sString == "global" && sProcCommandLine.find("<install>") != string::npos))
                    {
                        while (!fInclude.eof())
                        {
                            getline(fInclude, sProcCommandLine);
                            //nIncludeLine++;
                            StripSpaces(sProcCommandLine);
                            if (sProcCommandLine.substr(0,12) == "<endinstall>"
                                || (findCommand(sProcCommandLine).sString == "global" && sProcCommandLine.find("<endinstall>") != string::npos))
                                break;
                        }
                        sProcCommandLine = "";
                        continue;
                    }

                    if (sProcCommandLine.find("##") != string::npos)
                        sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find("##"));

                    if (sProcCommandLine.substr(0,2) == "#*" && sProcCommandLine.find("*#",2) == string::npos)
                    {
                        bBlockComment = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    if (bBlockComment && sProcCommandLine.find("*#") != string::npos)
                    {
                        bBlockComment = false;
                        if (sProcCommandLine.find("*#") == sProcCommandLine.length()-2)
                        {
                            sProcCommandLine = "";
                            continue;
                        }
                        else
                            sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("*#")+2);
                    }
                    else if (bBlockComment && sProcCommandLine.find("*#") == string::npos)
                    {
                        sProcCommandLine = "";
                        continue;
                    }
                    if (bSkipNextLine && sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
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
                    if (findCommand(sProcCommandLine).sString != "define"
                        && findCommand(sProcCommandLine).sString != "ifndef"
                        && findCommand(sProcCommandLine).sString != "ifndefined"
                        && findCommand(sProcCommandLine).sString != "redefine"
                        && findCommand(sProcCommandLine).sString != "redef"
                        && findCommand(sProcCommandLine).sString != "global"
                        && !bAppendNextLine)
                    {
                        if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                            bSkipNextLine = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    if (nIncludeType == 1
                        && findCommand(sProcCommandLine).sString != "define"
                        && findCommand(sProcCommandLine).sString != "ifndef"
                        && findCommand(sProcCommandLine).sString != "ifndefined"
                        && findCommand(sProcCommandLine).sString != "redefine"
                        && findCommand(sProcCommandLine).sString != "redef"
                        && !bAppendNextLine)
                    {
                        if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                            bSkipNextLine = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    else if (nIncludeType == 2
                        && findCommand(sProcCommandLine).sString != "global"
                        && !bAppendNextLine)
                    {
                        if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                            bSkipNextLine = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                        bAppendNextLine = true;
                    else
                        bAppendNextLine = false;

                    if (sProcCommandLine.back() == ';')
                    {
                        bProcSupressAnswer = true;
                        //bSupressAnswer = true;
                        sProcCommandLine.pop_back();
                    }

                    if (sProcCommandLine.find('$') != string::npos && sProcCommandLine.find('(', sProcCommandLine.find('$')) != string::npos)
                    {
                        int nReturn = procedureInterface(sProcCommandLine, _parser, _functions, _data, _out, _pData, _script, _option, nth_procedure);
                        if (nReturn == -1)
                        {
                            if (_option.getUseDebugger())
                            {
                                _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                            }
                            throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sProcCommandLine, SyntaxError::invalid_position);
                        }
                        else if (nReturn == -2)
                        {
                            sProcCommandLine = "";
                            bProcSupressAnswer = false;
                            //bSupressAnswer = false;
                            continue;
                        }
                    }

                    ProcCalc(sProcCommandLine, _parser, _functions, _data, _option, _out, _pData, _script);
                    sProcCommandLine = "";
                    if (bProcSupressAnswer)
                        bProcSupressAnswer = false;
                }

                if (fInclude.eof())
                {
                    fInclude.close();
                    //nIncludeLine = 0;
                    bReadingFromInclude = false;
                    nIncludeType = 0;
                }
            }
        }
        if (sProcCommandLine[0] != '@' && findCommand(sProcCommandLine).sString != "procedure")
            continue;
        else if (sProcCommandLine[0] == '@' && sProcCommandLine[1] != ' ' && !bReadingFromInclude)
        {
            string sIncludeFileName = "";
            if (sProcCommandLine[1] == '"')
                sIncludeFileName = sProcCommandLine.substr(2,sProcCommandLine.find('"', 2)-2);
            else
                sIncludeFileName = sProcCommandLine.substr(1,sProcCommandLine.find(' ')-1);
            if (sProcCommandLine.find(':') != string::npos)
            {
                if (sProcCommandLine.find("defines", sProcCommandLine.find(':')+1) != string::npos)
                {
                    nIncludeType = 1;
                }
                else if (sProcCommandLine.find("globals", sProcCommandLine.find(':')+1) != string::npos)
                {
                    nIncludeType = 2;
                }
                else if (sProcCommandLine.find("procedures", sProcCommandLine.find(':')+1) != string::npos)
                {
                    nIncludeType = 3;
                }
            }
            if (sIncludeFileName.find(':') != string::npos)
            {
                for (int __i = sIncludeFileName.length()-1; __i >= 0; __i--)
                {
                    if (sIncludeFileName[__i] == ':'
                        && (__i > 1
                            || (__i == 1 && sIncludeFileName.length() > (unsigned int)__i+1 && sIncludeFileName[__i+1] != '/')))
                    {
                        sIncludeFileName.erase(sIncludeFileName.find(':'));
                        break;
                    }
                }
            }
            if (sIncludeFileName.length())
                sIncludeFileName = _script.ValidFileName(sIncludeFileName, ".nscr");
            else
                continue;
            //cerr << sIncludeFileName << endl;
            bReadingFromInclude = true;
            fInclude.clear();
            fInclude.open(sIncludeFileName.c_str());
            if (fInclude.fail())
            {
                //sErrorToken = sIncludeFileName;
                bReadingFromInclude = false;
                fInclude.close();
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sProcCommandLine, SyntaxError::invalid_position, sIncludeFileName);
            }
            continue;
        }
        else if (sProcCommandLine[0] == '@')
            continue;
        if (findCommand(sProcCommandLine).sString != "procedure")
            continue;
        if (sProcCommandLine.find("$" + sProc) == string::npos || sProcCommandLine.find('(') == string::npos)
            continue;
        else
        {
            string sVarDeclarationList = sProcCommandLine.substr(sProcCommandLine.find('('));
            if (getMatchingParenthesis(sVarDeclarationList) == string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, sProcCommandLine.find('('));
            if (sVarDeclarationList.find("::", getMatchingParenthesis(sVarDeclarationList)) != string::npos)
            {
                unsigned int nMatch = sVarDeclarationList.find("::", getMatchingParenthesis(sVarDeclarationList));
                if (sVarDeclarationList.find("explicit", nMatch) != string::npos)
                    nFlags |= FLAG_EXPLICIT;
                if (sVarDeclarationList.find("private", nMatch) != string::npos)
                {
                    if (sThisNameSpace != sCallingNameSpace)
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
                }
                if (sVarDeclarationList.find("inline", nMatch) != string::npos)
                    nFlags |= FLAG_INLINE;
            }
            sCallingNameSpace = sThisNameSpace;
            sVarDeclarationList = sVarDeclarationList.substr(1, getMatchingParenthesis(sVarDeclarationList)-1);
            sVarDeclarationList = " " + sVarDeclarationList + " ";
            //cerr << "sVarDeclarationList: " << sVarDeclarationList << endl;
            if (findCommand(sVarList, "var").sString == "var" || findCommand(sVarDeclarationList, "var").sString == "var")
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(0, 0, 0, 0, 0, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "var");
            }
            if (findCommand(sVarList, "str").sString == "str" || findCommand(sVarDeclarationList, "str").sString == "str")
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(0, 0, 0, 0, 0, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "str");
            }
            if (findCommand(sVarList, "tab").sString == "tab" || findCommand(sVarDeclarationList, "tab").sString == "tab")
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(0, 0, 0, 0, 0, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sProcCommandLine, SyntaxError::invalid_position, "tab");
            }
            StripSpaces(sVarDeclarationList);
            StripSpaces(sVarList);

            mVarMap = _varfactory.createProcedureArguments(sVarDeclarationList, sVarList);

            _parser.mVarMapPntr = &mVarMap;
            break;
        }
    }
    if (ProcElement->isLastLine(currentLine.first))
    {
        sCallingNameSpace = "main";
        mVarMap.clear();
        if (_option.getUseDebugger())
            _option._debug.popStackItem();
        throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, "", SyntaxError::invalid_position, sProc);
    }
    sProcCommandLine = "";
    while (!ProcElement->isLastLine(currentLine.first))
    {
        bProcSupressAnswer = false;
        //cerr << nFlags << endl;

        //bSupressAnswer = false;
        if (!sCmdCache.length())
        {
            if (!bReadingFromInclude)
            {
                currentLine = ProcElement->getNextLine(currentLine.first);
                nCurrentLine = currentLine.first;
                sProcCommandLine = currentLine.second;
                if (_option.getUseDebugger() && NumeReKernel::_messenger.isBreakpoint(sCurrentProcedureName, nCurrentLine) && sProcCommandLine.substr(0,2) != "|>")
                    sProcCommandLine.insert(0, "|> ");
            }
            else
            {
                bool bSkipNextLine = false;
                bool bAppendNextLine = false;
                while (!fInclude.eof())
                {
                    getline(fInclude, sProcCommandLine);
                    //cerr << sProcCommandLine << endl;
                    //nIncludeLine++;
                    StripSpaces(sProcCommandLine);
                    if (!sProcCommandLine.length())
                        continue;
                    if (sProcCommandLine.substr(0,2) == "##")
                        continue;
                    if (sProcCommandLine.substr(0,9) == "<install>"
                        || (findCommand(sProcCommandLine).sString == "global" && sProcCommandLine.find("<install>") != string::npos))
                    {
                        while (!fInclude.eof())
                        {
                            getline(fInclude, sProcCommandLine);
                            //nIncludeLine++;
                            StripSpaces(sProcCommandLine);
                            if (sProcCommandLine.substr(0,12) == "<endinstall>"
                                || (findCommand(sProcCommandLine).sString == "global" && sProcCommandLine.find("<endinstall>") != string::npos))
                                break;
                        }
                        sProcCommandLine = "";
                        continue;
                    }

                    if (sProcCommandLine.find("##") != string::npos)
                        sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find("##"));

                    if (sProcCommandLine.substr(0,2) == "#*" && sProcCommandLine.find("*#",2) == string::npos)
                    {
                        bBlockComment = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    if (bBlockComment && sProcCommandLine.find("*#") != string::npos)
                    {
                        bBlockComment = false;
                        if (sProcCommandLine.find("*#") == sProcCommandLine.length()-2)
                        {
                            sProcCommandLine = "";
                            continue;
                        }
                        else
                            sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("*#")+2);
                    }
                    else if (bBlockComment && sProcCommandLine.find("*#") == string::npos)
                    {
                        sProcCommandLine = "";
                        continue;
                    }
                    if (bSkipNextLine && sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
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
                    if (findCommand(sProcCommandLine).sString != "define"
                        && findCommand(sProcCommandLine).sString != "ifndef"
                        && findCommand(sProcCommandLine).sString != "ifndefined"
                        && findCommand(sProcCommandLine).sString != "redefine"
                        && findCommand(sProcCommandLine).sString != "redef"
                        && findCommand(sProcCommandLine).sString != "global"
                        && !bAppendNextLine)
                    {
                        if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                            bSkipNextLine = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    if (nIncludeType == 1
                        && findCommand(sProcCommandLine).sString != "define"
                        && findCommand(sProcCommandLine).sString != "ifndef"
                        && findCommand(sProcCommandLine).sString != "ifndefined"
                        && findCommand(sProcCommandLine).sString != "redefine"
                        && findCommand(sProcCommandLine).sString != "redef"
                        && !bAppendNextLine)
                    {
                        if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                            bSkipNextLine = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    else if (nIncludeType == 2
                        && findCommand(sProcCommandLine).sString != "global"
                        && !bAppendNextLine)
                    {
                        if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                            bSkipNextLine = true;
                        sProcCommandLine = "";
                        continue;
                    }
                    if (sProcCommandLine.length() > 2 && sProcCommandLine.substr(sProcCommandLine.length()-2) == "\\\\")
                        bAppendNextLine = true;
                    else
                        bAppendNextLine = false;

                    if (sProcCommandLine.back() == ';')
                    {
                        bProcSupressAnswer = true;
                        //bSupressAnswer = true;
                        sProcCommandLine.pop_back();
                    }

                    if (sProcCommandLine.find('$') != string::npos && sProcCommandLine.find('(', sProcCommandLine.find('$')) != string::npos)
                    {
                        if (nFlags & FLAG_INLINE)
                        {
                            if (_option.getUseDebugger())
                                _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                            resetProcedure(_parser, bSupressAnswer_back);
                            throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sProcCommandLine, SyntaxError::invalid_position);
                        }

                        try
                        {
                            int nReturn = procedureInterface(sProcCommandLine, _parser, _functions, _data, _out, _pData, _script, _option, nth_procedure);
                            if (nReturn == -1)
                            {
                                if (_option.getUseDebugger())
                                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                                throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sProcCommandLine, SyntaxError::invalid_position);
                            }
                            else if (nReturn == -2)
                            {
                                sProcCommandLine = "";
                                bProcSupressAnswer = false;
                                //bSupressAnswer = false;
                                continue;
                            }
                        }
                        catch (...)
                        {
                            resetProcedure(_parser, bSupressAnswer_back);
                            throw;
                        }
                    }

                    try
                    {
                        ProcCalc(sProcCommandLine, _parser, _functions, _data, _option, _out, _pData, _script);
                    }
                    catch (...)
                    {
                        if (_option.getUseDebugger())
                            _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                        resetProcedure(_parser, bSupressAnswer_back);
                        throw;
                    }

                    sProcCommandLine = "";
                    if (bProcSupressAnswer)
                        bProcSupressAnswer = false;
                }

                if (fInclude.eof())
                {
                    fInclude.close();
                    //nIncludeLine = 0;
                    bReadingFromInclude = false;
                    nIncludeType = 0;
                }
            }
            if (sProcCommandLine == "endprocedure")
                break;
            while (sProcCommandLine.back() == ';')
            {
                bProcSupressAnswer = true;
                //bSupressAnswer = true;
                sProcCommandLine.pop_back();
                StripSpaces(sProcCommandLine);
            }
            sProcCommandLine = " " + sProcCommandLine + " ";
            if (sProcCommandLine.find('(') != string::npos || sProcCommandLine.find('{') != string::npos)
            {
                if (!validateParenthesisNumber(sProcCommandLine))
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, SyntaxError::invalid_position);
                }
            }

            while (sProcCommandLine.find("<this>") != string::npos)
                sProcCommandLine.replace(sProcCommandLine.find("<this>"),6,sCurrentProcedureName.substr(0,sCurrentProcedureName.rfind('/')));

            for (unsigned int n = 0; n < sProcCommandLine.length(); n++)
            {
                if (sProcCommandLine[n] == '\t')
                    sProcCommandLine[n] = ' ';
            }

            if (findCommand(sProcCommandLine).sString != "namespace" && sProcCommandLine[1] != '@')
            {
                sProcCommandLine = _varfactory.resolveVariables(sProcCommandLine);
            }
        }

        if (sCmdCache.length() || sProcCommandLine.find(';') != string::npos)
        {
            //cerr << sCmdCache << endl;
            //cerr << sProcCommandLine << endl;
            if (sCmdCache.length())
            {
                while (sCmdCache.front() == ';' || sCmdCache.front() == ' ')
                    sCmdCache.erase(0,1);
                if (!sCmdCache.length())
                    continue;
                if (sCmdCache.find(';') != string::npos)
                {
                    for (unsigned int i = 0; i < sCmdCache.length(); i++)
                    {
                        if (sCmdCache[i] == ';' && !isInQuotes(sCmdCache, i))
                        {
                            //bSupressAnswer = true;
                            bProcSupressAnswer = true;
                            sProcCommandLine = sCmdCache.substr(0,i);
                            sCmdCache.erase(0,i+1);
                            break;
                        }
                        if (i == sCmdCache.length()-1)
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
            else if (sProcCommandLine.find(';') == sProcCommandLine.length()-1)
            {
                //bSupressAnswer = true;
                bProcSupressAnswer = true;
                sProcCommandLine.pop_back();
            }
            else
            {
                for (unsigned int i = 0; i < sProcCommandLine.length(); i++)
                {
                    if (sProcCommandLine[i] == ';' && !isInQuotes(sProcCommandLine, i))
                    {
                        if (i != sProcCommandLine.length()-1)
                            sCmdCache = sProcCommandLine.substr(i+1);
                        sProcCommandLine.erase(i);
                        //bSupressAnswer = true;
                        bProcSupressAnswer = true;
                    }
                    if (i == sProcCommandLine.length()-1)
                    {
                        break;
                    }
                }
            }
        }

        if (sProcCommandLine.substr(sProcCommandLine.find_first_not_of(' '),2) == "|>" && !getLoop())
        {
            sProcCommandLine.erase(sProcCommandLine.find_first_not_of(' '),2);
            StripSpaces(sProcCommandLine);
            if (_option.getUseDebugger())
            {
                _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                evalDebuggerBreakPoint(_parser, _option, _data.getStringVars());
            }
            if (!sProcCommandLine.length())
                continue;
            sProcCommandLine.insert(0,1,' ');
        }


        if (findCommand(sProcCommandLine).sString == "var" && sProcCommandLine.length() > 6)
        {
            _varfactory.createLocalVars(sProcCommandLine.substr(sProcCommandLine.find("var")+3));

            sProcCommandLine = "";
            sVars = _varfactory.sLocalVars;
            dVars = _varfactory.dLocalVars;
            nVarSize = _varfactory.nLocalVarMapSize;
            continue;
        }
        if (findCommand(sProcCommandLine).sString == "str" && sProcCommandLine.length() > 6)
        {
            _varfactory.createLocalStrings(sProcCommandLine.substr(sProcCommandLine.find("str")+3));

            sProcCommandLine = "";
            sStrings = _varfactory.sLocalStrings;
            nStrSize = _varfactory.nLocalStrMapSize;
            continue;
        }
        if (findCommand(sProcCommandLine).sString == "tab" && sProcCommandLine.length() > 6)
        {
            _varfactory.createLocalTables(sProcCommandLine.substr(sProcCommandLine.find("tab")+3));

            sProcCommandLine = "";

            continue;
        }

        if (findCommand(sProcCommandLine).sString == "namespace" && !getLoop())
        {
            sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("namespace")+9);
            StripSpaces(sProcCommandLine);
            if (sProcCommandLine.length())
            {
                if (sProcCommandLine.find(' ') != string::npos)
                    sProcCommandLine = sProcCommandLine.substr(0,sProcCommandLine.find(' '));
                if (sProcCommandLine.substr(0,5) == "this~" || sProcCommandLine == "this")
                    sProcCommandLine.replace(0,4,sThisNameSpace);
                if (sProcCommandLine != "main")
                {
                    sNameSpace = sProcCommandLine;
                    if (sNameSpace[sNameSpace.length()-1] != '~')
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

        if (sProcCommandLine[1] == '@' && sProcCommandLine[2] != ' ' && !bReadingFromInclude)
        {
            StripSpaces(sProcCommandLine);
            string sIncludeFileName = "";
            if (sProcCommandLine[1] == '"')
                sIncludeFileName = sProcCommandLine.substr(2,sProcCommandLine.find('"', 2)-2);
            else
                sIncludeFileName = sProcCommandLine.substr(1,sProcCommandLine.find(' ')-1);
            if (sProcCommandLine.find(':') != string::npos)
            {
                if (sProcCommandLine.find("defines", sProcCommandLine.find(':')+1) != string::npos)
                {
                    nIncludeType = 1;
                }
                else if (sProcCommandLine.find("globals", sProcCommandLine.find(':')+1) != string::npos)
                {
                    nIncludeType = 2;
                }
                else if (sProcCommandLine.find("procedures", sProcCommandLine.find(':')+1) != string::npos)
                {
                    nIncludeType = 3;
                }
            }
            if (sIncludeFileName.find(':') != string::npos)
            {
                for (int __i = sIncludeFileName.length()-1; __i >= 0; __i--)
                {
                    if (sIncludeFileName[__i] == ':'
                        && (__i > 1
                            || (__i == 1 && sIncludeFileName.length() > (unsigned int)__i+1 && sIncludeFileName[__i+1] != '/')))
                    {
                        sIncludeFileName.erase(sIncludeFileName.find(':'));
                        break;
                    }
                }
            }
            if (sIncludeFileName.length())
                sIncludeFileName = _script.ValidFileName(sIncludeFileName, ".nscr");
            else
                continue;
            //cerr << sIncludeFileName << endl;
            bReadingFromInclude = true;
            fInclude.clear();
            fInclude.open(sIncludeFileName.c_str());
            if (fInclude.fail())
            {
                //sErrorToken = sIncludeFileName;
                bReadingFromInclude = false;
                fInclude.close();
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                resetProcedure(_parser, bSupressAnswer_back);
                throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sProcCommandLine, SyntaxError::invalid_position, sIncludeFileName);
            }
            sProcCommandLine = "";
            continue;
        }
        else if (sProcCommandLine[1] == '@')
        {
            sProcCommandLine = "";
            continue;
        }
        if (nFlags & FLAG_INLINE)
        {
            if (findCommand(sProcCommandLine).sString == "for"
                || findCommand(sProcCommandLine).sString == "if"
                || findCommand(sProcCommandLine).sString == "while")
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                resetProcedure(_parser, bSupressAnswer_back);
                throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sProcCommandLine, SyntaxError::invalid_position);
            }
        }

        if (sProcCommandLine.find('$') != string::npos && sProcCommandLine.find('(', sProcCommandLine.find('$')) != string::npos && !getLoop())
        {
            if (nFlags & FLAG_INLINE)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                resetProcedure(_parser, bSupressAnswer_back);

                throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sProcCommandLine, SyntaxError::invalid_position);
            }

            Procedure _procedure(*this);

            unsigned int nPos = 0;
            int nProc = 0;
            while (sProcCommandLine.find('$', nPos) != string::npos && sProcCommandLine.find('(', sProcCommandLine.find('$', nPos)) != string::npos)
            {
                unsigned int nParPos = 0;
                nPos = sProcCommandLine.find('$', nPos)+1;
                string __sName = sProcCommandLine.substr(nPos, sProcCommandLine.find('(', nPos)-nPos);

                if (__sName.find('~') == string::npos)
                    //__sName = __sName.substr(0,__sName.find('$')+1) + sNameSpace + __sName.substr(__sName.find('$')+1);
                    __sName = sNameSpace + __sName;
                if (__sName.substr(0,5) == "this~")
                    __sName.replace(0,4,sThisNameSpace);
                string __sVarList = "";
                if (sProcCommandLine[nPos] == '\'')
                {
                    __sName = sProcCommandLine.substr(nPos+1, sProcCommandLine.find('\'', nPos+1)-nPos-1);
                    nParPos = sProcCommandLine.find('(', nPos+1+__sName.length());
                }
                else
                    nParPos = sProcCommandLine.find('(', nPos);
                __sVarList = sProcCommandLine.substr(nParPos);
                if (getMatchingParenthesis(sProcCommandLine.substr(nParPos)) == string::npos)
                {
                    if (_option.getUseDebugger())
                        _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                    resetProcedure(_parser, bSupressAnswer_back);
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, nParPos);
                }
                nParPos += getMatchingParenthesis(sProcCommandLine.substr(nParPos));
                __sVarList = __sVarList.substr(1,getMatchingParenthesis(__sVarList)-1);
                unsigned int nVarPos = 0;
                while (__sVarList.find('$', nVarPos) != string::npos)
                {
                    nVarPos = __sVarList.find('$', nVarPos) + 1;
                    if (__sVarList.substr(nVarPos, __sVarList.find('(', nVarPos)-nVarPos).find('~') == string::npos)
                    {
                        __sVarList = __sVarList.substr(0,nVarPos) + sNameSpace + __sVarList.substr(nVarPos);
                    }
                }

                if (!isInQuotes(sProcCommandLine, nPos, true))
                {
                    try
                    {
                        Returnvalue _return = _procedure.execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script, nth_procedure+1);
                        if (!_procedure.nReturnType)
                            sProcCommandLine = sProcCommandLine.substr(0, nPos-1) + sProcCommandLine.substr(nParPos+1);
                        else
                        {
                            replaceReturnVal(sProcCommandLine, _parser, _return, nPos-1, nParPos+1, "PROC~["+__sName+"~"+toString(nProc)+"_"+toString((int)nth_procedure)+"_"+toString((int)nCurrentLine)+"]");
                            nProc++;
                        }
                    }
                    catch (...)
                    {
                        if (_option.getUseDebugger())
                            _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                        resetProcedure(_parser, bSupressAnswer_back);
                        throw;
                    }
                    nReturnType = 1;
                }
                nPos += __sName.length()+__sVarList.length()+1;
                bReturnSignal = false;
            }
            _parser.mVarMapPntr = &mVarMap;
            StripSpaces(sProcCommandLine);
            if (!sProcCommandLine.length())
                continue;
        }
        else if (sProcCommandLine.find('$') != string::npos && !getLoop() && !isInQuotes(sProcCommandLine,sProcCommandLine.find('$')))
        {
            sProcCommandLine = "";
            continue;
        }
        if (!(nFlags & FLAG_EXPLICIT) && isPluginCmd(sProcCommandLine))
        {
            if (nFlags & FLAG_INLINE)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                resetProcedure(_parser, bSupressAnswer_back);
                throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sProcCommandLine, SyntaxError::invalid_position);
            }

            Procedure _procedure(*this);
            if (_procedure.evalPluginCmd(sProcCommandLine))
            {
                Returnvalue _return;
                if (!_option.getSystemPrintStatus())
                    _return = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nth_procedure+1);
                else
                {
                    _option.setSystemPrintStatus(false);
                    _return = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nth_procedure+1);
                    _option.setSystemPrintStatus(true);
                }
                if (sProcCommandLine.length())
                {
                    if (sProcCommandLine.find("<<RETURNVAL>>") != string::npos)
                    {
                        if (_return.vStringVal.size())
                        {
                            string sReturn = "{";
                            for (unsigned int v = 0; v < _return.vStringVal.size(); v++)
                                sReturn += _return.vStringVal[v] + ",";
                            sReturn.back() = '}';
                            sProcCommandLine.replace(sProcCommandLine.find("<<RETURNVAL>>"), 13, sReturn);
                            //sProcCommandLine.replace(sProcCommandLine.find("<<RETURNVAL>>"), 13, _return.sStringVal);
                        }
                        else
                        {
                            _parser.SetVectorVar("_~PLUGIN["+_procedure.getPluginProcName()+"~"+toString((int)nth_procedure)+"_"+toString((int)nCurrentLine)+"]", _return.vNumVal);
                            sProcCommandLine.replace(sProcCommandLine.find("<<RETURNVAL>>"),13,"_~PLUGIN["+_procedure.getPluginProcName()+"~"+toString((int)nth_procedure)+_procedure.getPluginProcName()+"_"+toString((int)nCurrentLine)+"]");
                            //sProcCommandLine.replace(sProcCommandLine.find("<<RETURNVAL>>"),13,toCmdString(_return.dNumVal));
                        }
                    }
                }
                else
                    continue;
            }
            else
            {
                continue;
            }
        }
        if (findCommand(sProcCommandLine).sString == "explicit")
        {
            sProcCommandLine.erase(findCommand(sProcCommandLine).nPos, 8);
            StripSpaces(sProcCommandLine);
        }
        if (findCommand(sProcCommandLine).sString == "install" || findCommand(sProcCommandLine).sString == "script")
        {
            if (_option.getUseDebugger())
                _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
            resetProcedure(_parser, bSupressAnswer_back);
            throw SyntaxError(SyntaxError::INSTALL_CMD_FOUND, sProcCommandLine, SyntaxError::invalid_position);
        }
        if (findCommand(sProcCommandLine).sString == "throw" && !getLoop())
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
            if (_option.getUseDebugger())
                _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
            resetProcedure(_parser, bSupressAnswer_back);

            throw SyntaxError(SyntaxError::PROCEDURE_THROW, sProcCommandLine, SyntaxError::invalid_position, sErrorToken);
        }
        if (findCommand(sProcCommandLine).sString == "return" && !getLoop())
        {
            try
            {
                if (sProcCommandLine.find("void", sProcCommandLine.find("return")+1) != string::npos)
                {
                    string sReturnValue = sProcCommandLine.substr(sProcCommandLine.find("return")+6);
                    StripSpaces(sReturnValue);
                    if (sReturnValue == "void")
                    {
                        nReturnType = 0;
                    }
                    else
                    {
                        sReturnValue += " ";
                        _ReturnVal = ProcCalc(sReturnValue, _parser, _functions, _data, _option, _out, _pData, _script);
                    }
                }
                else if (sProcCommandLine.length() > 6)
                    _ReturnVal = ProcCalc(sProcCommandLine.substr(sProcCommandLine.find("return") + 6), _parser, _functions, _data, _option, _out, _pData, _script);
                break;
            }
            catch (...)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                resetProcedure(_parser, bSupressAnswer_back);
                throw;
            }
        }
        else
        {
            try
            {
                ProcCalc(sProcCommandLine, _parser, _functions, _data, _option, _out, _pData, _script);
                if (getReturnSignal())
                {
                    _ReturnVal = getReturnValue();
                    break;
                }
            }
            catch (...)
            {
                if (_option.getUseDebugger())
                    _option._debug.gatherInformations(_varfactory.sLocalVars, _varfactory.nLocalVarMapSize, _varfactory.dLocalVars, _varfactory.sLocalStrings, _varfactory.nLocalStrMapSize, _data.getStringVars(), sProcCommandLine, sCurrentProcedureName, nCurrentLine);
                resetProcedure(_parser, bSupressAnswer_back);
                throw;
            }
        }

        sProcCommandLine = "";
    }
    fProc_in.close();

    _option._debug.popStackItem();
    resetProcedure(_parser, bSupressAnswer_back);

    if (nReturnType && !_ReturnVal.vNumVal.size() && !_ReturnVal.vStringVal.size())
        _ReturnVal.vNumVal.push_back(1.0);
    return _ReturnVal;
}

int Procedure::procedureInterface(string& sLine, Parser& _parser, Define& _functions, Datafile& _data, Output& _out, PlotData& _pData, Script& _script, Settings& _option, unsigned int nth_procedure, int nth_command)
{
    //cerr << sLine << endl;
    Procedure _procedure(*this);
    //cerr << "interface: " << _procedure.nVarSize << endl;
    int nReturn = 1;
    if (sLine.find('$') != string::npos && sLine.find('(', sLine.find('$')) != string::npos)
    {
        sLine += " ";
        unsigned int nPos = 0;
        int nProc = 0;
        while (sLine.find('$', nPos) != string::npos && sLine.find('(', sLine.find('$', nPos)) != string::npos)
        {
            unsigned int nParPos = 0;
            nPos = sLine.find('$', nPos)+1;
            string __sName = sLine.substr(nPos, sLine.find('(', nPos)-nPos);

            if (__sName.find('~') == string::npos)
                //__sName = __sName.substr(0,__sName.find('$')+1) + sNameSpace + __sName.substr(__sName.find('$')+1);
                __sName = sNameSpace + __sName;
            if (__sName.substr(0,5) == "this~")
                __sName.replace(0,4,sThisNameSpace);
            //cerr << __sName << endl;
            string __sVarList = "";
            if (sLine[nPos] == '\'')
            {
                __sName = sLine.substr(nPos+1, sLine.find('\'', nPos+1)-nPos-1);
                nParPos = sLine.find('(', nPos+1+__sName.length());
            }
            else
                nParPos = sLine.find('(', nPos);
            __sVarList = sLine.substr(nParPos);
            if (getMatchingParenthesis(sLine.substr(nParPos)) == string::npos)
            {
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, nParPos);
            }
            nParPos += getMatchingParenthesis(sLine.substr(nParPos));
            __sVarList = __sVarList.substr(1,getMatchingParenthesis(__sVarList)-1);

            unsigned int nVarPos = 0;
            while (__sVarList.find('$', nVarPos) != string::npos)
            {
                nVarPos = __sVarList.find('$', nVarPos) + 1;
                if (__sVarList.substr(nVarPos, __sVarList.find('(', nVarPos)-nVarPos).find('~') == string::npos)
                {
                    __sVarList = __sVarList.substr(0,nVarPos) + sNameSpace + __sVarList.substr(nVarPos);
                }
            }

            //cerr << __sName << " " << __sVarList << endl;
            if (!isInQuotes(sLine, nPos, true))
            {
                //cerr << "entering procedure" << endl;
                Returnvalue tempreturnval = _procedure.execute(__sName, __sVarList, _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion+1);
                //cerr << "returned" << endl;
                if (!_procedure.nReturnType)
                    sLine = sLine.substr(0,nPos-1) + sLine.substr(nParPos+1);
                else
                {
                    replaceReturnVal(sLine, _parser, tempreturnval, nPos-1, nParPos+1, "PROC~["+__sName+"~"+toString(nProc)+"_"+toString((int)nth_procedure)+"_"+toString((int)(nth_command+nth_procedure))+"]");
                }

                nReturnType = 1;
            }
            nPos += __sName.length() + __sVarList.length() + 1;
            //cerr << nPos << endl;
        }
        nReturn = 2;
        StripSpaces(sLine);
        _parser.mVarMapPntr = &mVarMap;
        if (!sLine.length())
            return -2;
    }
    else if (sLine.find('$') != string::npos)
    {
        sLine = "";
        return -1;
    }

    //cerr << _procedure.getPluginCount()  << endl;
    if (!(nFlags & FLAG_EXPLICIT) && _procedure.isPluginCmd(sLine))
    {
        if (_procedure.evalPluginCmd(sLine))
        {
            Returnvalue _return;
            if (!_option.getSystemPrintStatus())
                _return = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion+1);
            else
            {
                _option.setSystemPrintStatus(false);
                _return = _procedure.execute(_procedure.getPluginProcName(), _procedure.getPluginVarList(), _parser, _functions, _data, _option, _out, _pData, _script, nthRecursion+1);
                _option.setSystemPrintStatus(true);
            }
            _parser.mVarMapPntr = &mVarMap;
            if (sLine.length())
            {
                if (sLine.find("<<RETURNVAL>>") != string::npos)
                {
                    if (_return.vStringVal.size())
                    {
                        string sReturn = "{";
                        for (unsigned int v = 0; v < _return.vStringVal.size(); v++)
                            sReturn += _return.vStringVal[v]+",";
                        sReturn.back() = '}';
                        sLine.replace(sLine.find("<<RETURNVAL>>"), 13, sReturn);
                        //sLine.replace(sLine.find("<<RETURNVAL>>"), 13, _return.sStringVal);
                    }
                    else
                    {
                        _parser.SetVectorVar("_~PLUGIN["+_procedure.getPluginProcName()+"~"+toString((int)nth_procedure)+"]", _return.vNumVal);
                        sLine.replace(sLine.find("<<RETURNVAL>>"),13,"_~PLUGIN["+_procedure.getPluginProcName()+"~"+toString((int)(nth_command+nth_procedure))+"]");
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

int Procedure::procedureCmdInterface(string& sLine)
{
    string sCommand = findCommand(sLine).sString;
    if (sCommand == "var" || sCommand == "str" || sCommand == "tab")
    {
        return 1;
    }
    else if (sCommand == "namespace")
    {
        sLine = sLine.substr(sLine.find("namespace")+9);
        StripSpaces(sLine);
        if (sLine.length())
        {
            if (sLine.find(' ') != string::npos)
                sLine = sLine.substr(0, sLine.find(' '));
            if (sLine.substr(0,5) == "this~" || sLine == "this")
                sLine.replace(0,4,sThisNameSpace);
            if (sLine != "main")
            {
                sNameSpace = sLine;
                if (sNameSpace[sNameSpace.length()-1] != '~')
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

bool Procedure::writeProcedure(string sProcedureLine)
{
    string sAppendedLine = "";
    if (sProcedureLine.substr(0,9) == "procedure"
        && sProcedureLine.find('$') != string::npos
        && sProcedureLine.find('(', sProcedureLine.find('$')) != string::npos)
    {
        bool bAppend = false;
        bool bNamespaceline = false;
        nthBlock = 0;
        string sFileName = sProcedureLine.substr(sProcedureLine.find('$')+1, sProcedureLine.find('(', sProcedureLine.find('$'))-sProcedureLine.find('$')-1);
        StripSpaces(sFileName);
        if (!setProcName(sFileName))
            return false;
        //cerr << sFileName << endl;
        if (sFileName.substr(0,9) == "thisfile~")
            bAppend = true;
        if (sLastWrittenProcedureFile.find("|namespace") != string::npos)
            bNamespaceline = true;
        if (sCurrentProcedureName.find('~') != string::npos)
        {
            FileSystem _fSys;
            _fSys.setPath(sCurrentProcedureName.substr(0,sCurrentProcedureName.rfind('~')), true, sTokens[5][1]);
        }
        if (sCurrentProcedureName.find('/') != string::npos)
        {
            FileSystem _fSys;
            _fSys.setPath(sCurrentProcedureName.substr(0,sCurrentProcedureName.rfind('/')), true, sTokens[5][1]);
        }
        if (bAppend)
            fProcedure.open(sCurrentProcedureName.c_str(), ios_base::out | ios_base::app);
        else
            fProcedure.open(sCurrentProcedureName.c_str(), ios_base::out | ios_base::trunc);
        if (fProcedure.fail())
        {
            fProcedure.close();
            return false;
        }
        else
        {
            string sProcName = "";
            if (sFileName.find('~') != string::npos)
                sProcName = sFileName.substr(sFileName.rfind('~')+1);
            else
                sProcName = sFileName;
            sLastWrittenProcedureFile = sCurrentProcedureName;
            bWritingTofile = true;
            if (bAppend && !bNamespaceline)
            {
                unsigned int nLength = _lang.get("PROC_NAMESPACE_THISFILE_MESSAGE").length();
                fProcedure << endl << endl
                           << "#**" << std::setfill('*') << std::setw(nLength+2) << "**" << endl
                           << " * NAMESPACE: THISFILE" << std::setfill(' ') << std::setw(nLength-17) << " *" << endl
                           << " * " << _lang.get("PROC_NAMESPACE_THISFILE_MESSAGE") << " *" << endl
                           << " **" << std::setfill('*') << std::setw(nLength+3) << "**#" << endl << endl << endl;
                sLastWrittenProcedureFile += "|namespace";
            }
            else if (bAppend)
                fProcedure << endl << endl;
            unsigned int nLength = _lang.get("COMMON_PROCEDURE").length();
            fProcedure << "#*********" << std::setfill('*') << std::setw(nLength+2) << "***" << std::setfill('*') << std::setw(max(21u,sProcName.length()+2)) << "*" << endl;
            fProcedure << " * NUMERE-" << toUpperCase(_lang.get("COMMON_PROCEDURE")) << ": $" << sProcName << "()" << endl;
            fProcedure << " * =======" << std::setfill('=') << std::setw(nLength+2) << "===" << std::setfill('=') << std::setw(max(21u,sProcName.length()+2)) << "=" << endl;
            fProcedure << " * " << _lang.get("PROC_ADDED_DATE") << ": " << getTimeStamp(false) << " *#" << endl;
            fProcedure << endl;
            fProcedure << "procedure $";
            if (sFileName.find('~') != string::npos)
                fProcedure << sFileName.substr(sFileName.rfind('~')+1);
            else
                fProcedure << sFileName;
            fProcedure << sProcedureLine.substr(sProcedureLine.find('(')) << endl;
            return true;
        }
    }
    else if (sProcedureLine.substr(0,12) == "endprocedure")
        bWritingTofile = false;
    else
    {
        if (sProcedureLine.find('(') != string::npos
            && (sProcedureLine.substr(0,3) == "for"
                || sProcedureLine.substr(0,3) == "if "
                || sProcedureLine.substr(0,3) == "if("
                || sProcedureLine.substr(0,6) == "elseif"
                || sProcedureLine.substr(0,5) == "while"))
        {
            sAppendedLine = sProcedureLine.substr(getMatchingParenthesis(sProcedureLine)+1);
            sProcedureLine.erase(getMatchingParenthesis(sProcedureLine)+1);
        }
        else if (sProcedureLine.find(' ',4) != string::npos
            && (sProcedureLine.substr(0,5) == "else "
                || sProcedureLine.substr(0,6) == "endif "
                || sProcedureLine.substr(0,7) == "endfor "
                || sProcedureLine.substr(0,9) == "endwhile ")
            && sProcedureLine.find_first_not_of(' ', sProcedureLine.find(' ', 4)) != string::npos
            && sProcedureLine[sProcedureLine.find_first_not_of(' ', sProcedureLine.find(' ',4))] != '-')
        {
            sAppendedLine = sProcedureLine.substr(sProcedureLine.find(' ',4));
            sProcedureLine.erase(sProcedureLine.find(' ',4));
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
            || sProcedureLine.find(" while ") != string::npos
            || sProcedureLine.find(" while(") != string::npos
            || sProcedureLine.find(" endwhile") != string::npos)
        {
            for (unsigned int n = 0; n < sProcedureLine.length(); n++)
            {
                if (sProcedureLine[n] == ' ' && !isInQuotes(sProcedureLine, n))
                {
                    if (sProcedureLine.substr(n,5) == " for "
                        || sProcedureLine.substr(n,5) == " for("
                        || sProcedureLine.substr(n,7) == " endfor"
                        || sProcedureLine.substr(n,4) == " if "
                        || sProcedureLine.substr(n,4) == " if("
                        || sProcedureLine.substr(n,5) == " else"
                        || sProcedureLine.substr(n,8) == " elseif "
                        || sProcedureLine.substr(n,8) == " elseif("
                        || sProcedureLine.substr(n,6) == " endif"
                        || sProcedureLine.substr(n,7) == " while "
                        || sProcedureLine.substr(n,7) == " while("
                        || sProcedureLine.substr(n,9) == " endwhile")
                    {
                        sAppendedLine = sProcedureLine.substr(n+1);
                        sProcedureLine.erase(n);
                        break;
                    }
                }
            }
        }
        if (findCommand(sProcedureLine).sString == "endif"
            || findCommand(sProcedureLine).sString == "endwhile"
            || findCommand(sProcedureLine).sString == "endfor"
            || findCommand(sProcedureLine).sString == "endcompose"
            || findCommand(sProcedureLine).sString == "elseif"
            || findCommand(sProcedureLine).sString == "else")
            nthBlock--;
        string sTabs = "\t";
        for (int i = 0; i < nthBlock; i++)
            sTabs += '\t';
        sProcedureLine = sTabs + sProcedureLine;
        if (findCommand(sProcedureLine).sString == "if"
            || findCommand(sProcedureLine).sString == "while"
            || findCommand(sProcedureLine).sString == "for"
            || findCommand(sProcedureLine).sString == "compose"
            || findCommand(sProcedureLine).sString == "elseif"
            || findCommand(sProcedureLine).sString == "else")
            nthBlock++;
    }
    if (fProcedure.is_open())
        fProcedure << sProcedureLine << endl;
    if (!bWritingTofile && fProcedure.is_open())
    {
        fProcedure << endl;
        fProcedure << "#* " << _lang.get("PROC_END_OF_PROCEDURE") << endl;
        fProcedure << " * " << _lang.get("PROC_FOOTER") << endl;
        fProcedure << " * https://sites.google.com/site/numereframework/" << endl;
        fProcedure << " **" << std::setfill('*') << std::setw(_lang.get("PROC_FOOTER").length()+1) << "#" << endl;

        fProcedure.close();
        if (nthBlock)
        {
            //sErrorToken = sCurrentProcedureName;
            throw SyntaxError(SyntaxError::IF_OR_LOOP_SEEMS_NOT_TO_BE_CLOSED, sProcedureLine, SyntaxError::invalid_position, sCurrentProcedureName);
        }
        sCurrentProcedureName = "";
    }
    StripSpaces(sAppendedLine);
    if (sAppendedLine.length())
        return writeProcedure(sAppendedLine);
    return true;
}

bool Procedure::isInline(const string& sProc)
{
    ifstream fProc_in;
    bool bBlockComment = false;
    string sProcCommandLine;
    int nProcedureFlags = 0;
    if (sProc.find('$') == string::npos)
        return false;
    if (sProc.find('$') != string::npos && sProc.find('(', sProc.find('$')) != string::npos)
    {
        //sProc += " ";
        unsigned int nPos = 0;
        while (sProc.find('$', nPos) != string::npos && sProc.find('(', sProc.find('$', nPos)) != string::npos)
        {
            nPos = sProc.find('$', nPos)+1;
            string __sName = sProc.substr(nPos, sProc.find('(', nPos)-nPos);

            if (__sName.find('~') == string::npos)
                //__sName = __sName.substr(0,__sName.find('$')+1) + sNameSpace + __sName.substr(__sName.find('$')+1);
                __sName = sNameSpace + __sName;
            if (__sName.substr(0,5) == "this~")
                __sName.replace(0,4,sThisNameSpace);
            //cerr << __sName << endl;

            if (sProc[nPos] == '\'')
            {
                __sName = sProc.substr(nPos+1, sProc.find('\'', nPos+1)-nPos-1);
            }
            //cerr << __sName << endl; // !isInQuotes(sProc, nPos, true)
            if (!isInQuotes(sProc, nPos, true))
            {
                string __sFileName = __sName;
                if (__sFileName.find('~') != string::npos)
                {
                    if (__sFileName.substr(0,9) == "thisfile~")
                    {
                        if (sProcNames.length())
                            __sFileName = sProcNames.substr(sProcNames.rfind(';')+1);
                        else
                        {
                            //sErrorToken = "thisfile";
                            throw SyntaxError(SyntaxError::PRIVATE_PROCEDURE_CALLED, sProc, SyntaxError::invalid_position, "thisfile");
                        }
                        __sFileName = ValidFileName(__sFileName, ".nprc");
                    }
                    else
                    {
                        for (unsigned int i = 0; i < __sFileName.length(); i++)
                        {
                            if (__sFileName[i] == '~')
                            {
                                if (__sFileName.length() > 5 && i >= 4 && __sFileName.substr(i-4,5) == "main~")
                                    __sFileName = __sFileName.substr(0,i-4) + __sFileName.substr(i+1);
                                else
                                    __sFileName[i] = '/';
                            }
                        }
                    }
                }
                __sFileName = ValidFileName(__sFileName, ".nprc");
                if (__sFileName[1] != ':')
                {
                    __sFileName = "<procpath>/" + __sFileName;
                    __sFileName = ValidFileName(__sFileName, ".nprc");
                }
                //cerr << __sFileName << endl;
                fProc_in.clear();
                fProc_in.open(__sFileName.c_str());
                if (fProc_in.fail())
                {
                    fProc_in.close();
                    throw SyntaxError(SyntaxError::FILE_NOT_EXIST, "$" + sProc + "(...)", SyntaxError::invalid_position, __sFileName);
                }
                if (__sName.find('/') != string::npos)
                {
                    __sName.erase(0,__sName.rfind('/')+1);
                }
                if (__sName.find('~') != string::npos)
                    __sName.erase(0,__sName.rfind('~')+1);
                if (__sName.find('.') != string::npos)
                    __sName.erase(__sName.find('.'));

                while (!fProc_in.eof())
                {
                    getline(fProc_in, sProcCommandLine);
                    StripSpaces(sProcCommandLine);
                    if (!sProcCommandLine.length())
                        continue;
                    if (sProcCommandLine.substr(0,2) == "##")
                        continue;
                    if (sProcCommandLine.find("##") != string::npos)
                        sProcCommandLine = sProcCommandLine.substr(0, sProcCommandLine.find("##"));
                    if (sProcCommandLine.substr(0,2) == "#*" && sProcCommandLine.find("*#",2) == string::npos)
                    {
                        bBlockComment = true;
                        continue;
                    }
                    if (bBlockComment && sProcCommandLine.find("*#") != string::npos)
                    {
                        bBlockComment = false;
                        if (sProcCommandLine.find("*#") == sProcCommandLine.length()-2)
                        {
                            continue;
                        }
                        else
                            sProcCommandLine = sProcCommandLine.substr(sProcCommandLine.find("*#")+2);
                    }
                    else if (bBlockComment && sProcCommandLine.find("*#") == string::npos)
                    {
                        continue;
                    }
                    if (sProcCommandLine[0] != '@' && findCommand(sProcCommandLine).sString != "procedure")
                        continue;
                    else if (sProcCommandLine[0] == '@')
                        continue;
                    if (findCommand(sProcCommandLine).sString != "procedure")
                        continue;
                    if (sProcCommandLine.find("$" + __sName) == string::npos || sProcCommandLine.find('(') == string::npos)
                        continue;
                    else
                    {
                        string sVarDeclarationList = sProcCommandLine.substr(sProcCommandLine.find('('));
                        if (getMatchingParenthesis(sVarDeclarationList) == string::npos)
                            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sProcCommandLine, sProcCommandLine.find('('));
                        if (sVarDeclarationList.find("::", getMatchingParenthesis(sVarDeclarationList)) != string::npos)
                        {
                            unsigned int nMatch = sVarDeclarationList.find("::", getMatchingParenthesis(sVarDeclarationList));
                            if (sVarDeclarationList.find("explicit", nMatch) != string::npos)
                                nProcedureFlags += 1;
                            if (sVarDeclarationList.find("inline", nMatch) != string::npos)
                                nProcedureFlags += 2;
                        }
                        if (!(nProcedureFlags & 2))
                        {
                            fProc_in.close();
                            return false;
                        }
                        fProc_in.close();
                    }
                }
                if (fProc_in.is_open() && fProc_in.eof())
                {
                    throw SyntaxError(SyntaxError::PROCEDURE_NOT_FOUND, sProcCommandLine, SyntaxError::invalid_position, sProc);
                }
            }
            //cerr << nPos << endl;
        }
    }
    else
        return false;
    return (nProcedureFlags & 2);
 }

void Procedure::evalDebuggerBreakPoint(Parser& _parser, Settings& _option, const map<string,string>& sStringMap)
{
    // if the stack is empty, it has to be a breakpoint from a script
    if (!_option._debug.getStackSize())
    {
        NumeReKernel::evalDebuggerBreakPoint(_option, sStringMap, "", &_parser);
        return;
    }
    _option._debug.gatherInformations(sVars, nVarSize, dVars, sStrings, nStrSize, sStringMap, "", sCurrentProcedureName, nCurrentLine);
    NumeReKernel::showDebugEvent(_lang.get("DBG_HEADLINE"), _option._debug.getModuleInformations(), _option._debug.getStackTrace(), _option._debug.getNumVars(), _option._debug.getStringVars());
    NumeReKernel::gotoLine(_option._debug.getErrorModule(), _option._debug.getLineNumber()+1);
    _option._debug.resetBP();
    NumeReKernel::waitForContinue();
    return;
}

void Procedure::replaceReturnVal(string& sLine, Parser& _parser, const Returnvalue& _return, unsigned int nPos, unsigned int nPos2, const string& sReplaceName)
{
    if (_return.vStringVal.size())
    {
        string sReturn = "{";
        for (unsigned int v = 0; v < _return.vStringVal.size(); v++)
            sReturn += _return.vStringVal[v]+",";
        sReturn.back() = '}';
        sLine = sLine.substr(0, nPos) + sReturn + sLine.substr(nPos2);
    }
    else if (_return.vNumVal.size())
    {
        string __sRplcNm = sReplaceName;
        if (sReplaceName.find('\\') != string::npos || sReplaceName.find('/') != string::npos || sReplaceName.find(':') != string::npos)
        {
            for (unsigned int i = 0; i < __sRplcNm.length(); i++)
            {
                if (__sRplcNm[i] == '\\' || __sRplcNm[i] == '/' || __sRplcNm[i] == ':')
                    __sRplcNm[i] = '~';
            }
        }
        //std::cerr << __sRplcNm << endl;
        //std::cerr << _return.vNumVal.size() << endl;
        _parser.SetVectorVar(__sRplcNm, _return.vNumVal);
        sLine = sLine.substr(0,nPos) + __sRplcNm +  sLine.substr(nPos2);
    }
    else
        sLine = sLine.substr(0,nPos) + "nan" + sLine.substr(nPos2);
    return;
}

void Procedure::resetProcedure(Parser& _parser, bool bSupressAnswer)
{
    sCallingNameSpace = "main";
    sThisNameSpace = "";
    mVarMap.clear();
    NumeReKernel::bSupressAnswer = bSupressAnswer;
    _parser.mVarMapPntr = 0;
    if (sProcNames.length())
    {
        sProcNames.erase(sProcNames.rfind(';'));
    }

    sVars = 0;
    sStrings = 0;
    dVars = 0;

    nVarSize = 0;
    nStrSize = 0;
    return;
}

