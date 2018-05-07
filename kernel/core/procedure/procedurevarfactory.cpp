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

#define FLAG_EXPLICIT 1
#define FLAG_INLINE 2

ProcedureVarFactory::ProcedureVarFactory()
{
    init();
}

ProcedureVarFactory::ProcedureVarFactory(Procedure* _procedure, Parser* _parser, Datafile* _data, Settings* _option, Define* _functions, Output* _out, PlotData* _pData, Script* _script, const string& sProc, unsigned int currentProc)
{
    init();
    _currentProcedure = _procedure;
    _parserRef = _parser;
    _dataRef = _data;
    _optionRef = _option;
    _functionRef = _functions;
    _outRef = _out;
    _pDataRef = _pData;
    _scriptRef = _script;

    sProcName = replaceProcedureName(sProc);
    nth_procedure = currentProc;
}

ProcedureVarFactory::~ProcedureVarFactory()
{
    if (nArgumentMapSize)
    {
        for (size_t i = 0; i < nArgumentMapSize; i++)
            delete[] sArgumentMap[i];
        delete[] sArgumentMap;
        sArgumentMap = nullptr;
        nArgumentMapSize = 0;
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
        delete[] dLocalVars;
        sLocalVars = nullptr;
        dLocalVars = nullptr;
        nLocalVarMapSize = 0;
    }
    if (nLocalStrMapSize)
    {
        for (size_t i = 0; i < nLocalStrMapSize; i++)
        {
            if (_dataRef)
                _dataRef->removeStringVar(sLocalStrings[i][1]);
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
                _dataRef->deleteCache(sLocalTables[i][1]);
            delete[] sLocalTables[i];
        }
        delete[] sLocalTables;
        sLocalTables = nullptr;
        nLocalTableSize = 0;
    }
}

void ProcedureVarFactory::init()
{
    _currentProcedure = nullptr;
    _parserRef = nullptr;
    _dataRef = nullptr;
    _optionRef = nullptr;
    _functionRef = nullptr;
    _outRef = nullptr;
    _pDataRef = nullptr;
    _scriptRef = nullptr;

    sProcName = "";
    nth_procedure = 0;

    sArgumentMap = nullptr;
    sLocalVars = nullptr;
    sLocalStrings = nullptr;
    sLocalTables = nullptr;

    dLocalVars = nullptr;

    nArgumentMapSize = 0;
    nLocalVarMapSize = 0;
    nLocalStrMapSize = 0;
    nLocalTableSize = 0;
}

// replaces path characters etc.
string ProcedureVarFactory::replaceProcedureName(string sProcedureName)
{
    for (size_t i = 0; i < sProcedureName.length(); i++)
    {
        if (sProcedureName[i] == ':' || sProcedureName[i] == '\\' || sProcedureName[i] == '/' || sProcedureName[i] == ' ')
            sProcedureName[i] = '_';
    }
    return sProcedureName;
}

unsigned int ProcedureVarFactory::countVarListElements(const string& sVarList)
{
    int nParenthesis = 0;
    unsigned int nElements = 1;
    for (unsigned int i = 0; i < sVarList.length(); i++)
    {
        if (sVarList[i] == '(' && !isInQuotes(sVarList, i))
            nParenthesis++;
        if (sVarList[i] == ')' && !isInQuotes(sVarList, i))
            nParenthesis--;
        if (sVarList[i] == ',' && !nParenthesis && !isInQuotes(sVarList, i))
            nElements++;
    }
    return nElements;
}

