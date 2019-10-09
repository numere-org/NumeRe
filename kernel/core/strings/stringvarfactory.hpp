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

#ifndef STRINGVARFACTORY_HPP
#define STRINGVARFACTORY_HPP

#include <string>
#include <map>
#include <vector>

using namespace std;

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class handles the creation and
    /// deletion of string vector variables.
    /////////////////////////////////////////////////
    class StringVarFactory
    {
        private:
            map<string,vector<string> > m_mStringVectorVars;
            map<string,vector<string> > m_mTempStringVectorVars;
            map<string,string> m_mStringVars;

            bool isNumericCandidate(const string& sComponent);
            bool checkStringvarDelimiter(const string& sToken) const;
            void replaceStringVectorVars(map<string,vector<string> >& m_mVectorVarMap, string& currentline, size_t nCurrentComponent, bool& bHasComponents);

        protected:
            vector<string> evaluateStringVectors(string sLine);
            void expandStringVectorComponents(vector<string>& vStringVector);
            void removeStringVectorVars();
            string createStringVectorVar(const vector<string>& vStringVector);

        public:
            bool containsStringVectorVars(const string& sLine);
            string createTempStringVectorVar(const vector<string>& vStringVector);
            void removeTempStringVectorVars();
            bool containsStringVars(const string& sLine) const;
            void getStringValues(string& sLine, unsigned int nPos = 0);
            void setStringValue(const string& sVar, const string& sValue);
            void removeStringVar(const string& sVar);

            // Returns a reference to the internal string variable map
            inline const map<string, string>& getStringVars() const
            {
                return m_mStringVars;
            }
    };
}


#endif // STRINGVARFACTORY_HPP

