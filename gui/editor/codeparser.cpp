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


#include "wx/stc/stc.h"
#include "codeparser.hpp"
#include "../../kernel/core/io/logger.hpp"
#include "../../kernel/core/utils/tools.hpp"
#include "../../kernel/core/procedure/includer.hpp"
#include "../../kernel/core/symdef.hpp"
#include "../../kernel/kernel.hpp"


/////////////////////////////////////////////////
/// \brief Template class to make reverse range
/// iteration possible.
/////////////////////////////////////////////////
template<typename T>
class reverse_range
{
    private:
        T& iterable_;

    public:
          explicit reverse_range(T& iterable) : iterable_{iterable} {}
          auto begin() const { return std::rbegin(iterable_); }
          auto end() const { return std::rend(iterable_); }
};




/////////////////////////////////////////////////
/// \brief This member function tries to
/// determine the symbol's type by using type
/// prefix naming convention as a heuristic.
///
/// \return void
///
/////////////////////////////////////////////////
void ParserSymbol::resolveTypeByHeuristic()
{
    bool isArg = m_symbol.front() == '_';

    if (m_symbol.length() > (1ull+isArg)
        && islower(m_symbol[0+isArg])
        && isupper(m_symbol[1+isArg]))
    {
        char typeChar = m_symbol[0+isArg];

        if (typeChar == 's' || typeChar == 'c')
        {
            m_type = "string";
            m_heuristicType = true;
        }
        else if (typeChar == 'f' || typeChar == 'n' || typeChar == 'm')
        {
            m_type = "value";
            m_heuristicType = true;
        }
        else if (typeChar == 'o')
        {
            m_type = "object";
            m_heuristicType = true;
        }
        else if (typeChar == 't')
        {
            m_type = "datetime";
            m_heuristicType = true;
        }
    }
    else if (m_symbol.length() == 1
             && (m_symbol == "x"
                 || m_symbol == "y"
                 || m_symbol == "z"
                 || m_symbol == "t"
                 || m_symbol == "p"
                 || m_symbol == "q"
                 || m_symbol == "n"
                 || m_symbol == "m"
                 || m_symbol == "l"
                 || m_symbol == "k"))
    {
        m_type = "value";
        m_heuristicType = true;
    }
    else if (m_symbol.length() == 2
             && (m_symbol.front() == 'x'
                 || m_symbol.front() == 'y'
                 || m_symbol.front() == 'z'
                 || m_symbol.front() == 't'
                 || m_symbol.front() == 'a')
             && isdigit(m_symbol.back()))
    {
        m_type = "value";
        m_heuristicType = true;
    }
}


/////////////////////////////////////////////////
/// \brief Return the symbol as a printable
/// string.
///
/// \return NumeRe::CallTip
///
/////////////////////////////////////////////////
NumeRe::CallTip ParserSymbol::print() const
{
    std::string sScope;

    switch (m_class)
    {
    case ParserSymbol::GLOBAL:
        sScope = "global";
        break;
    case ParserSymbol::GLOBALCONST:
        sScope = "global const";
        break;
    case ParserSymbol::GLOBALENUM:
        sScope = "global enum";
        break;
    case ParserSymbol::GLOBALFUNC:
        return NumeRe::CallTip("<global func> " + m_symbol + "(...) -> " + m_type + (m_heuristicType ? "?" : ""),
                               "",
                               14,
                               19+m_symbol.length());
    case ParserSymbol::LOCAL:
        sScope = "local";
        break;
    case ParserSymbol::LOCALFUNC:
        return NumeRe::CallTip("<local func> " + m_symbol + "(...) -> " + m_type + (m_heuristicType ? "?" : ""),
                               "",
                               13,
                               18+m_symbol.length());
    case ParserSymbol::ARG:
        sScope = "arg";
        break;
    case ParserSymbol::ARGREF:
        sScope = "arg&";
        break;
    case ParserSymbol::INDEX:
        sScope = "index";
        break;
    case ParserSymbol::ITERATOR:
        sScope = "iterator";
        break;
    case ParserSymbol::NAMESPACE:
        return NumeRe::CallTip("<namespace> " + m_symbol,
                               "",
                               12,
                               12+m_symbol.length());
    }

    std::string suffix;

    if (m_type == "table")
        suffix = "()";
    else if (m_type == "cluster")
        suffix = "{}";

    size_t startPos = 3 + sScope.length();

    return NumeRe::CallTip("<" + sScope + "> " + m_symbol + suffix + ": " + m_type + (m_heuristicType ? "?" : ""),
                           "",
                           startPos,
                           startPos+suffix.length()+m_symbol.length());
}




/////////////////////////////////////////////////
/// \brief Insert a new line right after the
/// selected line advancing the positions of all
/// following symbols.
///
/// \param afterLine int
/// \return void
///
/////////////////////////////////////////////////
void ParserScope::insertLine(int afterLine)
{
    if (afterLine > m_end)
        return;

    for (auto& iter : reverse_range(m_symbols))
    {
        if (iter.first > afterLine)
        {
            auto node = m_symbols.extract(iter.first);
            node.key()++;
            m_symbols.insert(std::move(node));
        }
    }

    if (afterLine < m_start)
        m_start++;

    if (afterLine < m_end)
        m_end++;

    if (afterLine < m_validUntil)
        m_validUntil = afterLine;
}