map<string,string> ProcedureVarFactory::createProcedureArguments(string sArgumentList, string sArgumentValues)
{
    map<string,string> mVarMap;

    if (!sArgumentList.length() && sArgumentValues.length())
    {
        if (_optionRef->getUseDebugger())
            _optionRef->_debug.popStackItem();
        throw SyntaxError(SyntaxError::TOO_MANY_ARGS, sArgumentList, SyntaxError::invalid_position);
    }
    if (sArgumentList.length())
    {
        if (!validateParenthesisNumber(sArgumentList))
        {
            if (_optionRef->getUseDebugger())
                _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sArgumentList, SyntaxError::invalid_position);
        }
        nArgumentMapSize = countVarListElements(sArgumentList);

        sArgumentMap = new string*[nArgumentMapSize];
        for (unsigned int i = 0; i < nArgumentMapSize; i++)
        {
            sArgumentMap[i] = new string[2];
            sArgumentMap[i][0] = getNextArgument(sArgumentList);
            StripSpaces(sArgumentMap[i][0]);
            if (findCommand(sArgumentMap[i][0]).sString == "var")
            {
                for (unsigned int j = 0; j <= i; j++)
                    delete[] sArgumentMap[j];
                delete[] sArgumentMap;
                nArgumentMapSize = 0;
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                //sErrorToken = "var";
                throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "var");
            }
            if (findCommand(sArgumentMap[i][0]).sString == "str")
            {
                for (unsigned int j = 0; j <= i; j++)
                    delete[] sArgumentMap[j];
                delete[] sArgumentMap;
                nArgumentMapSize = 0;
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                //sErrorToken = "str";
                throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "str");
            }
            if (findCommand(sArgumentMap[i][0]).sString == "tab")
            {
                for (unsigned int j = 0; j <= i; j++)
                    delete[] sArgumentMap[j];
                delete[] sArgumentMap;
                nArgumentMapSize = 0;
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                //sErrorToken = "str";
                throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "tab");
            }

            if (i < nArgumentMapSize-1 && sArgumentValues.length() && sArgumentValues.find(',') != string::npos)
            {
                sArgumentMap[i][1] = getNextArgument(sArgumentValues);
                StripSpaces(sArgumentMap[i][1]);
                if (findCommand(sArgumentMap[i][1]).sString == "var")
                {
                    for (unsigned int j = 0; j <= i; j++)
                        delete[] sArgumentMap[j];
                    delete[] sArgumentMap;
                    nArgumentMapSize = 0;
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    //sErrorToken = "var";
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "var");
                }
                if (findCommand(sArgumentMap[i][1]).sString == "str")
                {
                    for (unsigned int j = 0; j <= i; j++)
                        delete[] sArgumentMap[j];
                    delete[] sArgumentMap;
                    nArgumentMapSize = 0;
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    //sErrorToken = "str";
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "str");
                }
                if (findCommand(sArgumentMap[i][1]).sString == "tab")
                {
                    for (unsigned int j = 0; j <= i; j++)
                        delete[] sArgumentMap[j];
                    delete[] sArgumentMap;
                    nArgumentMapSize = 0;
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    //sErrorToken = "str";
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "tab");
                }
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
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.popStackItem();
                    throw SyntaxError(SyntaxError::MISSING_DEFAULT_VALUE, sArgumentList, sErrorToken, sErrorToken);
                }
            }
            else if (sArgumentValues.length())
            {
                sArgumentMap[i][1] = sArgumentValues;
                sArgumentValues = "";
                StripSpaces(sArgumentMap[i][1]);
                if (findCommand(sArgumentMap[i][1]).sString == "var")
                {
                    for (unsigned int j = 0; j <= i; j++)
                        delete[] sArgumentMap[j];
                    delete[] sArgumentMap;
                    nArgumentMapSize = 0;
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    //sErrorToken = "var";
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "var");
                }
                if (findCommand(sArgumentMap[i][1]).sString == "str")
                {
                    for (unsigned int j = 0; j <= i; j++)
                        delete[] sArgumentMap[j];
                    delete[] sArgumentMap;
                    nArgumentMapSize = 0;
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    //sErrorToken = "str";
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "str");
                }
                if (findCommand(sArgumentMap[i][1]).sString == "tab")
                {
                    for (unsigned int j = 0; j <= i; j++)
                        delete[] sArgumentMap[j];
                    delete[] sArgumentMap;
                    nArgumentMapSize = 0;
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    //sErrorToken = "str";
                    throw SyntaxError(SyntaxError::WRONG_ARG_NAME, sArgumentList, SyntaxError::invalid_position, "tab");
                }
                if (sArgumentMap[i][0].find('=') != string::npos)
                {
                    sArgumentMap[i][0] = sArgumentMap[i][0].substr(0,sArgumentMap[i][0].find('='));
                    StripSpaces(sArgumentMap[i][0]);
                }
            }
            else if (!sArgumentValues.length())
            {
                if (sArgumentMap[i][0].find('=') != string::npos)
                {
                    sArgumentMap[i][1] = sArgumentMap[i][0].substr(sArgumentMap[i][0].find('=')+1);
                    sArgumentMap[i][0].erase(sArgumentMap[i][0].find('='));
                    StripSpaces(sArgumentMap[i][0]);
                    StripSpaces(sArgumentMap[i][1]);
                }
                else
                {
                    string sErrorToken = sArgumentMap[i][0];
                    for (unsigned int j = 0; j <= i; j++)
                        delete[] sArgumentMap[j];
                    delete[] sArgumentMap;
                    nArgumentMapSize = 0;
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.popStackItem();
                    throw SyntaxError(SyntaxError::MISSING_DEFAULT_VALUE, sArgumentList, sErrorToken, sErrorToken);
                }
            }
        }
        for (unsigned int i = 0; i < nArgumentMapSize; i++)
        {
            if (sArgumentMap[i][1].find('$') != string::npos && sArgumentMap[i][1].find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sArgumentList, SyntaxError::invalid_position);
                }

                int nReturn = _currentProcedure->procedureInterface(sArgumentMap[i][1], *_parserRef, *_functionRef, *_dataRef, *_outRef, *_pDataRef, *_scriptRef, *_optionRef, nth_procedure);
                if (nReturn == -1)
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(0, 0, 0, 0, 0, _dataRef->getStringVars(), sArgumentList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                    throw SyntaxError(SyntaxError::PROCEDURE_ERROR, sArgumentList, SyntaxError::invalid_position);
                }
                else if (nReturn == -2)
                    sArgumentMap[i][1] = "false";

            }
            else if (sArgumentMap[i][0].length() > 2 && sArgumentMap[i][0].substr(sArgumentMap[i][0].length()-2) == "()")
            {
                sArgumentMap[i][0].pop_back();
                if (sArgumentMap[i][1].find('(') != string::npos)
                    sArgumentMap[i][1].erase(sArgumentMap[i][1].find('('));
            }
        }
        for (unsigned int i = 0; i < nArgumentMapSize; i++)
        {
            mVarMap[sArgumentMap[i][0]] = sArgumentMap[i][1];
        }
    }
    return mVarMap;
}

