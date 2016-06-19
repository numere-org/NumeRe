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

void StripSpaces(string&);


Debugger::Debugger()
{
    nLineNumber = 0;
    sErraticCommand = "";
    sErraticModule = "";
    bAlreadyThrown = false;
}

void Debugger::reset()
{
    vStackTrace.clear();
    resetBP();
    return;
}

void Debugger::resetBP()
{
    nLineNumber = 0;
    sErraticCommand = "";
    sErraticModule = "";
    mLocalVars.clear();
    mLocalStrings.clear();
    //mVarMap.clear();
    bAlreadyThrown = false;
    return;
}

void Debugger::pushStackItem(const string& sStackItem)
{
    vStackTrace.push_back(sStackItem);
    for (unsigned int i = 0; i < vStackTrace.back().length(); i++)
    {
        if ((!i && vStackTrace.back()[i] == '$') || (i && vStackTrace.back()[i] == '$' && vStackTrace.back()[i-1] != '\\'))
            vStackTrace.back().insert(i,1,'\\');
    }
    return;
}

void Debugger::popStackItem()
{
    if (vStackTrace.size())
        vStackTrace.pop_back();
    return;
}

void Debugger::gatherInformations(string** sLocalVars,
                        unsigned int nLocalVarMapSize,
                        double* dLocalVars,
                        string** sLocalStrings,
                        unsigned int nLocalStrMapSize,
                        const map<string,string>& sStringMap,
                        /*string** sVarMap,
                        unsigned int nVarMapSize,*/
                        const string& _sErraticCommand,
                        const string& _sErraticModule,
                        unsigned int _nLineNumber)
{
    if (bAlreadyThrown)
        return;
    if (!sErraticCommand.length())
        sErraticCommand = _sErraticCommand;
    StripSpaces(sErraticCommand);
    for (unsigned int i = 0; i < sErraticCommand.length(); i++)
    {
        if ((!i && sErraticCommand[i] == '$') || (i && sErraticCommand[i] == '$' && sErraticCommand[i-1] != '\\'))
            sErraticCommand.insert(i,1,'\\');
    }

    sErraticModule = _sErraticModule;
    nLineNumber = _nLineNumber - nLineNumber; // nLineNumber ist entweder 0 oder gleich der Loop-Line.
    bAlreadyThrown = true;
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        while (sErraticCommand.find(sLocalVars[i][1]) != string::npos)
            sErraticCommand.replace(sErraticCommand.find(sLocalVars[i][1]), sLocalVars[i][1].length(), sLocalVars[i][0]);
        mLocalVars[sLocalVars[i][0]] = dLocalVars[i];
    }
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        while (sErraticCommand.find(sLocalStrings[i][1]) != string::npos)
            sErraticCommand.replace(sErraticCommand.find(sLocalStrings[i][1]), sLocalStrings[i][1].length(), sLocalStrings[i][0]);
        mLocalStrings[sLocalStrings[i][0]] = sStringMap.at(sLocalStrings[i][1]);
    }

    return;
}

void Debugger::gatherLoopBasedInformations(const string& _sErraticCommand, unsigned int _nLineNumber, map<string,string>& mVarMap, double** vVarArray, string* sVarArray, int nVarArray)
{
    sErraticCommand = _sErraticCommand;
    nLineNumber = _nLineNumber;
    for (int i = 0; i < nVarArray; i++)
    {
        for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
        {
            if (iter->second == sVarArray[i])
            {
                mLocalVars[iter->first] = vVarArray[i][0];
                while (sErraticCommand.find(iter->second) != string::npos)
                    sErraticCommand.replace(sErraticCommand.find(iter->second), (iter->second).length(), iter->first);
            }
        }
    }
    return;
}


string Debugger::printModuleInformations()
{
    /*string sModuleInformations = "Fehlerhafter Ausdruck:   " + sErraticCommand
                            + "\\nFehlerhaftes Modul:      " + sErraticModule
                            + "\\nZeilennummer:            " + toString(nLineNumber);*/
    return _lang.get("DBG_MODULE_TEMPLATE", sErraticCommand, sErraticModule, toString(nLineNumber));
    //return sModuleInformations;
}

string Debugger::printNonErrorModuleInformations()
{
    /*string sModuleInformations = "Aktueller Ausdruck:      " + sErraticCommand
                            + "\\nAktuelles Modul:         " + sErraticModule
                            + "\\nZeilennummer:            " + toString(nLineNumber);*/
    return _lang.get("DBG_MODULE_TEMPLATE_BP", sErraticCommand, sErraticModule, toString(nLineNumber));
    //return sModuleInformations;
}

string Debugger::printStackTrace()
{
    string sStackTrace = "";
    for (int i = vStackTrace.size()-1; i >= 0; i--)
    {
        if (vStackTrace.size() > 15 && vStackTrace.size()-i > 10)
        {
            sStackTrace += "   [...]\\n";
            i = 2;
        }
        sStackTrace += "   \\$"+vStackTrace[i] + "\\n";
    }
    if (sStackTrace.length())
    {
        sStackTrace.replace(0,2,"->");
        sStackTrace.erase(sStackTrace.length()-2);
    }
    else
        sStackTrace = _lang.get("DBG_STACK_EMPTY");
    return sStackTrace;
}

string Debugger::printLocalVars()
{
    string sLocalVars = "";
    unsigned int nLength = 0;
    for (auto iter = mLocalVars.begin(); iter != mLocalVars.end(); ++iter)
    {
        if (nLength < (iter->first).length())
            nLength = (iter->first).length();
    }
    for (auto iter = mLocalVars.begin(); iter != mLocalVars.end(); ++iter)
    {
        sLocalVars += iter->first;
        sLocalVars.append(nLength - (iter->first).length()+2,' ');
        sLocalVars += " =   " + toString(iter->second, 7) + "\\n";
    }
    if (sLocalVars.length())
        sLocalVars.erase(sLocalVars.length()-2);
    else
        sLocalVars = _lang.get("DBG_LOCALVARS_EMPTY");
    return sLocalVars;
}

string Debugger::printLocalStrings()
{
    string sLocalStrings = "";
    unsigned int nLength = 0;
    for (auto iter = mLocalStrings.begin(); iter != mLocalStrings.end(); ++iter)
    {
        if ((iter->first).length() > nLength)
            nLength = (iter->first).length();
    }
    for (auto iter = mLocalStrings.begin(); iter != mLocalStrings.end(); ++iter)
    {
        sLocalStrings += iter->first;
        sLocalStrings.append(nLength - (iter->first).length()+2,' ');
        sLocalStrings += " =   \"" + iter->second + "\"\\n";
    }
    if (sLocalStrings.length())
        sLocalStrings.erase(sLocalStrings.length()-2);
    else
        sLocalStrings = _lang.get("DBG_LOCALSTRINGS_EMPTY");
    return sLocalStrings;
}

