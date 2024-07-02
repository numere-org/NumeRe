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


#ifndef MUPARSERSTATE_HPP
#define MUPARSERSTATE_HPP

#include <vector>
#include <string>
#include <utility>
#include "muParserDef.h"
#include "muParserBytecode.h"

class StringView;

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Describes an already evaluated data
    /// access, which can be reconstructed from the
    /// current parser state.
    /////////////////////////////////////////////////
	struct CachedDataAccess
	{
	    enum
	    {
	        NO_FLAG = 0x0,
	        IS_CLUSTER = 0x1,
	        IS_TABLE_METHOD = 0x2,
	        IS_INLINE_METHOD = 0x4
	    };

		std::string sAccessEquation; // Passed to parser_getIndices -> returns the indices for the current access
		std::string sVectorName; // target of the created vector -> use SetVectorVar
		std::string sCacheName; // needed for reading the data -> create a vector var
		int flags;
	};


    /////////////////////////////////////////////////
    /// \brief This structure contains the necessary
    /// data to resolve all preevaluated vectors.
    /////////////////////////////////////////////////
	struct VectorEvaluation
	{
	    enum EvalType
	    {
	        EVALTYPE_NONE,
	        EVALTYPE_MULTIARGFUNC,
	        EVALTYPE_VECTOR,
	        EVALTYPE_VECTOR_EXPANSION
	    };

	    EvalType m_type;
	    std::string m_mafunc;
	    std::string m_targetVect;
	    std::vector<int> m_componentDefs;

        /////////////////////////////////////////////////
        /// \brief Create a standard vector pre-
        /// evaluation.
        ///
        /// \param sTargetVect const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
	    void create(const std::string& sTargetVect)
	    {
	        m_type = EVALTYPE_VECTOR;
	        m_targetVect = sTargetVect;
	    }

        /////////////////////////////////////////////////
        /// \brief Create a multi-argument function pre-
        /// evaluation for vector arguments.
        ///
        /// \param sTargetVect const std::string&
        /// \param sMaFunc const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
	    void create(const std::string& sTargetVect, const std::string& sMaFunc)
	    {
	        m_type = EVALTYPE_MULTIARGFUNC;
	        m_targetVect = sTargetVect;
	        m_mafunc = sMaFunc;
	    }

        /////////////////////////////////////////////////
        /// \brief Create a vector expansion pre-
        /// evaluation.
        ///
        /// \param sTargetVect const std::string&
        /// \param vCompDefs const std::vector<int>&
        /// \return void
        ///
        /////////////////////////////////////////////////
	    void create(const std::string& sTargetVect, const std::vector<int>& vCompDefs)
	    {
	        m_type = EVALTYPE_VECTOR_EXPANSION;
	        m_targetVect = sTargetVect;
	        m_componentDefs = vCompDefs;
	    }

	    void clear()
	    {
	        m_type = EVALTYPE_NONE;
	        m_mafunc.clear();
	        m_targetVect.clear();
	        m_componentDefs.clear();
	    }
	};


    /////////////////////////////////////////////////
    /// \brief Defines a single parser state, which
    /// contains all necessary information for
    /// evaluating a single expression.
    /////////////////////////////////////////////////
	struct State
	{
	    ParserByteCode m_byteCode;
	    std::string m_expr;
	    int m_valid;
	    int m_numResults;
	    valbuf_type m_stackBuffer;
	    varmap_type m_usedVar;
	    VectorEvaluation m_vectEval;

	    State() : m_valid(1), m_numResults(0) {}

	    void clear()
	    {
	        m_byteCode.clear();
	        m_expr.clear();
	        m_valid = 1;
	        m_numResults = 0;
	        m_stackBuffer.clear();
	        m_usedVar.clear();
	        m_vectEval.clear();
	    }
	};


    /////////////////////////////////////////////////
    /// \brief Describes the cache of a single
    /// expression. Might contain multiple cached
    /// data accesses.
    /////////////////////////////////////////////////
	struct Cache
	{
	    std::vector<CachedDataAccess> m_accesses;
	    std::string m_expr;
	    std::string m_target;
	    bool m_enabled;

	    void clear()
	    {
	        m_accesses.clear();
	        m_expr.clear();
	        m_target.clear();
	        m_enabled = true;
	    }

	    Cache() : m_enabled(true) {}
	};


    /////////////////////////////////////////////////
    /// \brief This structure defines the overall
    /// expression target, if it is composed out of a
    /// temporary vector like {a,b}.
    /////////////////////////////////////////////////
	struct ExpressionTarget
	{
	    VarArray m_targets;

	    void create(StringView sTargets, const varmap_type& usedVars);
	    void assign(const valbuf_type& buffer, int nResults);

	    void clear()
	    {
	        m_targets.clear();
	    }

	    bool isValid() const
	    {
	        return m_targets.size();
	    }
	};


    /////////////////////////////////////////////////
    /// \brief This is the parser state stack for a
    /// whole command line. Might contain multiple
    /// single states and cached data accesses.
    /////////////////////////////////////////////////
	struct LineStateStack
	{
	    std::vector<State> m_states;
	    Cache m_cache;
	    ExpressionTarget m_target;

	    LineStateStack() : m_states(std::vector<State>(1)) {}

	    void clear()
	    {
	        m_states.clear();
	        m_cache.clear();
	        m_target.clear();
	    }
	};


    /////////////////////////////////////////////////
    /// \brief This is a stack of all parser line
    /// state stacks. Can be used to gather a bunch
    /// of already parsed command lines together.
    /////////////////////////////////////////////////
	struct StateStacks
	{
	    std::vector<LineStateStack> m_stacks;

	    State& operator()(size_t i, size_t j)
	    {
	        if (i < m_stacks.size() && j < m_stacks[i].m_states.size())
                return m_stacks[i].m_states[j];

            return m_stacks.back().m_states.back();
	    }

	    LineStateStack& operator[](size_t i)
	    {
	        if (i < m_stacks.size())
                return m_stacks[i];

            return m_stacks.back();
	    }

	    void resize(size_t s)
	    {
	        m_stacks.resize(s);
	    }

	    void clear()
	    {
	        m_stacks.clear();
	    }

	    size_t size() const
	    {
	        return m_stacks.size();
	    }
	};
}

#endif // MUPARSERSTATE_HPP