void ProcedureVarFactory::createLocalVars(string sVarList)
{
    // already declared the local vars?
    if (nLocalVarMapSize)
        return;

    if (!_currentProcedure)
        return;

    nLocalVarMapSize = countVarListElements(sVarList);

    sLocalVars = new string*[nLocalVarMapSize];
    dLocalVars = new double[nLocalVarMapSize];

    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        sLocalVars[i] = new string[2];
        sLocalVars[i][0] = getNextArgument(sVarList, true);
        if (sLocalVars[i][0].find('=') != string::npos)
        {
            string sVarValue = sLocalVars[i][0].substr(sLocalVars[i][0].find('=')+1);
            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, _dataRef->getStringVars(), sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            _parserRef->RemoveVar(sLocalVars[j][1]);
                        delete[] sLocalVars[j];
                    }
                    delete[] sLocalVars;
                    nLocalVarMapSize = 0;
                    throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sVarList, SyntaxError::invalid_position);
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
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, _dataRef->getStringVars(), sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            _parserRef->RemoveVar(sLocalVars[j][1]);
                        delete[] sLocalVars[j];
                    }
                    delete[] sLocalVars;
                    nLocalVarMapSize = 0;
                    throw;
                }
            }
            try
            {
                if (containsStrings(sVarValue) || _dataRef->containsStringVars(sVarValue))
                {
                    string sTemp;
                    if (!parser_StringParser(sVarValue, sTemp, *_dataRef, *_parserRef, *_optionRef, true))
                    {
                        if (_optionRef->getUseDebugger())
                            _optionRef->_debug.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, _dataRef->getStringVars(), sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                        for (unsigned int j = 0; j <= i; j++)
                        {
                            if (j < i)
                                _parserRef->RemoveVar(sLocalVars[j][1]);
                            delete[] sLocalVars[j];
                        }
                        delete[] sLocalVars;
                        nLocalVarMapSize = 0;
                        throw SyntaxError(SyntaxError::STRING_ERROR, sVarList, SyntaxError::invalid_position);
                    }
                }
                if (sVarValue.find("data(") != string::npos || _dataRef->containsCacheElements(sVarValue))
                {
                    getDataElements(sVarValue, *_parserRef, *_dataRef, *_optionRef);
                }
                for (unsigned int j = 0; j < i; j++)
                {
                    unsigned int nPos = 0;
                    while (sVarValue.find(sLocalVars[j][0], nPos) != string::npos)
                    {
                        nPos = sVarValue.find(sLocalVars[j][0], nPos);
                        if (((nPos+sLocalVars[j][0].length()+1 > sVarValue.length() && checkDelimiter(sVarValue.substr(nPos-1, sLocalVars[j][0].length()+1) + " "))
                            || (nPos+sLocalVars[j][0].length()+1 <= sVarValue.length() && checkDelimiter(sVarValue.substr(nPos-1, sLocalVars[j][0].length()+2))))
                            && (!isInQuotes(sVarValue, nPos, true)
                                || isToCmd(sVarValue, nPos)))
                        {
                            sVarValue.replace(nPos, sLocalVars[j][0].length(), sLocalVars[j][1]);
                            nPos += sLocalVars[j][1].length();
                        }
                        else
                            nPos += sLocalVars[j][0].length();
                    }
                }
                _parserRef->SetExpr(sVarValue);
                sLocalVars[i][0] = sLocalVars[i][0].substr(0,sLocalVars[i][0].find('='));
                dLocalVars[i] = _parserRef->Eval();
            }
            catch (...)
            {
                if (_optionRef->getUseDebugger())
                    _optionRef->_debug.gatherInformations(sLocalVars, i, dLocalVars, sLocalStrings, nLocalStrMapSize, _dataRef->getStringVars(), sVarList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                for (unsigned int j = 0; j <= i; j++)
                {
                    if (j < i)
                        _parserRef->RemoveVar(sLocalVars[j][1]);
                    delete[] sLocalVars[j];
                }
                delete[] sLocalVars;
                nLocalVarMapSize = 0;
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

void ProcedureVarFactory::createLocalStrings(string sStringList)
{
    // already declared the local vars?
    if (nLocalStrMapSize)
        return;

    if (!_currentProcedure)
        return;

    nLocalStrMapSize = countVarListElements(sStringList);
    sLocalStrings = new string*[nLocalStrMapSize];

    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        sLocalStrings[i] = new string[3];
        sLocalStrings[i][0] = getNextArgument(sStringList, true);
        if (sLocalStrings[i][0].find('=') != string::npos)
        {
            string sVarValue = sLocalStrings[i][0].substr(sLocalStrings[i][0].find('=')+1);
            if (sVarValue.find('$') != string::npos && sVarValue.find('(') != string::npos)
            {
                if (_currentProcedure->getProcedureFlags() & FLAG_INLINE)
                {
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, _dataRef->getStringVars(), sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            _dataRef->removeStringVar(sLocalStrings[j][1]);
                        delete[] sLocalStrings[j];
                    }
                    delete[] sLocalStrings;
                    nLocalStrMapSize = 0;
                    throw SyntaxError(SyntaxError::INLINE_PROCEDURE_IS_NOT_INLINE, sStringList, SyntaxError::invalid_position);
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
                    if (_optionRef->getUseDebugger())
                        _optionRef->_debug.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, _dataRef->getStringVars(), sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());

                    for (unsigned int j = 0; j <= i; j++)
                    {
                        if (j < i)
                            _dataRef->removeStringVar(sLocalStrings[j][1]);
                        delete[] sLocalStrings[j];
                    }
                    delete[] sLocalStrings;
                    nLocalStrMapSize = 0;
                    throw;
                }
            }
            try
            {
                for (unsigned int j = 0; j < i; j++)
                {
                    unsigned int nPos = 0;
                    while (sVarValue.find(sLocalStrings[j][0], nPos) != string::npos)
                    {
                        nPos = sVarValue.find(sLocalStrings[j][0], nPos);
                        if (((nPos+sLocalStrings[j][0].length()+1 > sVarValue.length() && checkDelimiter(sVarValue.substr(nPos-1, sLocalStrings[j][0].length()+1) + " "))
                            || (nPos+sLocalStrings[j][0].length()+1 <= sVarValue.length() && checkDelimiter(sVarValue.substr(nPos-1, sLocalStrings[j][0].length()+2))))
                            && (!isInQuotes(sVarValue, nPos, true)
                                || isToCmd(sVarValue, nPos)))
                        {
                            sVarValue.replace(nPos, sLocalStrings[j][0].length(), sLocalStrings[j][1]);
                            nPos += sLocalStrings[j][1].length();
                        }
                        else
                            nPos += sLocalStrings[j][0].length();
                    }
                }

                if (containsStrings(sVarValue) || _dataRef->containsStringVars(sVarValue))
                {
                    string sTemp;
                    if (!parser_StringParser(sVarValue, sTemp, *_dataRef, *_parserRef, *_optionRef, true))
                    {
                        if (_optionRef->getUseDebugger())
                            _optionRef->_debug.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, _dataRef->getStringVars(), sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
                        throw SyntaxError(SyntaxError::STRING_ERROR, sStringList, SyntaxError::invalid_position);
                    }
                }

                sLocalStrings[i][0] = sLocalStrings[i][0].substr(0,sLocalStrings[i][0].find('='));
                sLocalStrings[i][2] = sVarValue;
            }
            catch (...)
            {
                for (unsigned int j = 0; j <= i; j++)
                {
                    if (j < i)
                        _dataRef->removeStringVar(sLocalStrings[j][1]);
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
            _dataRef->setStringValue(sLocalStrings[i][1], sLocalStrings[i][2]);
        }
        catch (...)
        {
            if (_optionRef->getUseDebugger())
                _optionRef->_debug.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, _dataRef->getStringVars(), sStringList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            for (unsigned int j = 0; j <= i; j++)
            {
                if (j < i)
                    _dataRef->removeStringVar(sLocalStrings[j][1]);
                delete[] sLocalStrings[j];
            }
            delete[] sLocalStrings;
            nLocalStrMapSize = 0;
            throw;
        }
    }
}

void ProcedureVarFactory::createLocalTables(string sTableList)
{
    // already declared the local vars?
    if (nLocalTableSize)
        return;

    if (!_currentProcedure)
        return;

    nLocalTableSize = countVarListElements(sTableList);
    sLocalTables = new string*[nLocalTableSize];

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
            _dataRef->addCache(sLocalTables[i][1], *_optionRef);
        }
        catch (...)
        {
            if (_optionRef->getUseDebugger())
                _optionRef->_debug.gatherInformations(sLocalVars, nLocalVarMapSize, dLocalVars, sLocalStrings, i, _dataRef->getStringVars(), sTableList, _currentProcedure->getCurrentProcedureName(), _currentProcedure->GetCurrentLine());
            for (unsigned int j = 0; j <= i; j++)
            {
                if (j < i)
                    _dataRef->deleteCache(sLocalTables[j][1]);
                delete[] sLocalTables[j];
            }
            delete[] sLocalTables;
            nLocalTableSize = 0;
            throw;
        }
    }
}

