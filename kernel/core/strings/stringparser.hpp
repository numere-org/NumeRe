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

#ifndef STRINGPARSER_HPP
#define STRINGPARSER_HPP

#include <string>
#include <map>
#include "stringdatastructures.hpp"
#include "stringvarfactory.hpp"
#include "stringlogicparser.hpp"
#include "stringfunchandler.hpp"
#include "../ParserLib/muParser.h"
#include "../settings.hpp"
#include "../datamanagement/memorymanager.hpp"

using namespace std;

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class is the central string
    /// expression parser. It is designed as being
    /// a singleton with a persistent existence linked
    /// to the kernel class.
    /////////////////////////////////////////////////
    class StringParser : public StringLogicParser, public StringFuncHandler
    {
        private:
            map<string, int> m_mStringParams;
            mu::Parser& _parser;
            MemoryManager& _data;
            Settings& _option;

            string getDataForString(string sLine, size_t n_pos);
            string parseStringsInIndices(string sIndexExpression);
            void replaceDataOccurence(string& sLine, const string& sOccurence);
            string numToString(const string& sLine);
            int storeStringResults(StringResult& strRes, string sObject);
            string createStringOutput(StringResult& strRes, string& sLine, int parserFlags, bool bSilent);
            string createTerminalOutput(StringResult& strRes, int parserFlags);
            vector<bool> applyElementaryStringOperations(vector<string>& vFinal, bool& bReturningLogicals);
            void storeStringToDataObjects(StringResult& strRes, string& sObject, size_t& nCurrentComponent, size_t nStrings);
            void storeStringToStringObject(const vector<string>& vFinal, string& sObject, size_t& nCurrentComponent, size_t nStrings);
            int decodeStringParams(string& sLine);
            bool isSimpleString(const string& sLine);
            bool isToken(const char* sToken, const string& sLine, size_t pos);
            string maskControlCharacters(string sString);
            virtual StringResult eval(string& sLine, string sCache, bool bParseNumericals = true) override;

        public:
            enum StringParserRetVal
            {
                STRING_NUMERICAL = -1,
                STRING_SUCCESS = 1
            };

            StringParser(mu::Parser& parser, MemoryManager& data, Settings& option);
            virtual ~StringParser() {}
            StringParserRetVal evalAndFormat(string& sLine, string& sCache, bool bSilent = false);
            virtual bool isStringExpression(const string& sExpression) override;
    };
}


#endif // STRINGPARSER_HPP

