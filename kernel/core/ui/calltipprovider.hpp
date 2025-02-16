/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#ifndef CALLTIPPROVIDER_HPP
#define CALLTIPPROVIDER_HPP

#include <string>
namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This structure contains the data for
    /// a single calltip, which might be shown in the
    /// editor or the terminal.
    /////////////////////////////////////////////////
    struct CallTip
    {
        std::string sDefinition;
        std::string sDocumentation;
        size_t nStart;
        size_t nEnd;

        CallTip() : nStart(0u), nEnd(0u) {}
        CallTip(const std::string& sDef, const std::string& sDoc = "", size_t s = 0u, size_t e = 0u)
            : sDefinition(sDef), sDocumentation(sDoc), nStart(s), nEnd(e) {}
    };





    /////////////////////////////////////////////////
    /// \brief This class uses a global Language
    /// instance to obtain the language string
    /// associated with a distinct code symbol,
    /// prepares the layout of the calltip and
    /// returns the calltip with all necessary fields
    /// filled.
    /////////////////////////////////////////////////
    class CallTipProvider
    {
        private:
            size_t m_maxLineLength;
            bool m_returnUnmatchedTokens;

        public:
            CallTipProvider(size_t nMaxLineLength = 100u, bool returnUnmatched = true)
                : m_maxLineLength(nMaxLineLength), m_returnUnmatchedTokens(returnUnmatched) {}

            CallTip getCommand(std::string sToken) const;
            CallTip getFunction(std::string sToken) const;
            CallTip getProcedure(std::string sToken) const;
            CallTip getOption(std::string sToken) const;
            CallTip getMethod(std::string sToken) const;
            CallTip getPredef(std::string sToken) const;
            CallTip getConstant(std::string sToken) const;

            std::string getFunctionReturnValue(std::string sToken) const;
            std::string getMethodReturnValue(std::string sToken) const;
            std::string getProcedureReturnValue(std::string sToken) const;
    };




    CallTip FindProcedureDefinition(const std::string& pathname, const std::string& procedurename);
    CallTip addLinebreaks(CallTip _cTip, size_t maxLineLength = 100u);
    void AppendToDocumentation(std::string& sDocumentation, const std::string& sNewDocLine);
}

#endif // CALLTIPPROVIDER_HPP

