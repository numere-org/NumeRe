/*
                 __________
    _____   __ __\______   \_____  _______  ______  ____ _______
   /     \ |  |  \|     ___/\__  \ \_  __ \/  ___/_/ __ \\_  __ \
  |  Y Y  \|  |  /|    |     / __ \_|  | \/\___ \ \  ___/ |  | \/
  |__|_|  /|____/ |____|    (____  /|__|  /____  > \___  >|__|
        \/                       \/            \/      \/
  Copyright (C) 2012 Ingo Berg

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
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <stack>
#include <string>

#include "muParserTokenReader.h"
#include "muParserBase.h"
#include "../utils/tools.hpp"

size_t getMatchingParenthesis(const StringView&);

/** \file
    \brief This file contains the parser token reader implementation.
*/


namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Really simple helper to name the
    /// correct dimension variable.
    ///
    /// \param dimVar size_t
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    static std::string getDimVarName(size_t dimVar)
    {
        if (dimVar == 0)
            return "nrows";
        else if (dimVar == 1)
            return "ncols";

        return "ndim";
//        return "ndim[" + toString(dimVar+1) + "]";
    }


	// Forward declaration
	class ParserBase;

	//---------------------------------------------------------------------------
	/** \brief Copy constructor.

	    \sa Assign
	    \throw nothrow
	*/
	ParserTokenReader::ParserTokenReader(const ParserTokenReader& a_Reader)
	{
		Assign(a_Reader);
	}

	//---------------------------------------------------------------------------
	/** \brief Assignement operator.

	    Self assignement will be suppressed otherwise #Assign is called.

	    \param a_Reader Object to copy to this token reader.
	    \throw nothrow
	*/
	ParserTokenReader& ParserTokenReader::operator=(const ParserTokenReader& a_Reader)
	{
		if (&a_Reader != this)
			Assign(a_Reader);

		return *this;
	}

	//---------------------------------------------------------------------------
	/** \brief Assign state of a token reader to this token reader.

	    \param a_Reader Object from which the state should be copied.
	    \throw nothrow
	*/
	void ParserTokenReader::Assign(const ParserTokenReader& a_Reader)
	{
		m_pParser = a_Reader.m_pParser;
		m_strFormula = a_Reader.m_strFormula;
		m_iPos = a_Reader.m_iPos;
		m_iSynFlags = a_Reader.m_iSynFlags;

		m_UsedVar         = a_Reader.m_UsedVar;
		m_pFunDef         = a_Reader.m_pFunDef;
		m_pConstDef       = a_Reader.m_pConstDef;
		m_factory         = a_Reader.m_factory;
		m_pPostOprtDef    = a_Reader.m_pPostOprtDef;
		m_pInfixOprtDef   = a_Reader.m_pInfixOprtDef;
		m_pOprtDef        = a_Reader.m_pOprtDef;
		m_bIgnoreUndefVar = a_Reader.m_bIgnoreUndefVar;
		m_vIdentFun       = a_Reader.m_vIdentFun;
		m_iBrackets       = a_Reader.m_iBrackets;
		m_iVBrackets      = a_Reader.m_iVBrackets;
		m_iSqBrackets      = a_Reader.m_iSqBrackets;
		m_cArgSep         = a_Reader.m_cArgSep;
		m_lastTok         = a_Reader.m_lastTok;
		m_fZero           = a_Reader.m_fZero;
	}

	//---------------------------------------------------------------------------
	/** \brief Constructor.

	    Create a Token reader and bind it to a parser object.

	    \pre [assert] a_pParser may not be NULL
	    \post #m_pParser==a_pParser
	    \param a_pParent Parent parser object of the token reader.
	*/
	ParserTokenReader::ParserTokenReader(ParserBase* a_pParent)
		: m_pParser(a_pParent)
		, m_strFormula()
		, m_iPos(0)
		, m_iSynFlags(0)
		, m_bIgnoreUndefVar(false)
		, m_pFunDef(nullptr)
		, m_pPostOprtDef(nullptr)
		, m_pInfixOprtDef(nullptr)
		, m_pOprtDef(nullptr)
		, m_pConstDef(nullptr)
		, m_vIdentFun()
		, m_UsedVar()
		, m_fZero(mu::Value(0.0))
		, m_iBrackets(0)
		, m_iVBrackets(0)
		, m_iSqBrackets(0)
		, m_lastTok()
		, m_cArgSep(',')
	{
		assert(m_pParser);
		SetParent(m_pParser);
	}

	//---------------------------------------------------------------------------
	/** \brief Create instance of a ParserTokenReader identical with this
	            and return its pointer.

	    This is a factory method the calling function must take care of the object destruction.

	    \return A new ParserTokenReader object.
	    \throw nothrow
	*/
	ParserTokenReader* ParserTokenReader::Clone(ParserBase* a_pParent) const
	{
		std::unique_ptr<ParserTokenReader> ptr(new ParserTokenReader(*this));
		ptr->SetParent(a_pParent);
		return ptr.release();
	}

	//---------------------------------------------------------------------------
	ParserTokenReader::token_type ParserTokenReader::SaveBeforeReturn(token_type&& tok)
	{
		m_lastTok = tok;
		return std::move(tok);
	}

	//---------------------------------------------------------------------------
	void ParserTokenReader::AddValIdent(identfun_type a_pCallback)
	{
		// Use push_front is used to give user defined callbacks a higher priority than
		// the built in ones. Otherwise reading hex numbers would not work
		// since the "0" in "0xff" would always be read first making parsing of
		// the rest impossible.
		// reference:
		// http://sourceforge.net/projects/muparser/forums/forum/462843/topic/4824956
		m_vIdentFun.push_front(a_pCallback);
	}

	//---------------------------------------------------------------------------
	/** \brief Return the current position of the token reader in the formula string.

	    \return #m_iPos
	    \throw nothrow
	*/
	int ParserTokenReader::GetPos() const
	{
		return m_iPos;
	}

	//---------------------------------------------------------------------------
	/** \brief Return a reference to the formula.

	    \return #m_strFormula
	    \throw nothrow
	*/
	StringView ParserTokenReader::GetExpr() const
	{
		return m_strFormula;
	}

	//---------------------------------------------------------------------------
	/** \brief Return a map containing the used variables only. */
	varmap_type& ParserTokenReader::GetUsedVar()
	{
		return m_UsedVar;
	}

	//---------------------------------------------------------------------------
	/** \brief Initialize the token Reader.

	    Sets the formula position index to zero and set Syntax flags to default for initial formula parsing.
	    \pre [assert] triggered if a_szFormula==0
	*/
	void ParserTokenReader::SetFormula(StringView a_strFormula)
	{
		m_strFormula = a_strFormula;
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief Set Flag that contronls behaviour in case of undefined variables beeing found.

	  If true, the parser does not throw an exception if an undefined variable is found.
	  otherwise it does. This variable is used internally only!
	  It supresses a "undefined variable" exception in GetUsedVar().
	  Those function should return a complete list of variables including
	  those the are not defined by the time of it's call.
	*/
	void ParserTokenReader::IgnoreUndefVar(bool bIgnore)
	{
		m_bIgnoreUndefVar = bIgnore;
	}

	//---------------------------------------------------------------------------
	/** \brief Reset the token reader to the start of the formula.

	    The syntax flags will be reset to a value appropriate for the
	    start of a formula.
	    \post #m_iPos==0, #m_iSynFlags = noOPT | noBC | noPOSTOP | noSTR
	    \throw nothrow
	    \sa ESynCodes
	*/
	void ParserTokenReader::ReInit()
	{
		m_iPos = 0;
		m_iSynFlags = sfSTART_OF_LINE;
		m_iBrackets = 0;
		m_iVBrackets = 0;
		m_iSqBrackets = 0;
		m_UsedVar.clear();
		m_lastTok = token_type();

		while (m_indexedVars.size())
            m_indexedVars.pop();
	}

	//---------------------------------------------------------------------------
	/** \brief Read the next token from the string. */
	ParserTokenReader::token_type ParserTokenReader::ReadNextToken()
	{
		assert(m_pParser);

		std::stack<int> FunArgs;
		token_type tok;
		m_lastTok.GetCode();

		// Ignore all non printable characters when reading the expression
		while (m_strFormula[m_iPos] > 0 && m_strFormula[m_iPos] <= 0x20)
			++m_iPos;

		if ( IsEOF(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for end of formula
		if ( IsOprt(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for user defined binary operator
		if ( IsFunTok(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for function token
		if ( IsMethod(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for method token
		if ( IsBuiltIn(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check built in operators / tokens
		if ( IsArgSep(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for function argument separators
		if ( IsValTok(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for values / constant tokens
		if ( IsVarTok(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for variable tokens
		if ( IsString(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for String tokens
		if (IsInfixOpTok(tok))
			return SaveBeforeReturn(std::move(tok)); // Check for unary operators
		if ( IsPostOpTok(tok) )
			return SaveBeforeReturn(std::move(tok)); // Check for unary operators

		// Check String for undefined variable token. Done only if a
		// flag is set indicating to ignore undefined variables.
		// This is a way to conditionally avoid an error if
		// undefined variables occur.
		// (The GetUsedVar function must suppress the error for
		// undefined variables in order to collect all variable
		// names including the undefined ones.)
		if ( (m_bIgnoreUndefVar || m_factory) && IsUndefVarTok(tok) )
			return SaveBeforeReturn(std::move(tok));

		// Check for unknown token
		//
		// !!! From this point on there is no exit without an exception possible...
		//
		string_type strTok;
		int iEnd = ExtractToken(m_pParser->ValidNameChars(), strTok, m_iPos);
		if (iEnd != m_iPos)
			Error(ecUNASSIGNABLE_TOKEN, m_iPos, strTok);

		Error(ecUNASSIGNABLE_TOKEN, m_iPos, m_strFormula.subview(m_iPos).to_string());
		return token_type(); // never reached
	}

	//---------------------------------------------------------------------------
	void ParserTokenReader::SetParent(ParserBase* a_pParent)
	{
		m_pParser       = a_pParent;
		m_pFunDef       = &a_pParent->m_FunDef;
		m_pOprtDef      = &a_pParent->m_OprtDef;
		m_pInfixOprtDef = &a_pParent->m_InfixOprtDef;
		m_pPostOprtDef  = &a_pParent->m_PostOprtDef;
		m_factory       = a_pParent->m_factory;
		m_pConstDef     = &a_pParent->m_ConstDef;
	}

	//---------------------------------------------------------------------------
	/** \brief Extract all characters that belong to a certain charset.

	  \param a_szCharSet [in] Const char array of the characters allowed in the token.
	  \param a_strTok [out]  The string that consists entirely of characters listed in a_szCharSet.
	  \param a_iPos [in] Position in the string from where to start reading.
	  \return The Position of the first character not listed in a_szCharSet.
	  \throw nothrow
	*/
	int ParserTokenReader::ExtractToken(const char_type* a_szCharSet,
										string_type& a_sTok,
										int a_iPos) const
	{
		size_t iEnd = std::min(m_strFormula.find_first_not_of(a_szCharSet, a_iPos),
                               m_strFormula.length());

		// Assign token string if there was something found
		if (a_iPos != (int)iEnd)
			a_sTok = m_strFormula.subview(a_iPos, iEnd-a_iPos).to_string();

		return iEnd;
	}

	//---------------------------------------------------------------------------
	/** \brief Check Expression for the presence of a binary operator token.

	  Userdefined binary operator "++" gives inconsistent parsing result for
	  the equations "a++b" and "a ++ b" if alphabetic characters are allowed
	  in operator tokens. To avoid this this function checks specifically
	  for operator tokens.
	*/
	int ParserTokenReader::ExtractOperatorToken(string_type& a_sTok, int a_iPos) const
	{
		size_t iEnd = std::min(m_strFormula.find_first_not_of(m_pParser->ValidInfixOprtChars(), a_iPos),
                               m_strFormula.length());

		// Assign token string if there was something found
		if (a_iPos != (int)iEnd)
		{
			a_sTok = m_strFormula.subview(a_iPos, iEnd-a_iPos).to_string();
			return iEnd;
		}
		else
		{
			// There is still the chance of having to deal with an operator consisting exclusively
			// of alphabetic characters.
			return ExtractToken(MUP_CHARS, a_sTok, a_iPos);
		}
	}

	//---------------------------------------------------------------------------
	/** \brief Check if a built in operator or other token can be found
	    \param a_Tok  [out] Operator token if one is found. This can either be a binary operator or an infix operator token.
	    \return true if an operator token has been found.
	*/
	bool ParserTokenReader::IsBuiltIn(token_type& a_Tok)
	{
		const char_type** pOprtDef = m_pParser->GetOprtDef();

		// Compare token with function and operator strings
		// check string for operator/function
		for (int i = 0; pOprtDef[i]; i++)
		{
			std::size_t len( std::char_traits<char_type>::length(pOprtDef[i]) );

			if (m_strFormula.match(pOprtDef[i], m_iPos))
			{
			    const char* oprt = pOprtDef[i];

				switch (i)
				{
					//case cmAND:
					//case cmOR:
					//case cmXOR:
					case cmLAND:
					case cmLOR:
					case cmLT:
					case cmGT:
					case cmLE:
					case cmGE:
					case cmNEQ:
					case cmEQ:
					case cmADD:
					case cmSUB:
					case cmMUL:
					case cmDIV:
					case cmPOW:
					case cmASSIGN:
					case cmADDASGN:
					case cmSUBASGN:
					case cmMULASGN:
					case cmDIVASGN:
					case cmPOWASGN:
						//if (len!=sTok.length())
						//  continue;

						// The assignement operator need special treatment
						if (i >= cmASSIGN && i <= cmPOWASGN && m_iSynFlags & noASSIGN)
							Error(ecUNEXPECTED_OPERATOR, m_iPos, oprt);

						if (!m_pParser->HasBuiltInOprt())
							continue;

						if (m_iSynFlags & noOPT)
						{
							// Maybe its an infix operator not an operator
							// Both operator types can share characters in
							// their identifiers
							if ( IsInfixOpTok(a_Tok) )
								return true;

							Error(ecUNEXPECTED_OPERATOR, m_iPos, oprt);
						}

						m_iSynFlags  = noBC | noVC | noOPT | noARG_SEP | noPOSTOP | noASSIGN | noIF | noELSE | noMETHOD | noSqO | noSqC;
						m_iSynFlags |= ( (i != cmEND) && ( i != cmBC) ) ? noEND : 0;
						break;
                    case cmINCR:
                    case cmDECR:
						// The assignement operator need special treatment
						if (m_iSynFlags & noASSIGN)
							Error(ecUNEXPECTED_OPERATOR, m_iPos, oprt);

						if (!m_pParser->HasBuiltInOprt())
							continue;

						if (m_iSynFlags & noOPT)
						{
							// Maybe its an infix operator not an operator
							// Both operator types can share characters in
							// their identifiers
							if ( IsInfixOpTok(a_Tok) )
								return true;

							Error(ecUNEXPECTED_OPERATOR, m_iPos, oprt);
						}

                        m_iSynFlags = noVAL | noVAR | noFUN | noMETHOD | noBO | noVO | noPOSTOP | noSTR | noASSIGN | noSqO;
						break;

					case cmBO:
						if (m_iSynFlags & noBO)
							Error(ecUNEXPECTED_PARENS, m_iPos, oprt);

                        // Convert this to an index
                        if (m_lastTok.GetCode() == cmVAR)
                        {
                            // Only clusters are allowed here
                            if (m_lastTok.GetVar()->getCommonType() == TYPE_CLUSTER)
                                Error(ecUNEXPECTED_PARENS, m_iPos, oprt);

                            m_indexedVars.push(IndexedVar(m_lastTok.GetVar(), m_iPos, m_iBrackets+1, m_iVBrackets, m_iSqBrackets));

                            // Figure out, whether this is part of an assignment
                            if (IsLeftHandSide(m_iPos))
                                i = cmBIDXASGN;
                            else
                                i = cmBIDX;

                        }

						if (m_lastTok.GetCode() == cmFUNC || m_lastTok.GetCode() == cmMETHOD)
							m_iSynFlags = noOPT | noEND | noARG_SEP | noPOSTOP | noASSIGN | noIF | noELSE | noVC | noMETHOD | noSqO | noSqC;
						else
							m_iSynFlags = noBC | noOPT | noEND | noARG_SEP | noPOSTOP | noASSIGN | noIF | noELSE | noVC | noMETHOD | noSqO | noSqC;

						++m_iBrackets;
						break;

					case cmBC:
						if (m_iSynFlags & noBC)
						{
						    // Slicing with open boundaries
                            if (m_lastTok.GetCode() == cmELSE && m_indexedVars.size())
                            {
                                if (IsLeftHandSide(m_iPos))
                                {
                                    a_Tok.SetVal(m_pConstDef->at("inf"), "inf");
                                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;
                                }
                                else
                                {
                                    a_Tok.SetDimVar(m_indexedVars.top().var, getDimVarName(m_indexedVars.top().argC), m_indexedVars.top().argC);
                                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR | noASSIGN;
                                }

                                return true;
                            }

							Error(ecUNEXPECTED_PARENS, m_iPos, oprt);
                        }

						m_iSynFlags  = noBO | noVAR | noVAL | noFUN | noINFIXOP | noSTR | noVO | noSqO;

						if (m_indexedVars.size()
                            && m_indexedVars.top().braceMatch(m_iBrackets, m_iVBrackets, m_iSqBrackets))
                            m_indexedVars.pop();

						if (--m_iBrackets < 0)
							Error(ecUNEXPECTED_PARENS, m_iPos, oprt);

						break;

					case cmVO:
						if (m_iSynFlags & noVO)
							Error(ecUNEXPECTED_VPARENS, m_iPos, oprt);

						m_iSynFlags = noBC | noOPT | noEND | noARG_SEP | noPOSTOP | noASSIGN | noIF | noELSE | noMETHOD | noSqC | noSqO;

                        // Convert this to an index
                        if (m_lastTok.GetCode() == cmVAR)
                        {
                            // Only clusters are allowed here
                            if (m_lastTok.GetVar()->getCommonType() != TYPE_CLUSTER)
                                Error(ecUNEXPECTED_VPARENS, m_iPos, oprt);

                            m_indexedVars.push(IndexedVar(m_lastTok.GetVar(), m_iPos, m_iBrackets, m_iVBrackets+1, m_iSqBrackets));

                            // Figure out, whether this is part of an assignment
                            if (IsLeftHandSide(m_iPos))
                                i = cmIDXASGN;
                            else
                                i = cmIDX;
                        }

						++m_iVBrackets;
						break;

					case cmVC:
						if (m_iSynFlags & noVC)
						{
						    // Slicing with open boundaries
                            if (m_lastTok.GetCode() == cmELSE && m_indexedVars.size())
                            {
                                if (IsLeftHandSide(m_iPos))
                                {
                                    a_Tok.SetVal(m_pConstDef->at("inf"), "inf");
                                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;
                                }
                                else
                                {
                                    a_Tok.SetDimVar(m_indexedVars.top().var, getDimVarName(m_indexedVars.top().argC), m_indexedVars.top().argC);
                                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR | noASSIGN;
                                }

                                return true;
                            }

							Error(ecUNEXPECTED_VPARENS, m_iPos, oprt);
                        }

						m_iSynFlags  = noBO | noVAR | noVAL | noFUN | noINFIXOP | noSTR | noVO | noSqO;

						if (m_indexedVars.size()
                            && m_indexedVars.top().braceMatch(m_iBrackets, m_iVBrackets, m_iSqBrackets))
                            m_indexedVars.pop();

						if (--m_iVBrackets < 0)
							Error(ecUNEXPECTED_VPARENS, m_iPos, oprt);

						break;

					case cmSQO:
						if (m_iSynFlags & noSqO)
							Error(ecUNEXPECTED_SQPARENS, m_iPos, oprt);

						m_iSynFlags = noBC | noOPT | noEND | noARG_SEP | noPOSTOP | noASSIGN | noIF | noELSE | noMETHOD | noVC | noSqO;

                        // Convert this to an index
                        if (m_lastTok.GetCode() == cmVAR)
                        {
                            // No clusters are allowed here
                            if (m_lastTok.GetVar()->getCommonType() == TYPE_CLUSTER)
                                Error(ecUNEXPECTED_SQPARENS, m_iPos, oprt);

                            m_indexedVars.push(IndexedVar(m_lastTok.GetVar(), m_iPos, m_iBrackets, m_iVBrackets, m_iSqBrackets+1));

                            // Figure out, whether this is part of an assignment
                            if (IsLeftHandSide(m_iPos))
                                i = cmSQIDXASGN;
                            else
                                i = cmSQIDX;
                        }
                        else
                            Error(ecUNEXPECTED_SQPARENS, m_iPos, oprt);

						++m_iSqBrackets;
						break;

					case cmSQC:
						if (m_iSynFlags & noSqC)
                        {
                            // Slicing with open boundaries
                            if (m_lastTok.GetCode() == cmELSE && m_indexedVars.size())
                            {
                                if (IsLeftHandSide(m_iPos))
                                {
                                    a_Tok.SetVal(m_pConstDef->at("inf"), "inf");
                                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;
                                }
                                else
                                {
                                    a_Tok.SetDimVar(m_indexedVars.top().var, "nlen");
                                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR | noASSIGN;
                                }

                                return true;
                            }

							Error(ecUNEXPECTED_SQPARENS, m_iPos, oprt);
                        }

						m_iSynFlags  = noBO | noVAR | noVAL | noFUN | noINFIXOP | noSTR | noVO | noSqO;

						if (m_indexedVars.size()
                            && m_indexedVars.top().braceMatch(m_iBrackets, m_iVBrackets, m_iSqBrackets))
                            m_indexedVars.pop();

						if (--m_iSqBrackets < 0)
							Error(ecUNEXPECTED_SQPARENS, m_iPos, oprt);

						break;

					case cmELSE:
						if (m_iSynFlags & noELSE)
                        {
                            // Slicing with open boundaries
                            if (m_lastTok.GetCode() == cmIDX
                                || m_lastTok.GetCode() == cmSQIDX
                                || m_lastTok.GetCode() == cmIDXASGN
                                || m_lastTok.GetCode() == cmSQIDXASGN
                                || m_lastTok.GetCode() == cmBIDX
                                || m_lastTok.GetCode() == cmBIDXASGN
                                || (m_lastTok.GetCode() == cmARG_SEP && m_indexedVars.size()))
                            {
                                a_Tok.SetVal(mu::Value(1.0), "1");
                                m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;
                                return true;
                            }

							Error(ecUNEXPECTED_CONDITIONAL, m_iPos, oprt);
                        }

						m_iSynFlags = noBC | noVC | noPOSTOP | noEND | noOPT | noIF | noELSE | noMETHOD | noSqO | noSqC;
						break;

					case cmIF:
						if (m_iSynFlags & noIF)
							Error(ecUNEXPECTED_CONDITIONAL, m_iPos, oprt);

						m_iSynFlags = noBC | noVC | noPOSTOP | noEND | noOPT | noIF | noELSE | noMETHOD | noSqO | noSqC;
						break;

					default:      // The operator is listed in c_DefaultOprt, but not here. This is a bad thing...
						Error(ecINTERNAL_ERROR);
				} // switch operator id

				m_iPos += (int)len;
				a_Tok.Set((ECmdCode)i, oprt);
				return true;
			} // if operator string found
		} // end of for all operator strings

		return false;
	}

	//---------------------------------------------------------------------------
	bool ParserTokenReader::IsArgSep(token_type& a_Tok)
	{
		if (m_strFormula[m_iPos] == m_cArgSep)
		{
			// copy the separator into null terminated string
			char_type szSep[2];
			szSep[0] = m_cArgSep;
			szSep[1] = 0;

			if (m_iSynFlags & noARG_SEP)
				Error(ecUNEXPECTED_ARG_SEP, m_iPos, szSep);

            // Square index brackets are not allowed to have argument separators
            // i.e. to be multidimensional
            if (m_indexedVars.size()
                && m_strFormula[m_indexedVars.top().indexStart] == '['
                && m_indexedVars.top().braceMatch(m_iBrackets, m_iVBrackets, m_iSqBrackets))
                Error(ecUNEXPECTED_ARG_SEP, m_iPos, szSep);

            // Slicing with open boundaries
            if (m_lastTok.GetCode() == cmELSE && m_indexedVars.size()
                && m_strFormula[m_indexedVars.top().indexStart] != '['
                && m_indexedVars.top().braceMatch(m_iBrackets, m_iVBrackets, m_iSqBrackets))
            {
                if (IsLeftHandSide(m_indexedVars.top().indexStart))
                {
                    a_Tok.SetVal(m_pConstDef->at("inf"), "inf");
                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;
                }
                else
                {
                    a_Tok.SetDimVar(m_indexedVars.top().var, getDimVarName(m_indexedVars.top().argC), m_indexedVars.top().argC);
                    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR | noASSIGN;
                }

                return true;
            }

            if (m_indexedVars.size()
                && m_indexedVars.top().braceMatch(m_iBrackets, m_iVBrackets, m_iSqBrackets))
                m_indexedVars.top().argC++;

			m_iSynFlags  = noBC | noVC | noOPT | noEND | noARG_SEP | noPOSTOP | noASSIGN | noMETHOD | noSqO | noSqC | noELSE;
			m_iPos++;
			a_Tok.Set(cmARG_SEP, szSep);
			return true;
		}

		return false;
	}

	//---------------------------------------------------------------------------
	/** \brief Check for End of Formula.

	    \return true if an end of formula is found false otherwise.
	    \param a_Tok [out] If an eof is found the corresponding token will be stored there.
	    \throw nothrow
	    \sa IsOprt, IsFunTok, IsStrFunTok, IsValTok, IsVarTok, IsString, IsInfixOpTok, IsPostOpTok
	*/
	bool ParserTokenReader::IsEOF(token_type& a_Tok)
	{
		// check for EOF
		if (m_iPos >= (int)m_strFormula.length() || m_strFormula[m_iPos] == 0)
		{
			if (m_iSynFlags & noEND || m_indexedVars.size())
				Error(ecUNEXPECTED_EOF, m_iPos);

			if (m_iBrackets > 0)
				Error(ecMISSING_PARENS, m_iPos, _nrT(")"));

			m_iSynFlags = 0;
			a_Tok.Set(cmEND);
			return true;
		}

		return false;
	}

	//---------------------------------------------------------------------------
	/** \brief Check if a string position contains a unary infix operator.
	    \return true if a function token has been found false otherwise.
	*/
	bool ParserTokenReader::IsInfixOpTok(token_type& a_Tok)
	{
	    if (m_lastTok.GetCode() == cmVAR || m_lastTok.GetCode() == cmVAL || m_lastTok.GetCode() == cmBC || m_lastTok.GetCode() == cmVC)
            return false;

		string_type sTok;
		int iEnd = ExtractToken(m_pParser->ValidInfixOprtChars(), sTok, m_iPos);
		if (iEnd == m_iPos)
			return false;

        if (sTok.front() == '#')
        {
            if (m_strFormula.match("#<", m_iPos) && m_strFormula.find('>', m_iPos+1) != std::string::npos)
            {
                a_Tok.SetPathPlaceholder(m_strFormula.subview(m_iPos+2, m_strFormula.find('>', m_iPos+1) - m_iPos-2).to_string());

                if (m_iSynFlags & noINFIXOP)
                    Error(ecUNEXPECTED_OPERATOR, m_iPos, "#<" + a_Tok.GetAsString() + ">");

                m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noSqO | noSqC;
                m_iPos = m_strFormula.find('>', m_iPos+1)+1;
                return true;
            }

            size_t nEnd = std::min(sTok.length(), sTok.find_first_not_of("#~"));

            m_iPos += nEnd;
            a_Tok.SetVal2Str(nEnd);

            if (m_iSynFlags & noINFIXOP)
				Error(ecUNEXPECTED_OPERATOR, m_iPos, a_Tok.GetAsString());

			m_iSynFlags = noPOSTOP | noINFIXOP | noOPT | noBC | noVC | noASSIGN | noMETHOD | noSqO | noSqC;
			return true;
        }

		// iteraterate over all postfix operator strings
		funmap_type::const_reverse_iterator it = m_pInfixOprtDef->rbegin();
		for ( ; it != m_pInfixOprtDef->rend(); ++it)
		{
			if (sTok.find(it->first) != 0)
				continue;

			a_Tok.Set(it->second, it->first);
			m_iPos += (int)it->first.length();

			if (m_iSynFlags & noINFIXOP)
				Error(ecUNEXPECTED_OPERATOR, m_iPos, a_Tok.GetAsString());

			m_iSynFlags = noPOSTOP | noINFIXOP | noOPT | noBC | noVC | noASSIGN | noMETHOD | noSqO | noSqC;
			return true;
		}

		return false;

		/*
		    a_Tok.Set(item->second, sTok);
		    m_iPos = (int)iEnd;

		    if (m_iSynFlags & noINFIXOP)
		      Error(ecUNEXPECTED_OPERATOR, m_iPos, a_Tok.GetAsString());

		    m_iSynFlags = noPOSTOP | noINFIXOP | noOPT | noBC | noSTR | noASSIGN;
		    return true;
		*/
	}

	//---------------------------------------------------------------------------
	/** \brief Check whether the token at a given position is a function token.
	    \param a_Tok [out] If a value token is found it will be placed here.
	    \throw ParserException if Syntaxflags do not allow a function at a_iPos
	    \return true if a function token has been found false otherwise.
	    \pre [assert] m_pParser!=0
	*/
	bool ParserTokenReader::IsFunTok(token_type& a_Tok)
	{
		string_type strTok;
		int iEnd = ExtractToken(m_pParser->ValidNameChars(), strTok, m_iPos);
		if (iEnd == m_iPos)
			return false;

		funmap_type::const_iterator item = m_pFunDef->find(strTok);
		if (item == m_pFunDef->end())
			return false;

		// Check if the next sign is an opening bracket
		if (m_strFormula[iEnd] != '(')
			return false;

		a_Tok.Set(item->second, strTok);

		m_iPos = (int)iEnd;
		if (m_iSynFlags & noFUN)
			Error(ecUNEXPECTED_FUN, m_iPos - (int)a_Tok.GetAsString().length(), a_Tok.GetAsString());

		m_iSynFlags = noANY ^ noBO;
		return true;
	}

	bool ParserTokenReader::IsMethod(token_type& a_Tok)
	{
		string_type strTok;

		if (m_strFormula[m_iPos] != '.')
            return false;

		int iEnd = ExtractToken(m_pParser->ValidNameChars(), strTok, m_iPos+1);
		if (iEnd == m_iPos)
			return false;

		// Check if the next sign is an opening bracket
		bool noargs = m_strFormula[iEnd] != '(';

		a_Tok.SetMethod(strTok, noargs);

		m_iPos = (int)iEnd;
		if (m_iSynFlags & noMETHOD)
			Error(ecUNEXPECTED_METHOD, m_iPos - (int)a_Tok.GetAsString().length(), a_Tok.GetAsString());

		m_iSynFlags = noargs ? (noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO) : noANY ^ noBO;
		return true;
	}

	//---------------------------------------------------------------------------
	/** \brief Check if a string position contains a binary operator.
	    \param a_Tok  [out] Operator token if one is found. This can either be a binary operator or an infix operator token.
	    \return true if an operator token has been found.
	*/
	bool ParserTokenReader::IsOprt(token_type& a_Tok)
	{
		string_type strTok;

		int iEnd = ExtractOperatorToken(strTok, m_iPos);
		if (iEnd == m_iPos)
			return false;

		// Check if the operator is a built in operator, if so ignore it here
		const char_type** const pOprtDef = m_pParser->GetOprtDef();
		for (int i = 0; m_pParser->HasBuiltInOprt() && pOprtDef[i]; ++i)
		{
			if (string_type(pOprtDef[i]) == strTok)
				return false;
		}

		// Note:
		// All tokens in oprt_bin_maptype are have been sorted by their length
		// Long operators must come first! Otherwise short names (like: "add") that
		// are part of long token names (like: "add123") will be found instead
		// of the long ones.
		// Length sorting is done with ascending length so we use a reverse iterator here.
		funmap_type::const_reverse_iterator it = m_pOprtDef->rbegin();
		for ( ; it != m_pOprtDef->rend(); ++it)
		{
			if (m_strFormula.match(it->first, m_iPos))
			{
				a_Tok.Set(it->second, strTok);

				// operator was found
				if (m_iSynFlags & noOPT)
				{
					// An operator was found but is not expected to occur at
					// this position of the formula, maybe it is an infix
					// operator, not a binary operator. Both operator types
					// can share characters in their identifiers.
					if ( IsInfixOpTok(a_Tok) )
						return true;
					else
					{
						// nope, no infix operator
						return false;
						//Error(ecUNEXPECTED_OPERATOR, m_iPos, a_Tok.GetAsString());
					}

				}

				m_iPos += (int)(it->first.length());
				m_iSynFlags  = noBC | noOPT | noARG_SEP | noPOSTOP | noEND | noVC | noASSIGN | noMETHOD | noSqO | noSqC;
				return true;
			}
		}

		return false;
	}

	//---------------------------------------------------------------------------
	/** \brief Check if a string position contains a unary post value operator. */
	bool ParserTokenReader::IsPostOpTok(token_type& a_Tok)
	{
		// <ibg 20110629> Do not check for postfix operators if they are not allowed at
		//                the current expression index.
		//
		//  This will fix the bug reported here:
		//
		//  http://sourceforge.net/tracker/index.php?func=detail&aid=3343891&group_id=137191&atid=737979
		//
		if (m_iSynFlags & noPOSTOP)
			return false;
		// </ibg>

		// Tricky problem with equations like "3m+5":
		//     m is a postfix operator, + is a valid sign for postfix operators and
		//     for binary operators parser detects "m+" as operator string and
		//     finds no matching postfix operator.
		//
		// This is a special case so this routine slightly differs from the other
		// token readers.

		// Test if there could be a postfix operator
		string_type sTok;
		int iEnd = ExtractToken(m_pParser->ValidOprtChars(), sTok, m_iPos);
		if (iEnd == m_iPos)
			return false;

		// iteraterate over all postfix operator strings
		funmap_type::const_reverse_iterator it = m_pPostOprtDef->rbegin();
		for ( ; it != m_pPostOprtDef->rend(); ++it)
		{
			if (sTok.find(it->first) != 0)
				continue;

			a_Tok.Set(it->second, sTok);
			m_iPos += (int)it->first.length();

			m_iSynFlags = noVAL | noVAR | noFUN | noMETHOD | noBO | noVO | noPOSTOP | noSTR | noASSIGN | noSqO;
			return true;
		}

		return false;
	}

	//---------------------------------------------------------------------------
	/** \brief Check whether the token at a given position is a value token.

	  Value tokens are either values or constants.

	  \param a_Tok [out] If a value token is found it will be placed here.
	  \return true if a value token has been found.
	*/
	bool ParserTokenReader::IsValTok(token_type& a_Tok)
	{
		assert(m_pConstDef);
		assert(m_pParser);

		string_type strTok;
		Value fVal(0);
		int iEnd(0);

		// 2.) Check for user defined constant
		// Read everything that could be a constant name
		iEnd = ExtractToken(m_pParser->ValidNameChars(), strTok, m_iPos);
		if (iEnd != m_iPos)
		{
			valmap_type::const_iterator item = m_pConstDef->find(strTok);
			if (item != m_pConstDef->end())
			{
				m_iPos = iEnd;
				a_Tok.SetVal(item->second, strTok);

				if (m_iSynFlags & noVAL)
					Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

				m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;
				return true;
			}

			if (strTok == "nlen")
            {
                if (!m_indexedVars.size()
                    || m_strFormula[m_indexedVars.top().indexStart] == '(') // Accept [ and {, the latter for backwards compatibility
                    Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

                m_iPos = iEnd;
				a_Tok.SetDimVar(m_indexedVars.top().var, strTok);

				if (m_iSynFlags & noVAL)
					Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

				m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR | noASSIGN;
				return true;
            }

			if (strTok == "ndim")
            {
                if (!m_indexedVars.size()
                    || m_strFormula[m_indexedVars.top().indexStart] == '[') // Accept ( and {
                    Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

                m_iPos = iEnd;
				a_Tok.SetDimVar(m_indexedVars.top().var, strTok, m_indexedVars.top().argC);

				if (m_iSynFlags & noVAL)
					Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

				m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR | noASSIGN;
				return true;
            }

			if (strTok == "nrows" || strTok == "nlines" || strTok == "ncols")
            {
                if (!m_indexedVars.size()
                    || m_strFormula[m_indexedVars.top().indexStart] == '[') // Accept ( and {
                    return false; // Just return false in this case due to backwards compatibility
                    //Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

                m_iPos = iEnd;
				a_Tok.SetDimVar(m_indexedVars.top().var, strTok, strTok == "ncols" ? 1 : 0);

				if (m_iSynFlags & noVAL)
					Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

				m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR | noASSIGN;
				return true;
            }

		}

		// 3.call the value recognition functions provided by the user
		// Call user defined value recognition functions
		std::list<identfun_type>::const_iterator item = m_vIdentFun.begin();
		for (item = m_vIdentFun.begin(); item != m_vIdentFun.end(); ++item)
		{
			int iStart = m_iPos;
			if ( (*item)(m_strFormula.subview(m_iPos), &m_iPos, &fVal) == 1 )
			{
			    strTok = m_strFormula.subview(iStart, m_iPos-iStart).to_string();
				//strTok.assign(m_strFormula.c_str(), iStart, m_iPos);
				if (m_iSynFlags & noVAL)
					Error(ecUNEXPECTED_VAL, m_iPos - (int)strTok.length(), strTok);

				a_Tok.MoveVal(fVal, strTok);
				m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;
				return true;
			}
		}

		return false;
	}

	//---------------------------------------------------------------------------
	/** \brief Check wheter a token at a given position is a variable token.
	    \param a_Tok [out] If a variable token has been found it will be placed here.
	    \return true if a variable token has been found.
	*/
	bool ParserTokenReader::IsVarTok(token_type& a_Tok)
	{
		if (m_factory->Empty())
			return false;

		string_type strTok;
		int iEnd = ExtractToken(m_pParser->ValidNameChars(), strTok, m_iPos);
		if (iEnd == m_iPos)
			return false;

        Variable* var = m_factory->Get(strTok);

		if (!var)
			return false;

		if (m_iSynFlags & noVAR)
        {
            if (m_pPostOprtDef->find(strTok) != m_pPostOprtDef->end())
                return false;

			Error(ecUNEXPECTED_VAR, m_iPos, strTok);
        }

		m_iPos = iEnd;
		a_Tok.SetVar(var, strTok);
		m_UsedVar[strTok] = var;  // Add variable to used-var-list

        if (m_iPos < (int)m_strFormula.length()+1
            && (m_strFormula.subview(m_iPos, 2) == "{}" || m_strFormula.subview(m_iPos, 2) == "[]" || m_strFormula.subview(m_iPos, 2) == "()"))
        {
            m_iPos += 2;
            m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR;
        }
        else
            m_iSynFlags = noVAL | noVAR | noFUN | noINFIXOP | noSTR;

//  Zur Info hier die SynFlags von IsVal():
//    m_iSynFlags = noVAL | noVAR | noFUN | noBO | noINFIXOP | noSTR | noASSIGN;
		return true;
	}


	//---------------------------------------------------------------------------
	/** \brief Check wheter a token at a given position is an undefined variable.

	    \param a_Tok [out] If a variable tom_pParser->m_vStringBufken has been found it will be placed here.
	    \return true if a variable token has been found.
	    \throw nothrow
	*/
	bool ParserTokenReader::IsUndefVarTok(token_type& a_Tok)
	{
		string_type strTok;
		int iEnd( ExtractToken(m_pParser->ValidNameChars(), strTok, m_iPos) );
		if ( iEnd == m_iPos )
			return false;

		if (m_iSynFlags & noVAR)
		{
			// <ibg/> 20061021 added token string strTok instead of a_Tok.GetAsString() as the
			//                 token identifier.
			// related bug report:
			// http://sourceforge.net/tracker/index.php?func=detail&aid=1578779&group_id=137191&atid=737979
			Error(ecUNEXPECTED_VAR, m_iPos - (int)a_Tok.GetAsString().length(), strTok);
		}

		// If a factory is available implicitely create new variables
		if (m_factory)
		{
		    Variable* fVar;

		    if (iEnd < (int)m_strFormula.length() && m_strFormula[iEnd] == '{')
                fVar = m_factory->Create(strTok, TYPE_CLUSTER);
		    else
                fVar = m_factory->Create(strTok);

			a_Tok.SetVar(fVar, strTok);
			m_UsedVar[strTok] = fVar;  // Add variable to used-var-list
		}
		else
		{
			a_Tok.SetVar((Variable*)&m_fZero, strTok);
			m_UsedVar[strTok] = 0;  // Add variable to used-var-list
		}

		m_iPos = iEnd;

        if (m_strFormula.subview(m_iPos, 2) == "{}" || m_strFormula.subview(m_iPos, 2) == "[]" || m_strFormula.subview(m_iPos, 2) == "()")
        {
            m_iPos += 2;
            m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noSqO | noINFIXOP | noSTR;
        }
        else
            m_iSynFlags = noVAL | noVAR | noFUN | noINFIXOP | noSTR;

		return true;
	}


	//---------------------------------------------------------------------------
	/** \brief Check wheter a token at a given position is a string.
	    \param a_Tok [out] If a variable token has been found it will be placed here.
		  \return true if a string token has been found.
	    \sa IsOprt, IsFunTok, IsStrFunTok, IsValTok, IsVarTok, IsEOF, IsInfixOpTok, IsPostOpTok
	    \throw nothrow
	*/
	bool ParserTokenReader::IsString(token_type& a_Tok)
	{
		if (m_strFormula[m_iPos] != '"')
			return false;

		std::string strBuf = m_strFormula.subview(m_iPos+1).to_string();
		std::size_t iEnd(0), iSkip(0);

		// parser over escaped '\"' end replace them with '"'
		for (iEnd = (int)strBuf.find( _nrT("\"") ); iEnd != 0 && iEnd != string_type::npos; iEnd = (int)strBuf.find( _nrT("\""), iEnd))
		{
			if (strBuf[iEnd - 1] != '\\' || (iEnd > 1 && strBuf[iEnd - 2] == '\\'))
				break;

			strBuf.replace(iEnd - 1, 2, _nrT("\"") );
			iSkip++;
		}

		if (iEnd == string_type::npos)
			Error(ecUNTERMINATED_STRING, m_iPos, _nrT("\"") );

		string_type strTok(strBuf.begin(), strBuf.begin() + iEnd);

		if (m_iSynFlags & noSTR)
			Error(ecUNEXPECTED_STR, m_iPos, strTok);

		m_iPos += (int)strTok.length() +2+ (int)iSkip;  // +iSkip für entfernte escape zeichen+2 for quotation marks

		// Replace all other known escaped characters and remove the surrounding quotation marks
		a_Tok.MoveVal(mu::Value(toInternalString("\"" + strTok + "\"")), strTok);
		m_iSynFlags = noVAL | noVAR | noFUN | noBO | noVO | noINFIXOP | noSTR | noASSIGN | noSqO;

		return true;
	}

    /////////////////////////////////////////////////
    /// \brief Determine, whether the current
    /// position is part of an assignment target
    /// (i.e. left of the assignment operator).
    /// Expects an opening or closing brace at the
    /// selected position.
    ///
    /// \param pos int
    /// \return bool
    ///
    /////////////////////////////////////////////////
	bool ParserTokenReader::IsLeftHandSide(int pos) const
	{
	    // Figure out, whether this is part of an assignment
        size_t p = 0;

        if (m_strFormula[pos] == '(' || m_strFormula[pos] == '[' || m_strFormula[pos] == '{')
            p = getMatchingParenthesis(m_strFormula.subview(pos));

        if (p == std::string::npos || pos+p+2 >= m_strFormula.length())
            return false;

        size_t next = m_strFormula.find_first_not_of(' ', p+1+pos);

        return next != std::string::npos
            && m_strFormula[next] == '='
            && m_strFormula[next+1] != '=';
	}

	//---------------------------------------------------------------------------
	/** \brief Create an error containing the parse error position.

	  This function will create an Parser Exception object containing the error text and its position.

	  \param a_iErrc [in] The error code of type #EErrorCodes.
	  \param a_iPos [in] The position where the error was detected.
	  \param a_strTok [in] The token string representation associated with the error.
	  \throw ParserException always throws thats the only purpose of this function.
	*/
	void  ParserTokenReader::Error( EErrorCodes a_iErrc,
									int a_iPos,
									const string_type& a_sTok) const
	{
		m_pParser->Error(a_iErrc, a_iPos, a_sTok);
	}

	//---------------------------------------------------------------------------
	void ParserTokenReader::SetArgSep(char_type cArgSep)
	{
		m_cArgSep = cArgSep;
	}

	//---------------------------------------------------------------------------
	char_type ParserTokenReader::GetArgSep() const
	{
		return m_cArgSep;
	}
} // namespace mu

