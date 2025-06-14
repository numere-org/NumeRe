/*
                 __________
    _____   __ __\______   \_____  _______  ______  ____ _______
   /     \ |  |  \|     ___/\__  \ \_  __ \/  ___/_/ __ \\_  __ \
  |  Y Y  \|  |  /|    |     / __ \_|  | \/\___ \ \  ___/ |  | \/
  |__|_|  /|____/ |____|    (____  /|__|  /____  > \___  >|__|
        \/                       \/            \/      \/
  Copyright (C) 2004-2011 Ingo Berg

  Permission is hereby granted, free of charge, to any person obtaining a copy of this
  software and associated documentation files (the "Software"), to deal in the Software
  without restriction, including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MU_PARSER_ERROR_H
#define MU_PARSER_ERROR_H

#include <cassert>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <memory>

#include "muStringTypeDefs.hpp"


/** \file
    \brief This file defines the error class used by the parser.
*/

namespace mu
{
    /** \brief Error codes. */
    enum EErrorCodes
    {
        // Formula syntax errors
        ecUNEXPECTED_OPERATOR    = 0,  ///< Unexpected binary operator found
        ecUNASSIGNABLE_TOKEN     = 1,  ///< Token cant be identified.
        ecUNEXPECTED_EOF         = 2,  ///< Unexpected end of formula. (Example: "2+sin(")
        ecUNEXPECTED_ARG_SEP     = 3,  ///< An unexpected comma has been found. (Example: "1,23")
        ecUNEXPECTED_ARG         = 4,  ///< An unexpected argument has been found
        ecUNEXPECTED_VAL         = 5,  ///< An unexpected value token has been found
        ecUNEXPECTED_VAR         = 6,  ///< An unexpected variable token has been found
        ecUNEXPECTED_PARENS      = 7,  ///< Unexpected Parenthesis, opening or closing
        ecUNEXPECTED_VPARENS     = 8,  ///< Unexpected Vector Parenthesis, opening or closing
        ecUNEXPECTED_STR         = 9,  ///< A string has been found at an inapropriate position
        ecSTRING_EXPECTED        = 10,  ///< A string function has been called with a different type of argument
        ecVAL_EXPECTED           = 11, ///< A numerical function has been called with a non value type of argument
        ecMISSING_PARENS         = 12, ///< Missing parens. (Example: "3*sin(3")
        ecUNEXPECTED_FUN         = 13, ///< Unexpected function found. (Example: "sin(8)cos(9)")
        ecUNEXPECTED_METHOD      = 14, ///< Unexpected method found. (Example: "+.len")
        ecUNTERMINATED_STRING    = 15, ///< unterminated string constant. (Example: "3*valueof("hello)")
        ecTOO_MANY_PARAMS        = 16, ///< Too many function parameters
        ecTOO_FEW_PARAMS         = 17, ///< Too few function parameters. (Example: "ite(1<2,2)")
        ecOPRT_TYPE_CONFLICT     = 18, ///< binary operators may only be applied to value items of the same type
        ecSTR_RESULT             = 19, ///< result is a string

        // Invalid Parser input Parameters
        ecINVALID_NAME           = 20, ///< Invalid function, variable or constant name.
        ecINVALID_BINOP_IDENT    = 21, ///< Invalid binary operator identifier
        ecINVALID_INFIX_IDENT    = 22, ///< Invalid function, variable or constant name.
        ecINVALID_POSTFIX_IDENT  = 23, ///< Invalid function, variable or constant name.

        ecBUILTIN_OVERLOAD       = 24, ///< Trying to overload builtin operator
        ecINVALID_FUN_PTR        = 25, ///< Invalid callback function pointer
        ecINVALID_VAR_PTR        = 26, ///< Invalid variable pointer
        ecEMPTY_EXPRESSION       = 27, ///< The Expression is empty
        ecNAME_CONFLICT          = 28, ///< Name conflict
        ecOPT_PRI                = 29, ///< Invalid operator priority
        //
        ecDOMAIN_ERROR           = 30, ///< catch division by zero, sqrt(-1), log(0) (currently unused)
        ecGENERIC                = 31, ///< Generic error
        ecLOCALE                 = 32, ///< Conflict with current locale

