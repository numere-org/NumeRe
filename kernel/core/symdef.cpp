/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#include "symdef.hpp"
#include "utils/tools.hpp"
#include "maths/parser_functions.hpp"




/////////////////////////////////////////////////
/// \brief Remove all file-static constant
/// declarations.
///
/// \return void
///
/////////////////////////////////////////////////
void SymDefManager::clear()
{
    m_symDefs.clear();
}


/////////////////////////////////////////////////
/// \brief Resolve all file-static constant
/// declarations in the current line.
///
/// \param sCommandLine std::string&
/// \return void
///
/////////////////////////////////////////////////
void SymDefManager::resolveSymbols(std::string& sCommandLine) const
{
    // Go through all definitions
    for (auto iter = m_symDefs.begin(); iter != m_symDefs.end(); ++iter)
    {
        size_t pos = 0;

        // Find all occurences of the current variable
        while ((pos = findVariableInExpression(sCommandLine, iter->first, pos)) != std::string::npos)
        {
            sCommandLine.replace(pos, iter->first.length(), iter->second);
            pos += iter->second.length();
        }
    }
}


/////////////////////////////////////////////////
/// \brief Create one or more new file-static
/// constant declarations for the current file.
///
/// \param sCommandLine const std::string&
/// \return void
///
/////////////////////////////////////////////////
void SymDefManager::createSymbol(const std::string& sCommandLine)
{
    // Separate the list of declarations
    EndlessVector<std::string> symdefs = getAllArguments(sCommandLine);

    // Declare each of them, if the definition operator
    // is found and symbol and its definitions are of
    // non-zero length
    for (std::string symdef : symdefs)
    {
        size_t pos = symdef.find(":=");

        if (pos != std::string::npos)
        {
            std::string symbol = symdef.substr(0, pos);
            std::string definition = symdef.substr(pos+2);

            // Enable nested definitions
            resolveSymbols(definition);

            StripSpaces(symbol);
            StripSpaces(definition);

            if (definition.back() == ';')
            {
                definition.pop_back();
                StripSpaces(definition);
            }

            if (symbol.length() && definition.length() && symbol != definition)
                m_symDefs[symbol] = definition;
        }
        else if ((pos = symdef.find("->")) != std::string::npos && symdef.find("enum") < pos)
        {
            // Declare an enumeration
            // Get the enumerated symbols
            std::string definition = symdef.substr(pos+2);

            // enable nested definitions
            resolveSymbols(definition);

            // Remove the surrounding whitespace, if any
            StripSpaces(definition);

            // Strings are not allowed here
            if (containsStrings(definition))
                continue;

            // Get each single symbol
            std::vector<std::string> vEnumSymbols = toStrVector(definition);

            int nEnumVal = 1;

            // Will not create s.th. if the vector is empty
            for (const auto& sEnum : vEnumSymbols)
            {
                // Declare each single symbol
                if ((pos = sEnum.find('=')) == std::string::npos)
                    m_symDefs[sEnum] = toString(nEnumVal);
                else
                {
                    // Consider preset cases
                    std::string symbol = sEnum.substr(0, pos);
                    std::string value = sEnum.substr(pos+1);
                    // Allow using already defined enumerated symbols
                    resolveSymbols(value);
                    StripSpaces(symbol);

                    if (isConvertible(value, CONVTYPE_VALUE))
                        nEnumVal = intCast(StrToCmplx(value));

                    m_symDefs[symbol] = toString(nEnumVal);
                }

                nEnumVal++;
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Check, whether the passed string is a
/// defined symbol (will be used by the static
/// code analyzer).
///
/// \param sSymbol const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool SymDefManager::isSymbol(const std::string& sSymbol) const
{
    return m_symDefs.find(sSymbol) != m_symDefs.end();
}




