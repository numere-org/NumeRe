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
        std::vector<std::pair<std::string, Procedure*> > vStackTrace;
        size_t nLineNumber;
        std::string sErraticCommand;
        std::string sErraticModule;
        std::string sErrorMessage;
        std::map<std::string,mu::Array> mLocalVars;
        std::map<std::string,std::string> mLocalTables;
        std::map<std::string,std::string> mLocalClusters;
        std::map<std::string,std::string> mArguments;
        bool bAlreadyThrown;
        bool bExceptionHandled;
        size_t nCurrentStackElement;

        bool bDebuggerActive;

        int showEvent(const std::string& sTitle);
        void resetBP();
        void formatMessage();
        std::string decodeType(std::string& sArgumentValue, const std::string& sArgumentName = "");
        std::vector<std::string> getVars(mu::DataType dt);

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
        inline size_t getLineNumber() const
            {
                return nLineNumber;
            }
        inline std::string getErrorModule() const
            {
                return sErraticModule;
            }
        inline std::string getErrorMessage() const
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
        void showError(const std::string& sTitle);
        void showError(std::exception_ptr e_ptr);
        void throwException(SyntaxError error);

        int showBreakPoint();

        bool select(size_t nStackElement);

        void pushStackItem(const std::string& sStackItem, Procedure* _currentProcedure);
        void popStackItem();
        Procedure* getCurrentProcedure();

        void gatherInformations(ProcedureVarFactory* _varFactory,
                                const std::string& _sErraticCommand, const std::string& _sErraticModule, size_t _nLineNumber);

        void gatherInformations(const std::map<std::string, std::pair<std::string, mu::Variable*>>& _mLocalVars,
                                const std::map<std::string, std::string>& _mLocalTables,
                                const std::map<std::string, std::string>& _mLocalClusters,
                                const std::map<std::string, std::string>& _mArguments,
                                const std::string& _sErraticCommand, const std::string& _sErraticModule, size_t _nLineNumber);

        void gatherLoopBasedInformations(const std::string& _sErraticCommand, size_t _nLineNumber, std::map<std::string,std::string>& mVarMap, const std::vector<mu::Variable>& vVarArray, const std::vector<std::string>& sVarArray);

        std::vector<std::string> getModuleInformations();
        std::vector<std::string> getStackTrace();
        std::vector<std::string> getNumVars();
        std::vector<std::string> getStringVars();
        std::vector<std::string> getTables();
        std::vector<std::string> getClusters();
        std::vector<std::string> getArguments();
        std::vector<std::string> getGlobals();

        std::string getExecutedModule() const;
};


#endif // DEBUGGER_HPP

