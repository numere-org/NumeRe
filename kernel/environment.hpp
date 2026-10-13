/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <map>
#include <string>
#include <vector>
#include "core/ParserLib/muStructures.hpp"

/////////////////////////////////////////////////
/// \brief Represents a user-defined enviroment,
/// which can define user-specific constants.
/////////////////////////////////////////////////
class Environment
{
    private:
        std::map<std::string, mu::Value> m_environmentVars;
        std::string m_envFileName;

    public:
        Environment(const std::string& envFile = "");

        void load(const std::string& envFile);
        void save() const;

        const std::map<std::string, mu::Value>& getVars() const;
        std::vector<std::string> getVarList() const;
        std::string getVarDefinition(const std::string& varName) const;
        bool addVar(std::string& name, std::string& value);
        bool removeVar(const std::string& name);
        void clear();
};


#endif // ENVIRONMENT_HPP

