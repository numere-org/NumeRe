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
    return (a_iIdx<m_vErrMsg.size()) ? m_vErrMsg[a_iIdx] : string_type();
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
    :m_vErrMsg(0)
  {
    m_vErrMsg.resize(ecCOUNT);

    m_vErrMsg[ecUNASSIGNABLE_TOKEN]     = 	"ERR_MUP_UNASSIGNABLE_TOKEN";
    m_vErrMsg[ecINTERNAL_ERROR]         = 	"ERR_MUP_INTERNAL_ERROR";
    m_vErrMsg[ecINVALID_NAME]           = 	"ERR_MUP_INVALID_NAME";
    m_vErrMsg[ecINVALID_BINOP_IDENT]    = 	"ERR_MUP_INVALID_BINOP_IDENT";
    m_vErrMsg[ecINVALID_INFIX_IDENT]    = 	"ERR_MUP_INVALID_INFIX_IDENT";
    m_vErrMsg[ecINVALID_POSTFIX_IDENT]  = 	"ERR_MUP_INVALID_POSTFIX_IDENT";
    m_vErrMsg[ecINVALID_FUN_PTR]        = 	"ERR_MUP_INVALID_FUN_PTR";
    m_vErrMsg[ecEMPTY_EXPRESSION]       = 	"ERR_MUP_EMPTY_EXPRESSION";
    m_vErrMsg[ecINVALID_VAR_PTR]        = 	"ERR_MUP_INVALID_VAR_PTR";
    m_vErrMsg[ecUNEXPECTED_OPERATOR]    = 	"ERR_MUP_UNEXPECTED_OPERATOR";
    m_vErrMsg[ecUNEXPECTED_EOF]         = 	"ERR_MUP_UNEXPECTED_EOF";
    m_vErrMsg[ecUNEXPECTED_ARG_SEP]     = 	"ERR_MUP_UNEXPECTED_ARG_SEP";
    m_vErrMsg[ecUNEXPECTED_PARENS]      = 	"ERR_MUP_UNEXPECTED_PARENS";
    m_vErrMsg[ecUNEXPECTED_FUN]         = 	"ERR_MUP_UNEXPECTED_FUN";
    m_vErrMsg[ecUNEXPECTED_VAL]         = 	"ERR_MUP_UNEXPECTED_VAL";
    m_vErrMsg[ecUNEXPECTED_VAR]         = 	"ERR_MUP_UNEXPECTED_VAR";
    m_vErrMsg[ecUNEXPECTED_ARG]         = 	"ERR_MUP_UNEXPECTED_ARG";
    m_vErrMsg[ecMISSING_PARENS]         = 	"ERR_MUP_MISSING_PARENS";
    m_vErrMsg[ecTOO_MANY_PARAMS]        = 	"ERR_MUP_TOO_MANY_PARAMS";
    m_vErrMsg[ecTOO_FEW_PARAMS]         = 	"ERR_MUP_TOO_FEW_PARAMS";
    m_vErrMsg[ecDIV_BY_ZERO]            = 	"ERR_MUP_DIV_BY_ZERO";
    m_vErrMsg[ecDOMAIN_ERROR]           = 	"ERR_MUP_DOMAIN_ERROR";
    m_vErrMsg[ecNAME_CONFLICT]          = 	"ERR_MUP_NAME_CONFLICT";
    m_vErrMsg[ecOPT_PRI]                = 	"ERR_MUP_OPT_PRI";
    m_vErrMsg[ecBUILTIN_OVERLOAD]       = 	"ERR_MUP_BUILTIN_OVERLOAD";
    m_vErrMsg[ecUNEXPECTED_STR]         = 	"ERR_MUP_UNEXPECTED_STR";
    m_vErrMsg[ecUNTERMINATED_STRING]    = 	"ERR_MUP_UNTERMINATED_STRING";
    m_vErrMsg[ecSTRING_EXPECTED]        = 	"ERR_MUP_STRING_EXPECTED";
    m_vErrMsg[ecVAL_EXPECTED]           = 	"ERR_MUP_VAL_EXPECTED";
    m_vErrMsg[ecOPRT_TYPE_CONFLICT]     = 	"ERR_MUP_OPRT_TYPE_CONFLICT";
    m_vErrMsg[ecSTR_RESULT]             = 	"ERR_MUP_STR_RESULT";
    m_vErrMsg[ecGENERIC]                = 	"ERR_MUP_GENERIC";
    m_vErrMsg[ecLOCALE]                 = 	"ERR_MUP_LOCALE";
    m_vErrMsg[ecUNEXPECTED_CONDITIONAL] = 	"ERR_MUP_UNEXPECTED_CONDITIONAL";
    m_vErrMsg[ecMISSING_ELSE_CLAUSE]    = 	"ERR_MUP_MISSING_ELSE_CLAUSE";
    m_vErrMsg[ecMISPLACED_COLON]        = 	"ERR_MUP_MISPLACED_COLON";

    /*
    m_vErrMsg[ecUNASSIGNABLE_TOKEN]     = _T("Unerwartetes Objekt \"$TOK$\" an der Stelle $POS$ gefunden.$(Umlaute und Sonderzeichen können in mathematischen Ausdrücken nicht verwendet werden.)");
    m_vErrMsg[ecINTERNAL_ERROR]         = _T("Interner Fehler.");
    m_vErrMsg[ecINVALID_NAME]           = _T("Ungültige Funktions-, Variablen- oder Konstantenbezeichnung: \"$TOK$\".");
    m_vErrMsg[ecINVALID_BINOP_IDENT]    = _T("Ungültiger Verknüpfungsoperator: \"$TOK$\".");
    m_vErrMsg[ecINVALID_INFIX_IDENT]    = _T("Ungültiger Präfix-Operator: \"$TOK$\".");
    m_vErrMsg[ecINVALID_POSTFIX_IDENT]  = _T("Ungültiger Postfix-Operator: \"$TOK$\".");
    m_vErrMsg[ecINVALID_FUN_PTR]        = _T("Ungültiger Pointer auf eine Callback-Funktion.");
    m_vErrMsg[ecEMPTY_EXPRESSION]       = _T("Leerer Ausdruck.");
    m_vErrMsg[ecINVALID_VAR_PTR]        = _T("Ungültiger Pointer auf eine Variable.");
    m_vErrMsg[ecUNEXPECTED_OPERATOR]    = _T("Unerwarteter Operator \"$TOK$\" an der Stelle $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_EOF]         = _T("Unerwartetes Ende des Ausdrucks an der Stelle $POS$.");
    m_vErrMsg[ecUNEXPECTED_ARG_SEP]     = _T("Unerwartetes Argument-Trennzeichen an der Stelle $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_PARENS]      = _T("Unerwartete Klammer \"$TOK$\" an der Stelle $POS$ gefunden. Verknüpfungsoperator oder Ausdruck vergessen oder falscher/unbekannter Funktionsname?$(Siehe \"list -func\" und \"list -define\" für eine Liste der vorhandenen Funktionen. Ggf. befinden sich auch ein oder mehrere Leerzeichen zwischen Funktionsname und -argumentliste.)");
    m_vErrMsg[ecUNEXPECTED_FUN]         = _T("Unerwartete Funktion \"$TOK$\" an der Stelle $POS$ gefunden.");
    m_vErrMsg[ecUNEXPECTED_VAL]         = _T("Unerwarteter Wert \"$TOK$\" an der Stelle $POS$ gefunden. Verknüpfungsoperator vergessen?");
    m_vErrMsg[ecUNEXPECTED_VAR]         = _T("Unerwartete Variable \"$TOK$\" an der Stelle $POS$ gefunden. Verknüpfungsoperator vergessen?");
    m_vErrMsg[ecUNEXPECTED_ARG]         = _T("Funktionsargument(e) ohne Funktion verwendet (Position: $POS$).");
    m_vErrMsg[ecMISSING_PARENS]         = _T("Fehlende Klammer.");
    m_vErrMsg[ecTOO_MANY_PARAMS]        = _T("Zu viele Argumente für die Funktion \"$TOK$()\" im Ausdruck an der Stelle $POS$.$(Möglicherweise wurden Kommas statt Punkten als Dezimaltrennzeichen verwendet. Siehe auch \"list -func\" oder \"list -define\" für eine Liste der vorhandenen Funktionen.)");
    m_vErrMsg[ecTOO_FEW_PARAMS]         = _T("Zu wenige Argumente für die Funktion \"$TOK$()\" im Ausdruck an der Stelle $POS$.$(Siehe \"list -func\" oder \"list -define\" für eine Liste der vorhandenen Funktionen.)");
    m_vErrMsg[ecDIV_BY_ZERO]            = _T("Kann nicht durch Null teilen.");
    m_vErrMsg[ecDOMAIN_ERROR]           = _T("Domainfehler.");
    m_vErrMsg[ecNAME_CONFLICT]          = _T("Namenskonflikt.");
    m_vErrMsg[ecOPT_PRI]                = _T("Ungültiger Wert für Operatorpriorität (muss größer oder gleich 0 sein).");
    m_vErrMsg[ecBUILTIN_OVERLOAD]       = _T("Benutzerdefinierter Verknüpfungsoperator \"$TOK$\" steht mit einem Built-In-Operator in Konflikt.");
    m_vErrMsg[ecUNEXPECTED_STR]         = _T("Unerwartes Stringobjekt an der Stelle $POS$ gefunden.");
    m_vErrMsg[ecUNTERMINATED_STRING]    = _T("Unabgeschlossener String, der an der Stelle $POS$ startet.");
    m_vErrMsg[ecSTRING_EXPECTED]        = _T("Stringfunktion wurde mit einem Nicht-String-Parameter aufgerufen.");
    m_vErrMsg[ecVAL_EXPECTED]           = _T("String an einer Stelle verwendet, an der ein numerischer Wert erwartet war.");
    m_vErrMsg[ecOPRT_TYPE_CONFLICT]     = _T("Keine passende Überladung für den Operator \"$TOK$\" an der Stelle $POS$ gefunden.");
    m_vErrMsg[ecSTR_RESULT]             = _T("Funktionsergebnis ist ein String.");
    m_vErrMsg[ecGENERIC]                = _T("Parserfehler.");
    m_vErrMsg[ecLOCALE]                 = _T("Dezimaltrennzeichen ist identisch zum Argument-Trennzeichen.");
    m_vErrMsg[ecUNEXPECTED_CONDITIONAL] = _T("Der \"$TOK$\"-Operator muss auf eine schließende Klammer folgen.");
    m_vErrMsg[ecMISSING_ELSE_CLAUSE]    = _T("Der \"Wenn-Dann-Sonst\"-(A?x:y)-Operator besitzt keinen \"Sonst\"-Fall.");
    m_vErrMsg[ecMISPLACED_COLON]        = _T("Falsch gesetzter Doppelpunkt an der Stelle $POS$. Datenobjekt vergessen oder falsch verwendeter \"Wenn-Dann-Sonst\"-(A?x:y)-Operator?");
    */

    #if defined(_DEBUG)
      for (int i=0; i<ecCOUNT; ++i)
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
    :m_strMsg()
    ,m_strFormula()
    ,m_strTok()
    ,m_iPos(-1)
    ,m_iErrc(ecUNDEFINED)
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {
  }

  //------------------------------------------------------------------------------
  /** \brief This Constructor is used for internal exceptions only.

    It does not contain any information but the error code.
  */
  ParserError::ParserError(EErrorCodes /*a_iErrc*/)
    :m_ErrMsg(ParserErrorMsg::Instance())
  {
    Reset();
    m_strMsg = _T("parser error");
  }

  //------------------------------------------------------------------------------
  /** \brief Construct an error from a message text. */
  ParserError::ParserError(const string_type &sMsg)
    :m_ErrMsg(ParserErrorMsg::Instance())
  {
    Reset();
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
                            const string_type &sTok,
                            const string_type &sExpr,
                            int iPos )
    :m_strMsg()
    ,m_strFormula(sExpr)
    ,m_strTok(sTok)
    ,m_iPos(iPos)
    ,m_iErrc(iErrc)
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {
    m_strMsg = ::_lang.get(m_ErrMsg[m_iErrc]);
    stringstream_type stream;
    stream << (int)m_iPos;
    ReplaceSubString(m_strMsg, _T("$POS$"), stream.str());
    ReplaceSubString(m_strMsg, _T("$TOK$"), m_strTok);
  }

  //------------------------------------------------------------------------------
  /** \brief Construct an error object.
      \param [in] iErrc the error code.
      \param [in] iPos the position in the expression where the error occured.
      \param [in] sTok The token string related to this error.
  */
  ParserError::ParserError(EErrorCodes iErrc, int iPos, const string_type &sTok)
    :m_strMsg()
    ,m_strFormula()
    ,m_strTok(sTok)
    ,m_iPos(iPos)
    ,m_iErrc(iErrc)
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {
    m_strMsg = m_ErrMsg[m_iErrc];
    stringstream_type stream;
    stream << (int)m_iPos;
    ReplaceSubString(m_strMsg, _T("$POS$"), stream.str());
    ReplaceSubString(m_strMsg, _T("$TOK$"), m_strTok);
  }

  //------------------------------------------------------------------------------
  /** \brief Construct an error object.
      \param [in] szMsg The error message text.
      \param [in] iPos the position related to the error.
      \param [in] sTok The token string related to this error.
  */
  ParserError::ParserError(const char_type *szMsg, int iPos, const string_type &sTok)
    :m_strMsg(szMsg)
    ,m_strFormula()
    ,m_strTok(sTok)
    ,m_iPos(iPos)
    ,m_iErrc(ecGENERIC)
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {
    stringstream_type stream;
    stream << (int)m_iPos;
    ReplaceSubString(m_strMsg, _T("$POS$"), stream.str());
    ReplaceSubString(m_strMsg, _T("$TOK$"), m_strTok);
  }

  //------------------------------------------------------------------------------
  /** \brief Copy constructor. */
  ParserError::ParserError(const ParserError &a_Obj)
    :m_strMsg(a_Obj.m_strMsg)
    ,m_strFormula(a_Obj.m_strFormula)
    ,m_strTok(a_Obj.m_strTok)
    ,m_iPos(a_Obj.m_iPos)
    ,m_iErrc(a_Obj.m_iErrc)
    ,m_ErrMsg(ParserErrorMsg::Instance())
  {
  }

  //------------------------------------------------------------------------------
  /** \brief Assignment operator. */
  ParserError& ParserError::operator=(const ParserError &a_Obj)
  {
    if (this==&a_Obj)
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
  void ParserError::ReplaceSubString( string_type &strSource,
                                      const string_type &strFind,
                                      const string_type &strReplaceWith)
  {
    string_type strResult;
    string_type::size_type iPos(0), iNext(0);

    for(;;)
    {
      iNext = strSource.find(strFind, iPos);
      strResult.append(strSource, iPos, iNext-iPos);

      if( iNext==string_type::npos )
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
    m_strMsg = _T("");
    m_strFormula = _T("");
    m_strTok = _T("");
    m_iPos = -1;
    m_iErrc = ecUNDEFINED;
  }

  //------------------------------------------------------------------------------
  /** \brief Set the expression related to this error. */
  void ParserError::SetFormula(const string_type &a_strFormula)
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
