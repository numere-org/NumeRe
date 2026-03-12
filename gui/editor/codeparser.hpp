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

#ifndef CODEPARSER_HPP
#define CODEPARSER_HPP

#include <map>
#include <stdexcept>
#include "../../kernel/core/ui/calltipprovider.hpp"
#include "lexedstring.hpp"


/////////////////////////////////////////////////
/// \brief This struct represents a single code
/// symbol with its attached attributes.
/////////////////////////////////////////////////
struct ParserSymbol
{
    enum SymbolClass
    {
        GLOBAL,
        GLOBALCONST,
        GLOBALENUM,
        GLOBALFUNC,
        LOCAL,
        LOCALFUNC,
        ARG,
        ARGREF,
        INDEX,
        ITERATOR,
        NAMESPACE
    };

    std::string m_symbol;
    std::string m_type;
    bool m_heuristicType;
    SymbolClass m_class;

    /////////////////////////////////////////////////
    /// \brief Construct a ParserSymbol instance.
    ///
    /// \param symbol const std::string&
    /// \param const std::string& type
    /// \param cls SymbolClass
    ///
    /////////////////////////////////////////////////
    ParserSymbol(const std::string& symbol, const std::string& type = "void", SymbolClass cls = GLOBAL)
        : m_symbol(symbol), m_type(type), m_heuristicType(false), m_class(cls)
    {
        if (type == "void" || type == "any" || type == "{any}" || !type.length() || (type == "cluster" && cls == ITERATOR))
            resolveTypeByHeuristic();
    }

    void resolveTypeByHeuristic();

    /////////////////////////////////////////////////
    /// \brief Equality operator.
    ///
    /// \param sSymbol const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool operator==(const std::string& sSymbol) const
    {
        return m_symbol == sSymbol;
    }

    /////////////////////////////////////////////////
    /// \brief Check, if this symbol represents a
    /// function identifier.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isFunction() const
    {
        return m_class == GLOBALFUNC || m_class == LOCALFUNC;
    }

    /////////////////////////////////////////////////
    /// \brief Check, if this symbol represents a
    /// namespace.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isNameSpace() const
    {
        return m_class == NAMESPACE;
    }

    NumeRe::CallTip print() const;
};




/////////////////////////////////////////////////
/// \brief This struct represents a parsed scope
/// within the file, which is most likely
/// identical to either the whole file for the
/// global scope or individual procedures for
/// local scope.
/////////////////////////////////////////////////
struct ParserScope
{
    int m_start;
    int m_end;
    int m_validUntil;
    std::map<int,std::vector<ParserSymbol>> m_symbols;

    /////////////////////////////////////////////////
    /// \brief Construct a ParserScope instance.
    ///
    /// \param startLine int
    /// \param endLine int
    ///
    /////////////////////////////////////////////////
    ParserScope(int startLine = 0, int endLine = -1) : m_start(startLine), m_end(endLine), m_validUntil(startLine)
    { }

    /////////////////////////////////////////////////
    /// \brief Return the enclosed line range of this
    /// scope.
    ///
    /// \return std::pair<int,int>
    ///
    /////////////////////////////////////////////////
    std::pair<int,int> getRange() const
    {
        return std::make_pair(m_start, m_end);
    }

    /////////////////////////////////////////////////
    /// \brief Return, whether the passed line is
    /// contained within this scope.
    ///
    /// \param lineNum int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool contains(int lineNum) const
    {
        return m_start <= lineNum && (m_end < 0 || m_end >= lineNum);
    }

    /////////////////////////////////////////////////
    /// \brief Determine, whether this scope is valid.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isValid() const
    {
        return m_end > m_start && m_end > m_validUntil;
    }

    /////////////////////////////////////////////////
    /// \brief Determine, whether the passed line has
    /// a valid parsed state.
    ///
    /// \param lineNum int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isValidLine(int lineNum) const
    {
        return lineNum >= m_start && lineNum < m_validUntil;
    }

    /////////////////////////////////////////////////
    /// \brief Return, whether this scope is dirty.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isDirty() const
    {
        return m_validUntil < m_end;
    }

    void insertLine(int afterLine);
    void deleteLine(int lineNum);
    void dropLineState(int lineNum);

    bool isSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const;
    ParserSymbol& getSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions);
    const ParserSymbol& getSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const;
    std::vector<ParserSymbol> getCandidates(const std::string& sSymbolStart, int lineNum, bool includeFunctions) const;
    std::string getCurrentNameSpace(int lineNum) const;

    std::string dump() const;
};




/////////////////////////////////////////////////
/// \brief This class represents a per-file code
/// parser managing the scopes and symbols for
/// the watched file.
/////////////////////////////////////////////////
class CodeParser
{
    private:
        NumeRe::CallTipProvider m_provider;
        ParserScope m_globalScope;
        std::map<int, ParserScope> m_localScopes;
        std::map<std::string,std::string> m_localProcedures;
        std::string m_fileName;

        /////////////////////////////////////////////////
        /// \brief Get a reference to the enclosed scope
        /// of the passed line.
        ///
        /// \param lineNum int
        /// \return ParserScope&
        ///
        /////////////////////////////////////////////////
        ParserScope& getScope(int lineNum)
        {
            for (auto& iter : m_localScopes)
            {
                if (iter.second.contains(lineNum))
                    return iter.second;
            }

            return m_globalScope;
        }

