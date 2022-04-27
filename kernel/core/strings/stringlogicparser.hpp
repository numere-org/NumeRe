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

#ifndef STRINGLOGICPARSER_HPP
#define STRINGLOGICPARSER_HPP

#include <string>
#include <vector>

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class handles all logical
    /// operations on string expressions.
    /////////////////////////////////////////////////
    class StringLogicParser
    {
        private:
            std::string evalStringTernary(std::string sLine);
            std::vector<std::string> getStringTernaryExpression(std::string& sLine, size_t& nPos);
            size_t detectPathTokens(const std::string& sString, size_t nPos);
            std::string prepareComparisonValues(const std::string& _sLine);

        protected:
            bool detectStringLogicals(const std::string& sString);
            std::string evalStringLogic(std::string sLine, bool& bReturningLogicals);
            void concatenateStrings(std::string& sExpr);
    };
}


#endif // STRINGLOGICPARSER_HPP