string ProcedureVarFactory::resolveArguments(string sProcedureCommandLine)
{
    if (!nArgumentMapSize)
        return sProcedureCommandLine;

    for (unsigned int i = 0; i < nArgumentMapSize; i++)
    {
        unsigned int nPos = 0;
        while (sProcedureCommandLine.find(sArgumentMap[i][0].substr(0, sArgumentMap[i][0].length() - (sArgumentMap[i][0].back() == '(')), nPos) != string::npos)
        {
            nPos = sProcedureCommandLine.find(sArgumentMap[i][0].substr(0, sArgumentMap[i][0].length() - (sArgumentMap[i][0].back() == '(')), nPos);

            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~',nPos-1)] != '#')
                || (sArgumentMap[i][0].back() != '(' && sProcedureCommandLine[nPos+sArgumentMap[i][0].length()] == '('))
            {
                nPos += sArgumentMap[i][0].length();
                continue;
            }
            if (checkDelimiter(sProcedureCommandLine.substr(nPos-1, sArgumentMap[i][0].length()+1+(sArgumentMap[i][0].back() != '(')))
                && (!isInQuotes(sProcedureCommandLine, nPos, true)
                    || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, sArgumentMap[i][0].length()-(sArgumentMap[i][0].back() == '('), sArgumentMap[i][1]);
                nPos += sArgumentMap[i][1].length();
            }
            else
                nPos += sArgumentMap[i][0].length() - (sArgumentMap[i][0].back() == '(');
        }
    }
    return sProcedureCommandLine;
}

