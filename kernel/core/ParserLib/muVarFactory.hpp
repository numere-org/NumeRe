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

#ifndef MUVARFACTORY_HPP
#define MUVARFACTORY_HPP

#include "muParserDef.h"
#include <list>
#include <string>

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief This class represents the variable
    /// factory. New memory is allocated and managed
    /// in this class and stored in an internal list.
    /////////////////////////////////////////////////
    class VarFactory
    {
        public:
            VarFactory() = default;
            ~VarFactory();

            Variable* Create(const std::string& sVarSymbol);
            bool Add(const std::string& sVarSymbol, Variable* var);
            bool Remove(const std::string& sVarSymbol);

            Variable* Get(const std::string& sVarSymbol);

            void SetInitValue(const Value& init);

            void Clear();
            bool Empty() const;
            size_t Size() const;
            size_t ManagedSize() const;

            varmap_type m_VarDef;
            std::map<std::string,std::string>* m_VarAliases = nullptr;

        private:
            std::list<Variable*> m_varStorage;
            Value m_initValue;
    };
}

#endif // MUVARFACTORY_HPP

