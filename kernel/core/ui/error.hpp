/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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

#include <string>
#include <vector>
#include "language.hpp"
#include "../ParserLib/muParserDef.h"

#ifndef ERROR_HPP
#define ERROR_HPP

/////////////////////////////////////////////////
/// \brief Common exception class for all
/// exceptions thrown in NumeRe.
/////////////////////////////////////////////////
class SyntaxError
{
    private:
        std::string sFailingExpression;
        std::string sErrorToken;
        size_t nErrorPosition;
        int nErrorIndices[4];

    public:
        /// Errorcodes sind nun alphabetisch sortiert und werden auf eine spezielle Art durchnummeriert. Neue Elemente immer direkt vor den
        /// Elementen mit Konstanten Indices einfuegen!
        enum ErrorCode
        {
            EMPTY_ERROR_MESSAGE = -1,
            CACHE_ALREADY_EXISTS,
            CACHE_CANNOT_BE_RENAMED,
            CACHE_DOESNT_EXIST,
            CLUSTER_DOESNT_EXIST,
            ASSERTION_ERROR,
            /// INSERT HERE
            CANNOT_BE_A_FITTING_PARAM=100,
            CANNOT_CALL_SCRIPT_RECURSIVELY,
            CANNOT_CONTAIN_STRINGS,
            CANNOT_COPY_DATA,
            CANNOT_COPY_FILE,
            CANNOT_DELETE_ELEMENTS,
            CANNOT_EDIT_FILE_TYPE,
            CANNOT_EVAL_FOR,
            CANNOT_EVAL_IF,
            CANNOT_EVAL_WHILE,
            CANNOT_EXPORT_DATA,
            CANNOT_FIND_DEFINE_OPRT,
            CANNOT_FIND_FUNCTION_ARGS,
            CANNOT_GENERATE_DIRECTORY,
            CANNOT_GENERATE_FILE,
            CANNOT_GENERATE_PROCEDURE,
            CANNOT_GENERATE_SCRIPT,
            CANNOT_MOVE_DATA,
            CANNOT_MOVE_FILE,
            CANNOT_OPEN_FITLOG,
            CANNOT_OPEN_LOGFILE,
            CANNOT_OPEN_SOURCE,
            CANNOT_OPEN_TARGET,
            CANNOT_PLOT_STRINGS,
            CANNOT_READ_FILE,
            CANNOT_REGULARIZE_CACHE,
            CANNOT_RELOAD_DATA,
            CANNOT_REMOVE_FILE,
            CANNOT_RESAMPLE_CACHE,
            CANNOT_RETOQUE_CACHE,
            CANNOT_SAVE_CACHE,
            CANNOT_SAVE_FILE,
            CANNOT_SMOOTH_CACHE,
            CANNOT_SORT_CACHE,
            CANNOT_SORT_DATA,
            CANNOT_EVAL_SWITCH,
            CANNOT_PASS_LITERAL_PER_REFERENCE,
            CANNOT_ASSIGN_COLUMN_OF_DIFFERENT_TYPE,
            CANNOT_EVAL_TRY,
            /// INSERT HERE
            COL_COUNTS_DOESNT_MATCH=200,
            CUTOFF_MODE_INVALID,
            /// INSERT HERE
            DATAFILE_NOT_EXIST=300,
            DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING=400,
            DIFF_VAR_NOT_FOUND=500,
            ELLIPSIS_MUST_BE_LAST_ARG=600,
            EVAL_VAR_NOT_FOUND=700,
            EXTERNAL_PROGRAM_NOT_FOUND=800,
            EXTREMA_VAR_NOT_FOUND=900,
            EXECUTE_COMMAND_DISABLED,
            EXECUTE_COMMAND_UNSUCCESSFUL,
            FILETYPE_MAY_NOT_BE_WRITTEN=1000,
            FILE_IS_EMPTY=1100,
            FILE_NOT_EXIST,
            FITFUNC_NOT_CONTAINS=1200,
            FUNCTION_ALREADY_EXISTS=1300,
            FUNCTION_ARGS_MUST_NOT_CONTAIN_SIGN,
            FUNCTION_CANNOT_BE_FITTED,
            FUNCTION_ERROR,
            FUNCTION_IS_PREDEFINED,
            FUNCTION_NAMES_MUST_NOT_CONTAIN_SIGN,
            FUNCTION_STRING_IS_COMMAND,
            /// INSERT HERE
            HLPIDX_ENTRY_IS_MISSING=1400,
            HLP_FILE_MISSING,
            IF_OR_LOOP_SEEMS_NOT_TO_BE_CLOSED=1500,
            INCOMPLETE_VECTOR_SYNTAX=1600,
            INLINE_PROCEDURE_IS_NOT_INLINE=1700,
            INLINE_PROCEDURE_NEEDS_TABLE_REFERENCES,
            INSTALL_CMD_FOUND=1800,
            INSUFFICIENT_NUMERE_VERSION=1900,
            INTERNAL_RESAMPLER_ERROR,
            INVALID_AXIS,
            INVALID_CACHE_NAME=2000,
            INVALID_DATA_ACCESS,
            INVALID_ELEMENT,
            INVALID_FILETYPE,
            INVALID_HLPIDX,
            INVALID_INDEX,
            INVALID_INTEGRATION_PRECISION,
            INVALID_INTEGRATION_RANGES,
            INVALID_INTERVAL,
            INVALID_PROCEDURE_NAME,
            INVALID_SUBPLOT_INDEX,
            INVALID_WAVELET_TYPE,
            INVALID_WAVELET_COEFFICIENT,
            INVALID_CLUSTER_NAME,
            INVALID_SETTING,
            INVALID_REGEX,
            INVALID_WINDOW_ID,
            INVALID_WINDOW_ITEM_ID,
            INVALID_SYM_NAME,
            INVALID_FLOWCTRL_STATEMENT,
            INVALID_STATS_WINDOW_SIZE,
            INVALID_COMMAND,
            INCLUDE_NOT_EXIST,
            INVALID_MODE,
            INVALID_FILTER_SIZE,
            /// INSERT HERE
            LGS_HAS_NO_SOLUTION=2100,
            LGS_HAS_NO_UNIQUE_SOLUTION,
            LOOP_THROW=2200,
            MATRIX_IS_NOT_INVERTIBLE=2300,
            MATRIX_IS_NOT_SYMMETRIC,
            MATRIX_CANNOT_HAVE_ZERO_SIZE,
            MATRIX_CONTAINS_INVALID_VALUES,
            MISSING_DEFAULT_VALUE=2400,
            NO_CACHED_DATA=2500,
            NO_COLS,
            NO_DATA_AVAILABLE,
            NO_DATA_FOR_FIT,
            NO_DIFF_OPTIONS,
            NO_DIFF_VAR,
            NO_EVAL_OPTIONS,
            NO_EVAL_VAR,
            NO_EXPRESSION_FOR_ODE,
            NO_EXTREMA_OPTIONS,
            NO_EXTREMA_VAR,
            NO_FILENAME,
            NO_FUNCTION_FOR_FIT,
            NO_INTEGRATION_FUNCTION,
            NO_INTEGRATION_RANGES,
            NO_INTERVAL_FOR_ODE,
            NO_MATRIX,
            NO_MATRIX_FOR_MATOP,
            NO_NUMBER_AT_POS_1,
            NO_OPTIONS_FOR_ODE,
            NO_PARAMS_FOR_FIT,
            NO_ROWS,
            NO_STRING_FOR_WRITING,
            NO_TARGET,
            NO_ZEROES_OPTIONS,
            NO_ZEROES_VAR,
            NO_DEFAULTVALUE_FOR_DIALOG,
            /// INSERT HERE
            NUMBER_OF_FUNCTIONS_NOT_MATCHING=2600,
            OVERFITTING_ERROR=2700,
            PLOTDATA_IS_NAN=2800,
            PLOT_ERROR,
            PLUGINCMD_ALREADY_EXISTS=2900,
            PLUGINNAME_ALREADY_EXISTS,
            PLUGIN_HAS_NO_CMD,
            PLUGIN_HAS_NO_MAIN,
            PLUGIN_MAY_NOT_OVERRIDE,
            /// INSERT HERE
            PRIVATE_PROCEDURE_CALLED=3000,
            PROCEDURE_ERROR=3100,
            PROCEDURE_NOT_FOUND,
            PROCEDURE_THROW,
            PROCEDURE_WITHOUT_INSTALL_FOUND,
            PROCEDURE_STACK_OVERFLOW,
            PROCESS_ABORTED_BY_USER=3200,
            READ_ONLY_DATA=3300,
            SCRIPT_NOT_EXIST=3400,
            SEPARATOR_NOT_FOUND=3500,
            STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD=3600,
            STRINGVARS_MUSTNT_BEGIN_WITH_A_NUMBER,
            STRINGVARS_MUSTNT_CONTAIN,
            STRING_ERROR,
            /// INSERT HERE
            TABLE_DOESNT_EXIST=3700,
            TOO_FEW_ARGS=3800,
            TOO_FEW_COLS,
            TOO_FEW_DATAPOINTS,
            TOO_FEW_LINES,
            TOO_LARGE_BINWIDTH,
            TOO_LARGE_CACHE,
            TOO_MANY_ARGS,
            TOO_MANY_ARGS_FOR_DEFINE,
            TOO_MANY_FUNCTION_CALLS,
            TOO_MANY_VECTORS,
            /// INSERT HERE
            UNKNOWN_PATH_TOKEN=3900,
            UNMATCHED_PARENTHESIS=4000,
            URL_ERROR,
            WRONG_ARG_NAME=4100,
            WRONG_MATRIX_DIMENSIONS_FOR_MATOP,
            WRONG_PLOT_INTERVAL_FOR_LOGSCALE,
            WRONG_DATA_SIZE,
            WRONG_COLUMN_TYPE,
            /// INSERT HERE
            ZEROES_VAR_NOT_FOUND=4200,
        };