        ecUNEXPECTED_CONDITIONAL = 33,
        ecMISSING_ELSE_CLAUSE    = 34,
        ecMISPLACED_COLON        = 35,

        ecTYPE_NO_STR            = 36, ///< Type is not a string (Value does not contain a string type)
        ecTYPE_NO_VAL            = 37, ///< Type is not a value (Value does not contain a numerical type)
        ecTYPE_NO_CAT            = 38, ///< Type is not a value (Value does not contain a numerical type)
        ecTYPE_NO_ARR            = 39, ///< Type is not a value (Value does not contain a numerical type)
        ecTYPE_MISMATCH          = 40, ///< Something's wrong with the types (Value types do not match or operation not supported)
        ecTYPE_MISMATCH_OOB      = 41, ///< Something's wrong with the types (Value types do not match or index out of bounds)
        ecASSIGNED_TYPE_MISMATCH = 42, ///< Types in assignment do not match (Cannot assign a different type to an already initialized variable)
        ecMETHOD_ERROR           = 43, ///< Such a method does not exist (No such method or too few arguments)
        ecNOT_IMPLEMENTED        = 44, ///< Operation is not implemented for this data type.

        // internal errors
        ecINTERNAL_ERROR         = 45, ///< Internal error of any kind.

        // The last two are special entries
        ecCOUNT,                      ///< This is no error code, It just stores just the total number of error codes
        ecUNDEFINED              = -1  ///< Undefined message, placeholder to detect unassigned error messages
    };

//---------------------------------------------------------------------------
    /** \brief A class that handles the error messages.
    */
    class ParserErrorMsg
    {
        public:
            typedef ParserErrorMsg self_type;

            ParserErrorMsg& operator=(const ParserErrorMsg&);
            ParserErrorMsg(const ParserErrorMsg&);
            ParserErrorMsg();

            ~ParserErrorMsg();

            static const ParserErrorMsg& Instance();
            string_type operator[](unsigned a_iIdx) const;

        private:
            std::vector<string_type>  m_vErrMsg;  ///< A vector with the predefined error messages
            static const self_type m_Instance;    ///< The instance pointer
    };

//---------------------------------------------------------------------------
    /** \brief Error class of the parser.
        \author Ingo Berg

      Part of the math parser package.
    */
    class ParserError
    {
        private:

            /** \brief Replace all ocuurences of a substring with another string. */
            void ReplaceSubString( string_type& strSource,
                                   const string_type& strFind,
                                   const string_type& strReplaceWith);
            void Reset();

        public:

            ParserError();
            explicit ParserError(EErrorCodes a_iErrc);
            explicit ParserError(const string_type& sMsg);
            ParserError( EErrorCodes a_iErrc,
                         const string_type& sTok,
                         const string_type& sFormula = string_type(),
                         int a_iPos = -1);
            ParserError( EErrorCodes a_iErrc,
                         int a_iPos,
                         const string_type& sTok);
            ParserError( const char_type* a_szMsg,
                         int a_iPos = -1,
                         const string_type& sTok = string_type());
            ParserError(const ParserError& a_Obj);
            ParserError& operator=(const ParserError& a_Obj);
            ~ParserError();

            void SetFormula(const string_type& a_strFormula);
            const string_type& GetExpr() const;
            const string_type& GetMsg() const;
            std::size_t GetPos() const;
            const string_type& GetToken() const;
            EErrorCodes GetCode() const;

        private:
            string_type m_strMsg;     ///< The message string
            string_type m_strFormula; ///< Formula string
            string_type m_strTok;     ///< Token related with the error
            int m_iPos;               ///< Formula position related to the error
            EErrorCodes m_iErrc;      ///< Error code
            const ParserErrorMsg& m_ErrMsg;
    };

} // namespace mu

#endif