/////////////////////////////////////////////////
/// \brief Delete the selected line from this
/// scope.
///
/// \param lineNum int
/// \return void
///
/////////////////////////////////////////////////
void ParserScope::deleteLine(int lineNum)
{
    if (lineNum > m_end)
        return;

    if (m_symbols.find(lineNum) != m_symbols.end())
        m_symbols.erase(lineNum);

    for (auto& iter : m_symbols)
    {
        if (iter.first > lineNum)
        {
            auto node = m_symbols.extract(iter.first);
            node.key()--;
            m_symbols.insert(std::move(node));
        }
    }

    if (lineNum < m_start)
        m_start--;

    if (lineNum < m_end)
        m_end--;

    if (lineNum < m_validUntil)
        m_validUntil = lineNum-1;
}


/////////////////////////////////////////////////
/// \brief Drop the calculated state of the
/// selected line. Necessary to avoid the
/// aggregation of invalid symbols, if a line is
/// re-parsed.
///
/// \param lineNum int
/// \return void
///
/////////////////////////////////////////////////
void ParserScope::dropLineState(int lineNum)
{
    auto iter = m_symbols.find(lineNum);

    if (iter != m_symbols.end())
        m_symbols.erase(iter);
}


/////////////////////////////////////////////////
/// \brief Check, whether the passed symbol is
/// defined at or before the selected line.
///
/// \param sSymbol const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return bool
///
/////////////////////////////////////////////////
bool ParserScope::isSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const
{
    if (lineNum < m_start || lineNum > m_end)
        return false;

    for (const auto& iter : m_symbols)
    {
        if (iter.first > lineNum)
            return false;

        auto symbolIter = std::find(iter.second.begin(), iter.second.end(), sSymbol);

        if (symbolIter != iter.second.end() && !symbolIter->isNameSpace() && (includeFunctions || !symbolIter->isFunction()))
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Get a reference to the selected symbol
/// defined at or before the selected line.
///
/// \param sSymbol const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return ParserSymbol&
///
/////////////////////////////////////////////////
ParserSymbol& ParserScope::getSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions)
{
    if (lineNum < m_start || lineNum > m_end)
        throw std::out_of_range("Symbol " + sSymbol + " is not part of this scope.");

    for (auto& iter : m_symbols)
    {
        if (iter.first > lineNum)
            throw std::out_of_range("Symbol " + sSymbol + " is not part of this scope.");

        auto symbolIter = std::find(iter.second.begin(), iter.second.end(), sSymbol);

        if (symbolIter != iter.second.end() && !symbolIter->isNameSpace() && (includeFunctions || !symbolIter->isFunction()))
            return *symbolIter;
    }

    throw std::out_of_range("Symbol " + sSymbol + " is not part of this scope.");
}


/////////////////////////////////////////////////
/// \brief Get a const reference to the selected
/// symbol defined at or before the selected line.
///
/// \param sSymbol const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return const ParserSymbol&
///
/////////////////////////////////////////////////
const ParserSymbol& ParserScope::getSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const
{
    if (lineNum < m_start || lineNum > m_end)
        throw std::out_of_range("Symbol " + sSymbol + " is not part of this scope.");

    for (const auto& iter : m_symbols)
    {
        if (iter.first > lineNum)
            throw std::out_of_range("Symbol " + sSymbol + " is not part of this scope.");

        const auto symbolIter = std::find(iter.second.begin(), iter.second.end(), sSymbol);

        if (symbolIter != iter.second.end() && !symbolIter->isNameSpace() && (includeFunctions || !symbolIter->isFunction()))
            return *symbolIter;
    }

    throw std::out_of_range("Symbol " + sSymbol + " is not part of this scope.");
}


/////////////////////////////////////////////////
/// \brief Get autocompletion candidates for the
/// passed symbol start string, which are defined
/// at or before the selected line.
///
/// \param sSymbolStart const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return std::vector<ParserSymbol>
///
/////////////////////////////////////////////////
std::vector<ParserSymbol> ParserScope::getCandidates(const std::string& sSymbolStart, int lineNum, bool includeFunctions) const
{
    std::vector<ParserSymbol> candidates;

    if (lineNum < m_start)
        return candidates;

    for (const auto& iter : m_symbols)
    {
        if (iter.first > lineNum)
            return candidates;

        for (const auto& symbolsIter : iter.second)
        {
            if ((!includeFunctions && symbolsIter.isFunction()) || symbolsIter.isNameSpace())
                continue;

            std::string symbol = toLowerCase(symbolsIter.m_symbol);

            if (symbol.starts_with(sSymbolStart)
                || (sSymbolStart.front() != '_' && symbol.starts_with("_" + sSymbolStart)))
                candidates.push_back(symbolsIter);
        }
    }

    return candidates;
}


/////////////////////////////////////////////////
/// \brief Get the currently active namespace for
/// this scope.
///
/// \param lineNum int
/// \return std::string
///
/////////////////////////////////////////////////
std::string ParserScope::getCurrentNameSpace(int lineNum) const
{
    if (lineNum < m_start || lineNum > m_end)
        return "";

    for (const auto& iter : reverse_range(m_symbols))
    {
        if (iter.first > lineNum)
            continue;

        for (const auto& symbolsIter : iter.second)
        {
            if (symbolsIter.isNameSpace())
                return symbolsIter.m_symbol;
        }
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Dump the scope's contents into a
/// printable string.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string ParserScope::dump() const
{
    static const std::string INDENT(35ull, ' ');

    std::string content = "Scope{start=" + std::to_string(m_start+1) + ",end=" + std::to_string(m_end+1) + ",valid=" + std::to_string(m_validUntil+1) + "\n";

    for (const auto& iter : m_symbols)
    {
        content += INDENT + "    line=" + std::to_string(iter.first+1) + ":";

        for (const auto& symbols : iter.second)
        {
            content += " (" + symbols.print().sDefinition + ")";
        }

        content += "\n";
    }


    return content + INDENT + "}";
}




/////////////////////////////////////////////////
/// \brief Evaluate the contents of the curly
/// braces starting on char before pos.
///
/// \param lineNum int
/// \param line const LexedLine&
/// \param pos size_t&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::evaluateBraces(int lineNum, const LexedLine& line, size_t& pos) const
{
    std::string varType = "void";

    for (; pos < line.size(); pos++)
    {
        if (line[pos].m_str == "}")
        {
            pos++;
            break;
        }

        if (line[pos].m_str == "{")
        {
            pos++;
            std::string itemType = evaluateBraces(lineNum, line, pos);

            if (itemType != "void")
            {
                if (varType == "void")
                    varType = "{" + itemType + "}";
                else if (varType != itemType && varType != "{" + itemType + "}")
                {
                    line.advanceToClosingParens(pos, "{", "}");
                    return "cluster";
                }
            }
        }

        std::string itemType = getIdentifierType(lineNum, line, pos);

        if (itemType.length() && itemType != "void")
        {
            if (varType == "void")
                varType = "{" + itemType + "}";
            else if (varType != itemType && varType != "{" + itemType + "}")
            {
                line.advanceToClosingParens(pos, "{", "}");
                return "cluster";
            }
        }
    }

    return varType;
}


/////////////////////////////////////////////////
/// \brief Evaluate the return type of the method
/// chain starting at the position pos.
///
/// \param rootType std::string
/// \param lineNum int
/// \param line const LexedLine&
/// \param pos size_t&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::evaluateMethods(std::string rootType, int lineNum, const LexedLine& line, size_t& pos) const
{
    size_t tokenCount = line.size();

    while (tokenCount > pos+2 && line[pos+1].m_str == "." && line[pos+2].is(wxSTC_NSCR_METHOD))
    {
        pos += 2;
        std::string returnType = CodeParser::expandReturnTypes(m_provider.getMethodReturnValue(line[pos].m_str,
                                                                                               CodeParser::convertToMethodType(rootType).first));

        if (returnType != "{}")
            rootType = returnType;

        if (pos+2 < tokenCount && line[pos+1].m_str == "(")
        {
            pos += 2;
            line.advanceToClosingParens(pos, "(", ")");
        }
    }

    return rootType;
}


/////////////////////////////////////////////////
/// \brief Determine the identifier type at the
/// postion pos.
///
/// \param lineNum int
/// \param line const LexedLine&
/// \param pos size_t&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::getIdentifierType(int lineNum, const LexedLine& line, size_t& pos) const
{
    if (line[pos].is({wxSTC_NSCR_STRING, wxSTC_NSCR_STRING_PARSER}))
        return "string";

    if (line[pos].is(wxSTC_NSCR_NUMBERS))
        return "value";

    if (line[pos].m_str == "true" || line[pos].m_str == "false")
        return "logical";

    if (line[pos].is(wxSTC_NSCR_IDENTIFIER)
        && isSymbol(line[pos].m_str, lineNum, false))
    {
        std::string varType = getSymbol(line[pos].m_str, lineNum, false).m_type;

        if (pos+1 < line.size() && line[pos+1].m_str == "[")
        {
            pos += 2;
            line.advanceToClosingParens(pos, "[", "]");
        }

        return varType;
    }

    if (line[pos].m_str == "??")
    {
        if (pos+1 < line.size() && line[pos+1].m_str == "[")
        {
            pos += 2;
            line.advanceToClosingParens(pos, "[", "]");
        }

        return "any";
    }

    if (line[pos].is(wxSTC_NSCR_CLUSTER)
        && isSymbol(line[pos].m_str, lineNum, false))
    {
        std::string varType = getSymbol(line[pos].m_str, lineNum, false).m_type;
        pos += 2;
        line.advanceToClosingParens(pos, "{", "}");
        return varType;
    }

    if (line[pos].is(wxSTC_NSCR_CUSTOM_FUNCTION)
        && isSymbol(line[pos].m_str, lineNum, true))
    {
        std::string varType = getSymbol(line[pos].m_str, lineNum, true).m_type;
        pos += 2;
        line.advanceToClosingParens(pos, "(", ")");
        return varType;
    }

    if (line[pos].is(wxSTC_NSCR_FUNCTION))
    {
        std::string varType;
        if (line[pos].m_str == "path")
            varType = "object.path";
        else if (line[pos].m_str == "file")
            varType = "object.file";
        else if (line[pos].m_str == "queue")
            varType = "object.queue";
        else if (line[pos].m_str == "stack")
            varType = "object.stack";
        else
            varType = CodeParser::expandReturnTypes(m_provider.getFunctionReturnValue(line[pos].m_str));

        pos += 2;
        line.advanceToClosingParens(pos, "(", ")");
        return varType;
    }

    if (line[pos].is(wxSTC_NSCR_COMMAND))
        return CodeParser::expandReturnTypes(m_provider.getCommandReturnValue(line[pos].m_str));

    if (line[pos].is(wxSTC_NSCR_CONSTANTS))
        return m_provider.getConstant(line[pos].m_str).sDefinition.find('"') != std::string::npos ? "string" : "value";

    if (line[pos].is(wxSTC_NSCR_PROCEDURES))
    {
        // Handle namespaces
        std::string sNameSpace = getCurrentNameSpace(lineNum);
        std::string sProcName = line[pos].m_str;

        if (sProcName.find('~') == std::string::npos)
            sProcName.insert(1, sNameSpace + (sNameSpace.back() == '~' ? "" : "~"));

        if (!sProcName.starts_with("$thisfile~"))
            return CodeParser::expandReturnTypes(m_provider.getProcedureReturnValue(sProcName));

        // return local proc
        if (m_localScopes.size() && m_localProcedures.size())
        {
            auto iter = m_localProcedures.find(sProcName);

            if (iter != m_localProcedures.end())
                return CodeParser::expandReturnTypes(iter->second);
        }
    }

    return "void";
}


/////////////////////////////////////////////////
/// \brief Expand the brace'd assignment at the
/// postion pos.
///
/// \param lineNum int
/// \param line const LexedLine&
/// \param pos size_t&
/// \param scope ParserScope&
/// \param cls ParserSymbol::SymbolClass
/// \return void
///
/////////////////////////////////////////////////
void CodeParser::expandAssignment(int lineNum, const LexedLine& line, size_t& pos, ParserScope& scope, ParserSymbol::SymbolClass cls)
{
    size_t tokenCount = line.size();
    size_t closingBrace = pos+1;
    line.advanceToClosingParens(closingBrace, "{", "}");
    EndlessVector<std::string> types;

    if (tokenCount > closingBrace+2 && line[closingBrace+1].isAssignmentOperator())
    {
        closingBrace += 2;
        std::string varType = getExprType(lineNum, line, closingBrace);

        if (varType.front() == '{' && varType.back())
            types = getAllArguments(varType.substr(1, varType.length()-2));
        else
            types.push_back(varType);
    }
    else
        types.push_back("void");

    pos++;
    size_t varNum = 0;

    for (; pos < tokenCount; pos++)
    {
        if (line[pos].m_str == "}")
            break;

        if (line[pos].m_str == "{")
            expandAssignment(lineNum, line, pos, scope, cls);

        if (line[pos].is(wxSTC_NSCR_IDENTIFIER))
        {
            const std::string& varName = line[pos].m_str;
            std::string type = types.size() == 1ull ? types.front() : types[varNum];

            if (!isSymbol(varName, lineNum, false))
            {
                if (cls == ParserSymbol::INDEX || cls == ParserSymbol::ITERATOR)
                    scope.m_symbols[lineNum].push_back(ParserSymbol(varName,
                                                                    type,
                                                                    cls));
                else
                    m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(varName,
                                                                            type));
            }

            varNum++;
        }
    }
}


/////////////////////////////////////////////////
/// \brief Extract the type of the expression
/// starting at the position exprStart.
///
/// \param lineNum int
/// \param line const LexedLine&
/// \param exprStart size_t&
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::getExprType(int lineNum, const LexedLine& line, size_t& exprStart) const
{
    std::string exprType;

    if (line[exprStart].m_str == "-")
        exprType = "value";
    else if (line[exprStart].m_str == "(")
    {
        exprStart++;
        exprType = getExprType(lineNum, line, exprStart);
        line.advanceToClosingParens(exprStart, "(", ")");
    }
    else if (line[exprStart].m_str == "{")
    {
        exprStart++;
        exprType = evaluateBraces(lineNum, line, exprStart);
    }
    else
        exprType = getIdentifierType(lineNum, line, exprStart);

    if (line.size() > exprStart+2 && line[exprStart+1].m_str == "." && line[exprStart+2].is(wxSTC_NSCR_METHOD))
        exprType = evaluateMethods(exprType, lineNum, line, exprStart);

    line.advanceToNextExpression(exprStart);
    return exprType;
}


/////////////////////////////////////////////////
/// \brief Parse the line by extracting type
/// information based upon the lexed state and
/// update the internal parsed state.
///
/// \param lineNum int
/// \param line const LexedLine&
/// \return void
///
/////////////////////////////////////////////////
void CodeParser::parse(int lineNum, const LexedLine& line)
{
    // Get the correct scope first
    ParserScope& scope = getScope(lineNum);

    if (line.size() < 1)
    {
        scope.dropLineState(lineNum);
        m_globalScope.dropLineState(lineNum);
        scope.m_validUntil = lineNum+1;
        m_globalScope.m_validUntil = lineNum+1;
        return;
    }

    parseSingleLine(lineNum, line);

    scope.m_validUntil = lineNum+1;
    m_globalScope.m_validUntil = lineNum+1;
}


/////////////////////////////////////////////////
/// \brief Parse the line by extracting type
/// information based upon the lexed state
/// without updating the internal parsed state.
///
/// \param lineNum int
/// \param line const LexedLine&
/// \return void
///
/////////////////////////////////////////////////
void CodeParser::parseSingleLine(int lineNum, const LexedLine& line)
{
    size_t tokenCount = line.size();

    if (tokenCount < 1)
        return;

    // Get the correct scope first
    ParserScope& scope = getScope(lineNum);
    scope.dropLineState(lineNum);
    m_globalScope.dropLineState(lineNum);

    for (size_t pos = 0; pos < tokenCount; pos++)
    {
        const std::string& sSymbol = line[pos].m_str;
        std::string varType = "void";

        // Start with plain identifiers
        if (line[pos].is(wxSTC_NSCR_IDENTIFIER))
        {
            if (pos+2 < tokenCount && line[pos+1].isAssignmentOperator())
            {
                pos += 2;
                varType = getExprType(lineNum, line, pos);


                if (varType == "table" || varType == "any")
                    varType = "void";
            }

            if (isSymbol(sSymbol, lineNum, false))
            {
                ParserSymbol& symbol = getMutableSymbol(sSymbol, lineNum, false);

                if (varType != "void" && (symbol.m_type == "void" || symbol.m_heuristicType))
                {
                    symbol.m_type = varType;
                    symbol.m_heuristicType = false;
                }
            }
            else
                m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(sSymbol, varType));
        }
        else if (line[pos].is({wxSTC_NSCR_CLUSTER, wxSTC_NSCR_CUSTOM_FUNCTION}))
        {
            varType = line[pos].is(wxSTC_NSCR_CUSTOM_FUNCTION) ? "{value}" : "cluster";

            if (tokenCount > pos+4 && line[pos+3].isAssignmentOperator() && varType != "cluster")
            {
                pos += 4;
                varType = getExprType(lineNum, line, pos);
            }

            if (isSymbol(sSymbol, lineNum, true))
            {
                ParserSymbol& symbol = getMutableSymbol(sSymbol, lineNum, true);

                if (symbol.m_type == "void")
                    symbol.m_type = varType;
            }
            else
                m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(sSymbol, varType));
        }
        else if (line[pos].m_str == "{")
            expandAssignment(lineNum, line, pos, scope, ParserSymbol::GLOBAL);
        else if (line[pos].is(wxSTC_NSCR_COMMAND) && pos == 0)
        {
            if (sSymbol == "include" && tokenCount >= 2)
            {
                try
                {
                    // Creating this instance might fail
                    Includer incl("include " + line[1].m_str + " " + (tokenCount > 2 ? line[2].m_str : std::string()),
                                  m_fileName.substr(0, m_fileName.find_last_of("/\\")));
                    SymDefManager _symDefs;

                    // Go through all included lines in this file
                    while (incl.is_open())
                    {
                        std::string sLine = incl.getNextLine();
                        Match _mMatch = findCommand(sLine);

                        // Is it a declare?
                        if (_mMatch.sString == SYMDEF_COMMAND)
                            _symDefs.createSymbol(sLine.substr(_mMatch.nPos + _mMatch.sString.length()));

                        // Is it a define?
                        if (_mMatch.sString == "define"
                            || _mMatch.sString == "ifndefined"
                            || _mMatch.sString == "lclfunc"
                            || _mMatch.sString == "def"
                            || _mMatch.sString == "ifndef")
                        {
                            std::string sDefinition = sLine.substr(_mMatch.nPos + _mMatch.sString.length());
                            StripSpaces(sDefinition);

                            // If there is a declaration available
                            if (sDefinition.length())
                            {
                                // remove assignments and parentheses and strip the spaces
                                if (sDefinition.find('=') != std::string::npos)
                                    sDefinition.erase(sDefinition.find('='));

                                if (sDefinition.find_first_of("({") != std::string::npos)
                                    sDefinition.erase(sDefinition.find_first_of("({"));

                                StripSpaces(sDefinition);

                                if (_mMatch.sString == "lclfunc")
                                    scope.m_symbols[lineNum].push_back(ParserSymbol(sDefinition, "any", ParserSymbol::LOCALFUNC));
                                else
                                    m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(sDefinition, "any", ParserSymbol::GLOBALFUNC));
                            }
                        }
                    }

                    const std::map<std::string,std::string>& mSymbols = _symDefs.getSymbols();

                    for (const auto& iter : mSymbols)
                    {
                        if (std::isdigit(iter.second.front()))
                            m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(iter.first, "value", ParserSymbol::GLOBALCONST));
                        else if (iter.second.front() == '"')
                            m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(iter.first, "string", ParserSymbol::GLOBALCONST));
                        else
                            m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(iter.first, "void", ParserSymbol::GLOBALCONST));
                    }
                }
                catch (...)
                {
                    // Just catch all and continue
                }

                break;
            }
            else if (sSymbol == "declare")
            {
                for (size_t i = 1; i < tokenCount; i++)
                {
                    if (line[i].is(wxSTC_NSCR_IDENTIFIER)
                        && i+2 < tokenCount
                        && line[i+1].m_str == ":=")
                    {
                        const std::string& sConstant = line[i].m_str;
                        i += 2;
                        m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(sConstant,
                                                                                getIdentifierType(lineNum, line, i),
                                                                                ParserSymbol::GLOBALCONST));
                    }
                    else if (line[i].is(wxSTC_NSCR_PREDEFS)
                             && line[i].m_str == "enum"
                             && i+3 < tokenCount
                             && line[i+1].m_str == "->"
                             && line[i+2].m_str == "{")
                    {
                        i += 3;

                        for (; i < tokenCount; i++)
                        {
                            if (line[i].m_str == "}")
                                break;
                            else if (line[i].m_str == "{")
                                line.advanceToClosingParens(i, "{", "}");
                            else if (line[i].is(wxSTC_NSCR_IDENTIFIER)
                                     && i+1 < tokenCount
                                     && (line[i+1].m_str == "," || line[i+1].m_str == "=" || line[i+1].m_str == "}"))
                                m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(line[i].m_str,
                                                                                        "value",
                                                                                        ParserSymbol::GLOBALENUM));
                        }
                    }
                }

                break;
            }
            else if (sSymbol == "for")
            {
                if (tokenCount > pos+3 && line[pos+2].m_str == "{")
                {
                    pos += 2;
                    expandAssignment(lineNum, line, pos, scope, ParserSymbol::ITERATOR);
                    continue;
                }

                const std::string& varName = line[pos+2].m_str;

                if (tokenCount > pos+4 && line[pos+3].isAssignmentOperator())
                {
                    bool isIterator = line[pos+3].m_str == "->";
                    pos += 4;
                    varType = getExprType(lineNum, line, pos);
                    scope.m_symbols[lineNum].push_back(ParserSymbol(varName,
                                                                    varType.front() == '{' ? varType.substr(1, varType.length()-2) : varType,
                                                                    isIterator ? ParserSymbol::ITERATOR : ParserSymbol::INDEX));
                }
                else
                    scope.m_symbols[lineNum].push_back(ParserSymbol(varName, "value", ParserSymbol::INDEX));
            }
            else if (sSymbol == "lclfunc" && tokenCount > 5)
            {
                varType = "any";

                for (size_t i = 2; i < tokenCount; i++)
                {
                    if (line[i].isAssignmentOperator())
                    {
                        i++;
                        varType = getExprType(lineNum, line, i);
                        break;
                    }
                }

                scope.m_symbols[lineNum].push_back(ParserSymbol(line[1].m_str, varType, ParserSymbol::LOCALFUNC));
                break;
            }
            else if ((sSymbol == "define" || sSymbol == "ifndefined" || sSymbol == "ifndef") && tokenCount > 5)
            {
                varType = "any";

                for (size_t i = 2; i < tokenCount; i++)
                {
                    if (line[i].isAssignmentOperator())
                    {
                        i++;
                        varType = getExprType(lineNum, line, i);
                        break;
                    }
                }

                m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(line[1].m_str, varType, ParserSymbol::GLOBALFUNC));
                break;
            }
            else if ((sSymbol == "procedure" && lineNum > scope.m_start)
                     || (sSymbol == "endprocedure" && lineNum < scope.m_end))
            {
                // Adding a scope boundary somewhere in the current scope invalidates
                // the whole file
                clear();
                return;
            }
            else if (sSymbol == "procedure" && tokenCount > 4)
            {
                for (size_t i = 3; i < tokenCount; i++)
                {
                    const std::string& sArgument = line[i].m_str;

                    if (line[i].is(wxSTC_NSCR_IDENTIFIER))
                    {
                        bool isRef = line[i-1].m_str == "&" || (tokenCount > i+1 && line[i+1].m_str == "&");

                        if (i+2 < tokenCount
                            && line[i+1].m_str == "=")
                        {
                            i += 2;
                            scope.m_symbols[lineNum].push_back(ParserSymbol(sArgument,
                                                                            getExprType(lineNum, line, i),
                                                                            ParserSymbol::ARG));
                        }
                        else
                            scope.m_symbols[lineNum].push_back(ParserSymbol(sArgument,
                                                                            "void",
                                                                            isRef ? ParserSymbol::ARGREF : ParserSymbol::ARG));
                    }
                    else if (line[i].is(wxSTC_NSCR_CUSTOM_FUNCTION))
                    {
                        bool isRef = line[i-1].m_str == "&" || (tokenCount > i+3 && line[i+3].m_str == "&");
                        line.advanceToNextExpression(i);
                        scope.m_symbols[lineNum].push_back(ParserSymbol(sArgument,
                                                                        "table",
                                                                        isRef ? ParserSymbol::ARGREF : ParserSymbol::ARG));
                    }
                    else if (line[i].is(wxSTC_NSCR_CLUSTER))
                    {
                        bool isRef = line[i-1].m_str == "&" || (tokenCount > i+3 && line[i+3].m_str == "&");
                        line.advanceToNextExpression(i);
                        scope.m_symbols[lineNum].push_back(ParserSymbol(sArgument,
                                                                        "cluster",
                                                                        isRef ? ParserSymbol::ARGREF : ParserSymbol::ARG));
                    }
                }

                break;
            }
            else if (sSymbol == "var" || sSymbol == "str" || sSymbol == "obj")
            {
                if (sSymbol == "var")
                    varType = "value";
                else if (sSymbol == "str")
                    varType = "string";
                else
                    varType = "object";

                for (size_t i = 1; i < tokenCount; i++)
                {
                    if (line[i].is(wxSTC_NSCR_IDENTIFIER))
                    {
                        const std::string& sArgument = line[i].m_str;

                        if (i+2 < tokenCount
                            && line[i+1].m_str == "=")
                        {
                            i += 2;
                            std::string returnType = getExprType(lineNum, line, i);

                            if (returnType == "void" || returnType == "any" || returnType == "{any}" || !returnType.length())
                                returnType = varType;

                            scope.m_symbols[lineNum].push_back(ParserSymbol(sArgument,
                                                                            returnType,
                                                                            ParserSymbol::LOCAL));
                        }
                        else
                            scope.m_symbols[lineNum].push_back(ParserSymbol(sArgument,
                                                                            varType,
                                                                            ParserSymbol::LOCAL));
                    }
                }

                break;
            }
            else if (sSymbol == "tab" || sSymbol == "cst")
            {
                varType = sSymbol == "tab" ? "table" : "cluster";

                for (size_t i = 1; i < tokenCount; i++)
                {
                    if (line[i].is({wxSTC_NSCR_CUSTOM_FUNCTION, wxSTC_NSCR_CLUSTER}))
                    {
                        const std::string& sVariable = line[i].m_str;
                        line.advanceToNextExpression(i);
                        scope.m_symbols[lineNum].push_back(ParserSymbol(sVariable,
                                                                        varType,
                                                                        ParserSymbol::LOCAL));
                    }
                }

                break;
            }
            else if (sSymbol == "new")
            {
                for (size_t i = 1; i < tokenCount; i++)
                {
                    if (line[i].is(wxSTC_NSCR_CUSTOM_FUNCTION) && !isSymbol(line[i].m_str, lineNum, true))
                        m_globalScope.m_symbols[lineNum].push_back(ParserSymbol(line[i].m_str, "table"));
                }

                break;
            }
            else if (sSymbol == "namespace" && tokenCount > 1 && (&scope != &m_globalScope))
            {
                std::string sNameSpace;

                // Combine line back together
                for (size_t i = 0; i < tokenCount; i++)
                {
                    sNameSpace += line[i].m_str;
                }

                scope.m_symbols[lineNum].push_back(ParserSymbol(decodeNameSpace(sNameSpace, getThisNameSpace()),
                                                                "",
                                                                ParserSymbol::NAMESPACE));

                break;
            }
            else
            {
                auto iter = line.find(LexedString("totable", wxSTC_NSCR_OPTION));

                if (iter == line.end())
                    iter = line.find(LexedString("target", wxSTC_NSCR_OPTION));

                if (iter != line.end()
                    && iter+3 != line.end()
                    && (iter+2)->is(wxSTC_NSCR_CUSTOM_FUNCTION)
                    && !isSymbol((iter+2)->m_str, lineNum, true))
                    m_globalScope.m_symbols[lineNum].push_back(ParserSymbol((iter+2)->m_str, "table"));

                break;
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Return, whether the internal parsing
/// state is valid.
///
/// \return bool
///
/////////////////////////////////////////////////
bool CodeParser::isValid() const
{
    return m_fileName.length() && m_globalScope.m_end > 0;
}


/////////////////////////////////////////////////
/// \brief Insert a line after the passed line.
///
/// \param afterLine int
/// \return void
///
/////////////////////////////////////////////////
void CodeParser::insertLine(int afterLine)
{
    if (!isValid())
        return;

    for (auto& iter : reverse_range(m_localScopes))
    {
        iter.second.insertLine(afterLine);

        if (iter.first > afterLine)
        {
            auto node = m_localScopes.extract(iter.first);
            node.key()++;
            m_localScopes.insert(std::move(node));
        }
    }

    m_globalScope.insertLine(afterLine);
    //g_logger.info(dump());
}


/////////////////////////////////////////////////
/// \brief Remove the selected line.
///
/// \param lineNum int
/// \return void
///
/////////////////////////////////////////////////
void CodeParser::deleteLine(int lineNum)
{
    if (!isValid())
        return;

    for (auto& iter : m_localScopes)
    {
        // If scope boundaries are directly affected,
        // invalidate everything
        if (iter.first == lineNum || iter.second.m_end == lineNum)
        {
            clear();
            return;
        }

        iter.second.deleteLine(lineNum);

        if (iter.first > lineNum)
        {
            auto node = m_localScopes.extract(iter.first);
            node.key()--;
            m_localScopes.insert(std::move(node));
        }
    }

    m_globalScope.deleteLine(lineNum);
    //g_logger.info(dump());
}


/////////////////////////////////////////////////
/// \brief Mark the scope containing the passed
/// line as dirty so that it gets re-parsed.
///
/// \param lineNum int
/// \return void
///
/////////////////////////////////////////////////
void CodeParser::markDirty(int lineNum)
{
    if (!isValid())
        return;

    if (m_globalScope.contains(lineNum))
        getScope(lineNum).m_validUntil = lineNum;
}


/////////////////////////////////////////////////
/// \brief Return true, if the parsing state
/// contains a dirty scope or is not valid at all.
///
/// \return bool
///
/////////////////////////////////////////////////
bool CodeParser::isDirty() const
{
    if (!isValid())
        return true;

    for (const auto& iter : m_localScopes)
    {
        if (iter.second.isDirty())
            return true;
    }

    return m_globalScope.isDirty();
}


/////////////////////////////////////////////////
/// \brief Returns true, if a full re-parse is
/// preferrable compared to a single scope
/// re-parse.
///
/// \return bool
///
/////////////////////////////////////////////////
bool CodeParser::needsFullReparse() const
{
    if (!isValid() || (m_globalScope.isDirty() && 2 * m_globalScope.m_validUntil < m_globalScope.m_end))
        return true;

    int firstDirtyLine = m_globalScope.m_end;
    int lastDirtyLine = m_globalScope.m_start;

    for (const auto& iter : m_localScopes)
    {
        if (iter.second.isDirty())
        {
            firstDirtyLine = std::min(firstDirtyLine, iter.second.m_start);
            lastDirtyLine = std::max(lastDirtyLine, iter.second.m_end);
        }
    }

    return (2 * (lastDirtyLine-firstDirtyLine+1) > m_globalScope.m_end);
}


/////////////////////////////////////////////////
/// \brief Get a const reference to the next
/// dirty scope in this parser.
///
/// \return const ParserScope&
///
/////////////////////////////////////////////////
const ParserScope& CodeParser::getNextDirtyScope() const
{
    if (!isValid())
        throw std::out_of_range("Parser state is invalid.");

    for (const auto& iter : m_localScopes)
    {
        if (iter.second.isDirty())
            return iter.second;
    }

    if (m_globalScope.isDirty())
        return m_globalScope;

    throw std::out_of_range("No dirty scope available.");
}


/////////////////////////////////////////////////
/// \brief Check, whether the passed string
/// corresponds to a defined symbol at or before
/// this line.
///
/// \param sSymbol const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return bool
///
/////////////////////////////////////////////////
bool CodeParser::isSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const
{
    if (!isValid())
        return false;

    for (const auto& iter : m_localScopes)
    {
        if (iter.second.isSymbol(sSymbol, lineNum, includeFunctions))
            return true;

        if (iter.first > lineNum)
            break;
    }

    return m_globalScope.isSymbol(sSymbol, lineNum, includeFunctions);
}


/////////////////////////////////////////////////
/// \brief Get a mutable reference to the symbol
/// defined at or before this line. This member
/// function is private and for internal use only.
///
/// \param sSymbol const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return ParserSymbol&
///
/////////////////////////////////////////////////
ParserSymbol& CodeParser::getMutableSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions)
{
    if (!isValid())
        throw std::out_of_range("Parser state is invalid.");

    for (auto& iter : m_localScopes)
    {
        if (iter.second.isSymbol(sSymbol, lineNum, includeFunctions))
            return iter.second.getSymbol(sSymbol, lineNum, includeFunctions);

        if (iter.first > lineNum)
            break;
    }

    return m_globalScope.getSymbol(sSymbol, lineNum, includeFunctions);
}