string ProcedureVarFactory::resolveLocalVars(string sProcedureCommandLine)
{
    if (!nLocalVarMapSize)
        return sProcedureCommandLine;
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        unsigned int nPos = 0;
        while (sProcedureCommandLine.find(sLocalVars[i][0], nPos) != string::npos)
        {
            nPos = sProcedureCommandLine.find(sLocalVars[i][0], nPos);

            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~',nPos-1)] != '#')
                || sProcedureCommandLine[nPos+sLocalVars[i][0].length()] == '(')
            {
                nPos += sLocalVars[i][0].length();
                continue;
            }
            if (checkDelimiter(sProcedureCommandLine.substr(nPos-1, sLocalVars[i][0].length()+2))
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

string ProcedureVarFactory::resolveLocalStrings(string sProcedureCommandLine)
{
    if (!nLocalStrMapSize)
        return sProcedureCommandLine;
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        unsigned int nPos = 0;
        while (sProcedureCommandLine.find(sLocalStrings[i][0], nPos) != string::npos)
        {
            nPos = sProcedureCommandLine.find(sLocalStrings[i][0], nPos);

            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos+sLocalStrings[i][0].length()] == '(')
            {
                nPos += sLocalStrings[i][0].length();
                continue;
            }
            if (checkDelimiter(sProcedureCommandLine.substr(nPos-1, sLocalStrings[i][0].length()+2), true)
                && (!isInQuotes(sProcedureCommandLine, nPos, true) || isToCmd(sProcedureCommandLine, nPos)))
            {
                sProcedureCommandLine.replace(nPos, sLocalStrings[i][0].length(), sLocalStrings[i][1]);
                nPos += sLocalStrings[i][1].length();
            }
            else
                nPos += sLocalStrings[i][0].length();
        }
    }
    return sProcedureCommandLine;
}

string ProcedureVarFactory::resolveLocalTables(string sProcedureCommandLine)
{
    if (!nLocalTableSize)
        return sProcedureCommandLine;
    for (unsigned int i = 0; i < nLocalTableSize; i++)
    {
        unsigned int nPos = 0;
        while (sProcedureCommandLine.find(sLocalTables[i][0], nPos) != string::npos)
        {
            nPos = sProcedureCommandLine.find(sLocalTables[i][0], nPos);

            if ((sProcedureCommandLine[nPos-1] == '~' && sProcedureCommandLine[sProcedureCommandLine.find_last_not_of('~', nPos-1)] != '#')
                || sProcedureCommandLine[nPos-1] == '$')
            {
                nPos += sLocalTables[i][0].length();
                continue;
            }
            if (checkDelimiter(sProcedureCommandLine.substr(nPos-1, sLocalTables[i][0].length()+2), true)
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


