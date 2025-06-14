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
#ifndef MU_PARSER_BASE_H
#define MU_PARSER_BASE_H

//--- Standard includes ------------------------------------------------------------------------
#include <cmath>
#include <string>
#include <iostream>
#include <map>
#include <memory>
#include <locale>
#include <list>

//--- Parser includes --------------------------------------------------------------------------
#include "muParserDef.h"
#include "muParserStack.h"
#include "muParserTokenReader.h"
#include "muParserBytecode.h"
#include "muParserError.h"
#include "muParserState.hpp"
#include "muVarFactory.hpp"

class StringView;
class MutableStringView;

namespace mu
{
	/** \file
	    \brief This file contains the class definition of the muparser engine.
	*/

	//--------------------------------------------------------------------------------------------------
	/** \brief Mathematical expressions parser (base parser engine).
	    \author (C) 2012 Ingo Berg

	  This is the implementation of a bytecode based mathematical expressions parser.
	  The formula will be parsed from string and converted into a bytecode.
	  Future calculations will be done with the bytecode instead the formula string
	  resulting in a significant performance increase.
	  Complementary to a set of internally implemented functions the parser is able to handle
	  user defined functions and variables.
	*/
	class ParserBase
	{
		private:
			friend class ParserTokenReader;

			/** \brief Typedef for the parse functions.

			  The parse function do the actual work. The parser exchanges
			  the function pointer to the parser function depending on
			  which state it is in. (i.e. bytecode parser vs. string parser)
			*/
			typedef void (ParserBase::*ParseFunction)();

			/** \brief Type for a vector of strings. */
			typedef std::vector<string_type> stringbuf_type;

			/** \brief Typedef for the token reader. */
			typedef ParserTokenReader token_reader_type;

			/** \brief Type used for parser tokens. */
			typedef ParserToken token_type;

		public:

			/** \brief Type of the error class.

			  Included for backwards compatibility.
			*/
			typedef ParserError exception_type;

			void SetVarAliases(std::map<std::string, std::string>* aliases);

			// Bytecode caching and loop caching interface section
			void ActivateLoopMode(size_t _nLoopLength);
			void DeactivateLoopMode();
			void SetIndex(size_t _nLoopElement);
			void SetCompiling(bool _bCompiling = true);
			bool IsCompiling();
			void CacheCurrentAccess(const CachedDataAccess& _access);
			size_t HasCachedAccess();
			void DisableAccessCaching();
			bool CanCacheAccess();
			const CachedDataAccess& GetCachedAccess(size_t nthAccess);
			void CacheCurrentEquation(const std::string& sEquation);
			const std::string& GetCachedEquation() const;
			void CacheCurrentTarget(const std::string& sEquation);
			const std::string& GetCachedTarget() const;
			int IsValidByteCode(size_t _nthLoopElement = -1, size_t _nthPartEquation = 0);
			bool ActiveLoopMode() const;
			bool IsLockedPause() const;
			void LockPause(bool _bLock = true);
			void PauseLoopMode(bool _bPause = true);
			bool IsAlreadyParsed(StringView sNewEquation);
			bool IsNotLastStackItem() const;

			static void EnableDebugDump(bool bDumpCmd, bool bDumpStack);

			ParserBase();
			ParserBase(const ParserBase& a_Parser);
			ParserBase& operator=(const ParserBase& a_Parser);

			virtual ~ParserBase();

			const Array& Eval();
			const StackItem* Eval(int& nStackSize);

			void SetExpr(StringView a_sExpr);

			void SetDecSep(char_type cDecSep);
			void SetInitValue(const Value& init);
			void SetThousandsSep(char_type cThousandsSep = 0);
			void ResetLocale();

			void EnableOptimizer(bool a_bIsOn = true);
			void EnableBuiltInOprt(bool a_bIsOn = true);

			bool HasBuiltInOprt() const;
			void AddValIdent(identfun_type a_pCallback);

