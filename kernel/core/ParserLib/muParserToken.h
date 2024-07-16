/*
                 __________
    _____   __ __\______   \_____  _______  ______  ____ _______
   /     \ |  |  \|     ___/\__  \ \_  __ \/  ___/_/ __ \\_  __ \
  |  Y Y  \|  |  /|    |     / __ \_|  | \/\___ \ \  ___/ |  | \/
  |__|_|  /|____/ |____|    (____  /|__|  /____  > \___  >|__|
        \/                       \/            \/      \/
  Copyright (C) 2004-2012 Ingo Berg

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

#ifndef MU_PARSER_TOKEN_H
#define MU_PARSER_TOKEN_H

#include <cassert>
#include <string>
#include <stack>
#include <vector>
#include <memory>

#include "muParserError.h"
#include "muParserCallback.h"

/** \file
    \brief This file contains the parser token definition.
*/

namespace mu
{
    /** \brief Encapsulation of the data for a single formula token.

      Formula token implementation. Part of the Math Parser Package.
      Formula tokens can be either one of the following:
      <ul>
        <li>value</li>
        <li>variable</li>
        <li>function with numerical arguments</li>
        <li>functions with a string as argument</li>
        <li>prefix operators</li>
        <li>infix operators</li>
        <li>binary operator</li>
      </ul>

     \author (C) 2004-2012 Ingo Berg
    */
    class ParserToken
    {
        private:

            ECmdCode  m_iCode;      ///< Type of the token; The token type is a constant of type #ECmdCode.
            ETypeCode m_iType;
            Variable*  m_var;       ///< Stores Token pointer; not applicable for all tokens
            int  m_iIdx;            ///< An otional index to an external buffer storing the token data
            std::string m_strTok;   ///< Token string
            std::string m_strVal;   ///< Value for string variables
            Array m_fVal;           ///< the value
            VarArray m_varArray;
            std::unique_ptr<ParserCallback> m_pCallback;
            size_t m_val2StrLen;    ///< Length of val2str operator

        public:

            //---------------------------------------------------------------------------
            /** \brief Constructor (default).

                Sets token to an neutral state of type cmUNKNOWN.
                \throw nothrow
                \sa ECmdCode
            */
            ParserToken()
                : m_iCode(cmUNKNOWN)
                , m_iType(tpVOID)
                , m_var(nullptr)
                , m_iIdx(-1)
                , m_strTok()
                , m_pCallback()
            {}

            //------------------------------------------------------------------------------
            /** \brief Create token from another one.

                Implemented by calling Assign(...)
                \throw nothrow
                \post m_iType==cmUNKNOWN
                \sa #Assign
            */
            ParserToken(const ParserToken& a_Tok)
            {
                Assign(a_Tok);
            }

            //------------------------------------------------------------------------------
            /** \brief Assignement operator.

                Copy token state from another token and return this.
                Implemented by calling Assign(...).
                \throw nothrow
            */
            ParserToken& operator=(const ParserToken& a_Tok)
            {
                Assign(a_Tok);
                return *this;
            }

            //------------------------------------------------------------------------------
            /** \brief Copy token information from argument.

                \throw nothrow
            */
            void Assign(const ParserToken& a_Tok)
            {
                m_iCode = a_Tok.m_iCode;
                m_var = a_Tok.m_var;
                m_strTok = a_Tok.m_strTok;
                m_iIdx = a_Tok.m_iIdx;
                m_strVal = a_Tok.m_strVal;
                m_iType = a_Tok.m_iType;
                m_fVal = a_Tok.m_fVal;
                // create new callback object if a_Tok has one
                m_pCallback.reset(a_Tok.m_pCallback.get() ? a_Tok.m_pCallback->Clone() : 0);
                m_val2StrLen = a_Tok.m_val2StrLen;
                m_varArray = a_Tok.m_varArray;
            }