        ErrorCode errorcode;
        static size_t invalid_position;
        static int invalid_index;

        /////////////////////////////////////////////////
        /// \brief Default constructor to create an
        /// invalid exception.
        /////////////////////////////////////////////////
        SyntaxError() : nErrorPosition(invalid_position), errorcode(EMPTY_ERROR_MESSAGE)
			{
                nErrorIndices[0] = -1;
                nErrorIndices[1] = -1;
                nErrorIndices[2] = -1;
                nErrorIndices[3] = -1;
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and position.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param n_pos size_t
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, size_t n_pos) : SyntaxError()
            {
                sFailingExpression = sExpr;
                nErrorPosition = n_pos;
                errorcode = _err;
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and position and provides the
        /// possibility to define an additional token.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param n_pos size_t
        /// \param sToken const std::string&
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, size_t n_pos, const std::string& sToken) : SyntaxError()
            {
                sFailingExpression = sExpr;
                sErrorToken = sToken;
                nErrorPosition = n_pos;
                errorcode = _err;
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and position and provides the
        /// possibility to set additional error indices.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param n_pos size_t
        /// \param nInd1 int
        /// \param nInd2 int
        /// \param nInd3 int
        /// \param nInd4 int
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, size_t n_pos, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), nErrorPosition(n_pos), errorcode(_err)
            {
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and position and provides the
        /// possibility to set an additional token and
        /// the error indices.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param n_pos size_t
        /// \param sToken const std::string&
        /// \param nInd1 int
        /// \param nInd2 int
        /// \param nInd3 int
        /// \param nInd4 int
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, size_t n_pos, const std::string& sToken, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), sErrorToken(sToken), nErrorPosition(n_pos), errorcode(_err)
            {
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and an error token, which is used
        /// to locate the position of the error in the
        /// expression.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param sErrTok const std::string&
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, const std::string& sErrTok) : SyntaxError()
            {
                sFailingExpression = sExpr;
                errorcode = _err;
                nErrorPosition = sFailingExpression.find(sErrTok);
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and an error token, which is used
        /// to locate the position of the error in the
        /// expression. Additionally, one can add a token
        /// with further information.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param sErrTok const std::string&
        /// \param sToken const std::string&
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, const std::string& sErrTok, const std::string& sToken) : SyntaxError()
            {
                sFailingExpression = sExpr;
                sErrorToken = sToken;
                errorcode = _err;
                nErrorPosition = sFailingExpression.find(sErrTok);
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and an error token, which is used
        /// to locate the position of the error in the
        /// expression. Additionally, one can add a set
        /// of error indices.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param sErrTok const std::string&
        /// \param nInd1 int
        /// \param nInd2 int
        /// \param nInd3 int
        /// \param nInd4 int
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, const std::string& sErrTok, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), errorcode(_err)
            {
                nErrorPosition = sFailingExpression.find(sErrTok);
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }

