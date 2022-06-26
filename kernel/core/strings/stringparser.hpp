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
            std::map<std::string, int> m_mStringParams;
            mu::Parser& _parser;
            MemoryManager& _data;
            Settings& _option;

            std::string getDataForString(std::string sLine, size_t n_pos);
            std::string parseStringsInIndices(std::string sIndexExpression);
            void replaceDataOccurence(std::string& sLine, const std::string& sOccurence);
            std::string numToString(const std::string& sLine);
            int storeStringResults(StringResult& strRes, std::string sObject);
            std::string createStringOutput(StringResult& strRes, std::string& sLine, int parserFlags, bool bSilent);
            std::string createTerminalOutput(StringResult& strRes, int parserFlags);
            std::vector<StringStackItem> createStack(StringView sExpr) const;
            StringVector evaluateStack(const std::vector<StringStackItem>& rpnStack, size_t from, size_t to);
            StringResult createAndEvaluateStack(StringView sExpr);
            std::vector<bool> applyElementaryStringOperations(std::vector<std::string>& vFinal, bool& bReturningLogicals);
            void storeStringToDataObjects(StringResult& strRes, std::string& sObject, size_t& nCurrentComponent, size_t nStrings);
            void storeStringToStringObject(const std::vector<std::string>& vFinal, std::string& sObject, size_t& nCurrentComponent, size_t nStrings);
            int decodeStringParams(std::string& sLine);
            bool isSimpleString(const std::string& sLine);
            bool isToken(const char* sToken, const std::string& sLine, size_t pos);
            std::string maskControlCharacters(std::string sString);
            virtual StringResult eval(std::string& sLine, std::string sCache, bool bParseNumericals = true) override;

        public:
            enum StringParserRetVal
            {
                STRING_NUMERICAL = -1,
                STRING_SUCCESS = 1
            };

            StringParser(mu::Parser& parser, MemoryManager& data, Settings& option);
            virtual ~StringParser() {}
            StringParserRetVal evalAndFormat(std::string& sLine, std::string& sCache, bool bSilent = false, bool bCheckAssertions = false);
            virtual bool isStringExpression(const std::string& sExpression) override;
    };
}


#endif // STRINGPARSER_HPP

