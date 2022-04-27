/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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


#ifndef STRINGFUNCHANDLER_HPP
#define STRINGFUNCHANDLER_HPP

#include <string>
#include <map>
#include "stringdatastructures.hpp"
#include "stringvarfactory.hpp"

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class provides the complete
    /// function evaluation logic to the StringParser
    /// class.
    /////////////////////////////////////////////////
    class StringFuncHandler : public StringVarFactory
    {
        private:
            std::map<std::string,StringFuncHandle> m_mStringFuncs;

            std::string addMaskedStrings(const std::string& sString);
            void evalFunction(std::string& sLine, const std::string& sFuncName, StringFuncHandle);
            size_t argumentParser(const std::string&, n_vect&);
            size_t argumentParser(const std::string&, d_vect&);
            size_t argumentParser(const std::string&, s_vect&, bool& bLogicalOnly);
            size_t argumentParser(const std::string&, s_vect&, d_vect&);
            size_t argumentParser(const std::string&, s_vect&, n_vect&, n_vect&);
            size_t argumentParser(const std::string&, s_vect&, n_vect&, n_vect&, s_vect&);
            size_t argumentParser(const std::string&, s_vect&, s_vect&, n_vect&, n_vect&);
            size_t argumentParser(const std::string&, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&);

            std::vector<std::string> callFunction(StringFuncHandle, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&, d_vect&, size_t);
            std::vector<std::string> callFunctionParallel(StringFuncHandle, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&, d_vect&, size_t);
            std::vector<std::string> callMultiFunction(StringFuncHandle, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&, d_vect&, size_t);
            std::vector<std::string> callMultiFunctionParallel(StringFuncHandle, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&, d_vect&, size_t);

        protected:
            std::string applySpecialStringFuncs(std::string sLine);
            std::string applyStringFuncs(std::string sLine);
            void declareStringFuncs(const std::map<std::string,StringFuncHandle>& mStringFuncs);
            size_t findNextFunction(const std::string& sFunc, const std::string& sLine, size_t nStartPos, size_t& nEndPosition, bool searchForMethods = false);
            std::string getFunctionArgumentList(const std::string& sFunc, const std::string& sLine, size_t nStartPosition, size_t nEndPosition);
            std::string printValue(const mu::value_type& value);
            size_t getStringFuncMapSize() const
            {
                return m_mStringFuncs.size();
            }
            virtual StringResult eval(std::string& sLine, std::string sCache, bool bParseNumericals = true) = 0;

        public:
            virtual ~StringFuncHandler() {}
            virtual bool isStringExpression(const std::string& sExpression) = 0;

    };
}

#endif // STRINGFUNCHANDLER_HPP