        ParserSymbol& getMutableSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions);

        std::string evaluateBraces(int lineNum, const LexedLine& line, size_t& pos) const;
        std::string evaluateMethods(std::string rootType, int lineNum, const LexedLine& line, size_t& pos) const;
        std::string getIdentifierType(int lineNum, const LexedLine& line, size_t& pos) const;
        void expandAssignment(int lineNum, const LexedLine& line, size_t& pos, ParserScope& scope, ParserSymbol::SymbolClass cls);
        std::string getExprType(int lineNum, const LexedLine& line, size_t& exprStart) const;

    public:
        /////////////////////////////////////////////////
        /// \brief Construct a CodeParser instance.
        ///
        /// \param provider const NumeRe::CallTipProvider&
        ///
        /////////////////////////////////////////////////
        CodeParser(const NumeRe::CallTipProvider& provider) : m_provider(provider)
        { }

        /////////////////////////////////////////////////
        /// \brief Set the filename associated with the
        /// watched file.
        ///
        /// \param sFileName const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setFileName(const std::string& sFileName)
        {
            m_fileName = sFileName;
        }

        /////////////////////////////////////////////////
        /// \brief Set the file line count.
        ///
        /// \param lineCount int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setFileLineCount(int lineCount)
        {
            m_globalScope.m_end = lineCount-1;
        }

        /////////////////////////////////////////////////
        /// \brief Set the local procedures in the
        /// current file with their return values.
        ///
        /// \param std::map<std::string const
        /// \param localProcs std::string>&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void setLocalProcedures(const std::map<std::string,std::string>& localProcs)
        {
            m_localProcedures = localProcs;
        }

        /////////////////////////////////////////////////
        /// \brief Add a local scope.
        ///
        /// \param scope const ParserScope&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void addLocalScope(const ParserScope& scope)
        {
            m_localScopes[scope.m_start] = scope;
        }

        /////////////////////////////////////////////////
        /// \brief Update a local scope by updating its
        /// boundaries.
        ///
        /// \param scope const ParserScope&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void updateLocalScope(const ParserScope& scope)
        {
            auto iter = m_localScopes.find(scope.m_start);

            if (iter != m_localScopes.end())
            {
                iter->second.m_validUntil = std::min(iter->second.m_validUntil, scope.m_end);
                iter->second.m_end = scope.m_end;
            }
        }

        /////////////////////////////////////////////////
        /// \brief Returns, whether the parser has a
        /// local scope enclosing the passed line.
        ///
        /// \param lineNum int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool hasLocalScope(int lineNum) const
        {
            for (auto iter : m_localScopes)
            {
                if (iter.second.contains(lineNum))
                    return true;
            }

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Get a constant reference to the scope
        /// enclosing the passed line.
        ///
        /// \param lineNum int
        /// \return const ParserScope&
        ///
        /////////////////////////////////////////////////
        const ParserScope& getScope(int lineNum) const
        {
            for (const auto& iter : m_localScopes)
            {
                if (iter.second.contains(lineNum))
                    return iter.second;
            }

            return m_globalScope;
        }

        /////////////////////////////////////////////////
        /// \brief Determine, whether the passed line is
        /// enclosed in a valid scope.
        ///
        /// \param lineNum int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isValidScope(int lineNum) const
        {
            if (!isValid())
                return false;

            for (auto iter : m_localScopes)
            {
                if (iter.second.contains(lineNum))
                    return iter.second.isValid();
            }

            return m_globalScope.isValid();
        }

        // Shall automatically follow includes
        void parse(int lineNum, const LexedLine& line); // We want to use -1 for nothing
        void parseSingleLine(int lineNum, const LexedLine& line);

        bool isValid() const;

        /////////////////////////////////////////////////
        /// \brief Determine, if the passed line has a
        /// valid state managed within this CodeParser
        /// instance.
        ///
        /// \param lineNum int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isValidLine(int lineNum) const
        {
            if (!isValid())
                return false;

            for (auto iter : m_localScopes)
            {
                if (iter.second.contains(lineNum))
                    return iter.second.isValidLine(lineNum);
            }

            return m_globalScope.isValidLine(lineNum);
        }

        void insertLine(int afterLine);
        void deleteLine(int lineNum);

        // Indicates that this line and everything beyond has to be reparsed
        void reparseInclude(const std::string& sIncludedFile);
        void markDirty(int lineNum);
        bool isDirty() const;
        bool needsFullReparse() const;
        const ParserScope& getNextDirtyScope() const;

        // Get symbol information
        bool isSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const;
        const ParserSymbol& getSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const;
        std::vector<ParserSymbol> getCandidates(const std::string& sSymbolStart, int lineNum, bool includeFunctions) const;
        std::string getCurrentNameSpace(int lineNum) const;
        std::string getThisNameSpace() const;

        /////////////////////////////////////////////////
        /// \brief Clear and reset this instance.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void clear()
        {
            m_fileName.clear();
            m_globalScope = ParserScope();
            m_localScopes.clear();
            m_localProcedures.clear();
        }

        std::string dump() const;
        static std::string expandReturnTypes(std::string sReturnType);
        static std::pair<std::string, bool> convertToMethodType(const std::string& rootType);
};


#endif // CODEPARSER_HPP