            //------------------------------------------------------------------------------
            /** \brief Assign a token type.

              Token may not be of type value, variable or function. Those have seperate set functions.

              \pre [assert] a_iType!=cmVAR
              \pre [assert] a_iType!=cmVAL
              \pre [assert] a_iType!=cmFUNC
              \post m_fVal = 0
              \post m_vars = 0
            */
            ParserToken& Set(ECmdCode a_iType, const std::string& a_strTok = std::string())
            {
                // The following types cant be set this way, they have special Set functions
                assert(a_iType != cmVAR);
                assert(a_iType != cmVAL);
                assert(a_iType != cmFUNC);

                m_iCode = a_iType;
                m_iType = tpVOID;
                m_var = nullptr;
                m_strTok = a_strTok;
                m_iIdx = -1;
                m_val2StrLen = 0;

                return *this;
            }

            ParserToken& SetVal2Str(size_t len)
            {
                m_iCode = cmVAL2STR;
                m_iType = tpVOID;
                m_var = nullptr;
                m_strTok = "#";
                m_iIdx = -1;
                m_val2StrLen = len;

                return *this;
            }

            //------------------------------------------------------------------------------
            /** \brief Set Callback type. */
            ParserToken& Set(const ParserCallback& a_pCallback, const std::string& a_sTok)
            {
                assert(a_pCallback.GetAddr());

                m_iCode = a_pCallback.GetCode();
                m_iType = tpVOID;
                m_strTok = a_sTok;
                m_pCallback.reset(new ParserCallback(a_pCallback));

                m_var = nullptr;
                m_iIdx = -1;
                m_val2StrLen = 0;

                return *this;
            }

            //------------------------------------------------------------------------------
            /** \brief Make this token a value token.

                Member variables not necessary for value tokens will be invalidated.
                \throw nothrow
            */
            ParserToken& SetVal(const Array& a_fVal, const std::string& a_strTok = std::string())
            {
                m_iCode = cmVAL;
                m_iType = tpDBL;
                m_fVal = a_fVal;
                m_strTok = a_strTok;
                m_iIdx = -1;

                m_var = nullptr;
                m_pCallback.reset(0);

                return *this;
            }

            //------------------------------------------------------------------------------
            /** \brief make this token a variable token.

                Member variables not necessary for variable tokens will be invalidated.
                \throw nothrow
            */
            ParserToken& SetVar(Variable* a_pVar, const std::string& a_strTok)
            {
                m_iCode = cmVAR;
                m_iType = tpDBL;
                m_strTok = a_strTok;
                m_iIdx = -1;
                m_var = a_pVar;
                m_pCallback.reset(0);
                return *this;
            }

            ParserToken& SetVarArray(const VarArray& a_varArray, const std::string& a_strTok)
            {
                m_iCode = cmVARARRAY;
                m_iType = tpDBL;
                m_strTok = a_strTok;
                m_iIdx = -1;
                m_var = nullptr;
                m_pCallback.reset(0);
                m_varArray = a_varArray;
                return *this;
            }

            //------------------------------------------------------------------------------
            /** \brief Set an index associated with the token related data.

                In cmSTRFUNC - This is the index to a string table in the main parser.
                \param a_iIdx The index the string function result will take in the bytecode parser.
                \throw exception_type if #a_iIdx<0 or #m_iType!=cmSTRING
            */
            void SetIdx(int a_iIdx)
            {
                if (m_iCode != cmSTRING || a_iIdx < 0)
                    throw ParserError(ecINTERNAL_ERROR);

                m_iIdx = a_iIdx;
            }

            //------------------------------------------------------------------------------
            /** \brief Return Index associated with the token related data.

                In cmSTRFUNC - This is the index to a string table in the main parser.

                \throw exception_type if #m_iIdx<0 or #m_iType!=cmSTRING
                \return The index the result will take in the Bytecode calculatin array (#m_iIdx).
            */
            int GetIdx() const
            {
                if (m_iIdx < 0 || m_iCode != cmSTRING )
                    throw ParserError(ecINTERNAL_ERROR);

                return m_iIdx;
            }

