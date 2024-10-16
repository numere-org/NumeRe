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

#ifndef GITHUBAPI_HPP
#define GITHUBAPI_HPP

#include <string>
#include <vector>

namespace GitHub
{
    /////////////////////////////////////////////////
    /// \brief Represents a GitHub issue.
    /////////////////////////////////////////////////
    struct Issue
    {
        std::string title;
        std::string body;
        std::vector<std::string> assignees;
        std::vector<std::string> labels;

        std::string jsonUrlEncode() const;
    };

    std::string create(const Issue& issue, const std::string& sRepoUrl, const std::string& sUserAuth);
}

#endif // GITHUBAPI_HPP

