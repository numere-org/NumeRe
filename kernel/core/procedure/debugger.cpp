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

void StripSpaces(string&);


NumeReDebugger::NumeReDebugger()
{
    nLineNumber = 0;
    sErraticCommand = "";
    sErraticModule = "";
    bAlreadyThrown = false;
}

void NumeReDebugger::reset()
{
    vStackTrace.clear();
    resetBP();
    return;
}

void NumeReDebugger::resetBP()
{
    nLineNumber = 0;
    sErraticCommand = "";
    sErraticModule = "";
    mLocalVars.clear();
    mLocalStrings.clear();
    mLocalTables.clear();
    //mVarMap.clear();
    bAlreadyThrown = false;
    return;
}

void NumeReDebugger::pushStackItem(const string& sStackItem)
{
    vStackTrace.push_back(sStackItem);
    for (unsigned int i = 0; i < vStackTrace.back().length(); i++)
    {
        if ((!i && vStackTrace.back()[i] == '$') || (i && vStackTrace.back()[i] == '$' && vStackTrace.back()[i-1] != '\\'))
            vStackTrace.back().insert(i,1,'\\');
    }
    if (vStackTrace.back().find('/') != string::npos && vStackTrace.back().find('/') < vStackTrace.back().find('('))
    {
        vStackTrace.back().insert(vStackTrace.back().find('$')+1, "'");
        vStackTrace.back().insert(vStackTrace.back().find('('), "'");
    }
    return;
}

void NumeReDebugger::popStackItem()
{
    if (vStackTrace.size())
        vStackTrace.pop_back();
    return;
}

void NumeReDebugger::gatherInformations(string** sLocalVars,
                        unsigned int nLocalVarMapSize,
                        double* dLocalVars,
                        string** sLocalStrings,
                        unsigned int nLocalStrMapSize,
                        string** sLocalTables,
                        unsigned int nLocalTableMapSize,
                        const string& _sErraticCommand,
                        const string& _sErraticModule,
                        unsigned int _nLineNumber)
{
    if (bAlreadyThrown)
        return;

    NumeReKernel* instance = NumeReKernel::getInstance();

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
        if (sLocalVars[i][0] != sLocalVars[i][1])
        {
            while (sErraticCommand.find(sLocalVars[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalVars[i][1]), sLocalVars[i][1].length(), sLocalVars[i][0]);
        }

        mLocalVars[sLocalVars[i][0]] = dLocalVars[i];
    }

    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        if (sLocalStrings[i][0] != sLocalStrings[i][1])
        {
            while (sErraticCommand.find(sLocalStrings[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalStrings[i][1]), sLocalStrings[i][1].length(), sLocalStrings[i][0]);
        }

        mLocalStrings[sLocalStrings[i][0]] = instance->getData().getStringVars().at(sLocalStrings[i][1]);
    }

    for (unsigned int i = 0; i < nLocalTableMapSize; i++)
    {
        if (sLocalTables[i][0] != sLocalTables[i][1])
        {
            while (sErraticCommand.find(sLocalTables[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalTables[i][1]), sLocalTables[i][1].length(), sLocalTables[i][0]);
        }

        string sTableData;

        if (sLocalTables[i][1] == "string")
        {
            sTableData = toString(instance->getData().getStringElements()) + " x " + toString(instance->getData().getStringCols());
            sTableData += "\tstring\t{\"" + instance->getData().minString() + "\", ..., \"" + instance->getData().maxString() + "\"}";
        }
        else
        {
            sTableData = toString(instance->getData().getLines(sLocalTables[i][1], false)) + " x " + toString(instance->getData().getCols(sLocalTables[i][1], false));
            sTableData += "\tdouble\t{" + toString(instance->getData().min(sLocalTables[i][1], "")[0], 5) + ", ..., " + toString(instance->getData().max(sLocalTables[i][1], "")[0], 5) + "}";
        }

        mLocalTables[sLocalTables[i][0] + "()"] = sTableData;
    }

    return;
}

void NumeReDebugger::gatherLoopBasedInformations(const string& _sErraticCommand, unsigned int _nLineNumber, map<string,string>& mVarMap, double** vVarArray, string* sVarArray, int nVarArray)
{
    if (bAlreadyThrown)
        return;

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


vector<string> NumeReDebugger::getModuleInformations()
{
    vector<string> vModule;
    vModule.push_back(sErraticCommand);
    vModule.push_back(sErraticModule);
    vModule.push_back(toString(nLineNumber+1));
    return vModule;
}

vector<string> NumeReDebugger::getStackTrace()
{
    vector<string> vStack;
    if (!vStackTrace.size())
    {
        vStack.push_back(_lang.get("DBG_STACK_EMPTY"));
        return vStack;
    }
    for (int i = vStackTrace.size()-1; i >= 0; i--)
    {
        if (i == (int)vStackTrace.size()-1)
            vStack.push_back("-> $" + vStackTrace[i]);
        else
            vStack.push_back("$" + vStackTrace[i]);
    }
    return vStack;
}

vector<string> NumeReDebugger::getNumVars()
{
    vector<string> vNumVars;
    for (auto iter = mLocalVars.begin(); iter != mLocalVars.end(); ++iter)
    {
        vNumVars.push_back(iter->first + "\t1 x 1\tdouble\t" + toString(iter->second, 7));
    }
    return vNumVars;
}

vector<string> NumeReDebugger::getStringVars()
{
    vector<string> vStringVars;
    for (auto iter = mLocalStrings.begin(); iter != mLocalStrings.end(); ++iter)
    {
        vStringVars.push_back(iter->first + "\t1 x 1\tstring\t\"" + iter->second + "\"");
    }
    return vStringVars;
}

vector<string> NumeReDebugger::getTables()
{
    vector<string> vTables;
    for (auto iter = mLocalTables.begin(); iter != mLocalTables.end(); ++iter)
    {
        vTables.push_back(iter->first + "\t" + iter->second);
    }
    return vTables;
}