        /////////////////////////////////////////////////
        /// \brief Creates an exception based from an
        /// expression and an error token, which is used
        /// to locate the position of the error in the
        /// expression. Additionally, one can add a token
        /// and a set of error indices bearing more
        /// information about the issue.
        ///
        /// \param _err ErrorCode
        /// \param sExpr const std::string&
        /// \param sErrTok const std::string&
        /// \param sToken const std::string&
        /// \param nInd1 int
        /// \param nInd2 int
        /// \param nInd3 int
        /// \param nInd4 int
        ///
        /////////////////////////////////////////////////
        SyntaxError(ErrorCode _err, const std::string& sExpr, const std::string& sErrTok, const std::string& sToken, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), sErrorToken(sToken), errorcode(_err)
            {
                nErrorPosition = sFailingExpression.find(sErrTok);
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }

        /////////////////////////////////////////////////
        /// \brief Returns the erroneous expression.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string getExpr() const
            {return sFailingExpression;}

        /////////////////////////////////////////////////
        /// \brief Returns the error token containing
        /// additional information about the error.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string getToken() const
            {return sErrorToken;}

        /////////////////////////////////////////////////
        /// \brief Returns the position of the error in
        /// the erroneous expression.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t getPosition() const
            {return nErrorPosition;}

        /////////////////////////////////////////////////
        /// \brief Returns a pointer to the internal
        /// array of 4 error indices.
        ///
        /// \return const int*
        ///
        /////////////////////////////////////////////////
        const int* getIndices() const
            {return nErrorIndices;}

};