			/** \fn void mu::ParserBase::DefineFun(const string_type &a_strName, fun_type0 a_pFun, bool optimizeAway = true)
			    \brief Define a parser function without arguments.
			    \param a_strName Name of the function
			    \param a_pFun Pointer to the callback function
			    \param optimizeAway A flag indicating this function may be optimized
			*/
			template<typename T>
			void DefineFun(const string_type& a_strName, T a_pFun, bool optimizeAway = true, int numOpt = 0)
			{
				AddCallback( a_strName, ParserCallback(a_pFun, optimizeAway, numOpt), m_FunDef, ValidNameChars() );
			}

			void DefineOprt(const string_type& a_strName,
							fun_type2 a_pFun,
							unsigned a_iPri = 0,
							EOprtAssociativity a_eAssociativity = oaLEFT,
							bool optimizeAway = true);
			void DefineConst(const string_type& a_sName, Value a_fVal);
			Variable* CreateVar(const string_type& a_sName);
			void DefineVar(const string_type& a_sName, Variable* a_fVar);
			void DefinePostfixOprt(const string_type& a_strFun, fun_type1 a_pOprt, bool optimizeAway = true);
			void DefineInfixOprt(const string_type& a_strName, fun_type1 a_pOprt, int a_iPrec = prINFIX, bool optimizeAway = true);

			// Clear user defined variables, constants or functions
			void ClearVar();
			void ClearFun();
			void ClearConst();
			void ClearInfixOprt();
			void ClearPostfixOprt();
			void ClearOprt();

			void RemoveVar(const string_type& a_strVarName);
			const varmap_type& GetUsedVar();
			const varmap_type& GetVar() const;
			const varmap_type& GetInternalVars() const;
			const valmap_type& GetConst() const;
			const string_type& GetExpr() const;
			const funmap_type& GetFunDef() const;
			string_type GetVersion(EParserVersionInfo eInfo = pviFULL) const;

			const char_type** GetOprtDef() const;
			void DefineNameChars(const char_type* a_szCharset);
			void DefineOprtChars(const char_type* a_szCharset);
			void DefineInfixOprtChars(const char_type* a_szCharset);

			const char_type* ValidNameChars() const;
			const char_type* ValidOprtChars() const;
			const char_type* ValidInfixOprtChars() const;

			void SetArgSep(char_type cArgSep);
			char_type GetArgSep() const;

			void  Error(EErrorCodes a_iErrc,
						int a_iPos = (int)mu::string_type::npos,
						const string_type& a_strTok = string_type() ) const;
            void  Error(EErrorCodes a_iErrc,
                        const string_type& a_Expr,
						int a_iPos = (int)mu::string_type::npos,
						const string_type& a_strTok = string_type() ) const;

			std::string CreateTempVar(const Array& vVar);
			void SetInternalVar(const std::string& sVarName, const Array& vVar);
			Variable* GetInternalVar(const std::string& sVarName);
			void ClearInternalVars(bool bIgnoreProcedureVects = false);
			bool ContainsInternalVars(StringView sExpr, bool ignoreSingletons);
			bool ContainsStringVars(StringView sExpr);

		protected:

			void Init();

			virtual void InitCharSets() = 0;
			virtual void InitFun() = 0;
			virtual void InitConst() = 0;
			virtual void InitOprt() = 0;

			static const char_type* c_DefaultOprt[];
			static std::locale s_locale;  ///< The locale used by the parser
			static bool g_DbgDumpCmdCode;
			static bool g_DbgDumpStack;

			/** \brief A facet class used to change decimal and thousands separator. */
			template<class TChar>
			class change_dec_sep : public std::numpunct<TChar>
			{
				public:

					explicit change_dec_sep(char_type cDecSep, char_type cThousandsSep = 0, int nGroup = 3)
						: std::numpunct<TChar>()
						, m_nGroup(nGroup)
						, m_cDecPoint(cDecSep)
						, m_cThousandsSep(cThousandsSep)
					{}

				protected:

					virtual char_type do_decimal_point() const
					{
						return m_cDecPoint;
					}

					virtual char_type do_thousands_sep() const
					{
						return m_cThousandsSep;
					}

					virtual std::string do_grouping() const
					{
						return std::string(1, m_nGroup);
					}

				private:

					int m_nGroup;
					char_type m_cDecPoint;
					char_type m_cThousandsSep;
			};