/////////////////////////////////////////////////
/// \brief Get a const reference to the symbol
/// defined at or before this line.
///
/// \param sSymbol const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return const ParserSymbol&
///
/////////////////////////////////////////////////
const ParserSymbol& CodeParser::getSymbol(const std::string& sSymbol, int lineNum, bool includeFunctions) const
{
    if (!isValid())
        throw std::out_of_range("Parser state is invalid.");

    for (const auto& iter : m_localScopes)
    {
        if (iter.second.isSymbol(sSymbol, lineNum, includeFunctions))
            return iter.second.getSymbol(sSymbol, lineNum, includeFunctions);

        if (iter.first > lineNum)
            break;
    }

    return m_globalScope.getSymbol(sSymbol, lineNum, includeFunctions);
}


/////////////////////////////////////////////////
/// \brief Get a list of autocompletion
/// candidates starting with the selected string
/// and defined at or before this line.
///
/// \param sSymbolStart const std::string&
/// \param lineNum int
/// \param includeFunctions bool
/// \return std::vector<ParserSymbol>
///
/////////////////////////////////////////////////
std::vector<ParserSymbol> CodeParser::getCandidates(const std::string& sSymbolStart, int lineNum, bool includeFunctions) const
{
    if (!sSymbolStart.length() || !isValid())
        return std::vector<ParserSymbol>();

    std::vector<ParserSymbol> candidates = m_globalScope.getCandidates(toLowerCase(sSymbolStart), lineNum, includeFunctions);

    for (auto iter : m_localScopes)
    {
        if (iter.second.contains(lineNum))
        {
            std::vector<ParserSymbol> scopedCandidates = iter.second.getCandidates(toLowerCase(sSymbolStart), lineNum, includeFunctions);
            candidates.insert(candidates.end(), scopedCandidates.begin(), scopedCandidates.end());
        }
    }

    return candidates;
}


