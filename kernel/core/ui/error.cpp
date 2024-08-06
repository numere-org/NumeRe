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
#include "../../kernel.hpp"
#include "../maths/matdatastructures.hpp"

size_t SyntaxError::invalid_position = std::string::npos;
int SyntaxError::invalid_index = INT_MIN;
Assertion _assertionHandler;
static std::string sLastErrorMessage;
static ErrorType nLastErrorType;



/////////////////////////////////////////////////
/// \brief This function obtains the error type
/// of a catched exception and sets the last
/// error message.
///
/// \param e_ptr std::exception_ptr
/// \return ErrorType
///
/////////////////////////////////////////////////
ErrorType getErrorType(std::exception_ptr e_ptr)
{
    sLastErrorMessage.clear();

    try
    {
        rethrow_exception(e_ptr);
    }
    catch (mu::Parser::exception_type& e)
    {
        // Parser exception
        sLastErrorMessage = e.GetMsg();
        return (nLastErrorType = TYPE_MATHERROR);
    }
    catch (const std::bad_alloc& e)
    {
        sLastErrorMessage = _lang.get("ERR_STD_BA_HEAD") + "\n" + e.what();
        return (nLastErrorType = TYPE_CRITICALERROR);
    }
    catch (const std::exception& e)
    {
        // C++ Standard exception
        sLastErrorMessage = _lang.get("ERR_STD_INTERNAL_HEAD") + "\n" + e.what();
        return (nLastErrorType = TYPE_INTERNALERROR);
    }
    catch (SyntaxError& e)
    {
        // Internal exception
        if (e.errorcode == SyntaxError::PROCESS_ABORTED_BY_USER)
        {
            // the user pressed ESC
            return (nLastErrorType = TYPE_ABORT);
        }
        else
        {
            if (e.getToken().length() && (e.errorcode == SyntaxError::PROCEDURE_THROW || e.errorcode == SyntaxError::LOOP_THROW))
            {
                sLastErrorMessage = e.getToken();
                return (nLastErrorType = TYPE_CUSTOMERROR);
            }
            else if (e.errorcode == SyntaxError::ASSERTION_ERROR)
            {
                sLastErrorMessage = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(),
                                              toString(e.getIndices()[0]), toString(e.getIndices()[1]),
                                              toString(e.getIndices()[2]), toString(e.getIndices()[3]));
                sLastErrorMessage = sLastErrorMessage;
                return (nLastErrorType = TYPE_ASSERTIONERROR);
            }
            else
            {
                sLastErrorMessage = _lang.get("ERR_NR_" + toString((int)e.errorcode) + "_0_*", e.getToken(),
                                              toString(e.getIndices()[0]), toString(e.getIndices()[1]),
                                              toString(e.getIndices()[2]), toString(e.getIndices()[3]));

                // This error message does not exist
                if (sLastErrorMessage.starts_with("ERR_NR_"))
                    sLastErrorMessage = _lang.get("ERR_GENERIC_0", toString((int)e.errorcode));

                return (nLastErrorType = TYPE_SYNTAXERROR);
            }
        }
    }
    catch (...)
    {
        sLastErrorMessage = _lang.get("ERR_CATCHALL_HEAD");
        return (nLastErrorType = TYPE_GENERICERROR);
    }

    return (nLastErrorType = TYPE_NOERROR);
}


/////////////////////////////////////////////////
/// \brief Return the last error message, which
/// was catched by the getErrorType() function.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getLastErrorMessage()
{
    return sLastErrorMessage;
}


/////////////////////////////////////////////////
/// \brief Return the last error type, which was
/// catched by the getErrorType() function.
///
/// \return ErrorType
///
/////////////////////////////////////////////////
ErrorType getLastErrorType()
{
    return nLastErrorType;
}


/////////////////////////////////////////////////
/// \brief Return the error type converted to a
/// human readable string.
///
/// \param e ErrorType
/// \return std::string
///
/////////////////////////////////////////////////
std::string errorTypeToString(ErrorType e)
{
    switch (e)
    {
        case TYPE_NOERROR:
            return "none";
        case TYPE_ABORT:
            return "abort";
        case TYPE_MATHERROR:
            return "expression";
        case TYPE_SYNTAXERROR:
            return "error";
        case TYPE_ASSERTIONERROR:
            return "assertion";
        case TYPE_CUSTOMERROR:
            return "thrown";
        case TYPE_INTERNALERROR:
            return "internal";
        case TYPE_CRITICALERROR:
            return "critical";
        case TYPE_GENERICERROR:
            return "generic";
    }

    return "none";
}


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
    if (assertionMode)
    {
        assertionMode = false;
        sAssertedExpression.clear();
    }
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
/// \param v mu::Array*
/// \param nNum int
/// \return void
///
/////////////////////////////////////////////////
void Assertion::checkAssertion(mu::Array* v, int nNum)
{
    // Only do something, if the assertion mode is
    // active
    if (assertionMode)
    {
        for (int i = 0; i < nNum; i++)
        {
            // If a single value is zero,
            // throw the assertion error
            if (!v[i])
                assertionFail();
        }

        stats.succeeded();
    }
}


/////////////////////////////////////////////////
/// \brief Checks the return value of the matrix
/// operation.
///
/// \param _mMatrix const Matrix&
/// \return void
///
/////////////////////////////////////////////////
void Assertion::checkAssertion(const Matrix& _mMatrix)
{
    // Only do something, if the assertion mode is
    // active
    if (assertionMode)
    {
        for (const mu::Value& val : _mMatrix.data())
        {
            // If a single value is zero,
            // throw the assertion error
            if (!val)
                assertionFail();
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





