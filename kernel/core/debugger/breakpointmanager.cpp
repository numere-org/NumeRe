/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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
#include "breakpointmanager.hpp"
#include "../utils/tools.hpp"
#include "../../kernel.hpp"


/////////////////////////////////////////////////
/// \brief Evaluates, whether the current
/// breakpoint is active by evaluating the
/// expression.
///
/// \param needsLocks bool
/// \return bool
///
/////////////////////////////////////////////////
bool Breakpoint::isActive(bool needsLocks)
{
    if (!m_enabled)
        return false;

    g_logger.info("Breakpoint condition: " + m_condition);
    NumeReKernel::issueWarning(m_condition);

    if (m_condition == "true" || m_condition == "1")
        return true;
    else if (m_condition == "false" || m_condition == "0")
        return false;

    NumeReKernel* instance = NumeReKernel::getInstance();
    mu::Parser& _parser = instance->getParser();
    std::string sCache;
    int nNum;

    // Replace the function definitions, if not already done
    if (!instance->getDefinitions().call(m_condition))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, m_condition, SyntaxError::invalid_position);

    // Catch and evaluate all data and cache calls
    if (instance->getMemoryManager().containsTablesOrClusters(m_condition)
        && !instance->getStringParser().isStringExpression(m_condition))
    {
        //if (!_parser.HasCachedAccess()
        //    && _parser.CanCacheAccess()
        //    && !_parser.GetCachedEquation().length())
        //    _parser.SetCompiling(true);

        sCache = getDataElements(m_condition, _parser, instance->getMemoryManager(), instance->getSettings());

        //if (_parser.IsCompiling()
        //    && _parser.CanCacheAccess())
        //{
        //    _parser.CacheCurrentEquation(m_condition);
        //    _parser.CacheCurrentTarget(sCache);
        //}
        //
        //_parser.SetCompiling(false);
    }

    // Evaluate std::strings
    if (instance->getStringParser().isStringExpression(m_condition))
    {
        if (needsLocks)
            _parser.PauseLoopMode();

        // Call the std::string parser
        auto retVal = instance->getStringParser().evalAndFormat(m_condition, sCache, true, false, true);

        if (needsLocks)
            _parser.PauseLoopMode(false);

        // Evaluate the return value
        if (retVal != NumeRe::StringParser::STRING_NUMERICAL)
        {
            StripSpaces(m_condition);

            if (m_condition != "\"\"")
                return true;

            return false;
        }

        // It's possible that the user might have done
        // something weird with std::string operations transformed
        // into a regular expression. Replace the local
        // variables here
        //replaceLocalVars(m_condition);
    }

    // Evalute the already prepared equation
    if (!_parser.IsAlreadyParsed(m_condition))
        _parser.SetExpr(m_condition);

    mu::value_type* v = _parser.Eval(nNum);

    for (int i = 0; i < nNum; i++)
    {
        if (v[i] == 0.0 || mu::isnan(v[i]))
            return false;
    }

    return true;
}




/////////////////////////////////////////////////
/// \brief Constructor
/////////////////////////////////////////////////
BreakpointManager::BreakpointManager()
{
    mBreakpoints.clear();
}


/////////////////////////////////////////////////
/// \brief Copy constructor
///
/// \param _messenger const BreakpointManager&
///
/////////////////////////////////////////////////
BreakpointManager::BreakpointManager(const BreakpointManager& _messenger) : BreakpointManager()
{
    passBreakpoints(_messenger.mBreakpoints);
}


/////////////////////////////////////////////////
/// \brief This member function adds a breakpoint
/// to the passed file at the indicated line
/// number.
///
/// \param _sFilename const std::string&
/// \param nLine size_t
/// \param bp const Breakpoint&
/// \return void
///
/////////////////////////////////////////////////
void BreakpointManager::addBreakpoint(const std::string& _sFilename, size_t nLine, const Breakpoint& bp)
{
    mBreakpoints[replacePathSeparator(_sFilename)][nLine] = bp;
}


/////////////////////////////////////////////////
/// \brief This member function removes a
/// breakpoint from the passed file at the
/// indicated line number.
///
/// \param _sFilename const std::string&
/// \param nLine size_t
/// \return void
///
/////////////////////////////////////////////////
void BreakpointManager::removeBreakpoint(const std::string& _sFilename, size_t nLine)
{
    std::string sFile = replacePathSeparator(_sFilename);

    // Try to find the current file in the map
    if (mBreakpoints.find(sFile) != mBreakpoints.end())
    {
        // Try to find the current line
        auto iter = mBreakpoints[sFile].find(nLine);

        if (iter != mBreakpoints[sFile].end())
        {
            // Erase the breakpoint if it has been found
            mBreakpoints[sFile].erase(iter);

            // Remove the complete file, if it is now empty
            if (mBreakpoints[sFile].empty())
                mBreakpoints.erase(sFile);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function removes all
/// breakpoints from the passed file.
///
/// \param _sFilename const std::string&
/// \return void
///
/////////////////////////////////////////////////
void BreakpointManager::clearBreakpoints(const std::string& _sFilename)
{
    if (mBreakpoints.find(replacePathSeparator(_sFilename)) != mBreakpoints.end())
        mBreakpoints.erase(replacePathSeparator(_sFilename));
}


/////////////////////////////////////////////////
/// \brief This member function accepts the
/// breakpoints passed by a map of the
/// corresponding type.
///
/// \param _mBreakpoints const std::map<std::string, std::map<size_t, Breakpoint>>&
/// \return void
///
/////////////////////////////////////////////////
void BreakpointManager::passBreakpoints(const std::map<std::string, std::map<size_t, Breakpoint>>& _mBreakpoints)
{
    mBreakpoints.clear();
    mBreakpoints = _mBreakpoints;
}


/////////////////////////////////////////////////
/// \brief This member function returns true, if
/// the user has set a breakpoint in the passed
/// file at the passed line number.
///
/// \param _sFilename const std::string&
/// \param nLine size_t
/// \return bool
///
/////////////////////////////////////////////////
bool BreakpointManager::isBreakpoint(const std::string& _sFilename, size_t nLine) const
{
    // Try to find the current file in the map
    auto iter = mBreakpoints.find(replacePathSeparator(_sFilename));

    // Try to find the current line
    return iter != mBreakpoints.end() && iter->second.find(nLine) != iter->second.end();
}


/////////////////////////////////////////////////
/// \brief Get the contents of the breakpoint at
/// the desired line in the selected file.
///
/// \param _sFilename const std::string&
/// \param nLine size_t
/// \return Breakpoint
///
/////////////////////////////////////////////////
Breakpoint BreakpointManager::getBreakpoint(const std::string& _sFilename, size_t nLine) const
{
    // Try to find the current file in the map
    auto iter = mBreakpoints.find(replacePathSeparator(_sFilename));

    if (iter != mBreakpoints.end())
    {
        // Try to find the current line
        auto breakPoint = iter->second.find(nLine);

        if (breakPoint != iter->second.end())
            return breakPoint->second;
    }

    NumeReKernel::issueWarning("No breakpoint found in file \"" + _sFilename + "\" @ " + toString(nLine));
    return Breakpoint(false);
}


