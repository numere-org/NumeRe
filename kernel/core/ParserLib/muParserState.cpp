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

#include "muParserState.hpp"
#include "../utils/tools.hpp"
#include "../io/logger.hpp"


namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Create a expression target made up
    /// from multiple variables.
    ///
    /// \param sTargets StringView
    /// \param usedVars const varmap_type&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ExpressionTarget::create(StringView sTargets, const varmap_type& usedVars)
    {
        if (m_targets.size())
            m_targets.clear();

        while (sTargets.length())
        {
            StringView sExpr = getNextViewedArgument(sTargets);
            //g_logger.debug("Using " + sExpr.to_string());

            for (const auto& iter : usedVars)
            {
                if (sExpr == iter.first)
                {
                    m_targets.push_back(iter.second);
                    break;
                }
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief Assign the calculated values to the
    /// target vector components.
    ///
    /// \param buffer const std::vector<Array>&
    /// \param nResults int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ExpressionTarget::assign(const std::vector<Array>& buffer, int nResults)
    {
        m_targets = buffer[0];
        //g_logger.debug("Using target. Size = " + toString(m_targets.size()));
        // Assign the results to the target variables
        // and consider also singletons
        //for (size_t i = 0; i < m_targets.size(); i++)
        //    *m_targets[i] = buffer[nResults == 1 ? 0 : std::min((int)i, nResults-1)];
    }
}


