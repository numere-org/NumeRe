/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#ifndef STRINGEXPRESSION_HPP
#define STRINGEXPRESSION_HPP

#include <string>
#include "../utils/tools.hpp"

using namespace std;

namespace NumeRe
{

    struct StringExpression
    {
        string& sLine;
        string sAssignee;
        size_t nEqPos;

        StringExpression(string& _sLine, const string& sCache = "") : sLine(_sLine), sAssignee(sCache), nEqPos(1)
        {
            findAssignmentOperator();
        }

        /////////////////////////////////////////////////
        /// \brief This member function determines, whether
        /// the equal sign at \c eq_pos is an assignment
        /// operator and no boolean expression.
        ///
        /// \param eq_pos size_t
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isAssignmentOperator(size_t eq_pos)
        {
            if (!eq_pos || eq_pos >= sLine.length())
                return false;

            return sLine[eq_pos - 1] != '!' && sLine[eq_pos - 1] != '<' && sLine[eq_pos - 1] != '>' && sLine[eq_pos + 1] != '=' && sLine[eq_pos - 1] != '=';
        }

        void findAssignmentOperator()
        {
            if (!nEqPos)
                return;

            nEqPos = 0;

            if (sLine.find('=') == string::npos)
                return;

            size_t nQuotes = 0;

            for (size_t i = 0; i < sLine.length(); i++)
            {
                if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
                    nQuotes++;

                if (nQuotes % 2)
                    continue;

                if (sLine[i] == '(' || sLine[i] == '[' || sLine[i] == '{')
                    i += getMatchingParenthesis(sLine.substr(i));

                if (sLine[i] == '=' && isAssignmentOperator(i))
                {
                    nEqPos = i;
                    break;
                }
            }
        }

        void split()
        {
            if (nEqPos)
            {
                sAssignee = sLine.substr(0, nEqPos);
                sLine.erase(0, nEqPos+1);
                nEqPos = 0;
            }

            StripSpaces(sAssignee);
        }
    };

}

#endif // STRINGEXPRESSION_HPP

