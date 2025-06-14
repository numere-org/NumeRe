/*
                 __________
    _____   __ __\______   \_____  _______  ______  ____ _______
   /     \ |  |  \|     ___/\__  \ \_  __ \/  ___/_/ __ \\_  __ \
  |  Y Y  \|  |  /|    |     / __ \_|  | \/\___ \ \  ___/ |  | \/
  |__|_|  /|____/ |____|    (____  /|__|  /____  > \___  >|__|
        \/                       \/            \/      \/
  Copyright (C) 2011 Ingo Berg

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
#include "muParserError.h"
#include "../ui/language.hpp"
extern Language _lang;

namespace mu
{
    const ParserErrorMsg ParserErrorMsg::m_Instance;

    //------------------------------------------------------------------------------
    const ParserErrorMsg& ParserErrorMsg::Instance()
    {
        return m_Instance;
    }

    //------------------------------------------------------------------------------
    string_type ParserErrorMsg::operator[](unsigned a_iIdx) const
    {
        return (a_iIdx < m_vErrMsg.size()) ? m_vErrMsg[a_iIdx] : string_type();
    }

    //---------------------------------------------------------------------------
    ParserErrorMsg::~ParserErrorMsg()
    {}

    //---------------------------------------------------------------------------
    /** \brief Assignement operator is deactivated.
    */
    ParserErrorMsg& ParserErrorMsg::operator=(const ParserErrorMsg& )
    {
        assert(false);
        return *this;
    }

    //---------------------------------------------------------------------------
    ParserErrorMsg::ParserErrorMsg(const ParserErrorMsg&)
    {}

    //---------------------------------------------------------------------------
    ParserErrorMsg::ParserErrorMsg()
        : m_vErrMsg(0)
    {
        m_vErrMsg.resize(ecCOUNT);

        m_vErrMsg[ecUNEXPECTED_OPERATOR]    = 	"ERR_MUP_0_UNEXPECTED_OPERATOR";
        m_vErrMsg[ecUNASSIGNABLE_TOKEN]     = 	"ERR_MUP_1_UNASSIGNABLE_TOKEN";
        m_vErrMsg[ecUNEXPECTED_EOF]         = 	"ERR_MUP_2_UNEXPECTED_EOF";
        m_vErrMsg[ecUNEXPECTED_ARG_SEP]     = 	"ERR_MUP_3_UNEXPECTED_ARG_SEP";
        m_vErrMsg[ecUNEXPECTED_ARG]         = 	"ERR_MUP_4_UNEXPECTED_ARG";
        m_vErrMsg[ecUNEXPECTED_VAL]         = 	"ERR_MUP_5_UNEXPECTED_VAL";
        m_vErrMsg[ecUNEXPECTED_VAR]         = 	"ERR_MUP_6_UNEXPECTED_VAR";
        m_vErrMsg[ecUNEXPECTED_PARENS]      = 	"ERR_MUP_7_UNEXPECTED_PARENS";
        m_vErrMsg[ecUNEXPECTED_VPARENS]     = 	"ERR_MUP_8_UNEXPECTED_VPARENS";
        m_vErrMsg[ecUNEXPECTED_STR]         = 	"ERR_MUP_9_UNEXPECTED_STR";
        m_vErrMsg[ecSTRING_EXPECTED]        = 	"ERR_MUP_10_STRING_EXPECTED";
        m_vErrMsg[ecVAL_EXPECTED]           = 	"ERR_MUP_11_VAL_EXPECTED";
        m_vErrMsg[ecMISSING_PARENS]         = 	"ERR_MUP_12_MISSING_PARENS";
        m_vErrMsg[ecUNEXPECTED_FUN]         = 	"ERR_MUP_13_UNEXPECTED_FUN";
        m_vErrMsg[ecUNEXPECTED_METHOD]      = 	"ERR_MUP_14_UNEXPECTED_METHOD";
        m_vErrMsg[ecUNTERMINATED_STRING]    = 	"ERR_MUP_15_UNTERMINATED_STRING";
        m_vErrMsg[ecTOO_MANY_PARAMS]        = 	"ERR_MUP_16_TOO_MANY_PARAMS";
        m_vErrMsg[ecTOO_FEW_PARAMS]         = 	"ERR_MUP_17_TOO_FEW_PARAMS";
        m_vErrMsg[ecOPRT_TYPE_CONFLICT]     = 	"ERR_MUP_18_OPRT_TYPE_CONFLICT";
        m_vErrMsg[ecSTR_RESULT]             = 	"ERR_MUP_19_STR_RESULT";

        m_vErrMsg[ecINVALID_NAME]           = 	"ERR_MUP_20_INVALID_NAME";
        m_vErrMsg[ecINVALID_BINOP_IDENT]    = 	"ERR_MUP_21_INVALID_BINOP_IDENT";
        m_vErrMsg[ecINVALID_INFIX_IDENT]    = 	"ERR_MUP_22_INVALID_INFIX_IDENT";
        m_vErrMsg[ecINVALID_POSTFIX_IDENT]  = 	"ERR_MUP_23_INVALID_POSTFIX_IDENT";

        m_vErrMsg[ecBUILTIN_OVERLOAD]       = 	"ERR_MUP_24_BUILTIN_OVERLOAD";
        m_vErrMsg[ecINVALID_FUN_PTR]        = 	"ERR_MUP_25_INVALID_FUN_PTR";
        m_vErrMsg[ecINVALID_VAR_PTR]        = 	"ERR_MUP_26_INVALID_VAR_PTR";
        m_vErrMsg[ecEMPTY_EXPRESSION]       = 	"ERR_MUP_27_EMPTY_EXPRESSION";
        m_vErrMsg[ecNAME_CONFLICT]          = 	"ERR_MUP_28_NAME_CONFLICT";
        m_vErrMsg[ecOPT_PRI]                = 	"ERR_MUP_29_OPT_PRI";

        m_vErrMsg[ecDOMAIN_ERROR]           = 	"ERR_MUP_30_DOMAIN_ERROR";
        m_vErrMsg[ecGENERIC]                = 	"ERR_MUP_31_GENERIC";
        m_vErrMsg[ecLOCALE]                 = 	"ERR_MUP_32_LOCALE";

        m_vErrMsg[ecUNEXPECTED_CONDITIONAL] = 	"ERR_MUP_33_UNEXPECTED_CONDITIONAL";
        m_vErrMsg[ecMISSING_ELSE_CLAUSE]    = 	"ERR_MUP_34_MISSING_ELSE_CLAUSE";
        m_vErrMsg[ecMISPLACED_COLON]        = 	"ERR_MUP_35_MISPLACED_COLON";

        m_vErrMsg[ecTYPE_NO_STR]            =   "ERR_MUP_36_TYPE_NO_STR";
        m_vErrMsg[ecTYPE_NO_VAL]            =   "ERR_MUP_37_TYPE_NO_VAL";
        m_vErrMsg[ecTYPE_NO_CAT]            =   "ERR_MUP_38_TYPE_NO_CAT";
        m_vErrMsg[ecTYPE_NO_ARR]            =   "ERR_MUP_39_TYPE_NO_ARR";
        m_vErrMsg[ecTYPE_MISMATCH]          =   "ERR_MUP_40_TYPE_MISMATCH";
        m_vErrMsg[ecTYPE_MISMATCH_OOB]      =   "ERR_MUP_41_TYPE_MISMATCH_OOB";
        m_vErrMsg[ecASSIGNED_TYPE_MISMATCH] =   "ERR_MUP_42_ASSIGNED_TYPE_MISMATCH";
        m_vErrMsg[ecMETHOD_ERROR]           =   "ERR_MUP_43_METHOD_ERROR";
        m_vErrMsg[ecNOT_IMPLEMENTED]        =   "ERR_MUP_44_NOT_IMPLEMENTED";

        m_vErrMsg[ecINTERNAL_ERROR]         = 	"ERR_MUP_45_INTERNAL_ERROR";



#if defined(_DEBUG)
        for (int i = 0; i < ecCOUNT; ++i)
            if (!m_vErrMsg[i].length())
                assert(false);
#endif
    }

    //---------------------------------------------------------------------------
    //
    //  ParserError class
    //
    //---------------------------------------------------------------------------

    /** \brief Default constructor. */
    ParserError::ParserError()
        : m_strMsg()
        , m_strFormula()
        , m_strTok()
        , m_iPos(-1)
        , m_iErrc(ecUNDEFINED)
        , m_ErrMsg(ParserErrorMsg::Instance())
    {
    }

    //------------------------------------------------------------------------------
    /** \brief This Constructor is used for internal exceptions only.

      It does not contain any information but the error code.
    */
    ParserError::ParserError(EErrorCodes a_iErrc)
        : ParserError()
    {
        m_iErrc = a_iErrc;
        m_strMsg = m_ErrMsg[m_iErrc];

        if (!m_strMsg.length())
            m_strMsg = "Generic parser error";
        else
            m_strMsg = ::_lang.get(m_strMsg);
    }

    //------------------------------------------------------------------------------
    /** \brief Construct an error from a message text. */
    ParserError::ParserError(const string_type& sMsg)
        : ParserError()
    {
        m_strMsg = sMsg;
    }

    //------------------------------------------------------------------------------
    /** \brief Construct an error object.
        \param [in] a_iErrc the error code.
        \param [in] sTok The token string related to this error.
        \param [in] sExpr The expression related to the error.
        \param [in] a_iPos the position in the expression where the error occured.
    */
    ParserError::ParserError( EErrorCodes iErrc,
                              const string_type& sTok,
                              const string_type& sExpr,
                              int iPos )
        : m_strMsg()
        , m_strFormula(sExpr)
        , m_strTok(sTok)
        , m_iPos(iPos)
        , m_iErrc(iErrc)
        , m_ErrMsg(ParserErrorMsg::Instance())
    {
        m_strMsg = ::_lang.get(m_ErrMsg[m_iErrc]);
        stringstream_type stream;
        stream << (int)m_iPos;
        ReplaceSubString(m_strMsg, "$POS$", stream.str());
        ReplaceSubString(m_strMsg, "$TOK$", m_strTok);
    }

    //------------------------------------------------------------------------------
    /** \brief Construct an error object.
        \param [in] iErrc the error code.
        \param [in] iPos the position in the expression where the error occured.
        \param [in] sTok The token string related to this error.
    */
    ParserError::ParserError(EErrorCodes iErrc, int iPos, const string_type& sTok)
        : m_strMsg()
        , m_strFormula()
        , m_strTok(sTok)
        , m_iPos(iPos)
        , m_iErrc(iErrc)
        , m_ErrMsg(ParserErrorMsg::Instance())
    {
        m_strMsg = m_ErrMsg[m_iErrc];
        stringstream_type stream;
        stream << (int)m_iPos;
        ReplaceSubString(m_strMsg, "$POS$", stream.str());
        ReplaceSubString(m_strMsg, "$TOK$", m_strTok);
    }

    //------------------------------------------------------------------------------
    /** \brief Construct an error object.
        \param [in] szMsg The error message text.
        \param [in] iPos the position related to the error.
        \param [in] sTok The token string related to this error.
    */
    ParserError::ParserError(const char_type* szMsg, int iPos, const string_type& sTok)
        : m_strMsg(szMsg)
        , m_strFormula()
        , m_strTok(sTok)
        , m_iPos(iPos)
        , m_iErrc(ecGENERIC)
        , m_ErrMsg(ParserErrorMsg::Instance())
    {
        stringstream_type stream;
        stream << (int)m_iPos;
        ReplaceSubString(m_strMsg, "$POS$", stream.str());
        ReplaceSubString(m_strMsg, "$TOK$", m_strTok);
    }

    //------------------------------------------------------------------------------
    /** \brief Copy constructor. */
    ParserError::ParserError(const ParserError& a_Obj)
        : m_strMsg(a_Obj.m_strMsg)
        , m_strFormula(a_Obj.m_strFormula)
        , m_strTok(a_Obj.m_strTok)
        , m_iPos(a_Obj.m_iPos)
        , m_iErrc(a_Obj.m_iErrc)
        , m_ErrMsg(ParserErrorMsg::Instance())
    {
    }

    //------------------------------------------------------------------------------
    /** \brief Assignment operator. */
    ParserError& ParserError::operator=(const ParserError& a_Obj)
    {
        if (this == &a_Obj)
            return *this;

        m_strMsg = a_Obj.m_strMsg;
        m_strFormula = a_Obj.m_strFormula;
        m_strTok = a_Obj.m_strTok;
        m_iPos = a_Obj.m_iPos;
        m_iErrc = a_Obj.m_iErrc;
        return *this;
    }

    //------------------------------------------------------------------------------
    ParserError::~ParserError()
    {}

    //------------------------------------------------------------------------------
    /** \brief Replace all ocuurences of a substring with another string.
        \param strFind The string that shall be replaced.
        \param strReplaceWith The string that should be inserted instead of strFind
    */
    void ParserError::ReplaceSubString( string_type& strSource,
                                        const string_type& strFind,
                                        const string_type& strReplaceWith)
    {
        string_type strResult;
        string_type::size_type iPos(0), iNext(0);

        for (;;)
        {
            iNext = strSource.find(strFind, iPos);
            strResult.append(strSource, iPos, iNext - iPos);

            if ( iNext == string_type::npos )
                break;

            strResult.append(strReplaceWith);
            iPos = iNext + strFind.length();
        }

        strSource.swap(strResult);
    }

    //------------------------------------------------------------------------------
    /** \brief Reset the erro object. */
    void ParserError::Reset()
    {
        m_strMsg.clear();
        m_strFormula.clear();
        m_strTok.clear();
        m_iPos = -1;
        m_iErrc = ecUNDEFINED;
    }

    //------------------------------------------------------------------------------
    /** \brief Set the expression related to this error. */
    void ParserError::SetFormula(const string_type& a_strFormula)
    {
        m_strFormula = a_strFormula;
    }

    //------------------------------------------------------------------------------
    /** \brief gets the expression related tp this error.*/
    const string_type& ParserError::GetExpr() const
    {
        return m_strFormula;
    }

    //------------------------------------------------------------------------------
    /** \brief Returns the message string for this error. */
    const string_type& ParserError::GetMsg() const
    {
        return m_strMsg;
    }

    //------------------------------------------------------------------------------
    /** \brief Return the formula position related to the error.

      If the error is not related to a distinct position this will return -1
    */
    std::size_t ParserError::GetPos() const
    {
        return m_iPos;
    }

    //------------------------------------------------------------------------------
    /** \brief Return string related with this token (if available). */
    const string_type& ParserError::GetToken() const
    {
        return m_strTok;
    }

    //------------------------------------------------------------------------------
    /** \brief Return the error code. */
    EErrorCodes ParserError::GetCode() const
    {
        return m_iErrc;
    }
} // namespace mu
