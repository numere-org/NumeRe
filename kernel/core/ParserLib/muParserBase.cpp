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

#include "muParserBase.h"
#include "muParserTemplateMagic.h"
#include "muHelpers.hpp"
#include "../utils/tools.hpp"
#include "../utils/timer.hpp"
#include "../structures.hpp"

//--- Standard includes ------------------------------------------------------------------------
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
#include <deque>
#include <sstream>
#include <locale>
#include <omp.h>

using namespace std;

size_t getMatchingParenthesis(const StringView&);




/** \file
    \brief This file contains the basic implementation of the muparser engine.
*/




namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Helper function to convert a complex
    /// vector to a real vector.
    ///
    /// \param vVec const std::vector<std::complex<double>>&
    /// \return std::vector<double>
    ///
    /////////////////////////////////////////////////
    std::vector<double> real(const std::vector<std::complex<double>>& vVec)
    {
	    std::vector<double> vReal;

	    for (const auto& val : vVec)
            vReal.push_back(val.real());

        return vReal;
    }


	std::locale ParserBase::s_locale = std::locale(std::locale::classic(), new change_dec_sep<char_type>('.'));

	bool ParserBase::g_DbgDumpCmdCode = false;
	bool ParserBase::g_DbgDumpStack = false;

	//------------------------------------------------------------------------------
	/** \brief Identifiers for built in binary operators.

	    When defining custom binary operators with #AddOprt(...) make sure not to choose
	    names conflicting with these definitions.
	*/
	const char_type* ParserBase::c_DefaultOprt[] =
	{
		"<=", ">=", "!=",
		"==", "<",  ">",
		"=",  "+=", "-=",
		"*=", "/=", "^=",
		"++", "--", "+",
		"-",  "*",  "/",
		"^",  "&&", "||",
		"(",  ")",  "{",
        "}",  "?",  ":", 0
	};

	//------------------------------------------------------------------------------
	/** \brief Constructor.
	    \throw ParserException if a_szFormula is null.
	*/
	ParserBase::ParserBase()
		: m_pParseFormula(&ParserBase::ParseString)
		, m_compilingState()
		, m_pTokenReader()
		, m_FunDef()
		, m_PostOprtDef()
		, m_InfixOprtDef()
		, m_OprtDef()
		, m_ConstDef()
		, m_bBuiltInOp(true)
		, m_sNameChars()
		, m_sOprtChars()
		, m_sInfixOprtChars()
		, m_nIfElseCounter(0)
	{
	    m_factory.reset(new VarFactory);
		InitTokenReader();
		nthLoopElement = 0;
		nthLoopPartEquation = 0;
		nCurrVectorIndex = 0;
		bMakeLoopByteCode = false;
		bPauseLoopByteCode = false;
		bPauseLock = false;
		m_state = &m_compilingState;
		nMaxThreads = omp_get_max_threads();// std::min(omp_get_max_threads(), s_MaxNumOpenMPThreads);
	}

	//---------------------------------------------------------------------------
	/** \brief Copy constructor.

	  Tha parser can be safely copy constructed but the bytecode is reset during
	  copy construction.
	*/
	ParserBase::ParserBase(const ParserBase& a_Parser) : ParserBase()
	{
		Assign(a_Parser);
	}

	//---------------------------------------------------------------------------
	ParserBase::~ParserBase()
	{
	    //
	}

	//---------------------------------------------------------------------------
	/** \brief Assignement operator.

	  Implemented by calling Assign(a_Parser). Self assignement is suppressed.
	  \param a_Parser Object to copy to this.
	  \return *this
	  \throw nothrow
	*/
	ParserBase& ParserBase::operator=(const ParserBase& a_Parser)
	{
		Assign(a_Parser);
		return *this;
	}

	//---------------------------------------------------------------------------
	/** \brief Copy state of a parser object to this.

	  Clears Variables and Functions of this parser.
	  Copies the states of all internal variables.
	  Resets parse function to string parse mode.

	  \param a_Parser the source object.
	*/
	void ParserBase::Assign(const ParserBase& a_Parser)
	{
		if (&a_Parser == this)
			return;

		// Don't copy bytecode instead cause the parser to create new bytecode
		// by resetting the parse function.
		ReInit();

		m_ConstDef = a_Parser.m_ConstDef;         // Copy user define constants
		m_bBuiltInOp = a_Parser.m_bBuiltInOp;
		m_nIfElseCounter = a_Parser.m_nIfElseCounter;
		m_factory = a_Parser.m_factory; // Get a reference to the original var factory
		m_pTokenReader.reset(a_Parser.m_pTokenReader->Clone(this)); // Needs the correct factory

		// Copy function and operator callbacks
		m_FunDef = a_Parser.m_FunDef;             // Copy function definitions
		m_PostOprtDef = a_Parser.m_PostOprtDef;   // post value unary operators
		m_InfixOprtDef = a_Parser.m_InfixOprtDef; // unary operators for infix notation
		m_OprtDef = a_Parser.m_OprtDef;           // binary operators

		m_sNameChars = a_Parser.m_sNameChars;
		m_sOprtChars = a_Parser.m_sOprtChars;
		m_sInfixOprtChars = a_Parser.m_sInfixOprtChars;
		nCurrVectorIndex = a_Parser.nCurrVectorIndex;
	}

	//---------------------------------------------------------------------------
	/** \brief Set the decimal separator.
	    \param cDecSep Decimal separator as a character value.
	    \sa SetThousandsSep

	    By default muparser uses the "C" locale. The decimal separator of this
	    locale is overwritten by the one provided here.
	*/
	void ParserBase::SetDecSep(char_type cDecSep)
	{
		char_type cThousandsSep = std::use_facet< change_dec_sep<char_type> >(s_locale).thousands_sep();
		s_locale = std::locale(std::locale("C"), new change_dec_sep<char_type>(cDecSep, cThousandsSep));
	}


    /////////////////////////////////////////////////
    /// \brief Change the default variable
    /// initializing value.
    ///
    /// \param init const Value&
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::SetInitValue(const Value& init)
	{
	    m_factory->SetInitValue(init);
	}


	//---------------------------------------------------------------------------
	/** \brief Sets the thousands operator.
	    \param cThousandsSep The thousands separator as a character
	    \sa SetDecSep

	    By default muparser uses the "C" locale. The thousands separator of this
	    locale is overwritten by the one provided here.
	*/
	void ParserBase::SetThousandsSep(char_type cThousandsSep)
	{
		char_type cDecSep = std::use_facet< change_dec_sep<char_type> >(s_locale).decimal_point();
		s_locale = std::locale(std::locale("C"), new change_dec_sep<char_type>(cDecSep, cThousandsSep));
	}

	//---------------------------------------------------------------------------
	/** \brief Resets the locale.

	  The default locale used "." as decimal separator, no thousands separator and
	  "," as function argument separator.
	*/
	void ParserBase::ResetLocale()
	{
		s_locale = std::locale(std::locale("C"), new change_dec_sep<char_type>('.'));
		SetArgSep(',');
	}

	//---------------------------------------------------------------------------
	/** \brief Initialize the token reader.

	  Create new token reader object and submit pointers to function, operator,
	  constant and variable definitions.

	  \post m_pTokenReader.get()!=0
	  \throw nothrow
	*/
	void ParserBase::InitTokenReader()
	{
		m_pTokenReader.reset(new token_reader_type(this));
	}

	//---------------------------------------------------------------------------
	/** \brief Reset parser to string parsing mode and clear internal buffers.

	    Clear bytecode, reset the token reader.
	    \throw nothrow
	*/
	void ParserBase::ReInit()
	{
		m_pParseFormula = &ParserBase::ParseString;
		m_compilingState.clear();
		m_pTokenReader->ReInit();
		m_nIfElseCounter = 0;
	}

    /////////////////////////////////////////////////
    /// \brief Simple state-considering wrapper
    /// around the ExpressionTarget structure.
    ///
    /// \return ExpressionTarget&
    ///
    /////////////////////////////////////////////////
	ExpressionTarget& ParserBase::getTarget() const
	{
	    if (bMakeLoopByteCode && !bPauseLoopByteCode)
            return m_stateStacks[nthLoopElement].m_target;

        return m_compilingTarget;
	}

	//---------------------------------------------------------------------------
	void ParserBase::OnDetectVar(string_type* pExpr, int& nStart, int& nEnd)
	{
		/*if (mInternalVars.size())
		{
			if (mInternalVars.find(pExpr->substr(nStart, nEnd - nStart)) != mInternalVars.end())
				return;

			Array vVar;

			if (GetVar().find(pExpr->substr(nStart, nEnd - nStart)) != GetVar().end())
				vVar = *(GetVar().find(pExpr->substr(nStart, nEnd - nStart))->second);
			else
				vVar.push_back(Value(Numerical(0.0)));

			SetVectorVar(pExpr->substr(nStart, nEnd - nStart), vVar);
		}*/
	}

	//---------------------------------------------------------------------------
	/** \brief Returns the version of muparser.
	    \param eInfo A flag indicating whether the full version info should be
	                 returned or not.

	  Format is as follows: "MAJOR.MINOR (COMPILER_FLAGS)" The COMPILER_FLAGS
	  are returned only if eInfo==pviFULL.
	*/
	string_type ParserBase::GetVersion(EParserVersionInfo eInfo) const
	{
		string_type sCompileTimeSettings;

		stringstream_type ss;

		ss << MUP_VERSION;

		if (eInfo == pviFULL)
		{
			ss << _nrT(" (") << MUP_VERSION_DATE;
			ss << std::dec << _nrT("; ") << sizeof(void*) * 8 << _nrT("BIT");

#ifdef _DEBUG
			ss << _nrT("; DEBUG");
#else
			ss << _nrT("; RELEASE");
#endif

#ifdef _UNICODE
			ss << _nrT("; UNICODE");
#else
#ifdef _MBCS
			ss << _nrT("; MBCS");
#else
			ss << _nrT("; ASCII");
#endif
#endif

#ifdef MUP_USE_OPENMP
			ss << _nrT("; OPENMP");
//#else
//      ss << _nrT("; NO_OPENMP");
#endif

#if defined(MUP_MATH_EXCEPTIONS)
			ss << _nrT("; MATHEXC");
//#else
//      ss << _nrT("; NO_MATHEXC");
#endif

			ss << _nrT(")");
		}

		return ss.str();
	}

	//---------------------------------------------------------------------------
	/** \brief Add a value parsing function.

	    When parsing an expression muParser tries to detect values in the expression
	    string using different valident callbacks. Thuis it's possible to parse
	    for hex values, binary values and floating point values.
	*/
	void ParserBase::AddValIdent(identfun_type a_pCallback)
	{
		m_pTokenReader->AddValIdent(a_pCallback);
	}

	//---------------------------------------------------------------------------
	/** \brief Add a function or operator callback to the parser. */
	void ParserBase::AddCallback( const string_type& a_strName,
								  const ParserCallback& a_Callback,
								  funmap_type& a_Storage,
								  const char_type* a_szCharSet )
	{
		if (a_Callback.GetAddr() == 0)
			Error(ecINVALID_FUN_PTR);

		const funmap_type* pFunMap = &a_Storage;

		// Check for conflicting operator or function names
		if ( pFunMap != &m_FunDef && m_FunDef.find(a_strName) != m_FunDef.end() )
			Error(ecNAME_CONFLICT, -1, a_strName);

        if ( pFunMap != &m_PostOprtDef && pFunMap != &m_InfixOprtDef && m_PostOprtDef.find(a_strName) != m_PostOprtDef.end() )
            Error(ecNAME_CONFLICT, -1, a_strName);

		if ( pFunMap != &m_InfixOprtDef && pFunMap != &m_OprtDef && pFunMap != &m_PostOprtDef && m_InfixOprtDef.find(a_strName) != m_InfixOprtDef.end() )
			Error(ecNAME_CONFLICT, -1, a_strName);

		if ( pFunMap != &m_InfixOprtDef && pFunMap != &m_OprtDef && m_OprtDef.find(a_strName) != m_OprtDef.end() )
			Error(ecNAME_CONFLICT, -1, a_strName);

		CheckOprt(a_strName, a_Callback, a_szCharSet);
		a_Storage[a_strName] = a_Callback;
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief Check if a name contains invalid characters.

	    \throw ParserException if the name contains invalid charakters.
	*/
	void ParserBase::CheckOprt(const string_type& a_sName,
							   const ParserCallback& a_Callback,
							   const string_type& a_szCharSet) const
	{
		if ( !a_sName.length() ||
				(a_sName.find_first_not_of(a_szCharSet) != string_type::npos) ||
				(a_sName[0] >= '0' && a_sName[0] <= '9'))
		{
			switch (a_Callback.GetCode())
			{
				case cmOPRT_POSTFIX:
					Error(ecINVALID_POSTFIX_IDENT, -1, a_sName);
				case cmOPRT_INFIX:
					Error(ecINVALID_INFIX_IDENT, -1, a_sName);
				default:
					Error(ecINVALID_NAME, -1, a_sName);
			}
		}
	}

	//---------------------------------------------------------------------------
	/** \brief Check if a name contains invalid characters.

	    \throw ParserException if the name contains invalid charakters.
	*/
	void ParserBase::CheckName(const string_type& a_sName,
							   const string_type& a_szCharSet) const
	{
		if ( !a_sName.length() ||
				(a_sName.find_first_not_of(a_szCharSet) != string_type::npos) ||
				(a_sName[0] >= '0' && a_sName[0] <= '9'))
		{
			Error(ecINVALID_NAME);
		}
	}


    /////////////////////////////////////////////////
    /// \brief Set the expression. Triggers first
    /// time calculation thus the creation of the
    /// bytecode and scanning of used variables.
    ///
    /// \param a_sExpr StringView
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::SetExpr(StringView a_sExpr)
	{
		// Now check, whether the pre-evaluated formula was already parsed into the bytecode
		// -> Return, if that is true
		// -> Invalidate the bytecode for this formula, if necessary
		if (IsAlreadyParsed(a_sExpr))
			return;
		else if (bMakeLoopByteCode
                 && !bPauseLoopByteCode
                 && this->GetExpr().length()
                 && m_state->m_valid)
			m_state->m_valid = 0;

        // Pass the formula to the token reader
		m_pTokenReader->SetFormula(a_sExpr);

		// Re-initialize the parser
		ReInit();

		// Convert the string directly to bytecode
		ParseString();
	}


    /////////////////////////////////////////////////
    /// \brief This function pre-evaluates all
    /// vectors, which are contained in the
    /// expression passed through sExpr.
    ///
    /// \param sExpr MutableStringView containing the
    /// expression
    /// \return MutableStringView
    ///
    /////////////////////////////////////////////////
	MutableStringView ParserBase::compileVectors(MutableStringView sExpr)
	{
		std::vector<Array> vResults;

		// Resolve vectors, which are part of a multi-argument
		// function's parentheses
		for (auto iter = mInternalVars.begin(); iter != mInternalVars.end(); ++iter)
        {
            size_t match = 0;

            if (iter->second->size() == 1)
                continue;

            while ((match = sExpr.find(iter->first, match)) != string::npos)
            {
                if (!match || sExpr.is_delimited_sequence(match, iter->first.length(), StringViewBase::PARSER_DELIMITER))
                    compileVectorsInMultiArgFunc(sExpr, match);

                match++;
            }
        }

		// Walk through the whole expression
		for (size_t i = 0; i < sExpr.length(); i++)
		{
		    // search for vector parentheses
			if (sExpr[i] == '{' && (!i || isDelimiter(sExpr[i-1])) && sExpr.find('}', i) != string::npos)
			{
			    if (compileVectorsInMultiArgFunc(sExpr, i))
                    continue;

                vResults.clear();

                // Find the matching brace for the current vector brace
				size_t j = getMatchingParenthesis(sExpr.subview(i));

				if (j != std::string::npos)
					j += i; // if one is found, add the current position

                if (i+1 == j) // This is an empty brace
                    sExpr.replace(i, 2, "nan");
			    else if (j != std::string::npos && sExpr.subview(i, j - i).find(':') != std::string::npos)
				{
				    // This is vector expansion: e.g. "{1:10}"
					// Store the result in a new temporary vector
                    string sVectorVarName = CreateTempVar(vResults.front());

				    // Get the expression and evaluate the expansion
					compileVectorExpansion(sExpr.subview(i + 1, j - i - 1), sVectorVarName);

					sExpr.replace(i, j + 1 - i, sVectorVarName + " "); // Whitespace for constructs like {a:b}i
				}
				else
				{
					if (j != std::string::npos)
					{
					    // This is a normal vector, e.g. "{1,2,3}"
					    // Set the sub expression and evaluate it
						SetExpr(sExpr.subview(i + 1, j - i - 1));

						// Determine, whether the current vector is a target vector or not
						if (sExpr.find_first_not_of(' ') == i
								&& sExpr.find('=', j) != std::string::npos
								&& sExpr.find('=', j) < sExpr.length() - 1
								&& sExpr.find('!', j) != sExpr.find('=', j) - 1
								&& sExpr.find('<', j) != sExpr.find('=', j) - 1
								&& sExpr.find('>', j) != sExpr.find('=', j) - 1
								&& sExpr[sExpr.find('=', j) + 1] != '=')
						{
						    // This is a target vector
#warning FIXME (numere#3#07/08/24): Target vectors do not work yet -> NEW ISSUE
                            int nResults;
                            Array* v = Eval(nResults);
                            // Store the results in the target vector
                            vResults.insert(vResults.end(), v, v+nResults);
						    // Store the variable names
						    getTarget().create(sExpr.subview(i + 1, j - i - 1), m_pTokenReader->GetUsedVar());
							SetInternalVar("_~TRGTVCT[~]", vResults.front());
							sExpr.replace(i, j + 1 - i, "_~TRGTVCT[~]");
						}
						else
						{
						    // This is a usual vector
						    // Create a new temporary vector name
                            std::string sVectorVarName = CreateTempVar(vResults.front());
                            m_compilingState.m_vectEval.create(sVectorVarName);
                            int nResults;
                            // Calculate and store the results in the target vector
                            Eval(nResults);
						    // Update the expression
							sExpr.replace(i, j + 1 - i, sVectorVarName + " "); // Whitespace for constructs like {a,b}i
						}
					}
				}
			}

			if (sExpr.subview(i, 9) == "logtoidx(" || sExpr.subview(i, 9) == "idxtolog(")
            {
                i += 8;
                compileVectorsInMultiArgFunc(sExpr, i);
            }
		}

		return sExpr;
	}


    /////////////////////////////////////////////////
    /// \brief This function evaluates the vector
    /// expansion, e.g. "{1:4}" = {1, 2, 3, 4}.
    ///
    /// \param sSubExpr MutableStringView
    /// \param sVectorVarName const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::compileVectorExpansion(MutableStringView sSubExpr, const std::string& sVectorVarName)
	{
		int nResults = 0;

		EndlessVector<StringView> args = getAllArguments(sSubExpr);
		std::vector<int> vComponentType;
		std::string sCompiledExpression;
		const int SINGLETON = 1;

		// Determine the type of every part of the vector brace
		for (size_t n = 0; n < args.size(); n++)
        {
            if (args[n].find(':') == std::string::npos)
            {
                vComponentType.push_back(SINGLETON);

                if (sCompiledExpression.length())
                    sCompiledExpression += ",";

                sCompiledExpression += args[n].to_string();
            }
            else
            {
                int isExpansion = -1;
                MutableStringView sExpansion = args[n].make_mutable();

                // Replace the colons with commas. But ensure that this is not a conditional statement
                for (size_t i = 0; i < sExpansion.length(); i++)
                {
                    if (sExpansion[i] == '(' || sExpansion[i] == '[' || sExpansion[i] == '{')
                        i += getMatchingParenthesis(sSubExpr.subview(i));

                    if (sExpansion[i] == ':')
                    {
                        if (isExpansion == -1)
                            isExpansion = 1;

                        // This is a conditional operator
                        if (isExpansion == 0)
                            continue;

                        sExpansion[i] = ',';
                    }

                    // This is a conditional operator
                    if (sExpansion[i] == '?')
                    {
                        if (isExpansion == -1)
                            isExpansion = 0;

                        if (isExpansion == 1)
                            throw ParserError(ecUNEXPECTED_CONDITIONAL, "?", sExpansion.to_string(), i);
                    }
                }

                if (sCompiledExpression.length())
                    sCompiledExpression += ",";

                sCompiledExpression += sExpansion.to_string();
                vComponentType.push_back(getAllArguments(sExpansion).size());
            }
        }

		// Evaluate
		SetExpr(sCompiledExpression);
		m_compilingState.m_vectEval.create(sVectorVarName, vComponentType);
		Eval(nResults);
	}


    /////////////////////////////////////////////////
    /// \brief Implements the inline conditional
    /// operator.
    ///
    /// \param cond const Array&
    /// \param true_case const Array&
    /// \param false_case const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array ParserBase::evalIfElse(const Array& cond, const Array& true_case, const Array& false_case)
    {
        size_t elems = std::max({cond.size(), true_case.size(), false_case.size()});
        Array ret(elems);

        for (size_t i = 0; i < elems; i++)
        {
            ret.get(i) = cond.get(i) ? true_case.get(i) : false_case.get(i);
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Internal alias function to construct a
    /// vector from a list of elements.
    ///
    /// \param arrs const mu::Array*
    /// \param n int
    /// \return mu::Array
    ///
    /////////////////////////////////////////////////
    Array ParserBase::VectorCreate(const Array* arrs, int n)
    {
        // If no arguments have been passed, we simply
        // return void
        if (!n)
            return mu::Value();

        mu::Array res;

        for (int i = 0; i < n; i++)
        {
            if (arrs[i].isScalar())
                res.push_back(arrs[i].front());
            else if (arrs[i].getCommonType() == TYPE_GENERATOR)
            {
                Array ret;

                if (arrs[i].size() == 2)
                    ret = expandVector2(arrs[i].get(0), arrs[i].get(1));
                else if (arrs[i].size() == 3)
                    ret = expandVector3(arrs[i].get(0), arrs[i].get(1), arrs[i].get(2));

                res.insert(res.end(), ret.begin(), ret.end());
            }
            else
                res.push_back(arrs[i]);
        }

        return res;
    }


    /////////////////////////////////////////////////
    /// \brief Determines, whether the passed step is
    /// still in valid range and therefore can be
    /// done to expand the vector.
    ///
    /// \param current const std::complex<double>&
    /// \param last const std::complex<double>&
    /// \param d const std::complex<double>&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    static bool stepIsStillPossible(const std::complex<double>& current, const std::complex<double>& last, const std::complex<double>& d)
	{
	    std::complex<double> fact(d.real() >= 0.0 ? 1.0 : -1.0, d.imag() >= 0.0 ? 1.0 : -1.0);

	    return (current.real() * fact.real()) <= (last.real() * fact.real())
            && (current.imag() * fact.imag()) <= (last.imag() * fact.imag());
	}


    /////////////////////////////////////////////////
    /// \brief This function creates a vector
    /// generator for two indices.
    ///
    /// \param firstVal const Array&
    /// \param lastVal const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
	Array ParserBase::Vector2Generator(const Array& firstVal, const Array& lastVal)
	{
	    return Array(firstVal, lastVal);
	}


    /////////////////////////////////////////////////
    /// \brief This function creates a vector
    /// generator for three indices.
    ///
    /// \param firstVal const Array&
    /// \param incr const Array&
    /// \param lastVal const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
	Array ParserBase::Vector3Generator(const Array& firstVal, const Array& incr, const Array& lastVal)
	{
	    return Array(firstVal, incr, lastVal);
	}


   /////////////////////////////////////////////////
    /// \brief This function expands the vector from
    /// two indices.
    ///
    /// \param firstVal const Array&
    /// \param lastVal const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
	Array ParserBase::expandVector2(const Array& firstVal, const Array& lastVal)
	{
	    Array ret;
        Array diff = lastVal - firstVal;

        for (size_t v = 0; v < diff.size(); v++)
        {
            std::complex<double> d = diff[v].getNum().asCF64();
            d.real(d.real() > 0.0 ? 1.0 : (d.real() < 0.0 ? -1.0 : 0.0));
            d.imag(d.imag() > 0.0 ? 1.0 : (d.imag() < 0.0 ? -1.0 : 0.0));
            expandVector(firstVal.get(v).getNum().asCF64(),
                         lastVal.get(v).getNum().asCF64(),
                         d,
                         ret);
        }

        return ret;
	}


    /////////////////////////////////////////////////
    /// \brief This function expands the vector from
    /// three indices.
    ///
    /// \param firstVal const Array&
    /// \param incr const Array&
    /// \param lastVal const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
	Array ParserBase::expandVector3(const Array& firstVal, const Array& incr, const Array& lastVal)
	{
	    Array ret;

        for (size_t v = 0; v < std::max({firstVal.size(), lastVal.size(), incr.size()}); v++)
        {
            expandVector(firstVal.get(v).getNum().asCF64(),
                         lastVal.get(v).getNum().asCF64(),
                         incr[v].getNum().asCF64(),
                         ret);
        }

        return ret;
	}


    /////////////////////////////////////////////////
    /// \brief This function expands the vector.
    /// Private member used by
    /// ParserBase::compileVectorExpansion().
    ///
    /// \param dFirst std::complex<double>
    /// \param dLast const std::complex<double>&
    /// \param dIncrement const std::complex<double>&
    /// \param vResults vector<std::complex<double>>&
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::expandVector(std::complex<double> dFirst, const std::complex<double>& dLast, const std::complex<double>& dIncrement, Array& vResults)
	{
		// ignore impossible combinations. Store only
		// the accessible value
		if ((dFirst.real() < dLast.real() && dIncrement.real() < 0)
            || (dFirst.imag() < dLast.imag() && dIncrement.imag() < 0)
            || (dFirst.real() > dLast.real() && dIncrement.real() > 0)
            || (dFirst.imag() > dLast.imag() && dIncrement.imag() > 0)
            || dIncrement == 0.0)
		{
			vResults.push_back(Numerical::autoType(dFirst));
			return;
		}

		// Store the first value
		vResults.push_back(Numerical::autoType(dFirst));

		// As long as the next step is possible, add the increment
		while (stepIsStillPossible(dFirst+dIncrement, dLast+1e-10*dIncrement, dIncrement))
        {
            dFirst += dIncrement;

            if (dFirst.real()*dIncrement.real() > dLast.real()*dIncrement.real())
                dFirst.real(dLast.real());

            if (dFirst.imag()*dIncrement.real() > dLast.imag()*dIncrement.real())
                dFirst.imag(dLast.imag());

            vResults.push_back(Numerical::autoType(dFirst));
        }
	}


    /////////////////////////////////////////////////
    /// \brief This private function will try to find
    /// a surrounding multi-argument function,
    /// resolve the arguments, apply the function and
    /// store the result as a new vector.
    ///
    /// \param sExpr MutableStringView&
    /// \param nPos size_t&
    /// \return bool
    ///
    /////////////////////////////////////////////////
	bool ParserBase::compileVectorsInMultiArgFunc(MutableStringView& sExpr, size_t& nPos)
	{
        string sMultiArgFunc;
	    // Try to find a multi-argument function. The size_t will store the start position of the function name
        size_t nMultiArgParens = FindMultiArgFunc(sExpr, nPos, sMultiArgFunc);

        if (nMultiArgParens != std::string::npos)
        {
            // This is part of a multi-argument function
            // Find the matching parenthesis for the multi-argument function
            size_t nClosingParens = getMatchingParenthesis(sExpr.subview(nMultiArgParens)) + nMultiArgParens;

            // Set the argument of the function as expression and evaluate it recursively
            std::vector<Array> vResults;
            int nResults;
            string sVectorVarName = CreateTempVar(vResults.front());
            SetExpr(sExpr.subview(nMultiArgParens + 1, nClosingParens - nMultiArgParens - 1));
            m_compilingState.m_vectEval.create(sVectorVarName, sMultiArgFunc);
            Eval(nResults);

            // Store the result in a new temporary vector
            sExpr.replace(nMultiArgParens - sMultiArgFunc.length(),
                          nClosingParens - nMultiArgParens + 1 + sMultiArgFunc.length(),
                          sVectorVarName);

            // Set the position to the start of the multi-argument
            // function to avoid jumping over consecutive vectors
            nPos = nMultiArgParens-sMultiArgFunc.length();

            return true;
        }

        return false;
	}


    /////////////////////////////////////////////////
    /// \brief This function searches for the first
    /// multi-argument function found in the passed
    /// expression.
    ///
    /// \param sExpr StringView
    /// \param nPos size_t
    /// \param sMultArgFunc std::string& will contain
    /// the name of the function
    /// \return size_t the position of the opening
    /// parenthesis
    ///
    /////////////////////////////////////////////////
	size_t ParserBase::FindMultiArgFunc(StringView sExpr, size_t nPos, std::string& sMultArgFunc)
	{
	    // Walk through the expression
		for (int i = nPos; i >= 2; i--)
		{
		    // If there's a parenthesis and the character before is alphabetic
			if (sExpr[i] == '(' && isalpha(sExpr[i - 1]))
			{
			    // Get the matching parenthesis
				size_t nParPos = getMatchingParenthesis(sExpr.subview(i));

				if (nParPos == string::npos)
					return std::string::npos;
				else
					nParPos += i;

                // Ignore all results before nPos
				if (nParPos < nPos)
					continue;

                // Find the last character before the alphabetic part
				size_t nSep = sExpr.find_last_of(" +-*/(=:?&|<>!%{^,", i - 1) + 1;
				// Extract the function
				std::string sFunc = sExpr.subview(nSep, i - nSep).to_string();

				// Exclude the following functions
                if (sFunc == "polynomial"
                    || sFunc == "perlin"
                    || sFunc == "as_date"
                    || sFunc == "as_time")
                    continue;
                else if (sFunc == "logtoidx" || sFunc == "idxtolog")
                {
                    sMultArgFunc = sFunc;
                    return i;
                }

				// Compare the function with the set of known multi-argument functions
				auto iter = m_FunDef.find(sFunc);

				if (iter != m_FunDef.end() && iter->second.GetArgc() == -1)
				{
				    // If the function is a multi-argument function, store it and return the position
					sMultArgFunc = sFunc;
					return i;
				}
			}
		}

		// Return string::npos if nothing was found
		return std::string::npos;
	}

	//---------------------------------------------------------------------------
	/** \brief Get the default symbols used for the built in operators.
	    \sa c_DefaultOprt
	*/
	const char_type** ParserBase::GetOprtDef() const
	{
		return (const char_type**)(&c_DefaultOprt[0]);
	}

	//---------------------------------------------------------------------------
	/** \brief Define the set of valid characters to be used in names of
	           functions, variables, constants.
	*/
	void ParserBase::DefineNameChars(const char_type* a_szCharset)
	{
		m_sNameChars = a_szCharset;
	}

	//---------------------------------------------------------------------------
	/** \brief Define the set of valid characters to be used in names of
	           binary operators and postfix operators.
	*/
	void ParserBase::DefineOprtChars(const char_type* a_szCharset)
	{
		m_sOprtChars = a_szCharset;
	}

	//---------------------------------------------------------------------------
	/** \brief Define the set of valid characters to be used in names of
	           infix operators.
	*/
	void ParserBase::DefineInfixOprtChars(const char_type* a_szCharset)
	{
		m_sInfixOprtChars = a_szCharset;
	}

	//---------------------------------------------------------------------------
	/** \brief Virtual function that defines the characters allowed in name identifiers.
	    \sa #ValidOprtChars, #ValidPrefixOprtChars
	*/
	const char_type* ParserBase::ValidNameChars() const
	{
		assert(m_sNameChars.size());
		return m_sNameChars.c_str();
	}

	//---------------------------------------------------------------------------
	/** \brief Virtual function that defines the characters allowed in operator definitions.
	    \sa #ValidNameChars, #ValidPrefixOprtChars
	*/
	const char_type* ParserBase::ValidOprtChars() const
	{
		assert(m_sOprtChars.size());
		return m_sOprtChars.c_str();
	}

	//---------------------------------------------------------------------------
	/** \brief Virtual function that defines the characters allowed in infix operator definitions.
	    \sa #ValidNameChars, #ValidOprtChars
	*/
	const char_type* ParserBase::ValidInfixOprtChars() const
	{
		assert(m_sInfixOprtChars.size());
		return m_sInfixOprtChars.c_str();
	}

	//---------------------------------------------------------------------------
	/** \brief Add a user defined operator.
	    \post Will reset the Parser to string parsing mode.
	*/
	void ParserBase::DefinePostfixOprt(const string_type& a_sName,
									   fun_type1 a_pFun,
									   bool optimizeAway)
	{
		AddCallback(a_sName,
					ParserCallback(a_pFun, optimizeAway, 0, prPOSTFIX, cmOPRT_POSTFIX),
					m_PostOprtDef,
					ValidOprtChars() );
	}

	//---------------------------------------------------------------------------
	/** \brief Initialize user defined functions.

	  Calls the virtual functions InitFun(), InitConst() and InitOprt().
	*/
	void ParserBase::Init()
	{
		InitCharSets();
		InitFun();
		InitConst();
		InitOprt();
	}

	//---------------------------------------------------------------------------
	/** \brief Add a user defined operator.
	    \post Will reset the Parser to string parsing mode.
	    \param [in] a_sName  operator Identifier
	    \param [in] a_pFun  Operator callback function
	    \param [in] a_iPrec  Operator Precedence (default=prSIGN)
	    \param [in] optimizeAway  True if operator is volatile (default=false)
	    \sa EPrec
	*/
	void ParserBase::DefineInfixOprt(const string_type& a_sName,
									 fun_type1 a_pFun,
									 int a_iPrec,
									 bool optimizeAway)
	{
		AddCallback(a_sName,
					ParserCallback(a_pFun, optimizeAway, 0, a_iPrec, cmOPRT_INFIX),
					m_InfixOprtDef,
					ValidInfixOprtChars() );
	}


	//---------------------------------------------------------------------------
	/** \brief Define a binary operator.
	    \param [in] a_sName The identifier of the operator.
	    \param [in] a_pFun Pointer to the callback function.
	    \param [in] a_iPrec Precedence of the operator.
	    \param [in] a_eAssociativity The associativity of the operator.
	    \param [in] a_bAllowOpt If this is true the operator may be optimized away.

	    Adds a new Binary operator the the parser instance.
	*/
	void ParserBase::DefineOprt( const string_type& a_sName,
								 fun_type2 a_pFun,
								 unsigned a_iPrec,
								 EOprtAssociativity a_eAssociativity,
								 bool optimizeAway )
	{
		// Check for conflicts with built in operator names
		for (int i = 0; m_bBuiltInOp && i < cmENDIF; ++i)
			if (a_sName == string_type(c_DefaultOprt[i]))
				Error(ecBUILTIN_OVERLOAD, -1, a_sName);

		AddCallback(a_sName,
					ParserCallback(a_pFun, optimizeAway, a_iPrec, a_eAssociativity),
					m_OprtDef,
					ValidOprtChars() );
	}


    /////////////////////////////////////////////////
    /// \brief Create a new variable within the
    /// parser, which is handled by the parser's
    /// varfactory.
    ///
    /// \param a_sName const string_type&
    /// \return Variable*
    ///
    /////////////////////////////////////////////////
	Variable* ParserBase::CreateVar(const string_type& a_sName)
	{
	    // Test if a constant with that names already exists
		if (m_ConstDef.find(a_sName) != m_ConstDef.end())
			Error(ecNAME_CONFLICT);

		CheckName(a_sName, ValidNameChars());

		bool needsReInit = m_factory->m_VarDef.find(a_sName) != m_factory->m_VarDef.end();

	    Variable* var = m_factory->Create(a_sName);

	    if (needsReInit)
            ReInit();

	    return var;
	}


	//---------------------------------------------------------------------------
	/** \brief Add a user defined variable and without taking the ownership.
	    \param [in] a_sName the variable name
	    \param [in] a_pVar A pointer to the variable
	    \post Will reset the Parser to string parsing mode.
	    \throw ParserException in case the name contains invalid signs or a_pVar is NULL.
	*/
	void ParserBase::DefineVar(const string_type& a_sName, Variable* a_pVar)
	{
		if (a_pVar == 0)
			Error(ecINVALID_VAR_PTR);

		// Test if a constant with that names already exists
		if (m_ConstDef.find(a_sName) != m_ConstDef.end())
			Error(ecNAME_CONFLICT);

		CheckName(a_sName, ValidNameChars());

		if (m_factory->Add(a_sName, a_pVar))
            ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief Add a user defined constant.
	    \param [in] a_sName The name of the constant.
	    \param [in] a_fVal the value of the constant.
	    \post Will reset the Parser to string parsing mode.
	    \throw ParserException in case the name contains invalid signs.
	*/
	void ParserBase::DefineConst(const string_type& a_sName, Value a_fVal)
	{
		CheckName(a_sName, ValidNameChars());
		m_ConstDef[a_sName] = a_fVal;
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief Get operator priority.
	    \throw ParserException if a_Oprt is no operator code
	*/
	int ParserBase::GetOprtPrecedence(const token_type& a_Tok) const
	{
		switch (a_Tok.GetCode())
		{
			// built in operators
			case cmEND:
				return -5;
			case cmARG_SEP:
				return -4;
			case cmEXP2: // Vector expansions have higher priorities than argument separators
			case cmEXP3:
				return -3;
			case cmASSIGN:
			case cmADDASGN:
			case cmSUBASGN:
			case cmMULASGN:
			case cmDIVASGN:
			case cmPOWASGN:
				return -1;
			case cmELSE:
			case cmIF:
				return  0;
			case cmLAND:
				return  prLAND;
			case cmLOR:
				return  prLOR;
			case cmLT:
			case cmGT:
			case cmLE:
			case cmGE:
			case cmNEQ:
			case cmEQ:
				return  prCMP;
			case cmADD:
			case cmSUB:
				return  prADD_SUB;
			case cmMUL:
			case cmDIV:
				return  prMUL_DIV;
			case cmPOW:
				return  prPOW;
            case cmVAL2STR:
                return prINFIX;

			// user defined binary operators
			case cmOPRT_INFIX:
			case cmOPRT_BIN:
				return a_Tok.GetPri();
			default:
				Error(ecINTERNAL_ERROR, 5);
				return 999;
		}
	}

	//---------------------------------------------------------------------------
	/** \brief Get operator priority.
	    \throw ParserException if a_Oprt is no operator code
	*/
	EOprtAssociativity ParserBase::GetOprtAssociativity(const token_type& a_Tok) const
	{
		switch (a_Tok.GetCode())
		{
			case cmASSIGN:
			case cmADDASGN:
			case cmSUBASGN:
			case cmMULASGN:
			case cmDIVASGN:
			case cmPOWASGN:
			case cmINCR:
			case cmDECR:
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
				return oaLEFT;
			case cmPOW:
				return oaRIGHT;
			case cmOPRT_BIN:
				return a_Tok.GetAssociativity();
			default:
				return oaNONE;
		}
	}

	//---------------------------------------------------------------------------
	/** \brief Return a map containing the used variables only. */
	const varmap_type& ParserBase::GetUsedVar()
	{
		/*try
		{
			m_pTokenReader->IgnoreUndefVar(true);
			CreateRPN(); // try to create bytecode, but don't use it for any further calculations since it
			// may contain references to nonexisting variables.
			m_pParseFormula = &ParserBase::ParseString;
			m_pTokenReader->IgnoreUndefVar(false);
		}
		catch (exception_type& e)
		{
			// Make sure to stay in string parse mode, dont call ReInit()
			// because it deletes the array with the used variables
			m_pParseFormula = &ParserBase::ParseString;
			m_pTokenReader->IgnoreUndefVar(false);
			throw;
		}*/

		return m_pTokenReader->GetUsedVar();
	}

	//---------------------------------------------------------------------------
	/** \brief Return a map containing the used variables only. */
	const varmap_type& ParserBase::GetVar() const
	{
		return m_factory->m_VarDef;
	}

	//---------------------------------------------------------------------------
	/** \brief Return a map containing all parser constants. */
	const valmap_type& ParserBase::GetConst() const
	{
		return m_ConstDef;
	}

	//---------------------------------------------------------------------------
	/** \brief Return prototypes of all parser functions.
	    \return #m_FunDef
	    \sa FunProt
	    \throw nothrow

	    The return type is a map of the public type #funmap_type containing the prototype
	    definitions for all numerical parser functions. String functions are not part of
	    this map. The Prototype definition is encapsulated in objects of the class FunProt
	    one per parser function each associated with function names via a map construct.
	*/
	const funmap_type& ParserBase::GetFunDef() const
	{
		return m_FunDef;
	}

    /////////////////////////////////////////////////
    /// \brief Return a reference to the internal
    /// variables.
    ///
    /// \return const varmap_type&
    ///
    /////////////////////////////////////////////////
	const varmap_type& ParserBase::GetInternalVars() const
	{
	    return mInternalVars;
	}

	//---------------------------------------------------------------------------
	/** \brief Retrieve the formula. */
	const string_type& ParserBase::GetExpr() const
	{
	    if (g_DbgDumpStack)
            print("Current Eq: \"" + m_state->m_expr + "\"");

	    return m_state->m_expr;
	}

	//---------------------------------------------------------------------------
	/** \brief Apply a function token.
	    \param iArgCount int Number of Arguments actually gathered used only for multiarg functions.
	    \post The result is pushed to the value stack
	    \post The function token is removed from the stack
	    \throw exception_type if Argument count does not mach function requirements.
	*/
	void ParserBase::ApplyFunc(ParserStack<token_type>& a_stOpt,
                               ParserStack<token_type>& a_stVal,
                               int a_iArgCount) const
	{
		assert(m_pTokenReader.get());
		static ParserCallback ValidZeroArgument = m_FunDef.at(MU_VECTOR_CREATE);

		// Operator stack empty or does not contain tokens with callback functions
		if (a_stOpt.empty() || (a_stOpt.top().GetFuncAddr() == 0 && a_stOpt.top().GetCode() != cmMETHOD))
			return;

		token_type funTok = a_stOpt.pop();
		assert(funTok.GetFuncAddr() || funTok.GetCode() == cmMETHOD);

		// Binary operators must rely on their internal operator number
		// since counting of operators relies on commas for function arguments
		// binary operators do not have commas in their expression
		int iArgCount = (funTok.GetCode() == cmOPRT_BIN) ? funTok.GetArgCount() : a_iArgCount;

		int iArgRequired = iArgCount;
        int iArgOptional = 0;

        // In case of a function (not a method) evaluate the actual necessary arguments
		if (funTok.GetCode() != cmMETHOD)
        {
            // determine how many parameters the function needs. To remember iArgCount includes the
            // string parameter whilst GetArgCount() counts only numeric parameters.
            iArgRequired = funTok.GetArgCount();
            iArgOptional = funTok.GetOptArgCount();

            if (funTok.GetArgCount() >= 0 && iArgCount > iArgRequired)
                Error(ecTOO_MANY_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());

            if (funTok.GetCode() != cmOPRT_BIN && iArgCount < iArgRequired - iArgOptional)
                Error(ecTOO_FEW_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());
        }

		// Collect the numeric function arguments from the value stack and store them
		// in a vector
		std::vector<token_type> stArg;

		for (int i = 0; i < iArgCount+(funTok.GetCode() == cmMETHOD); ++i) // Methods have to remove their root element as well
		{
			stArg.push_back(std::move(a_stVal.pop()));
			if (stArg.back().GetType() == tpSTR && funTok.GetType() != tpSTR)
				Error(ecVAL_EXPECTED, m_pTokenReader->GetPos(), funTok.GetAsString());
		}

		if (iArgCount < iArgRequired)
        {
            int added = 0;
            while (iArgCount+added < iArgRequired)
            {
                m_compilingState.m_byteCode.AddVal(Array());
                added++;
            }
        }

		switch (funTok.GetCode())
		{
			case  cmOPRT_BIN:
			case  cmOPRT_POSTFIX:
			case  cmOPRT_INFIX:
			case  cmFUNC:
			    // Check, whether enough arguments are available (with some special exceptions)
				if (funTok.GetArgCount() == -1 && iArgCount == 0 && funTok.GetFuncAddr() != ValidZeroArgument.GetAddr())
					Error(ecTOO_FEW_PARAMS, m_pTokenReader->GetPos(), funTok.GetAsString());

                m_compilingState.m_byteCode.AddFun(funTok.GetFuncAddr(),
                                                   (funTok.GetArgCount() == -1) ? -iArgCount : iArgRequired,
                                                   funTok.IsOptimizable(),
                                                   funTok.GetAsString());
				break;
            case  cmMETHOD:
                m_compilingState.m_byteCode.AddMethod(funTok.GetAsString(), iArgCount+1);
            default:
                break;
                // nothing, just avoiding warnings
		}

		// Push dummy value representing the function result to the stack
		token_type token;
		token.MoveVal(Array(Value("rv@" + funTok.GetAsString())));
		a_stVal.push(std::move(token));
	}

	//---------------------------------------------------------------------------
	void ParserBase::ApplyIfElse(ParserStack<token_type>& a_stOpt,
								 ParserStack<token_type>& a_stVal) const
	{
		// Check if there is an if Else clause to be calculated
		while (a_stOpt.size() && a_stOpt.top().GetCode() == cmELSE)
		{
			token_type opElse = a_stOpt.pop();
			MUP_ASSERT(a_stOpt.size() > 0);

			// Take the value associated with the else branch from the value stack
			token_type vVal2 = a_stVal.pop();

			MUP_ASSERT(a_stOpt.size() > 0);
			MUP_ASSERT(a_stVal.size() >= 2);

			// it then else is a ternary operator Pop all three values from the value s
			// tack and just return the right value
			token_type vVal1 = a_stVal.pop();
			token_type vExpr = a_stVal.pop();

			a_stVal.push(std::move(all(vExpr.GetVal() != Array(Value(0.0))) ? vVal1 : vVal2));

			token_type opIf = a_stOpt.pop();
			MUP_ASSERT(opElse.GetCode() == cmELSE);
			MUP_ASSERT(opIf.GetCode() == cmIF);

			m_compilingState.m_byteCode.AddIfElse(cmENDIF);
		} // while pending if-else-clause found
	}

	//---------------------------------------------------------------------------
	/** \brief Performs the necessary steps to write code for
	           the execution of binary operators into the bytecode.
	*/
	void ParserBase::ApplyBinOprt(ParserStack<token_type>& a_stOpt,
								  ParserStack<token_type>& a_stVal) const
	{
		// is it a user defined binary operator?
		if (a_stOpt.top().GetCode() == cmOPRT_BIN)
		{
			ApplyFunc(a_stOpt, a_stVal, 2);
		}
		else
		{
			MUP_ASSERT(a_stVal.size() >= 2);
			token_type valTok1 = a_stVal.pop(),
					   valTok2 = a_stVal.pop(),
					   optTok  = a_stOpt.pop(),
					   resTok;

			if (optTok.GetCode() >= cmASSIGN && optTok.GetCode() <= cmPOWASGN)
			{
			    if (valTok2.GetCode() == cmVARARRAY)
                    m_compilingState.m_byteCode.AddAssignOp(valTok2.GetVarArray(), optTok.GetCode());
				else if (valTok2.GetCode() == cmVAR)
                    m_compilingState.m_byteCode.AddAssignOp(valTok2.GetVar(), optTok.GetCode());
                else
                    Error(ecUNEXPECTED_OPERATOR, -1, ParserBase::c_DefaultOprt[optTok.GetCode()]);
			}
			else
				m_compilingState.m_byteCode.AddOp(optTok.GetCode());

			resTok.MoveVal(Array(Value(1.0)));
			a_stVal.push(std::move(resTok));
		}
	}

	void ParserBase::ApplyVal2Str(ParserStack<token_type>& a_stOpt,
								  ParserStack<token_type>& a_stVal) const
	{
        MUP_ASSERT(a_stVal.size() >= 1);
        token_type valTok1 = a_stVal.pop(),
                   optTok  = a_stOpt.pop(),
                   resTok;

        m_compilingState.m_byteCode.AddVal(Value(int(optTok.GetLen())));
        m_compilingState.m_byteCode.AddOp(cmVAL2STR);

        // Push a dummy value to the stack
        resTok.MoveVal(Array(Value(1.0)));
        a_stVal.push(std::move(resTok));
	}

	//---------------------------------------------------------------------------
	/** \brief Apply a binary operator.
	    \param a_stOpt The operator stack
	    \param a_stVal The value stack
	*/
	void ParserBase::ApplyRemainingOprt(ParserStack<token_type>& stOpt,
										ParserStack<token_type>& stVal) const
	{
		while (stOpt.size() &&
				stOpt.top().GetCode() != cmBO &&
				stOpt.top().GetCode() != cmVO &&
				stOpt.top().GetCode() != cmEXP2 &&
				stOpt.top().GetCode() != cmEXP3 &&
				stOpt.top().GetCode() != cmIF)
		{
			token_type tok = stOpt.top();
			switch (tok.GetCode())
			{
				case cmOPRT_INFIX:
				case cmOPRT_BIN:
				case cmLE:
				case cmGE:
				case cmNEQ:
				case cmEQ:
				case cmLT:
				case cmGT:
				case cmADD:
				case cmSUB:
				case cmMUL:
				case cmDIV:
				case cmPOW:
				case cmLAND:
				case cmLOR:
				case cmASSIGN:
                case cmADDASGN:
                case cmSUBASGN:
                case cmMULASGN:
                case cmDIVASGN:
                case cmPOWASGN:
					if (stOpt.top().GetCode() == cmOPRT_INFIX)
						ApplyFunc(stOpt, stVal, 1);
					else
						ApplyBinOprt(stOpt, stVal);
					break;

				case cmELSE:
					ApplyIfElse(stOpt, stVal);
					break;

                case cmVAL2STR:
                    ApplyVal2Str(stOpt, stVal);
                    break;

				default:
					Error(ecINTERNAL_ERROR);
			}
		}
	}

	//---------------------------------------------------------------------------
	/** \brief Parse the command code.
	    \sa ParseString(...)

	    Command code contains precalculated stack positions of the values and the
	    associated operators. The Stack is filled beginning from index one the
	    value at index zero is not used at all.
	*/
	void ParserBase::ParseCmdCode()
	{
		int sidx(0);
		std::vector<StackItem>& Stack = m_state->m_stackBuffer;
		//return;

        for (SToken* pTok = m_state->m_byteCode.GetBase(); pTok->Cmd != cmEND ; ++pTok)
        {
            //continue;
            switch (pTok->Cmd)
            {
                // built in binary operators
                case  cmLE:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] <= Stack[sidx + 1];
                    continue;
                case  cmGE:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] >= Stack[sidx + 1];
                    continue;
                case  cmNEQ:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] != Stack[sidx + 1];
                    continue;
                case  cmEQ:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] == Stack[sidx + 1];
                    continue;
                case  cmLT:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] < Stack[sidx + 1];
                    continue;
                case  cmGT:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] > Stack[sidx + 1];
                    continue;
                case  cmADD:
                    --sidx;
                    Stack[sidx]  += Stack[1 + sidx];
                    continue;
                case  cmSUB:
                    --sidx;
                    Stack[sidx]  -= Stack[1 + sidx];
                    continue;
                case  cmMUL:
                    --sidx;
                    Stack[sidx]  *= Stack[1 + sidx];
                    continue;
                case  cmDIV:
                    --sidx;
                    Stack[sidx]  /= Stack[1 + sidx];
                    continue;

                case  cmPOW:
                    --sidx;
                    Stack[sidx]  ^= Stack[1 + sidx];
                    continue;

                case  cmLAND:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] && Stack[sidx + 1];
                    continue;
                case  cmLOR:
                    --sidx;
                    Stack[sidx]  = Stack[sidx] || Stack[sidx + 1];
                    continue;

                case  cmVAL2STR:
                    --sidx;
                    Stack[sidx]  = val2Str(Stack[sidx].get(), Stack[sidx+1].get().front().getNum().asUI64());
                    continue;

                case  cmASSIGN:
                    --sidx;
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var = Stack[sidx + 1].get();
                    else
                        Stack[sidx] = pTok->Oprt().var = Stack[sidx + 1].get();
                    continue;

                case  cmADDASGN:
                    --sidx;
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var += Stack[sidx + 1].get();
                    else
                        Stack[sidx] = pTok->Oprt().var += Stack[sidx + 1].get();
                    continue;

                case  cmSUBASGN:
                    --sidx;
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var -= Stack[sidx + 1].get();
                    else
                        Stack[sidx] = pTok->Oprt().var -= Stack[sidx + 1].get();
                    continue;

                case  cmMULASGN:
                    --sidx;
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var *= Stack[sidx + 1].get();
                    else
                        Stack[sidx] = pTok->Oprt().var *= Stack[sidx + 1].get();
                    continue;

                case  cmDIVASGN:
                    --sidx;
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var /= Stack[sidx + 1].get();
                    else
                        Stack[sidx] = pTok->Oprt().var /= Stack[sidx + 1].get();
                    continue;

                case  cmPOWASGN:
                    --sidx;
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var = pTok->Oprt().var.pow(Stack[sidx + 1].get());
                    else
                        Stack[sidx] = pTok->Oprt().var = pTok->Oprt().var.pow(Stack[sidx + 1].get());
                    continue;

                case  cmINCR:
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var += Value(1);
                    else
                        Stack[sidx] = pTok->Oprt().var += Value(1);
                    continue;

                case  cmDECR:
                    if (pTok->Oprt().var.isScalar())
                        pTok->Oprt().var -= Value(1);
                    else
                        Stack[sidx] = pTok->Oprt().var-= Value(1);
                    continue;

                case  cmIF: // Not needed right now, operator converted to a function
                    if (!all(Stack[sidx--].get()))
                        pTok += pTok->Oprt().offset;
                    continue;

                case  cmELSE: // Not needed right now, operator converted to a function
                    pTok += pTok->Oprt().offset;
                    continue;

                case  cmENDIF: // Not needed right now, operator converted to a function
                    continue;

                // value and variable tokens
                case  cmVAL:
                    Stack[++sidx].aliasOf(&pTok->Val().data2);
                    continue;

                case  cmVAR:
                    Stack[++sidx].aliasOf(pTok->Val().var);
                    continue;

                case  cmVARARRAY:
                    Stack[++sidx] = pTok->Oprt().var.asArray();
                    continue;

                case  cmVARPOW2:
                    Stack[++sidx].varPowN(*pTok->Val().var, 2);
                    continue;

                case  cmVARPOW3:
                    Stack[++sidx].varPowN(*pTok->Val().var, 3);
                    continue;

                case  cmVARPOW4:
                    Stack[++sidx].varPowN(*pTok->Val().var, 4);
                    continue;

                case  cmVARPOWN:
                    Stack[++sidx] = Array(*pTok->Val().var).pow(pTok->Val().data);
                    continue;

                case  cmVARMUL:
                    Stack[++sidx].varMul(pTok->Val().data, *pTok->Val().var, pTok->Val().data2);
                    continue;

                case  cmREVVARMUL:
                    Stack[++sidx].revVarMul(pTok->Val().data, *pTok->Val().var, pTok->Val().data2);
                    continue;

                case  cmDIVVAR:
                    Stack[++sidx].divVar(pTok->Val().data, *pTok->Val().var, pTok->Val().data2);
                    continue;

                case  cmVARCOPY:
                    Stack[++sidx].aliasOf(pTok->Oprt().var.front());
                    pTok->Oprt().var = *pTok->Oprt().src;
                    continue;

                case  cmVARINIT:
                    Stack[++sidx].aliasOf(pTok->Oprt().var.front());
                    pTok->Oprt().var = pTok->Oprt().val;
                    continue;

                // Next is treatment of numeric functions
                case  cmFUNC:
                    {
                        int iArgCount = pTok->Fun().argc;

                        // switch according to argument count
                        switch (iArgCount)
                        {
                            case 0:
                                sidx += 1;
                                Stack[sidx] = (*(fun_type0)pTok->Fun().ptr)();
                                continue;
                            case 1:
                                Stack[sidx] = (*(fun_type1)pTok->Fun().ptr)(Stack[sidx].get());
                                continue;
                            case 2:
                                sidx -= 1;
                                Stack[sidx] = (*(fun_type2)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get());
                                continue;
                            case 3:
                                sidx -= 2;
                                Stack[sidx] = (*(fun_type3)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get(),
                                                                            Stack[sidx + 2].get());
                                continue;
                            case 4:
                                sidx -= 3;
                                Stack[sidx] = (*(fun_type4)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get(),
                                                                            Stack[sidx + 2].get(),
                                                                            Stack[sidx + 3].get());
                                continue;
                            case 5:
                                sidx -= 4;
                                Stack[sidx] = (*(fun_type5)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get(),
                                                                            Stack[sidx + 2].get(),
                                                                            Stack[sidx + 3].get(),
                                                                            Stack[sidx + 4].get());
                                continue;
                            case 6:
                                sidx -= 5;
                                Stack[sidx] = (*(fun_type6)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get(),
                                                                            Stack[sidx + 2].get(),
                                                                            Stack[sidx + 3].get(),
                                                                            Stack[sidx + 4].get(),
                                                                            Stack[sidx + 5].get());
                                continue;
                            case 7:
                                sidx -= 6;
                                Stack[sidx] = (*(fun_type7)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get(),
                                                                            Stack[sidx + 2].get(),
                                                                            Stack[sidx + 3].get(),
                                                                            Stack[sidx + 4].get(),
                                                                            Stack[sidx + 5].get(),
                                                                            Stack[sidx + 6].get());
                                continue;
                            case 8:
                                sidx -= 7;
                                Stack[sidx] = (*(fun_type8)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get(),
                                                                            Stack[sidx + 2].get(),
                                                                            Stack[sidx + 3].get(),
                                                                            Stack[sidx + 4].get(),
                                                                            Stack[sidx + 5].get(),
                                                                            Stack[sidx + 6].get(),
                                                                            Stack[sidx + 7].get());
                                continue;
                            case 9:
                                sidx -= 8;
                                Stack[sidx] = (*(fun_type9)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                            Stack[sidx + 1].get(),
                                                                            Stack[sidx + 2].get(),
                                                                            Stack[sidx + 3].get(),
                                                                            Stack[sidx + 4].get(),
                                                                            Stack[sidx + 5].get(),
                                                                            Stack[sidx + 6].get(),
                                                                            Stack[sidx + 7].get(),
                                                                            Stack[sidx + 8].get());
                                continue;
                            case 10:
                                sidx -= 9;
                                Stack[sidx] = (*(fun_type10)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                             Stack[sidx + 1].get(),
                                                                             Stack[sidx + 2].get(),
                                                                             Stack[sidx + 3].get(),
                                                                             Stack[sidx + 4].get(),
                                                                             Stack[sidx + 5].get(),
                                                                             Stack[sidx + 6].get(),
                                                                             Stack[sidx + 7].get(),
                                                                             Stack[sidx + 8].get(),
                                                                             Stack[sidx + 9].get());
                                continue;
                            default:
                                if (iArgCount > 0) // function with variable arguments store the number as a negative value
                                    Error(ecINTERNAL_ERROR, 1);

                                {
                                    sidx -= -iArgCount - 1;
                                    std::vector<Array> args(-iArgCount);

                                    for (size_t i = 0; i < args.size(); i++)
                                    {
                                        args[i] = Stack[sidx+i].get();
                                    }

                                    Stack[sidx] = (*(multfun_type)pTok->Fun().ptr)(&args[0], args.size());
                                }
                                continue;
                        }
                    }

                case  cmMETHOD:
                    {
                        int iArgCount = pTok->Fun().argc;

                        // switch according to argument count
                        switch (iArgCount)
                        {
                            case 1:
                                Stack[sidx] = Stack[sidx].get().call(pTok->Fun().name);
                                continue;
                            case 2:
                                sidx -= 1;
                                Stack[sidx] = Stack[sidx].get().call(pTok->Fun().name,
                                                               Stack[sidx + 1].get());
                                continue;
                            case 3:
                                sidx -= 2;
                                Stack[sidx] = Stack[sidx].get().call(pTok->Fun().name,
                                                               Stack[sidx + 1].get(),
                                                               Stack[sidx + 2].get());
                                continue;
                            default:
                                Error(ecINTERNAL_ERROR, 1);
                        }
                    }

                default:
                    Error(ecINTERNAL_ERROR, 3);
            } // switch CmdCode
        } // for all bytecode tokens
	}


    /////////////////////////////////////////////////
    /// \brief OpenMP optimized parallel bytecode
    /// executor.
    ///
    /// \param nVectorLength size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::ParseCmdCodeBulkParallel(size_t nVectorLength)
	{
	    size_t nBufferOffset = m_state->m_stackBuffer.size() / nMaxThreads;
	    int nStackSize = m_state->m_numResults;

        #pragma omp parallel for //schedule(static, (nVectorLength-1)/nMaxThreads)
        for (size_t nOffset = 1; nOffset < nVectorLength; ++nOffset)
        {
            int nThreadID = omp_get_thread_num();
            StackItem* Stack = &m_state->m_stackBuffer[nThreadID * nBufferOffset];
            int sidx(0);

            // Run the bytecode
            for (SToken* pTok = m_state->m_byteCode.GetBase(); pTok->Cmd != cmEND ; ++pTok)
            {
                switch (pTok->Cmd)
                {
                    // built in binary operators
                    case  cmLE:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] <= Stack[sidx + 1];
                        continue;
                    case  cmGE:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] >= Stack[sidx + 1];
                        continue;
                    case  cmNEQ:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] != Stack[sidx + 1];
                        continue;
                    case  cmEQ:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] == Stack[sidx + 1];
                        continue;
                    case  cmLT:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] < Stack[sidx + 1];
                        continue;
                    case  cmGT:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] > Stack[sidx + 1];
                        continue;
                    case  cmADD:
                        --sidx;
                        Stack[sidx]  += Stack[1 + sidx];
                        continue;
                    case  cmSUB:
                        --sidx;
                        Stack[sidx]  -= Stack[1 + sidx];
                        continue;
                    case  cmMUL:
                        --sidx;
                        Stack[sidx]  *= Stack[1 + sidx];
                        continue;
                    case  cmDIV:
                        --sidx;
                        Stack[sidx]  /= Stack[1 + sidx];
                        continue;

                    case  cmPOW:
                        --sidx;
                        Stack[sidx]  ^= Stack[1 + sidx];
                        continue;

                    case  cmLAND:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] && Stack[sidx + 1];
                        continue;
                    case  cmLOR:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] || Stack[sidx + 1];
                        continue;

                    case  cmVAL2STR:
                        --sidx;
                        Stack[sidx]  = val2Str(Stack[sidx].get(), Stack[sidx+1].get().front().getNum().asUI64());
                        continue;

                    case  cmASSIGN:
                        --sidx;
                        if (pTok->Oprt().var.isScalar())
                            pTok->Oprt().var = Stack[sidx + 1].get();
                        else
                            Stack[sidx] = pTok->Oprt().var = Stack[sidx + 1].get();
                        continue;

                    case  cmADDASGN:
                        --sidx;
                        if (pTok->Oprt().var.isScalar())
                            pTok->Oprt().var += Stack[sidx + 1].get();
                        else
                            Stack[sidx] = pTok->Oprt().var += Stack[sidx + 1].get();
                        continue;

                    case  cmSUBASGN:
                        --sidx;
                        if (pTok->Oprt().var.isScalar())
                            pTok->Oprt().var -= Stack[sidx + 1].get();
                        else
                            Stack[sidx] = pTok->Oprt().var -= Stack[sidx + 1].get();
                        continue;

                    case  cmMULASGN:
                        --sidx;
                        if (pTok->Oprt().var.isScalar())
                            pTok->Oprt().var *= Stack[sidx + 1].get();
                        else
                            Stack[sidx] = pTok->Oprt().var *= Stack[sidx + 1].get();
                        continue;

                    case  cmDIVASGN:
                        --sidx;
                        if (pTok->Oprt().var.isScalar())
                            pTok->Oprt().var /= Stack[sidx + 1].get();
                        else
                            Stack[sidx] = pTok->Oprt().var /= Stack[sidx + 1].get();
                        continue;

                    case  cmPOWASGN:
                        --sidx;
                        if (pTok->Oprt().var.isScalar())
                            pTok->Oprt().var = pTok->Oprt().var.pow(Stack[sidx + 1].get());
                        else
                            Stack[sidx] = pTok->Oprt().var = pTok->Oprt().var.pow(Stack[sidx + 1].get());
                        continue;

                    case  cmINCR:
                        pTok->Oprt().var += Value(1);
                        continue;

                    case  cmDECR:
                        pTok->Oprt().var -= Value(1);
                        continue;

                    case  cmIF: // Not needed right now, operator converted to a function
                        if (!all(Stack[sidx--].get()))
                            pTok += pTok->Oprt().offset;
                        continue;

                    case  cmELSE: // Not needed right now, operator converted to a function
                        pTok += pTok->Oprt().offset;
                        continue;

                    case  cmENDIF: // Not needed right now, operator converted to a function
                        continue;

                    // value and variable tokens
                    case  cmVAL:
                        Stack[++sidx].aliasOf(&pTok->Val().data2);
                        continue;

                    case  cmVAR:
                        Stack[++sidx].aliasOf(pTok->Val().var);
                        continue;

                    case  cmVARARRAY:
                        Stack[++sidx] = pTok->Oprt().var.asArray();
                        continue;

                    case  cmVARPOW2:
                        Stack[++sidx].varPowN(*pTok->Val().var, 2);
                        continue;

                    case  cmVARPOW3:
                        Stack[++sidx].varPowN(*pTok->Val().var, 3);
                        continue;

                    case  cmVARPOW4:
                        Stack[++sidx].varPowN(*pTok->Val().var, 4);
                        continue;

                    case  cmVARPOWN:
                        Stack[++sidx] = Array(*pTok->Val().var).pow(pTok->Val().data);
                        continue;

                    case  cmVARMUL:
                        Stack[++sidx].varMul(pTok->Val().data, *pTok->Val().var, pTok->Val().data2);
                        continue;

                    case  cmREVVARMUL:
                        Stack[++sidx].revVarMul(pTok->Val().data, *pTok->Val().var, pTok->Val().data2);
                        continue;

                    case  cmDIVVAR:
                        Stack[++sidx].divVar(pTok->Val().data, *pTok->Val().var, pTok->Val().data2);
                        continue;

                    // Next is treatment of numeric functions
                    case  cmFUNC:
                        {
                            int iArgCount = pTok->Fun().argc;

                            // switch according to argument count
                            switch (iArgCount)
                            {
                                case 0:
                                    sidx += 1;
                                    Stack[sidx] = (*(fun_type0)pTok->Fun().ptr)();
                                    continue;
                                case 1:
                                    Stack[sidx] = (*(fun_type1)pTok->Fun().ptr)(Stack[sidx].get());
                                    continue;
                                case 2:
                                    sidx -= 1;
                                    Stack[sidx] = (*(fun_type2)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get());
                                    continue;
                                case 3:
                                    sidx -= 2;
                                    Stack[sidx] = (*(fun_type3)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get(),
                                                                                Stack[sidx + 2].get());
                                    continue;
                                case 4:
                                    sidx -= 3;
                                    Stack[sidx] = (*(fun_type4)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get(),
                                                                                Stack[sidx + 2].get(),
                                                                                Stack[sidx + 3].get());
                                    continue;
                                case 5:
                                    sidx -= 4;
                                    Stack[sidx] = (*(fun_type5)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get(),
                                                                                Stack[sidx + 2].get(),
                                                                                Stack[sidx + 3].get(),
                                                                                Stack[sidx + 4].get());
                                    continue;
                                case 6:
                                    sidx -= 5;
                                    Stack[sidx] = (*(fun_type6)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get(),
                                                                                Stack[sidx + 2].get(),
                                                                                Stack[sidx + 3].get(),
                                                                                Stack[sidx + 4].get(),
                                                                                Stack[sidx + 5].get());
                                    continue;
                                case 7:
                                    sidx -= 6;
                                    Stack[sidx] = (*(fun_type7)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get(),
                                                                                Stack[sidx + 2].get(),
                                                                                Stack[sidx + 3].get(),
                                                                                Stack[sidx + 4].get(),
                                                                                Stack[sidx + 5].get(),
                                                                                Stack[sidx + 6].get());
                                    continue;
                                case 8:
                                    sidx -= 7;
                                    Stack[sidx] = (*(fun_type8)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get(),
                                                                                Stack[sidx + 2].get(),
                                                                                Stack[sidx + 3].get(),
                                                                                Stack[sidx + 4].get(),
                                                                                Stack[sidx + 5].get(),
                                                                                Stack[sidx + 6].get(),
                                                                                Stack[sidx + 7].get());
                                    continue;
                                case 9:
                                    sidx -= 8;
                                    Stack[sidx] = (*(fun_type9)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                Stack[sidx + 1].get(),
                                                                                Stack[sidx + 2].get(),
                                                                                Stack[sidx + 3].get(),
                                                                                Stack[sidx + 4].get(),
                                                                                Stack[sidx + 5].get(),
                                                                                Stack[sidx + 6].get(),
                                                                                Stack[sidx + 7].get(),
                                                                                Stack[sidx + 8].get());
                                    continue;
                                case 10:
                                    sidx -= 9;
                                    Stack[sidx] = (*(fun_type10)pTok->Fun().ptr)(Stack[sidx].get(),
                                                                                 Stack[sidx + 1].get(),
                                                                                 Stack[sidx + 2].get(),
                                                                                 Stack[sidx + 3].get(),
                                                                                 Stack[sidx + 4].get(),
                                                                                 Stack[sidx + 5].get(),
                                                                                 Stack[sidx + 6].get(),
                                                                                 Stack[sidx + 7].get(),
                                                                                 Stack[sidx + 8].get(),
                                                                                 Stack[sidx + 9].get());
                                    continue;
                                default:
                                    if (iArgCount > 0) // function with variable arguments store the number as a negative value
                                        Error(ecINTERNAL_ERROR, 1);

                                    {
                                        sidx -= -iArgCount - 1;
                                        std::vector<Array> args(-iArgCount);

                                        for (size_t i = 0; i < args.size(); i++)
                                        {
                                            args[i] = Stack[sidx+i].get();
                                        }

                                        Stack[sidx] = (*(multfun_type)pTok->Fun().ptr)(&args[0], args.size());
                                    }
                                    continue;
                            }
                        }

                    case  cmMETHOD:
                        {
                            int iArgCount = pTok->Fun().argc;

                            // switch according to argument count
                            switch (iArgCount)
                            {
                                case 1:
                                    Stack[sidx] = Stack[sidx].get().call(pTok->Fun().name);
                                    continue;
                                case 2:
                                    sidx -= 1;
                                    Stack[sidx] = Stack[sidx].get().call(pTok->Fun().name,
                                                                   Stack[sidx + 1].get());
                                    continue;
                                case 3:
                                    sidx -= 2;
                                    Stack[sidx] = Stack[sidx].get().call(pTok->Fun().name,
                                                                   Stack[sidx + 1].get(),
                                                                   Stack[sidx + 2].get());
                                    continue;
                                default:
                                    Error(ecINTERNAL_ERROR, 1);
                            }
                        }

                    default:
                        Error(ecINTERNAL_ERROR, 3);
                } // switch CmdCode
            } // for all bytecode tokens

            // Copy the results
            for (int j = 0; j < nStackSize; j++)
            {
                m_buffer[nOffset*nStackSize + j] = m_state->m_stackBuffer[nThreadID*nBufferOffset + j + 1].get();
            }
        }

	}

	//---------------------------------------------------------------------------
	void ParserBase::CreateRPN()
	{
	    if (g_DbgDumpStack)
            print("Parsing: \"" + m_pTokenReader->GetExpr() + "\"");

		if (!m_pTokenReader->GetExpr().length())
			Error(ecUNEXPECTED_EOF, 0);

		ParserStack<token_type> stOpt, stVal;
		ParserStack<int> stArgCount;
		token_type opta, opt;  // for storing operators
		token_type val, tval;  // for storing value
		string_type strBuf;    // buffer for string function arguments
		int vectorCreateMode = 0;
		bool varArrayCandidate = false;

		//ReInit();

		// The outermost counter counts the number of seperated items
		// such as in "a=10,b=20,c=c+a"
		stArgCount.push(1);

		for (;;)
		{
			opt = m_pTokenReader->ReadNextToken();

			switch (opt.GetCode())
			{
				//
				// Next three are different kind of value entries
				//
				case cmVAR:
					stVal.push(opt);
					m_compilingState.m_byteCode.AddVar(opt.GetVar());
					break;

				case cmVAL:
				    varArrayCandidate = false;
					stVal.push(opt);
					m_compilingState.m_byteCode.AddVal(std::move(opt.GetVal()));
					break;

				case cmELSE:
				    varArrayCandidate = false;
				    if (stOpt.size())
                    {
                        if (stOpt.top().GetCode() == cmBO)
                            Error(ecMISPLACED_COLON, m_pTokenReader->GetPos());
                        else if (vectorCreateMode) // falls cmVO => index operator, braucht vllcjt noch ein cmIDX
						{
						    ApplyRemainingOprt(stOpt, stVal);

						    if (stOpt.top().GetCode() == cmVO && stVal.size() > 0)
                            {
                                ParserToken tok;
                                tok.Set(cmEXP2, MU_VECTOR_EXP2);
                                stOpt.push(tok);
                                break;
                            }
                            else if (stOpt.top().GetCode() == cmEXP2 && stVal.size() > 1)
                            {
                                stOpt.top().Set(cmEXP3, MU_VECTOR_EXP3);
                                break;
                            }
                            else if (stOpt.top().GetCode() != cmIF)
                                Error(ecMISPLACED_COLON, m_pTokenReader->GetPos());
						}
                    }

					m_nIfElseCounter--;

					if (m_nIfElseCounter < 0) // zweiter noetiger check
						Error(ecMISPLACED_COLON, m_pTokenReader->GetPos());

                // falltrough intended
				case cmARG_SEP:
					if (stArgCount.empty())
						Error(ecUNEXPECTED_ARG_SEP, m_pTokenReader->GetPos());

					++stArgCount.top();

				// fallthrough intentional (no break!)
				case cmEND:
					ApplyRemainingOprt(stOpt, stVal);

					if (stOpt.size())
                    {
                        if (stOpt.top().GetCode() == cmEXP2)
                        {
                            if (stVal.size() < 2)
                                Error(opt.GetCode() == cmEND ? ecUNEXPECTED_EOF : ecUNEXPECTED_ARG_SEP,
                                      m_pTokenReader->GetPos());

                            stOpt.top().Set(m_FunDef.at(MU_VECTOR_EXP2), MU_VECTOR_EXP2);
                            ApplyFunc(stOpt, stVal, 2);
                        }
                        else if (stOpt.top().GetCode() == cmEXP3 && stVal.size() > 2)
                        {
                            if (stVal.size() < 3)
                                Error(opt.GetCode() == cmEND ? ecUNEXPECTED_EOF : ecUNEXPECTED_ARG_SEP,
                                      m_pTokenReader->GetPos());

                            stOpt.top().Set(m_FunDef.at(MU_VECTOR_EXP3), MU_VECTOR_EXP3);
                            ApplyFunc(stOpt, stVal, 3);
                        }
                    }
					break;

				case cmVC:
				    vectorCreateMode--;
				    // fallthrough intended
				case cmBC:
					{
						// The argument count for parameterless functions is zero
						// by default an opening bracket sets parameter count to 1
						// in preparation of arguments to come. If the last token
						// was an opening bracket we know better...
						if ((opta.GetCode() == cmBO && opt.GetCode() == cmBC)
                            || (opta.GetCode() == cmVO && opt.GetCode() == cmVC))
							--stArgCount.top();

						ApplyRemainingOprt(stOpt, stVal);

						if (stOpt.size())
                        {
                            if (stOpt.top().GetCode() == cmEXP2)
                            {
                                if (stVal.size() < 2)
                                    Error(ecUNEXPECTED_PARENS, m_pTokenReader->GetPos());

                                stOpt.top().Set(m_FunDef.at(MU_VECTOR_EXP2), MU_VECTOR_EXP2);
                                ApplyFunc(stOpt, stVal, 2);
                            }
                            else if (stOpt.top().GetCode() == cmEXP3)
                            {
                                if (stVal.size() < 3)
                                    Error(ecUNEXPECTED_PARENS, m_pTokenReader->GetPos());

                                stOpt.top().Set(m_FunDef.at(MU_VECTOR_EXP3), MU_VECTOR_EXP3);
                                ApplyFunc(stOpt, stVal, 3);
                            }
                        }

                        if (stOpt.size() && stOpt.top().GetCode() == cmIF)
                        {
                            stArgCount.pop();
                            stOpt.top().Set(m_FunDef.at(MU_IF_ELSE), MU_IF_ELSE);
                            ApplyFunc(stOpt, stVal, 3);
                        }

						// Check if the bracket content has been evaluated completely
						if (stOpt.size() && ((stOpt.top().GetCode() == cmBO && opt.GetCode() == cmBC)
                                             || (stOpt.top().GetCode() == cmVO && opt.GetCode() == cmVC)))
						{
							// if opt is ")" and opta is "(" the bracket has been evaluated, now its time to check
							// if there is either a function or a sign pending
							// neither the opening nor the closing bracket will be pushed back to
							// the operator stack
							// Check if a function is standing in front of the opening bracket,
							// if yes evaluate it afterwards check for infix operators
							assert(stArgCount.size());
							int iArgCount = stArgCount.pop();

							stOpt.pop(); // Take opening bracket from stack

							if (varArrayCandidate && iArgCount)
                            {
                                // Remove the vector create function
                                stOpt.pop();

                                VarArray arr;

                                // Remove all vars except of the var array one
                                for (int i = 0; i < iArgCount-1; i++)
                                {
                                    arr.insert(arr.begin(), stVal.top().GetVar());
                                    stVal.pop();
                                    m_compilingState.m_byteCode.pop();
                                }

                                arr.insert(arr.begin(), stVal.top().GetVar());
                                stVal.top().SetVarArray(arr, "");
                                m_compilingState.m_byteCode.pop();
                                m_compilingState.m_byteCode.AddVarArray(arr);
                                varArrayCandidate = false;

                                break;
                            }

                            varArrayCandidate = false;

							if (iArgCount > 1
                                && (stOpt.size() == 0 || (stOpt.top().GetCode() != cmFUNC && stOpt.top().GetCode() != cmMETHOD)))
								Error(ecUNEXPECTED_ARG, m_pTokenReader->GetPos());

							// The opening bracket was popped from the stack now check if there
							// was a function before this bracket
							if (stOpt.size()
								&& stOpt.top().GetCode() != cmOPRT_INFIX
								&& stOpt.top().GetCode() != cmOPRT_BIN
								&& stOpt.top().GetCode() != cmIF
								&& (stOpt.top().GetFuncAddr() != nullptr || stOpt.top().GetCode() == cmMETHOD))
							{
								ApplyFunc(stOpt, stVal, iArgCount);
							}
						}
					} // if bracket content is evaluated
					break;

				//
				// Next are the binary operator entries
				//
				//case cmAND:   // built in binary operators
				//case cmOR:
				//case cmXOR:
				case cmIF:
					m_nIfElseCounter++;
				// fallthrough intentional (no break!)

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
				case cmOPRT_BIN:
                    varArrayCandidate = false;
					// A binary operator (user defined or built in) has been found.
					while ( stOpt.size() &&
							stOpt.top().GetCode() != cmBO &&
							stOpt.top().GetCode() != cmVO &&
							stOpt.top().GetCode() != cmIF)
					{
						int nPrec1 = GetOprtPrecedence(stOpt.top()),
							nPrec2 = GetOprtPrecedence(opt);

						if (stOpt.top().GetCode() == opt.GetCode())
						{
							// Deal with operator associativity
							EOprtAssociativity eOprtAsct = GetOprtAssociativity(opt);

                                if ((eOprtAsct == oaRIGHT && (nPrec1 <= nPrec2))
                                    || (eOprtAsct == oaLEFT && (nPrec1 <  nPrec2)))
								break;
						}
						else if (nPrec1 < nPrec2)
						{
							// In case the operators are not equal the precedence decides alone...
							break;
						}

						if (stOpt.top().GetCode() == cmOPRT_INFIX)
							ApplyFunc(stOpt, stVal, 1);
                        else if (stOpt.top().GetCode() == cmVAL2STR)
                            ApplyVal2Str(stOpt, stVal);
						else
							ApplyBinOprt(stOpt, stVal);
					} // while ( ... )

					if (opt.GetCode() == cmIF)
                        stArgCount.push(2); // This operator separates already two values

					// The operator can't be evaluated right now, push back to the operator stack
					stOpt.push(opt);
					break;

				//
				// Last section contains functions and operators implicitely mapped to functions
				//
				case cmVO:
                {
                    ParserToken tok;
                    tok.Set(m_FunDef.at(MU_VECTOR_CREATE), MU_VECTOR_CREATE);
				    stOpt.push(tok);
				    vectorCreateMode++;
				    varArrayCandidate = vectorCreateMode == 1;
                }
                // fallthrough intended
				case cmBO:
					stArgCount.push(1);
					stOpt.push(opt);
					if (opt.GetCode() == cmBO)
                        varArrayCandidate = false;
					break;

                case cmPATHPLACEHOLDER:
                    m_compilingState.m_byteCode.AddVal(Value(opt.GetAsString()));
                    m_compilingState.m_byteCode.AddFun((generic_fun_type)getPathToken, 1, false, opt.GetAsString());
                    stVal.push(ParserToken().MoveVal(Value(opt.GetAsString()), opt.GetAsString()));
                    break;

				case cmOPRT_INFIX:
				case cmVAL2STR:
				case cmFUNC:
                case cmMETHOD:
					stOpt.push(opt);

					if (opt.GetType() == tpNOARGS)
                        ApplyFunc(stOpt, stVal, 0);

					varArrayCandidate = false;
					break;

                case cmINCR:
                case cmDECR:
                {
                    token_type valTok = stVal.pop();

                    if (valTok.GetCode() == cmVARARRAY)
                        m_compilingState.m_byteCode.AddAssignOp(valTok.GetVarArray(), opt.GetCode());
                    else if (valTok.GetCode() == cmVAR)
                        m_compilingState.m_byteCode.AddAssignOp(valTok.GetVar(), opt.GetCode());
                    else
                        Error(ecUNEXPECTED_OPERATOR, -1, ParserBase::c_DefaultOprt[opt.GetCode()]);

                    stVal.push(ParserToken().MoveVal(Value(1.0)));
                    break;
                }

				case cmOPRT_POSTFIX:
					stOpt.push(opt);
					ApplyFunc(stOpt, stVal, 1);  // this is the postfix operator
					varArrayCandidate = false;
					break;

				default:
					Error(ecINTERNAL_ERROR, m_pTokenReader->GetPos());
			} // end of switch operator-token

			opta = opt;

			if (opt.GetCode() == cmEND)
			{
                while (stOpt.size())
                {
                    if (stOpt.top().GetCode() == cmIF)
                    {
                        if (stArgCount.top() != 3)
                            Error(ecUNEXPECTED_CONDITIONAL, m_pTokenReader->GetPos(), "?");

                        stArgCount.pop();
                        stOpt.top().Set(m_FunDef.at(MU_IF_ELSE), MU_IF_ELSE);
                        ApplyFunc(stOpt, stVal, 3);
                    }
                    else
                        ApplyRemainingOprt(stOpt, stVal);
                }

				m_compilingState.m_byteCode.Finalize();
				break;
			}

			// Commented out - might be necessary for deep debugging stuff
			//if (ParserBase::g_DbgDumpStack)
			//{
//				StackDump(stVal, stOpt);
//				m_compilingState.m_byteCode.AsciiDump();
			//}
		} // while (true)


#ifndef PARSERSTANDALONE
		if (ParserBase::g_DbgDumpCmdCode)
#endif // PARSERSTANDALONE
			m_compilingState.m_byteCode.AsciiDump();

		if (m_nIfElseCounter > 0)
			Error(ecMISSING_ELSE_CLAUSE);

		// get the last value (= final result) from the stack
		MUP_ASSERT(stArgCount.size() == 1);

		m_compilingState.m_numResults = stArgCount.top();

		if (m_compilingState.m_numResults == 0)
			Error(ecINTERNAL_ERROR, 9);

		if (stVal.size() == 0)
			Error(ecEMPTY_EXPRESSION);

		if (stVal.top().GetType() != tpDBL)
			Error(ecSTR_RESULT);

		m_compilingState.m_stackBuffer.resize(m_compilingState.m_byteCode.GetMaxStackSize() * nMaxThreads);
	}

	//---------------------------------------------------------------------------
	/** \brief One of the two main parse functions.
	    \sa ParseCmdCode(...)

	  Parse expression from input string. Perform syntax checking and create
	  bytecode. After parsing the string and creating the bytecode the function
	  pointer #m_pParseFormula will be changed to the second parse routine the
	  uses bytecode instead of string parsing.
	*/
	void ParserBase::ParseString()
	{
		CreateRPN();
        m_compilingState.m_usedVar = m_pTokenReader->GetUsedVar();
        m_compilingState.m_expr = m_pTokenReader->GetExpr().to_string();
        StripSpaces(m_compilingState.m_expr);

		if (bMakeLoopByteCode
            && !bPauseLoopByteCode
            && m_stateStacks(nthLoopElement, nthLoopPartEquation).m_valid)
		{
		    State& state = m_stateStacks(nthLoopElement, nthLoopPartEquation);
		    state = m_compilingState;
		    m_state = &state;
		}
		else
            m_state = &m_compilingState;

		m_pParseFormula = &ParserBase::ParseCmdCode;
		//(this->*m_pParseFormula)();
	}

	//---------------------------------------------------------------------------
	/** \brief Create an error containing the parse error position.

	  This function will create an Parser Exception object containing the error text and
	  its position.

	  \param a_iErrc [in] The error code of type #EErrorCodes.
	  \param a_iPos [in] The position where the error was detected.
	  \param a_strTok [in] The token string representation associated with the error.
	  \throw ParserException always throws thats the only purpose of this function.
	*/
	void  ParserBase::Error(EErrorCodes a_iErrc, int a_iPos, const string_type& a_sTok) const
	{
        throw exception_type(a_iErrc, a_sTok, m_pTokenReader->GetExpr().to_string(), a_iPos);
	}

	//---------------------------------------------------------------------------
	/** \brief Create an error containing the parse error position.

	  This function will create an Parser Exception object containing the error text and
	  its position.

	  \param a_iErrc [in] The error code of type #EErrorCodes.
	  \param a_Expr [in] The erroneous expression
	  \param a_iPos [in] The position where the error was detected.
	  \param a_strTok [in] The token string representation associated with the error.
	  \throw ParserException always throws thats the only purpose of this function.
	*/
	void  ParserBase::Error(EErrorCodes a_iErrc, const string_type& a_Expr, int a_iPos, const string_type& a_sTok) const
	{
		throw exception_type(a_iErrc, a_sTok, a_Expr, a_iPos);
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined variables.
	    \throw nothrow

	    Resets the parser to string parsing mode by calling #ReInit.
	*/
	void ParserBase::ClearVar()
	{
		m_factory->Clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Remove a variable from internal storage.
	    \throw nothrow

	    Removes a variable if it exists. If the variable was created internally, its
	    associated memory is freed. If the Variable does not exist nothing will be done.
	*/
	void ParserBase::RemoveVar(const string_type& a_strVarName)
	{
	    if (m_factory->Remove(a_strVarName))
            ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all functions.
	    \post Resets the parser to string parsing mode.
	    \throw nothrow
	*/
	void ParserBase::ClearFun()
	{
		m_FunDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined constants.

	    Both numeric and string constants will be removed from the internal storage.
	    \post Resets the parser to string parsing mode.
	    \throw nothrow
	*/
	void ParserBase::ClearConst()
	{
		m_ConstDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined postfix operators.
	    \post Resets the parser to string parsing mode.
	    \throw nothrow
	*/
	void ParserBase::ClearPostfixOprt()
	{
		m_PostOprtDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined binary operators.
	    \post Resets the parser to string parsing mode.
	    \throw nothrow
	*/
	void ParserBase::ClearOprt()
	{
		m_OprtDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear the user defined Prefix operators.
	    \post Resets the parser to string parser mode.
	    \throw nothrow
	*/
	void ParserBase::ClearInfixOprt()
	{
		m_InfixOprtDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Enable or disable the formula optimization feature.
	    \post Resets the parser to string parser mode.
	    \throw nothrow
	*/
	void ParserBase::EnableOptimizer(bool a_bIsOn)
	{
		m_compilingState.m_byteCode.EnableOptimizer(a_bIsOn);
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief Enable the dumping of bytecode amd stack content on the console.
	    \param bDumpCmd Flag to enable dumping of the current bytecode to the console.
	    \param bDumpStack Flag to enable dumping of the stack content is written to the console.

	   This function is for debug purposes only!
	*/
	void ParserBase::EnableDebugDump(bool bDumpCmd, bool bDumpStack)
	{
		ParserBase::g_DbgDumpCmdCode = bDumpCmd;
		ParserBase::g_DbgDumpStack   = bDumpStack;
	}

	//------------------------------------------------------------------------------
	/** \brief Enable or disable the built in binary operators.
	    \throw nothrow
	    \sa m_bBuiltInOp, ReInit()

	  If you disable the built in binary operators there will be no binary operators
	  defined. Thus you must add them manually one by one. It is not possible to
	  disable built in operators selectively. This function will Reinitialize the
	  parser by calling ReInit().
	*/
	void ParserBase::EnableBuiltInOprt(bool a_bIsOn)
	{
		m_bBuiltInOp = a_bIsOn;
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Query status of built in variables.
	    \return #m_bBuiltInOp; true if built in operators are enabled.
	    \throw nothrow
	*/
	bool ParserBase::HasBuiltInOprt() const
	{
		return m_bBuiltInOp;
	}

	//------------------------------------------------------------------------------
	/** \brief Get the argument separator character.
	*/
	char_type ParserBase::GetArgSep() const
	{
		return m_pTokenReader->GetArgSep();
	}

	//------------------------------------------------------------------------------
	/** \brief Set argument separator.
	    \param cArgSep the argument separator character.
	*/
	void ParserBase::SetArgSep(char_type cArgSep)
	{
		m_pTokenReader->SetArgSep(cArgSep);
	}

	//------------------------------------------------------------------------------
	/** \brief Dump stack content.

	    This function is used for debugging only.
	*/
	void ParserBase::StackDump(const ParserStack<token_type>& a_stVal,
							   const ParserStack<token_type>& a_stOprt) const
	{
		ParserStack<token_type> stOprt(a_stOprt),
					stVal(a_stVal);

        print("Value stack:");
        printFormatted("|-> ");

		while ( !stVal.empty() )
		{
			token_type val = stVal.pop();

            if (val.GetType() == tpSTR)
                printFormatted(" \"" + val.GetAsString() + "\" ");
            else if (val.GetCode() == cmVARARRAY)
                printFormatted(" VARARRAY ");
			else
                printFormatted(" " + val.GetVal().print() + " ");
		}

		printFormatted("  \n");
		printFormatted("|-> Operator stack:\n");

		while ( !stOprt.empty() )
		{
			if (stOprt.top().GetCode() <= cmLOR)
			{
			    printFormatted("|   OPRT_INTRNL \"" + string(ParserBase::c_DefaultOprt[stOprt.top().GetCode()]) + "\"\n");
			}
			else
			{
				switch (stOprt.top().GetCode())
				{
					case cmVAR:
					    printFormatted("|   VAR\n");
						break;
					case cmVAL:
						printFormatted("|   VAL\n");
						break;
					case cmFUNC:
					    printFormatted("|   FUNC \"" + stOprt.top().GetAsString() + "\"\n");
						break;
					case cmMETHOD:
					    printFormatted("|   METHOD \"" + stOprt.top().GetAsString() + "\"\n");
						break;
					case cmOPRT_INFIX:
						printFormatted("|   OPRT_INF \"" + stOprt.top().GetAsString() + "\"\n");
						break;
					case cmOPRT_BIN:
						printFormatted("|   OPRT_BIN \"" + stOprt.top().GetAsString() + "\"\n");
						break;
					case cmEND:
						printFormatted("|   END\n");
						break;
					case cmUNKNOWN:
						printFormatted("|   UNKNOWN\n");
						break;
					case cmBO:
						printFormatted("|   BRACKET \"(\"\n");
						break;
					case cmBC:
						printFormatted("|   BRACKET \")\"\n");
						break;
					case cmVO:
						printFormatted("|   VECTOR \"{\"\n");
						break;
					case cmVC:
						printFormatted("|   VECTOR \"}\"\n");
						break;
					case cmEXP2:
						printFormatted("|   VECT-EXP A:B\n");
						break;
					case cmEXP3:
						printFormatted("|   VECT-EXP A:B:C\n");
						break;
					case cmVAL2STR:
						printFormatted("|   VAL2STR\n");
						break;
					case cmIF:
						printFormatted("|   IF\n");
						break;
					case cmELSE:
						printFormatted("|   ELSE\n");
						break;
					case cmENDIF:
						printFormatted("|   ENDIF\n");
						break;
					default:
					    printFormatted("|   " + toString(stOprt.top().GetCode()) + "\n");
						break;
				}
			}

			stOprt.pop();
		}

	}


    /////////////////////////////////////////////////
    /// \brief This member function returns the next
    /// free vector index, which can be used to
    /// create a new temporary vector.
    ///
    /// \return string_type
    ///
    /// This function is called by
    /// ParserBase::CreateTempVar(), which
    /// creates the actual vector.
    /////////////////////////////////////////////////
	string_type ParserBase::getNextTempVarIndex()
	{
	    if (bMakeLoopByteCode)
        {
            string_type sIndex = toString(nthLoopElement) + "_" + toString(nthLoopPartEquation) + "_" + toString(nCurrVectorIndex);
            nCurrVectorIndex++;
            return sIndex;
        }
        else
            return toString(m_factory->ManagedSize());
	}


    /////////////////////////////////////////////////
    /// \brief Simple helper function to print the
    /// buffer's contents.
    ///
    /// \param buffer const std::vector<Array>&
    /// \param nElems int
    /// \return std::string
    ///
    /////////////////////////////////////////////////
	static std::string printVector(const std::vector<Array>& buffer, int nElems)
	{
	    std::string s;

	    for (int i = 0; i < nElems; i++)
            s += buffer[i].print() + ",";

        s.pop_back();

        return s;
	}


    /////////////////////////////////////////////////
    /// \brief Evaluate an expression containing
    /// comma seperated subexpressions.
    ///
    /// This member function can be used to retrieve
    /// all results of an expression made up of
    /// multiple comma seperated subexpressions (i.e.
    /// "x+y,sin(x),cos(y)").
    ///
    /// \param nStackSize int&
    /// \return Array*
    ///
    /////////////////////////////////////////////////
	Array* ParserBase::Eval(int& nStackSize)
	{
	    // Run the evaluation
        (this->*m_pParseFormula)();

        nStackSize = m_state->m_numResults;

        // Copy the actual results (ignore the 0-th term)
        m_buffer.resize(nStackSize);

        for (int i = 0; i < nStackSize; i++)
        {
            m_buffer[i] = m_state->m_stackBuffer[i+1].get();
        }

        // assign the results of the calculation to a possible
        // temporary vector
        /*
        ExpressionTarget& target = getTarget();

        if (target.isValid() && m_state->m_usedVar.find("_~TRGTVCT[~]") != m_state->m_usedVar.end())
            target.assign(m_buffer, nStackSize);*/

        if (g_DbgDumpStack)
            print("ParserBase::Eval() @ ["
                                + toString(nthLoopElement) + "," + toString(nthLoopPartEquation)
                                + "] m_buffer[:] = {" + printVector(m_buffer, nStackSize) + "}");

        if (bMakeLoopByteCode && !bPauseLoopByteCode)
        {
            nthLoopPartEquation++;
            nCurrVectorIndex = 0;

            if (m_stateStacks[nthLoopElement].m_states.size() <= nthLoopPartEquation)
                m_stateStacks[nthLoopElement].m_states.push_back(State());

            m_state = &m_stateStacks(nthLoopElement, nthLoopPartEquation);
        }

        return &m_buffer[0];
	}


    /////////////////////////////////////////////////
    /// \brief Single-value wrapper around the
    /// vectorized overload of this member function.
    ///
    /// \return Array
    ///
    /////////////////////////////////////////////////
	Array ParserBase::Eval() // declared as deprecated
	{
	    Array* v;
	    int nResults;
#ifdef PARSERSTANDALONE
	    {
	        Timer t("ParserBase::Eval");
            for (size_t i = 0; i < 1000; i++)
#endif
                v = Eval(nResults);
#ifdef PARSERSTANDALONE
	    }
#endif

	    return v[0];
	}


    /////////////////////////////////////////////////
    /// \brief Change or remove the internal variable
    /// aliasing map.
    ///
    /// \param std::map<std::string
    /// \param aliases std::string>*
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::SetVarAliases(std::map<std::string, std::string>* aliases)
	{
	    m_factory->m_VarAliases = aliases;
	}


    /////////////////////////////////////////////////
    /// \brief Activates the loop mode and prepares
    /// the internal arrays for storing the necessary
    /// data.
    ///
    /// \param _nLoopLength size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::ActivateLoopMode(size_t _nLoopLength)
    {
        if (!bMakeLoopByteCode)
        {
            if (g_DbgDumpStack)
                print("DEBUG: Activated loop mode");

            nthLoopElement = 0;
            nCurrVectorIndex = 0;
            nthLoopPartEquation = 0;
            bMakeLoopByteCode = true;
            DeactivateLoopMode();
            bMakeLoopByteCode = true;
            bCompiling = false;
            m_stateStacks.resize(_nLoopLength);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Deactivates the loop mode and resets
    /// the internal arrays.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::DeactivateLoopMode()
    {
        if (bMakeLoopByteCode)
        {
            if (g_DbgDumpStack)
                print("DEBUG: Deactivated loop mode");
            nthLoopElement = 0;
            nthLoopPartEquation = 0;
            nCurrVectorIndex = 0;
            bMakeLoopByteCode = false;
            bCompiling = false;
            m_stateStacks.clear();
            m_state = &m_compilingState;
        }
    }


    /////////////////////////////////////////////////
    /// \brief Activates the selected position in the
    /// internally stored bytecode.
    ///
    /// \param _nLoopElement size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::SetIndex(size_t _nLoopElement)
    {
        nthLoopElement = _nLoopElement;
        nCurrVectorIndex = 0;
        nthLoopPartEquation = 0;
        m_state = &m_stateStacks(nthLoopElement, nthLoopPartEquation);
    }


    /////////////////////////////////////////////////
    /// \brief Activate the compiling step for the
    /// parser.
    ///
    /// \param _bCompiling bool
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::SetCompiling(bool _bCompiling)
    {
        bCompiling = _bCompiling;
    }


    /////////////////////////////////////////////////
    /// \brief Returns true, if the parser is
    /// currently in compiling step.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ParserBase::IsCompiling()
    {
        return bCompiling;
    }


    /////////////////////////////////////////////////
    /// \brief Store the passed data access for this
    /// position internally.
    ///
    /// \param _access const CachedDataAccess&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::CacheCurrentAccess(const CachedDataAccess& _access)
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
            m_stateStacks[nthLoopElement].m_cache.m_accesses.push_back(_access);
    }


    /////////////////////////////////////////////////
    /// \brief Evaluate, whether there are any cached
    /// data accesses for this position.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t ParserBase::HasCachedAccess()
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode && m_stateStacks[nthLoopElement].m_cache.m_enabled)
            return m_stateStacks[nthLoopElement].m_cache.m_accesses.size();

        return 0;
    }


    /////////////////////////////////////////////////
    /// \brief Disable the data access caching for
    /// this position.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::DisableAccessCaching()
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
        {
            m_stateStacks[nthLoopElement].m_cache.m_enabled = false;
            m_stateStacks[nthLoopElement].m_cache.m_accesses.clear();
        }
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether the current position
    /// can cache any data accesses.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ParserBase::CanCacheAccess()
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
            return m_stateStacks[nthLoopElement].m_cache.m_enabled;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Returns the cached data access for the
    /// passed position.
    ///
    /// \param nthAccess size_t
    /// \return const CachedDataAccess&
    ///
    /////////////////////////////////////////////////
    const CachedDataAccess& ParserBase::GetCachedAccess(size_t nthAccess)
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode && m_stateStacks[nthLoopElement].m_cache.m_accesses.size() > nthAccess)
            return m_stateStacks[nthLoopElement].m_cache.m_accesses[nthAccess];

        return CachedDataAccess();
    }


    /////////////////////////////////////////////////
    /// \brief Caches the passed equation for this
    /// position.
    ///
    /// \param sEquation const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::CacheCurrentEquation(const string& sEquation)
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
            m_stateStacks[nthLoopElement].m_cache.m_expr = sEquation;
    }


    /////////////////////////////////////////////////
    /// \brief Returns the stored equation for this
    /// position.
    ///
    /// \return const std::string&
    ///
    /////////////////////////////////////////////////
    const std::string& ParserBase::GetCachedEquation() const
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
            return m_stateStacks[nthLoopElement].m_cache.m_expr;

        return EMPTYSTRING;
    }


    /////////////////////////////////////////////////
    /// \brief Caches the passed target equation for
    /// this position.
    ///
    /// \param sEquation const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::CacheCurrentTarget(const string& sEquation)
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
            m_stateStacks[nthLoopElement].m_cache.m_target = sEquation;
    }


    /////////////////////////////////////////////////
    /// \brief Returns the stored target equation for
    /// this position.
    ///
    /// \return const std::string&
    ///
    /////////////////////////////////////////////////
    const std::string& ParserBase::GetCachedTarget() const
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
            return m_stateStacks[nthLoopElement].m_cache.m_target;

        return EMPTYSTRING;
    }


    /////////////////////////////////////////////////
    /// \brief This member function returns, whether
    /// the current equation is already parsed and
    /// there's a valid bytecode for it.
    ///
    /// \param _nthLoopElement size_t
    /// \param _nthPartEquation size_t
    /// \return int
    ///
    /////////////////////////////////////////////////
    int ParserBase::IsValidByteCode(size_t _nthLoopElement, size_t _nthPartEquation)
    {
        if (!bMakeLoopByteCode)
            return 0;

        if (_nthLoopElement < m_stateStacks.size())
            return m_stateStacks(_nthLoopElement, _nthPartEquation).m_valid;
        else
            return m_stateStacks(nthLoopElement, _nthPartEquation).m_valid;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether the loop mode is
    /// active. This function returns true even if
    /// the loop mode is paused.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ParserBase::ActiveLoopMode() const
    {
        return bMakeLoopByteCode;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether the pause mode is
    /// locked.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ParserBase::IsLockedPause() const
    {
        return bPauseLock;
    }


    /////////////////////////////////////////////////
    /// \brief This member function locks the pause
    /// mode so that it cannot be accidentally
    /// activated.
    ///
    /// \param _bLock bool
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::LockPause(bool _bLock)
    {
        bPauseLock = _bLock;
    }


    /////////////////////////////////////////////////
    /// \brief This member function pauses the loop
    /// mode, so that the new assigned equation does
    /// not invalidate already parsed equations.
    ///
    /// \param _bPause bool
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ParserBase::PauseLoopMode(bool _bPause)
    {
        if (bMakeLoopByteCode)
        {
            if (g_DbgDumpStack)
                print("DEBUG: Set loop pause mode to: " + toString(_bPause));

            bPauseLoopByteCode = _bPause;

            if (!_bPause)
            {
                m_state = &m_stateStacks(nthLoopElement, nthLoopPartEquation);
                m_pParseFormula = &ParserBase::ParseCmdCode;
            }
            else
                m_state = &m_compilingState;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function checks, whether
    /// the passed expression is already parsed, so
    /// that the parsing step may be omitted.
    ///
    /// \param sNewEquation StringView
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ParserBase::IsAlreadyParsed(StringView sNewEquation)
    {
        StringView sCurrentEquation(GetExpr());
        sNewEquation.strip();

        if (sNewEquation == sCurrentEquation
            && (!bMakeLoopByteCode || bPauseLoopByteCode || m_state->m_valid))
            return true;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether there are more elements
    /// on the parsing stack remaining.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool ParserBase::IsNotLastStackItem() const
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode)
            return nthLoopPartEquation+1 < m_stateStacks[nthLoopElement].m_states.size();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function copies the passed
    /// vector into the internal storage referencing
    /// it with a auto-generated variable name.
    ///
    /// \param vVar const Array&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
	std::string ParserBase::CreateTempVar(const Array& vVar)
	{
	    std::string sTempVarName = "_~TV[" + getNextTempVarIndex() + "]";

        if (!vVar.size())
            SetInternalVar(sTempVarName, Value());
        else
            SetInternalVar(sTempVarName, vVar);

        return sTempVarName;
	}


    /////////////////////////////////////////////////
    /// \brief This member function copies the passed
    /// vector into the internal storage referencing
    /// it with the passed name.
    ///
    /// \param sVarName const std::string&
    /// \param vVar const Array&
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::SetInternalVar(const std::string& sVarName, const Array& vVar)
	{
		//if (!vVar.size())
		//	return;

        Variable* var = m_factory->Get(sVarName);

		if (!var)
		{
		    var = m_factory->Create(sVarName);
		    *var = vVar;
		}
		else
			var->overwrite(vVar); // Force-overwrite in this case

        mInternalVars[sVarName] = var;
	}


    /////////////////////////////////////////////////
    /// \brief This member function returns a pointer
    /// to the variable stored internally.
    ///
    /// \param sVarName const std::string&
    /// \return Variable*
    ///
    /////////////////////////////////////////////////
	Variable* ParserBase::GetInternalVar(const std::string& sVarName)
	{
		if (mInternalVars.find(sVarName) == mInternalVars.end())
			return nullptr;

		return mInternalVars[sVarName];
	}


    /////////////////////////////////////////////////
    /// \brief This member function cleares the
    /// internal variable storage.
    ///
    /// \param bIgnoreProcedureVects bool
    /// \return void
    ///
    /////////////////////////////////////////////////
	void ParserBase::ClearInternalVars(bool bIgnoreProcedureVects)
	{
		if (!mInternalVars.size())
			return;

		auto iter = mInternalVars.begin();

		while (iter != mInternalVars.end())
		{
			string siter = iter->first;

			if (siter.find('[') != string::npos && siter.find(']') != string::npos)
			{
				if (bIgnoreProcedureVects && siter.starts_with("_~PROC~["))
				{
					iter++;
					continue;
				}

				RemoveVar(siter);
				iter = mInternalVars.erase(iter);
			}
			else
				iter = mInternalVars.erase(iter); //iter++;
		}

		if (!bIgnoreProcedureVects || !mInternalVars.size())
        {
            //g_logger.debug("Clearing vector vars and target.");
			mInternalVars.clear();
            m_compilingTarget.clear();
        }
	}


    /////////////////////////////////////////////////
    /// \brief This member function checks, whether
    /// the passed expression contains a vector.
    ///
    /// \param sExpr StringView
    /// \param ignoreSingletons bool
    /// \return bool
    ///
    /////////////////////////////////////////////////
	bool ParserBase::ContainsInternalVars(StringView sExpr, bool ignoreSingletons)
	{
	    for (auto iter = mInternalVars.begin(); iter != mInternalVars.end(); ++iter)
        {
            if (ignoreSingletons && iter->second->size() == 1)
                continue;

            size_t nPos = sExpr.find(iter->first);

            if (nPos != string::npos && sExpr.is_delimited_sequence(nPos, iter->first.length(), StringViewBase::PARSER_DELIMITER))
                return true;
        }

        static std::vector<std::string> vNDVECTFUNCS = {"logtoidx", "idxtolog"};

        for (const auto& func : vNDVECTFUNCS)
        {
            size_t nPos = sExpr.find(func);

            if (nPos != string::npos && sExpr.is_delimited_sequence(nPos, func.length(), StringViewBase::PARSER_DELIMITER))
                return true;
        }


        return false;
	}


    /////////////////////////////////////////////////
    /// \brief This member function checks, whether
    /// the passed expression contains string
    /// variables.
    ///
    /// \param sExpr StringView
    /// \return bool
    ///
    /////////////////////////////////////////////////
	bool ParserBase::ContainsStringVars(StringView sExpr)
	{
	    const varmap_type& vmap = GetVar();

	    for (const auto& iter : vmap)
        {
            if (iter.second->getCommonType() != TYPE_STRING)
                continue;

            size_t nPos = sExpr.find(iter.first);

            if (nPos != string::npos && sExpr.is_delimited_sequence(nPos, iter.first.length(), StringViewBase::PARSER_DELIMITER))
                return true;
        }

        return false;
	}
} // namespace mu

