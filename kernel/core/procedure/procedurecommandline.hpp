/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include <string>

#ifndef PROCEDURECOMMANDLINE_HPP
#define PROCEDURECOMMANDLINE_HPP


// This class contains a single procedure command line and
// its corresponding byte code, if it is available. If the
// current line is a procedure head line, then one may extract
// the argument list and the flags directly
class ProcedureCommandLine
{
    private:
        int nFlags;
        int nType;
        int nByteCode;
        int nInlinable;
        std::string sCommandLine;
        std::string sArgumentList;

    public:
        enum Types
        {
            TYPE_UNSPECIFIED = 0,
            TYPE_PROCEDURE_HEAD,
            TYPE_PROCEDURE_FOOT,
            TYPE_PROCEDURE_BODY
        };

        enum Flags
        {
            FLAG_NONE = 0x0,
            FLAG_EXPLICIT = 0x1,
            FLAG_INLINE = 0x2,
            FLAG_MASK = 0x4,
            FLAG_PRIVATE = 0x8,
            FLAG_TEMPLATE = 0x10,
            FLAG_EVENT = 0x20
        };

        enum ByteCodes
        {
            BYTECODE_NOT_PARSED = 0x0,
            BYTECODE_COMMAND = 0x1,
            BYTECODE_NUMERICAL = 0x2,
            BYTECODE_STRING = 0x4,
            BYTECODE_DATAACCESS = 0x8,
            BYTECODE_VARDEF = 0x10,
            BYTECODE_NAMESPACE = 0x20,
            BYTECODE_FLOWCTRLSTATEMENT = 0x40,
            BYTECODE_RETURNCOMMAND = 0x80,
            BYTECODE_THROWCOMMAND = 0x100,
            BYTECODE_DEBUGBREAKPOINT = 0x200,
            BYTECODE_EXPLICIT = 0x400,
            BYTECODE_TOCOMMAND = 0x800,
            BYTECODE_PROCEDUREINTERFACE = 0x1000,
            BYTECODE_INCLUDE = 0x2000,
            BYTECODE_BREAKCMD = 0x4000,
            BYTECODE_PROMPT = 0x8000,
            BYTECODE_RECURSIVEEXPRESSION = 0x10000,
            BYTECODE_SUPPRESSANSWER = 0x20000
        };

        enum Inlineable
        {
            INLINING_UNKNOWN = -1,
            INLINING_IMPOSSIBLE = 0,
            INLINING_POSSIBLE = 1,
            INLINING_GLOBALINRETURN = 2
        };

        // Possible constructors
        ProcedureCommandLine()
            : nFlags(FLAG_NONE), nType(TYPE_UNSPECIFIED), nByteCode(BYTECODE_NOT_PARSED), nInlinable(INLINING_UNKNOWN), sCommandLine(""), sArgumentList("") {}
        ProcedureCommandLine(int _nFlags, int _nType, const std::string& _sCommandLine, const std::string& _sArgumentList = "")
            : nFlags(_nFlags), nType(_nType), nByteCode(BYTECODE_NOT_PARSED), nInlinable(INLINING_UNKNOWN), sCommandLine(_sCommandLine), sArgumentList(_sArgumentList) {}
        ProcedureCommandLine(const ProcedureCommandLine& _procCommandLine)
            : nFlags(_procCommandLine.nFlags), nType(_procCommandLine.nType), nByteCode(_procCommandLine.nByteCode), nInlinable(_procCommandLine.nInlinable),
              sCommandLine(_procCommandLine.sCommandLine), sArgumentList(_procCommandLine.sArgumentList) {}

        // Get the command line
        std::string getCommandLine() const
        {
            return sCommandLine;
        }

        // Get the argument list
        std::string getArgumentList() const
        {
            return sArgumentList;
        }

        // Get the type of the current command line
        int getType() const
        {
            return nType;
        }

        // Get the flags
        int getFlags() const
        {
            return nFlags;
        }

        // Get the byte code
        int getByteCode() const
        {
            return nByteCode;
        }

        // Set the byte code. This is reasonable only
        // if set from the procedure element class
        void setByteCode(int _nByteCode)
        {
            if (nByteCode == BYTECODE_NOT_PARSED)
                nByteCode = _nByteCode;
        }

        // Get the information on whether the
        // (whole) procedure is inlinable
        int isInlineable() const
        {
            if (nFlags & FLAG_INLINE)
                return nInlinable;

            return INLINING_IMPOSSIBLE;
        }

        // Declare the whole procedure as inlineable
        void setInlineable(int inlineable)
        {
            // do not declare other lines than the
            // procedure head
            if (nType != TYPE_PROCEDURE_HEAD)
                return;

            if (inlineable && nFlags & FLAG_INLINE)
                nInlinable = inlineable;
            else
                nInlinable = INLINING_IMPOSSIBLE;
        }
};





#endif // PROCEDURECOMMANDLINE_HPP

