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

#include "error.hpp"
#include "../utils/tools.hpp"
#include "../strings/stringdatastructures.hpp"
#include "../../kernel.hpp"

size_t SyntaxError::invalid_position = string::npos;
int SyntaxError::invalid_index = INT_MIN;
Assertion _assertionHandler;




/////////////////////////////////////////////////
/// \brief This member function is a wrapper
/// around the assertion error.
///
/// \return void
///
/////////////////////////////////////////////////
void Assertion::assertionFail()
{
    stats.failed();
    throw SyntaxError(SyntaxError::ASSERTION_ERROR, sAssertedExpression, findCommand(sAssertedExpression, "assert").nPos+7);
}


/////////////////////////////////////////////////
/// \brief Resets the assertion handler.
///
/// \return void
///
/////////////////////////////////////////////////
void Assertion::reset()
{
    assertionMode = false;
    sAssertedExpression.clear();
}


/////////////////////////////////////////////////
/// \brief Resets the internal statistic
/// variables for accumulating the total number
/// of executed and the number of failed tests.
///
/// \return void
///
/////////////////////////////////////////////////
void Assertion::resetStats()
{
    stats.reset();
}


/////////////////////////////////////////////////
/// \brief Enables the assertion handler using
/// the passed expression.
///
/// \param sExpr const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Assertion::enable(const std::string& sExpr)
{
    sAssertedExpression = sExpr;
    assertionMode = true;
}


/////////////////////////////////////////////////
/// \brief Checks the return value of a muParser
/// evaluated result.
///
/// \param v mu::value_type*
/// \param nNum int
/// \return void
///
/////////////////////////////////////////////////
void Assertion::checkAssertion(mu::value_type* v, int nNum)
{
    // Only do something, if the assertion mode is
    // active
    if (assertionMode)
    {
        for (int i = 0; i < nNum; i++)
        {
            // If a single value is zero,
            // throw the assertion error
            if (v[i] == 0.0)
                assertionFail();
        }

        stats.succeeded();
    }
}


/////////////////////////////////////////////////
/// \brief Checks the return value of the matrix
/// operation.
///
/// \param _mMatrix const std::vector<std::vector<mu::value_type>>&
/// \return void
///
/////////////////////////////////////////////////
void Assertion::checkAssertion(const std::vector<std::vector<mu::value_type>>& _mMatrix)
{
    // Only do something, if the assertion mode is
    // active
    if (assertionMode)
    {
        // Matrices are two-dimensional
        for (const std::vector<mu::value_type>& vRow : _mMatrix)
        {
            for (const mu::value_type& val : vRow)
            {
                // If a single value is zero,
                // throw the assertion error
                if (val == 0.0)
                    assertionFail();
            }
        }

        stats.succeeded();
    }
}


/////////////////////////////////////////////////
/// \brief Checks the return vale of the string
/// parser in the not-numerical-only case.
///
/// \param strRes const StringResult&
/// \return void
///
/////////////////////////////////////////////////
void Assertion::checkAssertion(const StringResult& strRes)
{
    // Only do something, if the assertion mode is
    // active and the strings are not only logicals
    if (assertionMode && !strRes.bOnlyLogicals)
    {
        for (size_t i = 0; i < strRes.vResult.size(); i++)
        {
            if (strRes.vResult[i] == "\"\"")
                assertionFail();
            else if (strRes.vNoStringVal[i])
            {
                NumeReKernel::getInstance()->getParser().SetExpr(strRes.vResult[i]);

                if (NumeReKernel::getInstance()->getParser().Eval() == 0.0)
                    assertionFail();
            }
        }

        stats.succeeded();
    }
}


/////////////////////////////////////////////////
/// \brief Returns the current tests stats.
///
/// \return AssertionStats
///
/////////////////////////////////////////////////
AssertionStats Assertion::getStats() const
{
    return stats;
}