            //------------------------------------------------------------------------------
            /** \brief Return the token type.

                \return #m_iType
                \throw nothrow
            */
            ECmdCode GetCode() const
            {
                if (m_pCallback.get())
                {
                    return m_pCallback->GetCode();
                }
                else
                {
                    return m_iCode;
                }
            }

            //------------------------------------------------------------------------------
            ETypeCode GetType() const
            {
                if (m_pCallback.get())
                {
                    return m_pCallback->GetType();
                }
                else
                {
                    return m_iType;
                }
            }

            //------------------------------------------------------------------------------
            int GetPri() const
            {
                if ( !m_pCallback.get())
                    throw ParserError(ecINTERNAL_ERROR);

                if ( m_pCallback->GetCode() != cmOPRT_BIN && m_pCallback->GetCode() != cmOPRT_INFIX)
                    throw ParserError(ecINTERNAL_ERROR);

                return m_pCallback->GetPri();
            }

            //------------------------------------------------------------------------------
            EOprtAssociativity GetAssociativity() const
            {
                if (m_pCallback.get() == nullptr || m_pCallback->GetCode() != cmOPRT_BIN)
                    throw ParserError(ecINTERNAL_ERROR);

                return m_pCallback->GetAssociativity();
            }


            bool IsOptimizable() const
            {
                if (!m_pCallback)
                    throw ParserError(ecINTERNAL_ERROR);

                return m_pCallback->IsOptimizable();
            }
            //------------------------------------------------------------------------------
            /** \brief Return the address of the callback function assoziated with
                       function and operator tokens.

                \return The pointer stored in #m_vars.
                \throw exception_type if token type is non of:
                       <ul>
                         <li>cmFUNC</li>
                         <li>cmSTRFUNC</li>
                         <li>cmPOSTOP</li>
                         <li>cmINFIXOP</li>
                         <li>cmOPRT_BIN</li>
                       </ul>
                \sa ECmdCode
            */
            generic_fun_type GetFuncAddr() const
            {
                return (m_pCallback.get()) ? (generic_fun_type)m_pCallback->GetAddr() : 0;
            }

            //------------------------------------------------------------------------------
            /** \biref Get value of the token.

                Only applicable to variable and value tokens.
                \throw exception_type if token is no value/variable token.
            */
            Array GetVal() const
            {
                switch (m_iCode)
                {
                    case cmVAL:
                        return m_fVal;
                    case cmVAR:
                        return *m_var;
                    default:
                        throw ParserError(ecVAL_EXPECTED);
                }
            }

            //------------------------------------------------------------------------------
            /** \brief Get address of a variable token.

              Valid only if m_iType==CmdVar.
              \throw exception_type if token is no variable token.
            */
            Variable* GetVar() const
            {
                if (m_iCode != cmVAR)
                    throw ParserError(ecINTERNAL_ERROR);

                return m_var;
            }

            VarArray GetVarArray() const
            {
                if (m_iCode != cmVARARRAY)
                    throw ParserError(ecINTERNAL_ERROR);

                return m_varArray;
            }

            //------------------------------------------------------------------------------
            /** \brief Return the number of function arguments.

              Valid only if m_iType==CmdFUNC.
            */
            int GetArgCount() const
            {
                assert(m_pCallback.get());

                if (!m_pCallback->GetAddr())
                    throw ParserError(ecINTERNAL_ERROR);

                return m_pCallback->GetArgc();
            }

            //------------------------------------------------------------------------------
            /** \brief Return the token identifier.

                If #m_iType is cmSTRING the token identifier is the value of the string argument
                for a string function.
                \return #m_strTok
                \throw nothrow
                \sa m_strTok
            */
            const std::string& GetAsString() const
            {
                return m_strTok;
            }

            size_t GetLen() const
            {
                return m_val2StrLen;
            }
    };
} // namespace mu

#endif
