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
#include "stringdatastructures.hpp"

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class handles the creation and
    /// deletion of string vector variables.
    /////////////////////////////////////////////////
    class StringVarFactory
    {
        private:
            std::map<std::string,StringVector> m_mStringVectorVars;
            std::map<std::string,StringVector> m_mTempStringVectorVars;
            std::map<std::string,std::string> m_mStringVars;

            bool isNumericCandidate(const std::string& sComponent);
            bool checkStringvarDelimiter(const std::string& sToken) const;
            void replaceStringVectorVars(std::map<std::string,StringVector>& mVectorVarMap, std::string& currentline, size_t nCurrentComponent, bool& bHasComponents);
            std::string findVectorInMap(const std::map<std::string,StringVector>& mVectorVarMap, const std::vector<std::string>& vStringVector);

        protected:
            StringVector evaluateStringVectors(std::string sLine);
            StringVector expandStringVectorComponents(std::vector<StringVector>& vStringVector);
            void removeStringVectorVars();
            std::string createStringVectorVar(const std::vector<std::string>& vStringVector);
            bool isStringVectorVar(const std::string& sVarName) const;
            const StringVector& getStringVectorVar(const std::string& sVarName) const;
            void getStringValuesAsInternalVar(std::string& sLine, size_t nPos = 0);

        public:
            bool containsStringVectorVars(const std::string& sLine);
            std::string createTempStringVectorVar(const std::vector<std::string>& vStringVector);
            void removeTempStringVectorVars();
            bool containsStringVars(const std::string& sLine) const;
            bool isStringVar(const std::string& sVarName) const;
            void getStringValues(std::string& sLine);
            std::string getStringValue(const std::string& sVar) const;
            void setStringValue(const std::string& sVar, const std::string& sValue);
            void removeStringVar(const std::string& sVar);

            // Returns a reference to the internal string variable map
            inline const std::map<std::string, std::string>& getStringVars() const
            {
                return m_mStringVars;
            }
    };
}


#endif // STRINGVARFACTORY_HPP

