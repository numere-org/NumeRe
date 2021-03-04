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
#include "language.hpp"

#ifndef ERROR_HPP
#define ERROR_HPP

using namespace std;

class SyntaxError
{
    private:
        string sFailingExpression;
        string sErrorToken;
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
            /// INSERT HERE
            COL_COUNTS_DOESNT_MATCH=200,
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
            INSTALL_CMD_FOUND=1800,
            INSUFFICIENT_NUMERE_VERSION=1900,
            INTERNAL_RESAMPLER_ERROR,
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
            /// INSERT HERE
            LGS_HAS_NO_SOLUTION=2100,
            LGS_HAS_NO_UNIQUE_SOLUTION,
            LOOP_THROW=2200,
            MATRIX_IS_NOT_INVERTIBLE=2300,
            MATRIX_IS_NOT_SYMMETRIC,
            MATRIX_CANNOT_HAVE_ZERO_SIZE,
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
            WRONG_ARG_NAME=4100,
            WRONG_MATRIX_DIMENSIONS_FOR_MATOP,
            WRONG_PLOT_INTERVAL_FOR_LOGSCALE,
            WRONG_DATA_SIZE,
            /// INSERT HERE
            ZEROES_VAR_NOT_FOUND=4200,
        };
        ErrorCode errorcode;
        static size_t invalid_position;
        static int invalid_index;

        SyntaxError() : nErrorPosition(invalid_position), errorcode(EMPTY_ERROR_MESSAGE)
			{
                nErrorIndices[0] = -1;
                nErrorIndices[1] = -1;
                nErrorIndices[2] = -1;
                nErrorIndices[3] = -1;
            }
        SyntaxError(ErrorCode _err, const string& sExpr, size_t n_pos) : SyntaxError()
            {
                sFailingExpression = sExpr;
                nErrorPosition = n_pos;
                errorcode = _err;
            }
        SyntaxError(ErrorCode _err, const string& sExpr, size_t n_pos, const string& sToken) : SyntaxError()
            {
                sFailingExpression = sExpr;
                sErrorToken = sToken;
                nErrorPosition = n_pos;
                errorcode = _err;
            }
        SyntaxError(ErrorCode _err, const string& sExpr, size_t n_pos, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), nErrorPosition(n_pos), errorcode(_err)
            {
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }
        SyntaxError(ErrorCode _err, const string& sExpr, size_t n_pos, const string& sToken, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), sErrorToken(sToken), nErrorPosition(n_pos), errorcode(_err)
            {
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }

        SyntaxError(ErrorCode _err, const string& sExpr, const string& sErrTok) : SyntaxError()
            {
                sFailingExpression = sExpr;
                errorcode = _err;
                nErrorPosition = sFailingExpression.find(sErrTok);
            }
        SyntaxError(ErrorCode _err, const string& sExpr, const string& sErrTok, const string& sToken) : SyntaxError()
            {
                sFailingExpression = sExpr;
                sErrorToken = sToken;
                errorcode = _err;
                nErrorPosition = sFailingExpression.find(sErrTok);
            }
        SyntaxError(ErrorCode _err, const string& sExpr, const string& sErrTok, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), errorcode(_err)
            {
                nErrorPosition = sFailingExpression.find(sErrTok);
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }
        SyntaxError(ErrorCode _err, const string& sExpr, const string& sErrTok, const string& sToken, int nInd1, int nInd2 = invalid_index, int nInd3 = invalid_index, int nInd4 = invalid_index) : sFailingExpression(sExpr), sErrorToken(sToken), errorcode(_err)
            {
                nErrorPosition = sFailingExpression.find(sErrTok);
                nErrorIndices[0] = nInd1;
                nErrorIndices[1] = nInd2;
                nErrorIndices[2] = nInd3;
                nErrorIndices[3] = nInd4;
            }

        string getExpr() const
            {return sFailingExpression;}
        string getToken() const
            {return sErrorToken;}
        size_t getPosition() const
            {return nErrorPosition;}
        const int* getIndices() const
            {return nErrorIndices;}

};


#endif // ERROR_HPP

extern Language _lang;