/////////////////////////////////////////////////
/// \brief Get the current active name space at
/// the selected line.
///
/// \param lineNum int
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::getCurrentNameSpace(int lineNum) const
{
    if (!hasLocalScope(lineNum))
        return "";

    return getScope(lineNum).getCurrentNameSpace(lineNum);
}


/////////////////////////////////////////////////
/// \brief Return the expanded representation of
/// the name space "this".
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::getThisNameSpace() const
{
    std::string sThisNameSpace = m_fileName.substr(0, m_fileName.find_last_of("\\/")+1);

    // Get the current default procedure path
    std::string sProcDefPath = NumeReKernel::getInstance()->getSettings().getProcPath();

    // If the default procedure path is part of the
    // thisfile namespace, remove this part and translate
    // it into an actual namespace
    if (sThisNameSpace.substr(0, sProcDefPath.length()) == sProcDefPath)
    {
        sThisNameSpace.erase(0, sProcDefPath.length());

        while (sThisNameSpace.front() == '/')
            sThisNameSpace.erase(0, 1);

        if (sThisNameSpace.length())
        {
            replaceAll(sThisNameSpace, "/", "~");

            while (sThisNameSpace.back() == '~')
                sThisNameSpace.pop_back();
        }
        else
            sThisNameSpace = "main";
    }
    else if (sThisNameSpace.length())
    {
        while (sThisNameSpace.back() == '/')
            sThisNameSpace.pop_back();
    }

    return sThisNameSpace;
}


