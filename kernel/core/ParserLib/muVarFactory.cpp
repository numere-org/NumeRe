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

#include "muVarFactory.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Destroy this var factory.
    /////////////////////////////////////////////////
    VarFactory::~VarFactory()
    {
        Clear();
    }


    /////////////////////////////////////////////////
    /// \brief Create a new variable with the passed
    /// symbol name. Checks for existence and will
    /// return the pointer to existing variable, if
    /// found.
    ///
    /// \param sVarSymbol const std::string&
    /// \return Variable*
    ///
    /////////////////////////////////////////////////
    Variable* VarFactory::Create(const std::string& sVarSymbol)
    {
        // Ensure that the symbol is not already defined
        if (m_VarDef.find(sVarSymbol) != m_VarDef.end())
            return m_VarDef[sVarSymbol];

        // Create the storage for a new variable initialized to void
        m_varStorage.push_back(new Variable(Value()));

        m_VarDef[sVarSymbol] = m_varStorage.back();

        // Return the address of the newly created storage
        return m_varStorage.back();
    }


    /////////////////////////////////////////////////
    /// \brief Add an external variable to the
    /// variable map. Returns true, if the variable
    /// existing previously and a re-init is
    /// necessary.
    ///
    /// \param sVarSymbol const std::string&
    /// \param var Variable*
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool VarFactory::Add(const std::string& sVarSymbol, Variable* var)
    {
        bool needsReInit = m_VarDef.find(sVarSymbol) != m_VarDef.end();
        m_VarDef[sVarSymbol] = var;

        return needsReInit;
    }


    /////////////////////////////////////////////////
    /// \brief Remove the variable with the passed
    /// symbol. Returns true, if the variable was
    /// found, removed and a re-init is necessary.
    ///
    /// \param sVarSymbol const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool VarFactory::Remove(const std::string& sVarSymbol)
    {
        varmap_type::iterator item = m_VarDef.find(sVarSymbol);

		if (item == m_VarDef.end())
            return false;

        // Search for the variable in the internal storage and
        // remove it
        for (auto iter = m_varStorage.begin(); iter != m_varStorage.end(); ++iter)
        {
            if (item->second == *iter)
            {
                delete *iter;
                m_varStorage.erase(iter);
                break;
            }
        }

        m_VarDef.erase(item);
        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Get the selected variable or a
    /// nullptr, if the variable does not exist.
    ///
    /// \param sVarSymbol const std::string&
    /// \return Variable*
    ///
    /////////////////////////////////////////////////
    Variable* VarFactory::Get(const std::string& sVarSymbol)
    {
        if (m_VarAliases)
        {
            // Find the symbol in the aliases map
            auto alias = m_VarAliases->find(sVarSymbol);

            // If something has been found, try to find the
            // aliased variable in the var map
            if (alias != m_VarAliases->end())
            {
                auto iter = m_VarDef.find(alias->second);

                if (iter != m_VarDef.end())
                    return iter->second;
            }
        }

        auto iter = m_VarDef.find(sVarSymbol);

        if (iter != m_VarDef.end())
            return iter->second;

        return nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Clear the variable storage and remove
    /// all variables.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void VarFactory::Clear()
    {
        m_VarDef.clear();

        for (auto iter = m_varStorage.begin(); iter != m_varStorage.end(); ++iter)
            delete *iter;

        m_varStorage.clear();
        m_VarAliases = nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether any variable is present.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool VarFactory::Empty() const
    {
        return m_VarDef.empty();
    }


    /////////////////////////////////////////////////
    /// \brief Return the number of known variables.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t VarFactory::Size() const
    {
        return m_VarDef.size();
    }


    /////////////////////////////////////////////////
    /// \brief Return the number of managed variables
    /// (which is less or equal to the number of
    /// known variables).
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t VarFactory::ManagedSize() const
    {
        return m_varStorage.size();
    }
}

