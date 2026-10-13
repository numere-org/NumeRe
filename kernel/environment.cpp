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

#include <regex>

#include "environment.hpp"
#include "core/utils/tools.hpp"
#include "core/io/logger.hpp"
#include "../externals/tinyxml2/tinyxml2.h"

/////////////////////////////////////////////////
/// \brief Environment constructor. Tries to load
/// the environment from the file, if the file
/// exists.
///
/// \param envFile const std::string&
///
/////////////////////////////////////////////////
Environment::Environment(const std::string& envFile)
{
    if (envFile.length())
        load(envFile);
}


/////////////////////////////////////////////////
/// \brief Load the environment from file.
/// Changes also the referenced environment file
/// name.
///
/// \param envFile const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Environment::load(const std::string& envFile)
{
    m_envFileName = envFile;

    if (!m_envFileName.length() || !fileExists(m_envFileName))
        return;

    tinyxml2::XMLDocument env;

    // Try to load the XML file
    try
    {
        env.LoadFile(envFile.c_str());

        tinyxml2::XMLElement* envRoot = env.FirstChildElement("environment");

        // We require "environment" as root node
        if (!envRoot)
        {
            g_logger.error("No 'environment' root node in " + envFile);
            return;
        }

        // Find the "vars" section
        tinyxml2::XMLElement* vars = envRoot->FirstChildElement("vars");

        if (vars)
        {
            m_environmentVars.clear();
            tinyxml2::XMLElement* var = vars->FirstChildElement();

            while (var)
            {
                std::string varName = var->Attribute("name");
                std::string varValue = var->GetText();

                m_environmentVars[varName] = StrToValue(varValue);

                var = var->NextSiblingElement();
            }
        }
    }
    catch (...)
    {
        g_logger.error("Cannot import environment in " + envFile);
    }
}


/////////////////////////////////////////////////
/// \brief Save the environment to the previously
/// used environment file.
///
/// \return void
///
/////////////////////////////////////////////////
void Environment::save() const
{
    tinyxml2::XMLDocument env;

    // Construct the "environment" and the "vars" nodes
    tinyxml2::XMLElement* envElement = env.NewElement("environment");
    tinyxml2::XMLElement* varsElement = env.NewElement("vars");

    env.InsertFirstChild(envElement);
    envElement->InsertFirstChild(varsElement);

    // Insert all known variables
    for (const auto& iter : m_environmentVars)
    {
        tinyxml2::XMLElement* currentVar = env.NewElement("var");
        currentVar->SetAttribute("name", iter.first.c_str());
        currentVar->SetText(iter.second.print(14, 0, false).c_str());

        varsElement->InsertEndChild(currentVar);
    }

    env.SaveFile(m_envFileName.c_str(), false);
}


/////////////////////////////////////////////////
/// \brief Returns a constant reference to the
/// internal map of environment variables.
///
/// \return const std::map<std::string, mu::Value>&
///
/////////////////////////////////////////////////
const std::map<std::string, mu::Value>& Environment::getVars() const
{
    return m_environmentVars;
}


/////////////////////////////////////////////////
/// \brief Returns all provided environment
/// variables as a vector of strings.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> Environment::getVarList() const
{
    std::vector<std::string> varList;

    for (const auto& iter : m_environmentVars)
    {
        varList.push_back(iter.first);
    }

    return varList;
}


/////////////////////////////////////////////////
/// \brief Returns the definition of the
/// environment variable for the calltip provider.
///
/// \param varName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string Environment::getVarDefinition(const std::string& varName) const
{
    auto iter = m_environmentVars.find(varName);

    if (iter != m_environmentVars.end())
        return "<global env> " + varName + ": " + iter->second.getTypeAsString() + " = " + iter->second.print();

    return "<global env> " + varName + " = <UNDEFINED>";
}


/////////////////////////////////////////////////
/// \brief Add a new environment variable. Checks
/// for valid characters and surrounds the
/// variable name with the underscores, if
/// missing.
///
/// \param name std::string&
/// \param value std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Environment::addVar(std::string& name, std::string& value)
{
    static const std::regex VALIDCHARS("[A-Za-z0-9_]*");

    // ensure var name is correct by checking for characters and the existence of
    // something else than "_"
    if (!std::regex_match(name, VALIDCHARS)
        || name.find_first_not_of('_') == std::string::npos
        || !value.length())
        return false;

    // Pre- and append the necessary underscores
    while (!name.starts_with("__"))
        name.insert(0, 1, '_');

    while (!name.ends_with("__"))
        name += '_';

    // Add the variable and update name and value
    m_environmentVars[name] = StrToValue(value);
    value = m_environmentVars[name].print();

    return true;
}


/////////////////////////////////////////////////
/// \brief Remove a variable from the environment.
///
/// \param name const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Environment::removeVar(const std::string& name)
{
    auto iter = m_environmentVars.find(name);

    if (iter == m_environmentVars.end())
        return false;

    m_environmentVars.erase(iter);
    return true;
}


/////////////////////////////////////////////////
/// \brief Remove all variables from the
/// environment.
///
/// \return void
///
/////////////////////////////////////////////////
void Environment::clear()
{
    m_environmentVars.clear();
}