/////////////////////////////////////////////////
/// \brief Dump the complete parser state into a
/// printable string.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::dump() const
{
    std::string content = "PARSERDUMP for " + m_fileName + "\n                                   Global" + m_globalScope.dump();

    for (const auto& iter : m_localScopes)
    {
        content += "\n                                   Local" + iter.second.dump();
    }

    if (m_localProcedures.size())
    {
        content += "\n                                   Local Procedures:";

        for (const auto& iter : m_localProcedures)
        {
            content += "\n                                   " + iter.first + " -> " + iter.second;
        }
    }

    return content;
}


/////////////////////////////////////////////////
/// \brief Static member function to replace any
/// abbreviated return types into their full type
/// names.
///
/// \param sReturnType std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string CodeParser::expandReturnTypes(std::string sReturnType)
{
    replaceAll(sReturnType, "STR", "string");
    replaceAll(sReturnType, "VAL", "value");
    replaceAll(sReturnType, "MAT", "{value}");
    replaceAll(sReturnType, "VEC", "{value}");
    replaceAll(sReturnType, "VAR", "value");
    replaceAll(sReturnType, "LOG", "logical");
    replaceAll(sReturnType, "TAB", "table");
    replaceAll(sReturnType, "CST", "cluster");
    replaceAll(sReturnType, "DCT", "dictstruct");
    replaceAll(sReturnType, "CAT", "category");
    replaceAll(sReturnType, "TIM", "datetime");
    replaceAll(sReturnType, "ARG", "any");

    return sReturnType;
}


/////////////////////////////////////////////////
/// \brief Static member function to convert
/// symbol types into types usable by the method
/// type selector algorithm.
///
/// \param rootType const std::string&
/// \return std::pair<std::string, bool>
///
/////////////////////////////////////////////////
std::pair<std::string, bool> CodeParser::convertToMethodType(const std::string& rootType)
{
    if (rootType == "any" || rootType == "ARG")
        return std::make_pair("*", true);

    bool isVect = rootType.front() == '{' && rootType.back() == '}';

    if (isVect)
        return std::make_pair(rootType.substr(1, rootType.length()-2), isVect);

    return std::make_pair(rootType, rootType == "table" || rootType == "cluster");
}




