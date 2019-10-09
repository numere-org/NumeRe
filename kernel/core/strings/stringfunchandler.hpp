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

using namespace std;

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
            map<string,StringFuncHandle> m_mStringFuncs;

            string addMaskedStrings(const string& sString);
            void evalFunction(string& sLine, const string& sFuncName, StringFuncHandle);
            size_t argumentParser(const string&, n_vect&);
            size_t argumentParser(const string&, s_vect&, bool& bLogicalOnly);
            size_t argumentParser(const string&, s_vect&, n_vect&, n_vect&);
            size_t argumentParser(const string&, s_vect&, n_vect&, n_vect&, s_vect&);
            size_t argumentParser(const string&, s_vect&, s_vect&, n_vect&, n_vect&);
            size_t argumentParser(const string&, s_vect&, s_vect&, s_vect&, n_vect&, n_vect&);

        protected:
            string applySpecialStringFuncs(string sLine);
            string applyStringFuncs(string sLine);
            void declareStringFuncs(const map<string,StringFuncHandle>& mStringFuncs);
            size_t getStringFuncMapSize()
            {
                return m_mStringFuncs.size();
            }

        public:
            virtual ~StringFuncHandler() {}
            virtual StringResult eval(string& sLine, string sCache, bool bParseNumericals = true) = 0;
            virtual bool isStringExpression(const string& sExpression) = 0;

    };
}

#endif // STRINGFUNCHANDLER_HPP

