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

// class: Debugger
// Kann aktiviert werden, um zusätzliche Informationen aus einem "throw" in einer Prozedur zu ziehen

#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <vector>
#include <string>
#include <map>
#include <exception>
#include "../ui/error.hpp"
#include "breakpointmanager.hpp"
#include "../ParserLib/muParserDef.h"

using namespace std;

// stacktrace
// line number
// var vals/names
// erratic command
// erratic module
class ProcedureVarFactory;
class Procedure;

class NumeReDebugger
{
    private:
        BreakpointManager _breakpointManager;
        vector<pair<string, Procedure*> > vStackTrace;
        unsigned int nLineNumber;
        string sErraticCommand;
        string sErraticModule;
        string sErrorMessage;
        map<string,mu::value_type> mLocalVars;
        map<string,string> mLocalStrings;
        map<string,string> mLocalTables;
        map<string,string> mLocalClusters;
        map<string,string> mArguments;
        bool bAlreadyThrown;
        bool bExceptionHandled;
        size_t nCurrentStackElement;

        bool bDebuggerActive;

        int showEvent(const string& sTitle);
        void resetBP();
        void formatMessage();
        string decodeType(string& sArgumentValue, const std::string& sArgumentName = "");

    public:
        NumeReDebugger();

        void reset();
        inline void finalize()
            {
                bExceptionHandled = false;
                reset();
            }
        inline void finalizeCatched()
            {
                bExceptionHandled = false;
                resetBP();
            }
        inline bool validDebuggingInformations() const
            {
                return bAlreadyThrown;
            }
        inline unsigned int getLineNumber() const
            {
                return nLineNumber;
            }
        inline string getErrorModule() const
            {
                return sErraticModule;
            }
        inline string getErrorMessage() const
            {
                return sErrorMessage;
            }
        inline size_t getStackSize() const
            {
                return vStackTrace.size();
            }
        inline bool isActive() const
            {
                return bDebuggerActive;
            }
        inline void setActive(bool active)
            {
                bDebuggerActive = active;
            }

        inline BreakpointManager& getBreakpointManager()
            {
                return _breakpointManager;
            }
        void showError(const string& sTitle);
        void showError(exception_ptr e_ptr);
        void throwException(SyntaxError error);

        int showBreakPoint();

        bool select(size_t nStackElement);

        void pushStackItem(const string& sStackItem, Procedure* _currentProcedure);
        void popStackItem();

        void gatherInformations(ProcedureVarFactory* _varFactory,
                                const string& _sErraticCommand, const string& _sErraticModule, unsigned int _nLineNumber);

        void gatherInformations(const std::map<std::string, std::pair<std::string, mu::value_type*>>& _mLocalVars,
                                const std::map<std::string, std::pair<std::string, std::string>>& _mLocalStrings,
                                const std::map<std::string, std::string>& _mLocalTables,
                                const std::map<std::string, std::string>& _mLocalClusters,
                                const std::map<std::string, std::string>& _mArguments,
                                const string& _sErraticCommand, const string& _sErraticModule, unsigned int _nLineNumber);

        void gatherInformations(string** sLocalVars, size_t nLocalVarMapSize, mu::value_type* dLocalVars,
                                string** sLocalStrings, size_t nLocalStrMapSize,
                                string** sLocalTables, size_t nLocalTableMapSize,
                                string** sLocalClusters, size_t nLocalClusterMapSize,
                                string** sArgumentMap, size_t nArgumentMapSize,
                                const string& _sErraticCommand, const string& _sErraticModule, unsigned int _nLineNumber);

        void gatherLoopBasedInformations(const string& _sErraticCommand, unsigned int _nLineNumber, map<string,string>& mVarMap, mu::value_type** vVarArray, string* sVarArray, int nVarArray);

        vector<string> getModuleInformations();
        vector<string> getStackTrace();
        vector<string> getNumVars();
        vector<string> getStringVars();
        vector<string> getTables();
        vector<string> getClusters();
        vector<string> getArguments();
        vector<string> getGlobals();
};


#endif // DEBUGGER_HPP

