/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#ifndef MANGLER_HPP
#define MANGLER_HPP

#include <regex>
#include "../utils/tools.hpp"

/////////////////////////////////////////////////
/// \brief Simple class to mangle and demangle
/// variable names in various locations.
/////////////////////////////////////////////////
class Mangler
{
    private:
        constexpr static const char* PATTERN = "_~~?[\\w|~]+[_~A]?_?\\d+~~\\w+";

    public:
        /////////////////////////////////////////////////
        /// \brief Mangle local variables within flow
        /// control blocks.
        ///
        /// \param sName const std::string&
        /// \param level size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        static std::string mangleFlowCtrlLocals(const std::string& sName, size_t level)
        {
            return "_~~FCL" + toString(level) + "~~" + sName;
        }

        /////////////////////////////////////////////////
        /// \brief Mangle local variables.
        ///
        /// \param sName const std::string&
        /// \param sPrefix const std::string&
        /// \param level size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        static std::string mangleVarName(const std::string& sName, const std::string& sPrefix, size_t level)
        {
            return "_~" + sPrefix + "_" + toString(level) + "~~" + sName;
        }

        /////////////////////////////////////////////////
        /// \brief Mangle procedure arguments.
        ///
        /// \param sName const std::string&
        /// \param sPrefix const std::string&
        /// \param level size_t
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        static std::string mangleArgName(const std::string& sName, const std::string& sPrefix, size_t level)
        {
            return "_~" + sPrefix + "_~A_" + toString(level) + "~~" + sName;
        }

        /////////////////////////////////////////////////
        /// \brief Demangle a symbol into its original
        /// name.
        ///
        /// \param sMangledName const std::string&
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        static std::string demangleName(const std::string& sMangledName)
        {
            if (sMangledName.starts_with("_~") && sMangledName.find("~~", 3) != std::string::npos)
                return sMangledName.substr(sMangledName.rfind("~~")+2);

            return sMangledName;
        }

        /////////////////////////////////////////////////
        /// \brief Demangle a whole expression.
        ///
        /// \param sMangledExpression std::string
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        static std::string demangleExpression(std::string sMangledExpression)
        {
            std::smatch match;

            while (true)
            {
                std::regex_search(sMangledExpression, match, std::regex(PATTERN));

                if (match.size())
                    sMangledExpression.replace(match.position(),
                                               match.str().length(),
                                               match.str().substr(match.str().rfind("~~")+2));
                else
                    break;
            }

            return sMangledExpression;
        }

        /////////////////////////////////////////////////
        /// \brief Demangle a whole expression and update
        /// a relative position.
        ///
        /// \param sMangledExpression std::string
        /// \param relPos size_t&
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        static std::string demangleExpressionWithPosition(std::string sMangledExpression, size_t& relPos)
        {
            std::smatch match;

            while (true)
            {
                std::regex_search(sMangledExpression, match, std::regex(PATTERN));

                if (match.size())
                {
                    // First update the position bc. the entries in match
                    // are only referenced, which are unusable after replacement
                    if (match.position()+match.str().length() < relPos)
                        relPos -= match.str().rfind("~~")+2;

                    sMangledExpression.replace(match.position(),
                                               match.str().length(),
                                               match.str().substr(match.str().rfind("~~")+2));
                }
                else
                    break;
            }

            return sMangledExpression;
        }
};


#endif // MANGLER_HPP