/////////////////////////////////////////////////
/// \brief Defines the possible error types,
/// which can be thrown in this application.
/////////////////////////////////////////////////
enum ErrorType
{
    TYPE_NOERROR,
    TYPE_ABORT,
    TYPE_MATHERROR,
    TYPE_SYNTAXERROR,
    TYPE_ASSERTIONERROR,
    TYPE_CUSTOMERROR,
    TYPE_INTERNALERROR,
    TYPE_CRITICALERROR,
    TYPE_GENERICERROR
};


ErrorType getErrorType(std::exception_ptr e_ptr);
std::string getLastErrorMessage();
ErrorType getLastErrorType();
std::string errorTypeToString(ErrorType e);


// Forward declaration for the Assertion class
struct StringResult;

/////////////////////////////////////////////////
/// \brief This structure accumulates the
/// statistics for the assertion handler.
/////////////////////////////////////////////////
struct AssertionStats
{
    size_t nCheckedAssertions;
    size_t nFailedAssertions;

    AssertionStats() : nCheckedAssertions(0), nFailedAssertions(0) {}

    /////////////////////////////////////////////////
    /// \brief Reset the statistics.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void reset()
    {
        nCheckedAssertions = 0;
        nFailedAssertions = 0;
    }

    /////////////////////////////////////////////////
    /// \brief Assertion was successful.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void succeeded()
    {
        nCheckedAssertions++;
    }

    /////////////////////////////////////////////////
    /// \brief Assertion failed.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void failed()
    {
        nCheckedAssertions++;
        nFailedAssertions++;
    }
};

class Matrix;

/////////////////////////////////////////////////
/// \brief This class handles assertions and
/// throws the corresponding exception, if the
/// assertion fails. It is currently used as
/// global singleton, but is not restricted to
/// this pattern.
/////////////////////////////////////////////////
class Assertion
{
    private:
        std::string sAssertedExpression;
        bool assertionMode;
        void assertionFail();
        AssertionStats stats;

    public:
        Assertion() : sAssertedExpression(), assertionMode(false) {}
        void reset();
        void resetStats();
        void enable(const std::string& sExpr);
        void checkAssertion(mu::value_type* v, int nNum);
        void checkAssertion(const Matrix& _mMatrix);
        void checkAssertion(const StringResult& strRes);
        AssertionStats getStats() const;
};


extern Language _lang;
extern Assertion _assertionHandler;

#endif // ERROR_HPP

