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
#include "../utils/tools.hpp"


// constructor
NumeReDebugger::NumeReDebugger()
{
    nLineNumber = 0;
    sErraticCommand = "";
    sErraticModule = "";
    bAlreadyThrown = false;
}

// This member function resets the debugger after a
// thrown and displayed error
void NumeReDebugger::reset()
{
    vStackTrace.clear();
    resetBP();
    return;
}

// This member function resets the debugger after
// an evaluated breakpoint. This excludes resetting
// the stacktrace
void NumeReDebugger::resetBP()
{
    nLineNumber = 0;
    sErraticCommand = "";
    sErraticModule = "";
    mLocalVars.clear();
    mLocalStrings.clear();
    mLocalTables.clear();
    bAlreadyThrown = false;
    return;
}

// This member function adds a new stack item to the
// monitored stack. Additionally, it cleanes the procedure's
// names for the stack trace display
void NumeReDebugger::pushStackItem(const string& sStackItem)
{
    vStackTrace.push_back(sStackItem);

    // Insert a leading backslash, if it missing
    for (unsigned int i = 0; i < vStackTrace.back().length(); i++)
    {
        if ((!i && vStackTrace.back()[i] == '$') || (i && vStackTrace.back()[i] == '$' && vStackTrace.back()[i-1] != '\\'))
            vStackTrace.back().insert(i,1,'\\');
    }

    // Insert the single quotation marks for explicit procedure
    // paths
    if (vStackTrace.back().find('/') != string::npos && vStackTrace.back().find('/') < vStackTrace.back().find('('))
    {
        vStackTrace.back().insert(vStackTrace.back().find('$')+1, "'");
        vStackTrace.back().insert(vStackTrace.back().find('('), "'");
    }

    return;
}

// This member function removes the last item from the stack
void NumeReDebugger::popStackItem()
{
    if (vStackTrace.size())
        vStackTrace.pop_back();

    return;
}