			static Array evalIfElse(const Array& cond,
                                    const Array& true_case,
                                    const Array& false_case);
            static Array VectorCreate(const MultiArgFuncParams&);  // vector creation
            static Array Vector2Generator(const Array& firstVal,
                                          const Array& lastVal);
            static Array Vector3Generator(const Array& firstVal,
                                          const Array& incr,
                                          const Array& lastVal);
            static Array expandVector2(const Array& firstVal,
                                       const Array& lastVal);
            static Array expandVector3(const Array& firstVal,
                                       const Array& incr,
                                       const Array& lastVal);
            static void expandVector(std::complex<double> dFirst,
                                     const std::complex<double>& dLast,
                                     const std::complex<double>& dIncrement,
                                     Array& vResults);

		private:
			string_type getNextTempVarIndex();
			void Assign(const ParserBase& a_Parser);
			void InitTokenReader();
			void ReInit();
            //ExpressionTarget& getTarget() const;

			void AddCallback( const string_type& a_strName,
							  const ParserCallback& a_Callback,
							  funmap_type& a_Storage,
							  const char_type* a_szCharSet );

			void ApplyRemainingOprt(ParserStack<token_type>& a_stOpt,
									ParserStack<token_type>& a_stVal) const;
			void ApplyBinOprt(ParserStack<token_type>& a_stOpt,
							  ParserStack<token_type>& a_stVal) const;
			void ApplyVal2Str(ParserStack<token_type>& a_stOpt,
							  ParserStack<token_type>& a_stVal) const;

			void ApplyIfElse(ParserStack<token_type>& a_stOpt,
							 ParserStack<token_type>& a_stVal) const;

			void ApplyFunc(ParserStack<token_type>& a_stOpt,
						   ParserStack<token_type>& a_stVal,
						   int iArgCount) const;

			int GetOprtPrecedence(const token_type& a_Tok) const;
			EOprtAssociativity GetOprtAssociativity(const token_type& a_Tok) const;

			void CreateRPN();

			void ParseString();
			void ParseCmdCode();
			void ParseCmdCodeBulkParallel(size_t nVectorLength);

			void CheckName(const string_type& a_strName, const string_type& a_CharSet) const;
			void CheckOprt(const string_type& a_sName,
                           const ParserCallback& a_Callback,
                           const string_type& a_szCharSet) const;

			void StackDump(const ParserStack<token_type >& a_stVal,
						   const ParserStack<token_type >& a_stOprt) const;

			/** \brief Pointer to the parser function.

			  Eval() calls the function whose address is stored there.
			*/
			mutable ParseFunction  m_pParseFormula;
			mutable State m_compilingState;
			//mutable ExpressionTarget m_compilingTarget;
			mutable StateStacks m_stateStacks;
			State* m_state;

			mutable varmap_type mInternalVars;

			size_t nthLoopElement;
			size_t nthLoopPartEquation;
			size_t nCurrVectorIndex;
			bool bMakeLoopByteCode;
			bool bPauseLoopByteCode;
			bool bPauseLock;
			bool bCompiling;
			int nMaxThreads;

			std::unique_ptr<token_reader_type> m_pTokenReader; ///< Managed pointer to the token reader object.

			funmap_type  m_FunDef;         ///< Map of function names and pointers.
			funmap_type  m_PostOprtDef;    ///< Postfix operator callbacks
			funmap_type  m_InfixOprtDef;   ///< unary infix operator.
			funmap_type  m_OprtDef;        ///< Binary operator callbacks
			valmap_type  m_ConstDef;       ///< user constants.

			std::shared_ptr<VarFactory> m_factory;

			bool m_bBuiltInOp;             ///< Flag that can be used for switching built in operators on and off

			string_type m_sNameChars;      ///< Charset for names
			string_type m_sOprtChars;      ///< Charset for postfix/ binary operator tokens
			string_type m_sInfixOprtChars; ///< Charset for infix operator tokens

			mutable int m_nIfElseCounter;  ///< Internal counter for keeping track of nested if-then-else clauses

			// items merely used for caching state information
			const std::string EMPTYSTRING;
	};

} // namespace mu

#endif