// This member function gathers all information from the current
// workspace and stores them internally to display them to the user
void NumeReDebugger::gatherInformations(string** sLocalVars, unsigned int nLocalVarMapSize, double* dLocalVars, string** sLocalStrings, unsigned int nLocalStrMapSize, string** sLocalTables, unsigned int nLocalTableMapSize, const string& _sErraticCommand, const string& _sErraticModule, unsigned int _nLineNumber)
{
    if (bAlreadyThrown)
        return;

    // Get the instance of the kernel
    NumeReKernel* instance = NumeReKernel::getInstance();

    // If the instance is zero, something really bad happened
    if (!instance)
        return;

    // Store the command line containing the error
    if (!sErraticCommand.length())
        sErraticCommand = _sErraticCommand;

    // Removes the leading and trailing whitespaces
    StripSpaces(sErraticCommand);

    // Add leading backspaces to all occuring procedure calls
    // in the stored command line
    for (unsigned int i = 0; i < sErraticCommand.length(); i++)
    {
        if ((!i && sErraticCommand[i] == '$') || (i && sErraticCommand[i] == '$' && sErraticCommand[i-1] != '\\'))
            sErraticCommand.insert(i,1,'\\');
    }

    sErraticModule = _sErraticModule;
    nLineNumber = _nLineNumber - nLineNumber; // nLineNumber ist entweder 0 oder gleich der Loop-Line.
    bAlreadyThrown = true;

    // Store the local numerical variables and replace their
    // occurence with their definition in the command lines
    for (unsigned int i = 0; i < nLocalVarMapSize; i++)
    {
        // Replace the occurences
        if (sLocalVars[i][0] != sLocalVars[i][1])
        {
            while (sErraticCommand.find(sLocalVars[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalVars[i][1]), sLocalVars[i][1].length(), sLocalVars[i][0]);
        }

        mLocalVars[sLocalVars[i][0] + "\t" + sLocalVars[i][1]] = dLocalVars[i];
    }

    // Store the local string variables and replace their
    // occurence with their definition in the command lines
    for (unsigned int i = 0; i < nLocalStrMapSize; i++)
    {
        // Replace the occurences
        if (sLocalStrings[i][0] != sLocalStrings[i][1])
        {
            while (sErraticCommand.find(sLocalStrings[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalStrings[i][1]), sLocalStrings[i][1].length(), sLocalStrings[i][0]);
        }

        mLocalStrings[sLocalStrings[i][0] + "\t" + sLocalStrings[i][1]] = replaceControlCharacters(instance->getData().getStringVars().at(sLocalStrings[i][1]));
    }

    // Store the local tables and replace their
    // occurence with their definition in the command lines
    for (unsigned int i = 0; i < nLocalTableMapSize; i++)
    {
        // Replace the occurences
        if (sLocalTables[i][0] != sLocalTables[i][1])
        {
            while (sErraticCommand.find(sLocalTables[i][1]) != string::npos)
                sErraticCommand.replace(sErraticCommand.find(sLocalTables[i][1]), sLocalTables[i][1].length(), sLocalTables[i][0]);
        }

        string sTableData;

        // Extract the minimal and maximal values of the tables
        // to display them in the variable viewer panel
        if (sLocalTables[i][1] == "string")
        {
            sTableData = toString(instance->getData().getStringElements()) + " x " + toString(instance->getData().getStringCols());
            sTableData += "\tstring\t{\"" + replaceControlCharacters(instance->getData().minString()) + "\", ..., \"" + replaceControlCharacters(instance->getData().maxString()) + "\"}\tstring()";
        }
        else
        {
            sTableData = toString(instance->getData().getLines(sLocalTables[i][1], false)) + " x " + toString(instance->getData().getCols(sLocalTables[i][1], false));
            sTableData += "\tdouble\t{" + toString(instance->getData().min(sLocalTables[i][1], "")[0], 5) + ", ..., " + toString(instance->getData().max(sLocalTables[i][1], "")[0], 5) + "}\t" + sLocalTables[i][1] + "()";
        }

        mLocalTables[sLocalTables[i][0] + "()"] = sTableData;
    }

    return;
}

// This member funciton gathers the necessary debugging informations
// from the current executed control flow block
void NumeReDebugger::gatherLoopBasedInformations(const string& _sErraticCommand, unsigned int _nLineNumber, map<string,string>& mVarMap, double** vVarArray, string* sVarArray, int nVarArray)
{
    if (bAlreadyThrown)
        return;

    // Store command line and line number
    sErraticCommand = _sErraticCommand;
    nLineNumber = _nLineNumber;

    // store variable names and replace their occurences with
    // their definitions
    for (int i = 0; i < nVarArray; i++)
    {
        for (auto iter = mVarMap.begin(); iter != mVarMap.end(); ++iter)
        {
            if (iter->second == sVarArray[i])
            {
                // Store the variables
                mLocalVars[iter->first] = vVarArray[i][0];

                // Replace the variables
                while (sErraticCommand.find(iter->second) != string::npos)
                    sErraticCommand.replace(sErraticCommand.find(iter->second), (iter->second).length(), iter->first);
            }
        }
    }

    return;
}

// This member function returns the module informations as a vector
vector<string> NumeReDebugger::getModuleInformations()
{
    vector<string> vModule;
    vModule.push_back(sErraticCommand);
    vModule.push_back(sErraticModule);
    vModule.push_back(toString(nLineNumber+1));
    return vModule;
}

// This member function returns the stack trace as a vector
vector<string> NumeReDebugger::getStackTrace()
{
    vector<string> vStack;

    // Return a corresponding message, if the stack is empty
    if (!vStackTrace.size())
    {
        vStack.push_back(_lang.get("DBG_STACK_EMPTY"));
        return vStack;
    }

    // Return the stack and indicate the current procedure
    // on the stack with a prefixed arrow
    for (int i = vStackTrace.size()-1; i >= 0; i--)
    {
        if (i == (int)vStackTrace.size()-1)
            vStack.push_back("-> $" + vStackTrace[i]);
        else
            vStack.push_back("$" + vStackTrace[i]);
    }

    return vStack;
}

// This member function returns the numerical variables as a vector
vector<string> NumeReDebugger::getNumVars()
{
    vector<string> vNumVars;

    for (auto iter = mLocalVars.begin(); iter != mLocalVars.end(); ++iter)
    {
        vNumVars.push_back((iter->first).substr(0, (iter->first).find('\t')) + "\t1 x 1\tdouble\t" + toString(iter->second, 7) + (iter->first).substr((iter->first).find('\t')));
    }

    return vNumVars;
}

// This member function returns the string variables as a vector
vector<string> NumeReDebugger::getStringVars()
{
    vector<string> vStringVars;

    for (auto iter = mLocalStrings.begin(); iter != mLocalStrings.end(); ++iter)
    {
        vStringVars.push_back((iter->first).substr(0, (iter->first).find('\t')) + "\t1 x 1\tstring\t\"" + iter->second + "\"" + (iter->first).substr((iter->first).find('\t')));
    }

    return vStringVars;
}

// This member function returns the tables as a vector
vector<string> NumeReDebugger::getTables()
{
    vector<string> vTables;

    for (auto iter = mLocalTables.begin(); iter != mLocalTables.end(); ++iter)
    {
        vTables.push_back(iter->first + "\t" + iter->second);
    }

    return vTables;
}



