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

//--- Standard includes ------------------------------------------------------------------------
#include <cassert>
#include <cmath>
#include <memory>
#include <vector>
#include <deque>
#include <sstream>
#include <locale>

#ifdef MUP_USE_OPENMP
#include <omp.h>
#endif

using namespace std;
string toString(int);
unsigned int getMatchingParenthesis(const string&);
mu::value_type parser_Num(const mu::value_type*, int);
mu::value_type parser_Cnt(const mu::value_type*, int);
mu::value_type parser_and(const mu::value_type*, int);
mu::value_type parser_or(const mu::value_type*, int);
mu::value_type parser_Norm(const mu::value_type*, int);
mu::value_type parser_product(const mu::value_type*, int);
mu::value_type parser_Sum(const mu::value_type*, int);
mu::value_type parser_Avg(const mu::value_type*, int);
mu::value_type parser_Med(const mu::value_type*, int);
mu::value_type parser_Pct(const mu::value_type*, int);
mu::value_type parser_Std(const mu::value_type*, int);
mu::value_type parser_compare(const mu::value_type*, int);
mu::value_type parser_Min(const mu::value_type*, int);
mu::value_type parser_Max(const mu::value_type*, int);

/** \file
    \brief This file contains the basic implementation of the muparser engine.
*/

namespace mu
{
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
        _nrT("<="), _nrT(">="),  _nrT("!="),
        _nrT("=="), _nrT("<"),   _nrT(">"),
        _nrT("+"),  _nrT("-"),   _nrT("*"),
        _nrT("/"),  _nrT("^"),   _nrT("&&"),
        _nrT("||"), _nrT("="),   _nrT("("),
        _nrT(")"),   _nrT("?"),  _nrT(":"), 0
    };

    //------------------------------------------------------------------------------
    /** \brief Constructor.
        \param a_szFormula the formula to interpret.
        \throw ParserException if a_szFormula is null.
    */
    ParserBase::ParserBase()
        : m_pParseFormula(&ParserBase::ParseString)
        , m_vRPN()
        , m_vStringBuf()
        , m_pTokenReader()
        , m_FunDef()
        , m_PostOprtDef()
        , m_InfixOprtDef()
        , m_OprtDef()
        , m_ConstDef()
        , m_StrVarDef()
        , m_VarDef()
        , m_bBuiltInOp(true)
        , m_sNameChars()
        , m_sOprtChars()
        , m_sInfixOprtChars()
        , m_nIfElseCounter(0)
        , m_vStackBuffer()
        , m_nFinalResultIdx(0)
    {
        InitTokenReader();
        nthLoopElement = 0;
        nLoopLength = 0;
        bMakeLoopByteCode = false;
        bPauseLoopByteCode = false;
        bPauseLock = false;
        nVectorIndex = 0;
        nVectorVarsSize = 200;
        dVectorVars = 0;
        mVarMapPntr = 0;
    }

    //---------------------------------------------------------------------------
    /** \brief Copy constructor.

      Tha parser can be safely copy constructed but the bytecode is reset during
      copy construction.
    */
    ParserBase::ParserBase(const ParserBase& a_Parser)
        : m_pParseFormula(&ParserBase::ParseString)
        , m_vRPN()
        , m_vStringBuf()
        , m_pTokenReader()
        , m_FunDef()
        , m_PostOprtDef()
        , m_InfixOprtDef()
        , m_OprtDef()
        , m_ConstDef()
        , m_StrVarDef()
        , m_VarDef()
        , m_bBuiltInOp(true)
        , m_sNameChars()
        , m_sOprtChars()
        , m_sInfixOprtChars()
        , m_nIfElseCounter(0)
    {
        m_pTokenReader.reset(new token_reader_type(this));
        nthLoopElement = 0;
        nLoopLength = 0;
        bMakeLoopByteCode = false;
        bPauseLoopByteCode = false;
        bPauseLock = false;
        nVectorIndex = 0;
        nVectorVarsSize = 200;
        dVectorVars = 0;
        mVarMapPntr = 0;

        Assign(a_Parser);
    }

    //---------------------------------------------------------------------------
    ParserBase::~ParserBase()
    {
        if (dVectorVars)
            delete[] dVectorVars;
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

        m_ConstDef        = a_Parser.m_ConstDef;         // Copy user define constants
        m_VarDef          = a_Parser.m_VarDef;           // Copy user defined variables
        m_bBuiltInOp      = a_Parser.m_bBuiltInOp;
        m_vStringBuf      = a_Parser.m_vStringBuf;
        m_vStackBuffer    = a_Parser.m_vStackBuffer;
        m_nFinalResultIdx = a_Parser.m_nFinalResultIdx;
        m_StrVarDef       = a_Parser.m_StrVarDef;
        m_vStringVarBuf   = a_Parser.m_vStringVarBuf;
        m_nIfElseCounter  = a_Parser.m_nIfElseCounter;
        m_pTokenReader.reset(a_Parser.m_pTokenReader->Clone(this));

        // Copy function and operator callbacks
        m_FunDef = a_Parser.m_FunDef;             // Copy function definitions
        m_PostOprtDef = a_Parser.m_PostOprtDef;   // post value unary operators
        m_InfixOprtDef = a_Parser.m_InfixOprtDef; // unary operators for infix notation
        m_OprtDef = a_Parser.m_OprtDef;           // binary operators

        m_sNameChars = a_Parser.m_sNameChars;
        m_sOprtChars = a_Parser.m_sOprtChars;
        m_sInfixOprtChars = a_Parser.m_sInfixOprtChars;
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
        m_vStringBuf.clear();
        m_vRPN.clear();
        m_pTokenReader->ReInit();
        m_nIfElseCounter = 0;
    }

    //---------------------------------------------------------------------------
    void ParserBase::OnDetectVar(string_type* pExpr, int& nStart, int& nEnd)
    {
        if (mVectorVars.size())
        {
            if (mVectorVars.find(pExpr->substr(nStart, nEnd - nStart)) != mVectorVars.end())
                return;
            std::vector<double> vVar;
            if (GetVar().find(pExpr->substr(nStart, nEnd - nStart)) != GetVar().end())
            {
                vVar[0] = *(GetVar().find(pExpr->substr(nStart, nEnd - nStart))->second);
            }
            else
                vVar[0] = 0.0;
            SetVectorVar(pExpr->substr(nStart, nEnd - nStart), vVar);
        }
        return;
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
    /** \brief Set a function that can create variable pointer for unknown expression variables.
        \param a_pFactory A pointer to the variable factory.
        \param pUserData A user defined context pointer.
    */
    void ParserBase::SetVarFactory(facfun_type a_pFactory, void* pUserData)
    {
        m_pTokenReader->SetVarCreator(a_pFactory, pUserData);
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

        if ( pFunMap != &m_PostOprtDef && m_PostOprtDef.find(a_strName) != m_PostOprtDef.end() )
            Error(ecNAME_CONFLICT, -1, a_strName);

        if ( pFunMap != &m_InfixOprtDef && pFunMap != &m_OprtDef && m_InfixOprtDef.find(a_strName) != m_InfixOprtDef.end() )
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

    //---------------------------------------------------------------------------
    /** \brief Set the formula.
        \param a_strFormula Formula as string_type
        \throw ParserException in case of syntax errors.

        Triggers first time calculation thus the creation of the bytecode and
        scanning of used variables.
    */
    void ParserBase::SetExpr(const string_type& a_sExpr)
    {
        //if (!bPauseLoopByteCode)
        //std::cerr << "EXPR: \"" << a_sExpr << "\"" << endl;
        // Check locale compatibility
        std::locale loc;
        if (m_pTokenReader->GetArgSep() == std::use_facet<numpunct<char_type> >(loc).decimal_point())
            Error(ecLOCALE);

        // <ibg> 20060222: Bugfix for Borland-Kylix:
        // adding a space to the expression will keep Borlands KYLIX from going wild
        // when calling tellg on a stringstream created from the expression after
        // reading a value at the end of an expression. (mu::Parser::IsVal function)
        // (tellg returns -1 otherwise causing the parser to ignore the value)
        string_type sBuf(a_sExpr + _nrT(" ") );
        if (sBuf.find('{') != string::npos && sBuf.find('}', sBuf.find('{')) != string::npos)
        {
            PreEvaluateVectors(sBuf);
        }
        //std::cerr << sBuf << endl;

        if (sBuf.find('{') != string::npos || sBuf.find('}') != string::npos)
            Error(ecMISSING_PARENS);
        /*if (bMakeLoopByteCode && !bPauseLoopByteCode && vValidByteCode[nthLoopElement])
            vLoopString[nthLoopElement] = sBuf;*/
        //std::cerr << 1 << endl;
        if (mVarMapPntr)
            replaceLocalVars(sBuf);
        //std::cerr << 2 << endl;
        m_pTokenReader->SetFormula(sBuf);
        //std::cerr << 3 << endl;
        ReInit();
    }

    void ParserBase::PreEvaluateVectors(std::string& sExpr)
    {
        PauseLoopMode();
        std::string sMultiArgFunc;
        vector<double> vResults;
        value_type* v = 0;
        int nResults;
        //int nVectors = 0;
        for (unsigned int i = 0; i < sExpr.length(); i++)
        {
            if (sExpr[i] == '{' && sExpr.find('}', i) != string::npos)
            {
                size_t nMultiArgParens = FindMultiArgFunc(sExpr, i, sMultiArgFunc);
                vResults.clear();
                if (nMultiArgParens != std::string::npos)
                {
                    size_t nClosingParens = getMatchingParenthesis(sExpr.substr(nMultiArgParens))+nMultiArgParens;
                    SetExpr(sExpr.substr(nMultiArgParens+1, nClosingParens-nMultiArgParens-1));
                    v = Eval(nResults);
                    if (sMultiArgFunc == "num")
                        vResults.push_back(parser_Num(v, nResults));
                    if (sMultiArgFunc == "cnt")
                        vResults.push_back(parser_Cnt(v, nResults));
                    if (sMultiArgFunc == "and")
                        vResults.push_back(parser_and(v, nResults));
                    if (sMultiArgFunc == "or")
                        vResults.push_back(parser_or(v, nResults));
                    if (sMultiArgFunc == "norm")
                        vResults.push_back(parser_Norm(v, nResults));
                    if (sMultiArgFunc == "prd")
                        vResults.push_back(parser_product(v, nResults));
                    if (sMultiArgFunc == "sum")
                        vResults.push_back(parser_Sum(v, nResults));
                    if (sMultiArgFunc == "avg")
                        vResults.push_back(parser_Avg(v, nResults));
                    if (sMultiArgFunc == "med")
                        vResults.push_back(parser_Med(v, nResults));
                    if (sMultiArgFunc == "pct")
                        vResults.push_back(parser_Pct(v, nResults));
                    if (sMultiArgFunc == "std")
                        vResults.push_back(parser_Std(v, nResults));
                    if (sMultiArgFunc == "cmp")
                        vResults.push_back(parser_compare(v, nResults));
                    if (sMultiArgFunc == "min")
                        vResults.push_back(parser_Min(v, nResults));
                    if (sMultiArgFunc == "max")
                        vResults.push_back(parser_Max(v, nResults));
                    sExpr.replace(nMultiArgParens-sMultiArgFunc.length(), nClosingParens-nMultiArgParens+1+sMultiArgFunc.length(), "~TMAFV[" + toString(nVectorIndex) + "]");
                    SetVectorVar("~TMAFV[" + toString(nVectorIndex) + "]", vResults);
                }
                else
                {
                    size_t j = getMatchingParenthesis(sExpr.substr(i));
                    if (j != std::string::npos)
                    {
                        j += i;
                        SetExpr(sExpr.substr(i + 1, j - i - 1));
                        v = Eval(nResults);
                        for (int n = 0; n < nResults; n++)
                        {
                            vResults.push_back(v[n]);
                        }
                        if (sExpr.find_first_not_of(' ') == i
                                && sExpr.find('=', j) != std::string::npos
                                && sExpr.find('=', j) < sExpr.length() - 1
                                && sExpr.find('!', j) != sExpr.find('=', j) - 1
                                && sExpr.find('<', j) != sExpr.find('=', j) - 1
                                && sExpr.find('>', j) != sExpr.find('=', j) - 1
                                && sExpr[sExpr.find('=', j) + 1] != '=')
                        {
                            sTargets = sExpr.substr(i + 1, j - i - 1);
                            SetVectorVar("~TRGTVCT[~]", vResults);
                            sExpr.replace(i, j + 1 - i, "~TRGTVCT[~]");
                            mTargets = GetUsedVar();
                        }
                        else
                        {
                            sExpr.replace(i, j + 1 - i, "~TV[" + toString(nVectorIndex) + "]");
                            SetVectorVar("~TV[" + toString(nVectorIndex) + "]", vResults);
                        }
                    }
                }
            }
        }
        PauseLoopMode(false);
    }

    size_t ParserBase::FindMultiArgFunc(const std::string& sExpr, size_t nPos, std::string& sMultArgFunc)
    {
        for (int i = nPos; i >= 2; i--)
        {
            if (sExpr[i] == '(' && isalpha(sExpr[i-1]))
            {
                size_t nParPos = getMatchingParenthesis(sExpr.substr(i));
                if (nParPos == string::npos)
                    return std::string::npos;
                else
                    nParPos += i;
                if (nParPos < nPos)
                    continue;
                size_t nSep = sExpr.find_last_of(" +-*/(=:?&|<>!%{^", i-1)+1;
                std::string sFunc = sExpr.substr(nSep, i-nSep);
                if (sFunc == "num"
                    || sFunc == "cnt"
                    || sFunc == "prd"
                    || sFunc == "sum"
                    || sFunc == "and"
                    || sFunc == "or"
                    || sFunc == "avg"
                    || sFunc == "med"
                    || sFunc == "pct"
                    || sFunc == "std"
                    || sFunc == "norm"
                    || sFunc == "cmp"
                    || sFunc == "min"
                    || sFunc == "max")
                {
                    sMultArgFunc = sFunc;
                    return i;
                }
            }
        }
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
                                       bool a_bAllowOpt)
    {
        AddCallback(a_sName,
                    ParserCallback(a_pFun, a_bAllowOpt, prPOSTFIX, cmOPRT_POSTFIX),
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
        \param [in] a_bAllowOpt  True if operator is volatile (default=false)
        \sa EPrec
    */
    void ParserBase::DefineInfixOprt(const string_type& a_sName,
                                     fun_type1 a_pFun,
                                     int a_iPrec,
                                     bool a_bAllowOpt)
    {
        AddCallback(a_sName,
                    ParserCallback(a_pFun, a_bAllowOpt, a_iPrec, cmOPRT_INFIX),
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
                                 bool a_bAllowOpt )
    {
        // Check for conflicts with built in operator names
        for (int i = 0; m_bBuiltInOp && i < cmENDIF; ++i)
            if (a_sName == string_type(c_DefaultOprt[i]))
                Error(ecBUILTIN_OVERLOAD, -1, a_sName);

        AddCallback(a_sName,
                    ParserCallback(a_pFun, a_bAllowOpt, a_iPrec, a_eAssociativity),
                    m_OprtDef,
                    ValidOprtChars() );
    }

    //---------------------------------------------------------------------------
    /** \brief Define a new string constant.
        \param [in] a_strName The name of the constant.
        \param [in] a_strVal the value of the constant.
    */
    void ParserBase::DefineStrConst(const string_type& a_strName, const string_type& a_strVal)
    {
        // Test if a constant with that names already exists
        if (m_StrVarDef.find(a_strName) != m_StrVarDef.end())
            Error(ecNAME_CONFLICT);

        CheckName(a_strName, ValidNameChars());

        m_vStringVarBuf.push_back(a_strVal);           // Store variable string in internal buffer
        m_StrVarDef[a_strName] = m_vStringBuf.size();  // bind buffer index to variable name

        ReInit();
    }

    //---------------------------------------------------------------------------
    /** \brief Add a user defined variable.
        \param [in] a_sName the variable name
        \param [in] a_pVar A pointer to the variable vaule.
        \post Will reset the Parser to string parsing mode.
        \throw ParserException in case the name contains invalid signs or a_pVar is NULL.
    */
    void ParserBase::DefineVar(const string_type& a_sName, value_type* a_pVar)
    {
        if (a_pVar == 0)
            Error(ecINVALID_VAR_PTR);

        // Test if a constant with that names already exists
        if (m_ConstDef.find(a_sName) != m_ConstDef.end())
            Error(ecNAME_CONFLICT);

        CheckName(a_sName, ValidNameChars());
        m_VarDef[a_sName] = a_pVar;
        ReInit();
    }

    //---------------------------------------------------------------------------
    /** \brief Add a user defined constant.
        \param [in] a_sName The name of the constant.
        \param [in] a_fVal the value of the constant.
        \post Will reset the Parser to string parsing mode.
        \throw ParserException in case the name contains invalid signs.
    */
    void ParserBase::DefineConst(const string_type& a_sName, value_type a_fVal)
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
            case cmASSIGN:
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
        try
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
            throw e;
        }

        return m_pTokenReader->GetUsedVar();
    }

    //---------------------------------------------------------------------------
    /** \brief Return a map containing the used variables only. */
    const varmap_type& ParserBase::GetVar() const
    {
        return m_VarDef;
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

    //---------------------------------------------------------------------------
    /** \brief Retrieve the formula. */
    const string_type& ParserBase::GetExpr() const
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode && vValidByteCode[nthLoopElement])
        {
            //std::cerr << vValidByteCode[nthLoopElement] << " / Expr[" << nthLoopElement << "]: \"" << vLoopString[nthLoopElement] << "\"" << endl;
            return vLoopString[nthLoopElement];
        }
        else
        {
            return m_pTokenReader->GetExpr();
        }
    }

    //---------------------------------------------------------------------------
    /** \brief Execute a function that takes a single string argument.
        \param a_FunTok Function token.
        \throw exception_type If the function token is not a string function
    */
    ParserBase::token_type ParserBase::ApplyStrFunc(const token_type& a_FunTok,
            const std::vector<token_type>& a_vArg) const
    {
        if (a_vArg.back().GetCode() != cmSTRING)
            Error(ecSTRING_EXPECTED, m_pTokenReader->GetPos(), a_FunTok.GetAsString());

        token_type  valTok;
        generic_fun_type pFunc = a_FunTok.GetFuncAddr();
        assert(pFunc);

        try
        {
            // Collect the function arguments from the value stack
            switch (a_FunTok.GetArgCount())
            {
                case 0:
                    valTok.SetVal( ((strfun_type1)pFunc)(a_vArg[0].GetAsString().c_str()) );
                    break;
                case 1:
                    valTok.SetVal( ((strfun_type2)pFunc)(a_vArg[1].GetAsString().c_str(),
                                                         a_vArg[0].GetVal()) );
                    break;
                case 2:
                    valTok.SetVal( ((strfun_type3)pFunc)(a_vArg[2].GetAsString().c_str(),
                                                         a_vArg[1].GetVal(),
                                                         a_vArg[0].GetVal()) );
                    break;
                default:
                    Error(ecINTERNAL_ERROR);
            }
        }
        catch (ParserError& /*e*/)
        {
            Error(ecVAL_EXPECTED, m_pTokenReader->GetPos(), a_FunTok.GetAsString());
        }

        // string functions won't be optimized
        m_vRPN.AddStrFun(pFunc, a_FunTok.GetArgCount(), a_vArg.back().GetIdx());

        return valTok;
    }

    //---------------------------------------------------------------------------
    /** \brief Apply a function token.
        \param iArgCount Number of Arguments actually gathered used only for multiarg functions.
        \post The result is pushed to the value stack
        \post The function token is removed from the stack
        \throw exception_type if Argument count does not mach function requirements.
    */
    void ParserBase::ApplyFunc( ParserStack<token_type>& a_stOpt,
                                ParserStack<token_type>& a_stVal,
                                int a_iArgCount) const
    {
        assert(m_pTokenReader.get());

        // Operator stack empty or does not contain tokens with callback functions
        if (a_stOpt.empty() || a_stOpt.top().GetFuncAddr() == 0 )
            return;

        token_type funTok = a_stOpt.pop();
        assert(funTok.GetFuncAddr());

        // Binary operators must rely on their internal operator number
        // since counting of operators relies on commas for function arguments
        // binary operators do not have commas in their expression
        int iArgCount = (funTok.GetCode() == cmOPRT_BIN) ? funTok.GetArgCount() : a_iArgCount;

        // determine how many parameters the function needs. To remember iArgCount includes the
        // string parameter whilst GetArgCount() counts only numeric parameters.
        int iArgRequired = funTok.GetArgCount() + ((funTok.GetType() == tpSTR) ? 1 : 0);

        // Thats the number of numerical parameters
        int iArgNumerical = iArgCount - ((funTok.GetType() == tpSTR) ? 1 : 0);

        if (funTok.GetCode() == cmFUNC_STR && iArgCount - iArgNumerical > 1)
            Error(ecINTERNAL_ERROR);

        if (funTok.GetArgCount() >= 0 && iArgCount > iArgRequired)
            Error(ecTOO_MANY_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());

        if (funTok.GetCode() != cmOPRT_BIN && iArgCount < iArgRequired )
            Error(ecTOO_FEW_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());

        if (funTok.GetCode() == cmFUNC_STR && iArgCount > iArgRequired )
            Error(ecTOO_MANY_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());

        // Collect the numeric function arguments from the value stack and store them
        // in a vector
        std::vector<token_type> stArg;
        for (int i = 0; i < iArgNumerical; ++i)
        {
            stArg.push_back( a_stVal.pop() );
            if ( stArg.back().GetType() == tpSTR && funTok.GetType() != tpSTR )
                Error(ecVAL_EXPECTED, m_pTokenReader->GetPos(), funTok.GetAsString());
        }

        switch (funTok.GetCode())
        {
            case  cmFUNC_STR:
                stArg.push_back(a_stVal.pop());

                if ( stArg.back().GetType() == tpSTR && funTok.GetType() != tpSTR )
                    Error(ecVAL_EXPECTED, m_pTokenReader->GetPos(), funTok.GetAsString());

                ApplyStrFunc(funTok, stArg);
                break;

            case  cmFUNC_BULK:
                m_vRPN.AddBulkFun(funTok.GetFuncAddr(), (int)stArg.size());
                break;

            case  cmOPRT_BIN:
            case  cmOPRT_POSTFIX:
            case  cmOPRT_INFIX:
            case  cmFUNC:
                if (funTok.GetArgCount() == -1 && iArgCount == 0)
                    Error(ecTOO_FEW_PARAMS, m_pTokenReader->GetPos(), funTok.GetAsString());

                m_vRPN.AddFun(funTok.GetFuncAddr(), (funTok.GetArgCount() == -1) ? -iArgNumerical : iArgNumerical);
                break;
        }

        // Push dummy value representing the function result to the stack
        token_type token;
        token.SetVal(1);
        a_stVal.push(token);
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

            a_stVal.push( (vExpr.GetVal() != 0) ? vVal1 : vVal2);

            token_type opIf = a_stOpt.pop();
            MUP_ASSERT(opElse.GetCode() == cmELSE);
            MUP_ASSERT(opIf.GetCode() == cmIF);

            m_vRPN.AddIfElse(cmENDIF);
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

            if ( valTok1.GetType() != valTok2.GetType() ||
                    (valTok1.GetType() == tpSTR && valTok2.GetType() == tpSTR) )
                Error(ecOPRT_TYPE_CONFLICT, m_pTokenReader->GetPos(), optTok.GetAsString());

            if (optTok.GetCode() == cmASSIGN)
            {
                if (valTok2.GetCode() != cmVAR)
                    Error(ecUNEXPECTED_OPERATOR, -1, _nrT("="));

                m_vRPN.AddAssignOp(valTok2.GetVar());
            }
            else
                m_vRPN.AddOp(optTok.GetCode());

            resTok.SetVal(1);
            a_stVal.push(resTok);
        }
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
                    if (stOpt.top().GetCode() == cmOPRT_INFIX)
                        ApplyFunc(stOpt, stVal, 1);
                    else
                        ApplyBinOprt(stOpt, stVal);
                    break;

                case cmELSE:
                    ApplyIfElse(stOpt, stVal);
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
    value_type ParserBase::ParseCmdCode()
    {
        return ParseCmdCodeBulk(0, 0);
    }

    //---------------------------------------------------------------------------
    /** \brief Evaluate the RPN.
        \param nOffset The offset added to variable addresses (for bulk mode)
        \param nThreadID OpenMP Thread id of the calling thread
    */
    value_type ParserBase::ParseCmdCodeBulk(int nOffset, int nThreadID)
    {
        assert(nThreadID <= s_MaxNumOpenMPThreads);

        // Note: The check for nOffset==0 and nThreadID here is not necessary but
        //       brings a minor performance gain when not in bulk mode.
        value_type* Stack = 0;
        if (!bMakeLoopByteCode || bPauseLoopByteCode || !vValidByteCode[nthLoopElement])
            Stack = ((nOffset == 0) && (nThreadID == 0)) ? &m_vStackBuffer[0] : &m_vStackBuffer[nThreadID * (m_vStackBuffer.size() / s_MaxNumOpenMPThreads)];
        else
            Stack = ((nOffset == 0) && (nThreadID == 0)) ? &vLoopStackBuf[nthLoopElement][0] : & vLoopStackBuf[nthLoopElement][nThreadID * (vLoopStackBuf[nthLoopElement].size() / s_MaxNumOpenMPThreads)];
        value_type buf;
        int sidx(0);
        if (!bMakeLoopByteCode || bPauseLoopByteCode || !vValidByteCode[nthLoopElement])
        {
            //m_vRPN.AsciiDump();
            for (const SToken* pTok = m_vRPN.GetBase(); pTok->Cmd != cmEND ; ++pTok)
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
                        Stack[sidx] += Stack[1 + sidx];
                        continue;
                    case  cmSUB:
                        --sidx;
                        Stack[sidx] -= Stack[1 + sidx];
                        continue;
                    case  cmMUL:
                        --sidx;
                        Stack[sidx] *= Stack[1 + sidx];
                        continue;
                    case  cmDIV:
                        --sidx;

#if defined(MUP_MATH_EXCEPTIONS)
                        if (Stack[1 + sidx] == 0)
                            Error(ecDIV_BY_ZERO);
#endif
                        Stack[sidx] /= Stack[1 + sidx];
                        continue;

                    case  cmPOW:
                        Stack[--sidx]  = MathImpl<value_type>::Pow(Stack[sidx], Stack[1 + sidx]); ;
                        continue;

                    case  cmLAND:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] && Stack[sidx + 1];
                        continue;
                    case  cmLOR:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] || Stack[sidx + 1];
                        continue;

                    case  cmASSIGN:
                        --sidx;
                        Stack[sidx] = *pTok->Oprt.ptr = Stack[sidx + 1];
                        continue;

                    //case  cmBO:  // unused, listed for compiler optimization purposes
                    //case  cmBC:
                    //      MUP_FAIL(INVALID_CODE_IN_BYTECODE);
                    //      continue;

                    case  cmIF:
                        if (Stack[sidx--] == 0)
                            pTok += pTok->Oprt.offset;
                        continue;

                    case  cmELSE:
                        pTok += pTok->Oprt.offset;
                        continue;

                    case  cmENDIF:
                        continue;

                    //case  cmARG_SEP:
                    //      MUP_FAIL(INVALID_CODE_IN_BYTECODE);
                    //      continue;

                    // value and variable tokens
                    case  cmVAR:
                        Stack[++sidx] = *(pTok->Val.ptr + nOffset);
                        continue;
                    case  cmVAL:
                        Stack[++sidx] =  pTok->Val.data2;
                        continue;

                    case  cmVARPOW2:
                        buf = *(pTok->Val.ptr + nOffset);
                        Stack[++sidx] = buf * buf;
                        continue;

                    case  cmVARPOW3:
                        buf = *(pTok->Val.ptr + nOffset);
                        Stack[++sidx] = buf * buf * buf;
                        continue;

                    case  cmVARPOW4:
                        buf = *(pTok->Val.ptr + nOffset);
                        Stack[++sidx] = buf * buf * buf * buf;
                        continue;

                    case  cmVARMUL:
                        Stack[++sidx] = *(pTok->Val.ptr + nOffset) * pTok->Val.data + pTok->Val.data2;
                        continue;

                    // Next is treatment of numeric functions
                    case  cmFUNC:
                        {
                            int iArgCount = pTok->Fun.argc;

                            // switch according to argument count
                            switch (iArgCount)
                            {
                                case 0:
                                    sidx += 1;
                                    Stack[sidx] = (*(fun_type0)pTok->Fun.ptr)();
                                    continue;
                                case 1:
                                    Stack[sidx] = (*(fun_type1)pTok->Fun.ptr)(Stack[sidx]);
                                    continue;
                                case 2:
                                    sidx -= 1;
                                    Stack[sidx] = (*(fun_type2)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1]);
                                    continue;
                                case 3:
                                    sidx -= 2;
                                    Stack[sidx] = (*(fun_type3)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2]);
                                    continue;
                                case 4:
                                    sidx -= 3;
                                    Stack[sidx] = (*(fun_type4)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3]);
                                    continue;
                                case 5:
                                    sidx -= 4;
                                    Stack[sidx] = (*(fun_type5)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4]);
                                    continue;
                                case 6:
                                    sidx -= 5;
                                    Stack[sidx] = (*(fun_type6)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5]);
                                    continue;
                                case 7:
                                    sidx -= 6;
                                    Stack[sidx] = (*(fun_type7)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6]);
                                    continue;
                                case 8:
                                    sidx -= 7;
                                    Stack[sidx] = (*(fun_type8)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7]);
                                    continue;
                                case 9:
                                    sidx -= 8;
                                    Stack[sidx] = (*(fun_type9)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8]);
                                    continue;
                                case 10:
                                    sidx -= 9;
                                    Stack[sidx] = (*(fun_type10)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8], Stack[sidx + 9]);
                                    continue;
                                default:
                                    if (iArgCount > 0) // function with variable arguments store the number as a negative value
                                        Error(ecINTERNAL_ERROR, 1);

                                    sidx -= -iArgCount - 1;
                                    Stack[sidx] = (*(multfun_type)pTok->Fun.ptr)(&Stack[sidx], -iArgCount);
                                    continue;
                            }
                        }

                    // Next is treatment of string functions
                    case  cmFUNC_STR:
                        {
                            sidx -= pTok->Fun.argc - 1;

                            // The index of the string argument in the string table
                            int iIdxStack = pTok->Fun.idx;
                            MUP_ASSERT( iIdxStack >= 0 && iIdxStack < (int)m_vStringBuf.size() );

                            switch (pTok->Fun.argc) // switch according to argument count
                            {
                                case 0:
                                    Stack[sidx] = (*(strfun_type1)pTok->Fun.ptr)(m_vStringBuf[iIdxStack].c_str());
                                    continue;
                                case 1:
                                    Stack[sidx] = (*(strfun_type2)pTok->Fun.ptr)(m_vStringBuf[iIdxStack].c_str(), Stack[sidx]);
                                    continue;
                                case 2:
                                    Stack[sidx] = (*(strfun_type3)pTok->Fun.ptr)(m_vStringBuf[iIdxStack].c_str(), Stack[sidx], Stack[sidx + 1]);
                                    continue;
                            }

                            continue;
                        }

                    case  cmFUNC_BULK:
                        {
                            int iArgCount = pTok->Fun.argc;

                            // switch according to argument count
                            switch (iArgCount)
                            {
                                case 0:
                                    sidx += 1;
                                    Stack[sidx] = (*(bulkfun_type0 )pTok->Fun.ptr)(nOffset, nThreadID);
                                    continue;
                                case 1:
                                    Stack[sidx] = (*(bulkfun_type1 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx]);
                                    continue;
                                case 2:
                                    sidx -= 1;
                                    Stack[sidx] = (*(bulkfun_type2 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1]);
                                    continue;
                                case 3:
                                    sidx -= 2;
                                    Stack[sidx] = (*(bulkfun_type3 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2]);
                                    continue;
                                case 4:
                                    sidx -= 3;
                                    Stack[sidx] = (*(bulkfun_type4 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3]);
                                    continue;
                                case 5:
                                    sidx -= 4;
                                    Stack[sidx] = (*(bulkfun_type5 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4]);
                                    continue;
                                case 6:
                                    sidx -= 5;
                                    Stack[sidx] = (*(bulkfun_type6 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5]);
                                    continue;
                                case 7:
                                    sidx -= 6;
                                    Stack[sidx] = (*(bulkfun_type7 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6]);
                                    continue;
                                case 8:
                                    sidx -= 7;
                                    Stack[sidx] = (*(bulkfun_type8 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7]);
                                    continue;
                                case 9:
                                    sidx -= 8;
                                    Stack[sidx] = (*(bulkfun_type9 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8]);
                                    continue;
                                case 10:
                                    sidx -= 9;
                                    Stack[sidx] = (*(bulkfun_type10)pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8], Stack[sidx + 9]);
                                    continue;
                                default:
                                    Error(ecINTERNAL_ERROR, 2);
                                    continue;
                            }
                        }

                    //case  cmSTRING:
                    //case  cmOPRT_BIN:
                    //case  cmOPRT_POSTFIX:
                    //case  cmOPRT_INFIX:
                    //      MUP_FAIL(INVALID_CODE_IN_BYTECODE);
                    //      continue;

                    //case  cmEND:
                    //     return Stack[m_nFinalResultIdx];

                    default:
                        Error(ecINTERNAL_ERROR, 3);
                        return 0;
                } // switch CmdCode
            } // for all bytecode tokens

        }
        else
        {
            m_nFinalResultIdx = vNumResultsIDX[nthLoopElement];
            //vLoopByteCode[nthLoopElement].AsciiDump();
            for (const SToken* pTok = vLoopByteCode[nthLoopElement].GetBase(); pTok->Cmd != cmEND ; ++pTok)
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
                        Stack[sidx] += Stack[1 + sidx];
                        continue;
                    case  cmSUB:
                        --sidx;
                        Stack[sidx] -= Stack[1 + sidx];
                        continue;
                    case  cmMUL:
                        --sidx;
                        Stack[sidx] *= Stack[1 + sidx];
                        continue;
                    case  cmDIV:
                        --sidx;

#if defined(MUP_MATH_EXCEPTIONS)
                        if (Stack[1 + sidx] == 0)
                            Error(ecDIV_BY_ZERO);
#endif
                        Stack[sidx] /= Stack[1 + sidx];
                        continue;

                    case  cmPOW:
                        Stack[--sidx]  = MathImpl<value_type>::Pow(Stack[sidx], Stack[1 + sidx]); ;
                        continue;

                    case  cmLAND:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] && Stack[sidx + 1];
                        continue;
                    case  cmLOR:
                        --sidx;
                        Stack[sidx]  = Stack[sidx] || Stack[sidx + 1];
                        continue;

                    case  cmASSIGN:
                        --sidx;
                        Stack[sidx] = *pTok->Oprt.ptr = Stack[sidx + 1];
                        continue;

                    //case  cmBO:  // unused, listed for compiler optimization purposes
                    //case  cmBC:
                    //      MUP_FAIL(INVALID_CODE_IN_BYTECODE);
                    //      continue;

                    case  cmIF:
                        if (Stack[sidx--] == 0)
                            pTok += pTok->Oprt.offset;
                        continue;

                    case  cmELSE:
                        pTok += pTok->Oprt.offset;
                        continue;

                    case  cmENDIF:
                        continue;

                    //case  cmARG_SEP:
                    //      MUP_FAIL(INVALID_CODE_IN_BYTECODE);
                    //      continue;

                    // value and variable tokens
                    case  cmVAR:
                        Stack[++sidx] = *(pTok->Val.ptr + nOffset);
                        continue;
                    case  cmVAL:
                        Stack[++sidx] =  pTok->Val.data2;
                        continue;

                    case  cmVARPOW2:
                        buf = *(pTok->Val.ptr + nOffset);
                        Stack[++sidx] = buf * buf;
                        continue;

                    case  cmVARPOW3:
                        buf = *(pTok->Val.ptr + nOffset);
                        Stack[++sidx] = buf * buf * buf;
                        continue;

                    case  cmVARPOW4:
                        buf = *(pTok->Val.ptr + nOffset);
                        Stack[++sidx] = buf * buf * buf * buf;
                        continue;

                    case  cmVARMUL:
                        Stack[++sidx] = *(pTok->Val.ptr + nOffset) * pTok->Val.data + pTok->Val.data2;
                        continue;

                    // Next is treatment of numeric functions
                    case  cmFUNC:
                        {
                            int iArgCount = pTok->Fun.argc;

                            // switch according to argument count
                            switch (iArgCount)
                            {
                                case 0:
                                    sidx += 1;
                                    Stack[sidx] = (*(fun_type0)pTok->Fun.ptr)();
                                    continue;
                                case 1:
                                    Stack[sidx] = (*(fun_type1)pTok->Fun.ptr)(Stack[sidx]);
                                    continue;
                                case 2:
                                    sidx -= 1;
                                    Stack[sidx] = (*(fun_type2)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1]);
                                    continue;
                                case 3:
                                    sidx -= 2;
                                    Stack[sidx] = (*(fun_type3)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2]);
                                    continue;
                                case 4:
                                    sidx -= 3;
                                    Stack[sidx] = (*(fun_type4)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3]);
                                    continue;
                                case 5:
                                    sidx -= 4;
                                    Stack[sidx] = (*(fun_type5)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4]);
                                    continue;
                                case 6:
                                    sidx -= 5;
                                    Stack[sidx] = (*(fun_type6)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5]);
                                    continue;
                                case 7:
                                    sidx -= 6;
                                    Stack[sidx] = (*(fun_type7)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6]);
                                    continue;
                                case 8:
                                    sidx -= 7;
                                    Stack[sidx] = (*(fun_type8)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7]);
                                    continue;
                                case 9:
                                    sidx -= 8;
                                    Stack[sidx] = (*(fun_type9)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8]);
                                    continue;
                                case 10:
                                    sidx -= 9;
                                    Stack[sidx] = (*(fun_type10)pTok->Fun.ptr)(Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8], Stack[sidx + 9]);
                                    continue;
                                default:
                                    if (iArgCount > 0) // function with variable arguments store the number as a negative value
                                        Error(ecINTERNAL_ERROR, 1);

                                    sidx -= -iArgCount - 1;
                                    Stack[sidx] = (*(multfun_type)pTok->Fun.ptr)(&Stack[sidx], -iArgCount);
                                    continue;
                            }
                        }

                    // Next is treatment of string functions
                    case  cmFUNC_STR:
                        {
                            sidx -= pTok->Fun.argc - 1;

                            // The index of the string argument in the string table
                            int iIdxStack = pTok->Fun.idx;
                            MUP_ASSERT( iIdxStack >= 0 && iIdxStack < (int)m_vStringBuf.size() );

                            switch (pTok->Fun.argc) // switch according to argument count
                            {
                                case 0:
                                    Stack[sidx] = (*(strfun_type1)pTok->Fun.ptr)(m_vStringBuf[iIdxStack].c_str());
                                    continue;
                                case 1:
                                    Stack[sidx] = (*(strfun_type2)pTok->Fun.ptr)(m_vStringBuf[iIdxStack].c_str(), Stack[sidx]);
                                    continue;
                                case 2:
                                    Stack[sidx] = (*(strfun_type3)pTok->Fun.ptr)(m_vStringBuf[iIdxStack].c_str(), Stack[sidx], Stack[sidx + 1]);
                                    continue;
                            }

                            continue;
                        }

                    case  cmFUNC_BULK:
                        {
                            int iArgCount = pTok->Fun.argc;

                            // switch according to argument count
                            switch (iArgCount)
                            {
                                case 0:
                                    sidx += 1;
                                    Stack[sidx] = (*(bulkfun_type0 )pTok->Fun.ptr)(nOffset, nThreadID);
                                    continue;
                                case 1:
                                    Stack[sidx] = (*(bulkfun_type1 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx]);
                                    continue;
                                case 2:
                                    sidx -= 1;
                                    Stack[sidx] = (*(bulkfun_type2 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1]);
                                    continue;
                                case 3:
                                    sidx -= 2;
                                    Stack[sidx] = (*(bulkfun_type3 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2]);
                                    continue;
                                case 4:
                                    sidx -= 3;
                                    Stack[sidx] = (*(bulkfun_type4 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3]);
                                    continue;
                                case 5:
                                    sidx -= 4;
                                    Stack[sidx] = (*(bulkfun_type5 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4]);
                                    continue;
                                case 6:
                                    sidx -= 5;
                                    Stack[sidx] = (*(bulkfun_type6 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5]);
                                    continue;
                                case 7:
                                    sidx -= 6;
                                    Stack[sidx] = (*(bulkfun_type7 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6]);
                                    continue;
                                case 8:
                                    sidx -= 7;
                                    Stack[sidx] = (*(bulkfun_type8 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7]);
                                    continue;
                                case 9:
                                    sidx -= 8;
                                    Stack[sidx] = (*(bulkfun_type9 )pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8]);
                                    continue;
                                case 10:
                                    sidx -= 9;
                                    Stack[sidx] = (*(bulkfun_type10)pTok->Fun.ptr)(nOffset, nThreadID, Stack[sidx], Stack[sidx + 1], Stack[sidx + 2], Stack[sidx + 3], Stack[sidx + 4], Stack[sidx + 5], Stack[sidx + 6], Stack[sidx + 7], Stack[sidx + 8], Stack[sidx + 9]);
                                    continue;
                                default:
                                    Error(ecINTERNAL_ERROR, 2);
                                    continue;
                            }
                        }

                    //case  cmSTRING:
                    //case  cmOPRT_BIN:
                    //case  cmOPRT_POSTFIX:
                    //case  cmOPRT_INFIX:
                    //      MUP_FAIL(INVALID_CODE_IN_BYTECODE);
                    //      continue;

                    //case  cmEND:
                    //     return Stack[m_nFinalResultIdx];

                    default:
                        Error(ecINTERNAL_ERROR, 3);
                        return 0;
                } // switch CmdCode
            } // for all bytecode tokens

        }
        return Stack[m_nFinalResultIdx];
    }

    //---------------------------------------------------------------------------
    void ParserBase::CreateRPN()
    {
        if (!m_pTokenReader->GetExpr().length())
            Error(ecUNEXPECTED_EOF, 0);

        ParserStack<token_type> stOpt, stVal;
        ParserStack<int> stArgCount;
        token_type opta, opt;  // for storing operators
        token_type val, tval;  // for storing value
        string_type strBuf;    // buffer for string function arguments

        ReInit();

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
                case cmSTRING:
                    opt.SetIdx((int)m_vStringBuf.size());      // Assign buffer index to token
                    stVal.push(opt);
                    m_vStringBuf.push_back(opt.GetAsString()); // Store string in internal buffer
                    break;

                case cmVAR:
                    stVal.push(opt);
                    m_vRPN.AddVar( static_cast<value_type*>(opt.GetVar()) );

                    if (mVectorVars.size())
                    {
                        if (mVectorVars.find(opt.GetAsString()) != mVectorVars.end())
                            break;
                        std::vector<double> vVar;
                        vVar.push_back(*(opt.GetVar()));
                        SetVectorVar(opt.GetAsString(), vVar, true);
                    }

                    break;

                case cmVAL:
                    stVal.push(opt);
                    m_vRPN.AddVal( opt.GetVal() );
                    break;

                case cmELSE:
                    m_nIfElseCounter--;
                    if (m_nIfElseCounter < 0)
                        Error(ecMISPLACED_COLON, m_pTokenReader->GetPos());

                    ApplyRemainingOprt(stOpt, stVal);
                    m_vRPN.AddIfElse(cmELSE);
                    stOpt.push(opt);
                    break;


                case cmARG_SEP:
                    if (stArgCount.empty())
                        Error(ecUNEXPECTED_ARG_SEP, m_pTokenReader->GetPos());

                    ++stArgCount.top();
                // fallthrough intentional (no break!)

                case cmEND:
                    ApplyRemainingOprt(stOpt, stVal);
                    break;

                case cmBC:
                    {
                        // The argument count for parameterless functions is zero
                        // by default an opening bracket sets parameter count to 1
                        // in preparation of arguments to come. If the last token
                        // was an opening bracket we know better...
                        if (opta.GetCode() == cmBO)
                            --stArgCount.top();

                        ApplyRemainingOprt(stOpt, stVal);

                        // Check if the bracket content has been evaluated completely
                        if (stOpt.size() && stOpt.top().GetCode() == cmBO)
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

                            if (iArgCount > 1 && ( stOpt.size() == 0 ||
                                                   (stOpt.top().GetCode() != cmFUNC &&
                                                    stOpt.top().GetCode() != cmFUNC_BULK &&
                                                    stOpt.top().GetCode() != cmFUNC_STR) ) )
                                Error(ecUNEXPECTED_ARG, m_pTokenReader->GetPos());

                            // The opening bracket was popped from the stack now check if there
                            // was a function before this bracket
                            if (stOpt.size() &&
                                    stOpt.top().GetCode() != cmOPRT_INFIX &&
                                    stOpt.top().GetCode() != cmOPRT_BIN &&
                                    stOpt.top().GetFuncAddr() != 0)
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
                case cmOPRT_BIN:

                    // A binary operator (user defined or built in) has been found.
                    while ( stOpt.size() &&
                            stOpt.top().GetCode() != cmBO &&
                            stOpt.top().GetCode() != cmELSE &&
                            stOpt.top().GetCode() != cmIF)
                    {
                        int nPrec1 = GetOprtPrecedence(stOpt.top()),
                            nPrec2 = GetOprtPrecedence(opt);

                        if (stOpt.top().GetCode() == opt.GetCode())
                        {

                            // Deal with operator associativity
                            EOprtAssociativity eOprtAsct = GetOprtAssociativity(opt);
                            if ( (eOprtAsct == oaRIGHT && (nPrec1 <= nPrec2)) ||
                                    (eOprtAsct == oaLEFT  && (nPrec1 <  nPrec2)) )
                            {
                                break;
                            }
                        }
                        else if (nPrec1 < nPrec2)
                        {
                            // In case the operators are not equal the precedence decides alone...
                            break;
                        }

                        if (stOpt.top().GetCode() == cmOPRT_INFIX)
                            ApplyFunc(stOpt, stVal, 1);
                        else
                            ApplyBinOprt(stOpt, stVal);
                    } // while ( ... )

                    if (opt.GetCode() == cmIF)
                        m_vRPN.AddIfElse(opt.GetCode());

                    // The operator can't be evaluated right now, push back to the operator stack
                    stOpt.push(opt);
                    break;

                //
                // Last section contains functions and operators implicitely mapped to functions
                //
                case cmBO:
                    stArgCount.push(1);
                    stOpt.push(opt);
                    break;

                case cmOPRT_INFIX:
                case cmFUNC:
                case cmFUNC_BULK:
                case cmFUNC_STR:
                    stOpt.push(opt);
                    break;

                case cmOPRT_POSTFIX:
                    stOpt.push(opt);
                    ApplyFunc(stOpt, stVal, 1);  // this is the postfix operator
                    break;

                default:
                    Error(ecINTERNAL_ERROR, 3);
            } // end of switch operator-token

            opta = opt;

            if ( opt.GetCode() == cmEND )
            {
                m_vRPN.Finalize();
                break;
            }

            if (ParserBase::g_DbgDumpStack)
            {
                StackDump(stVal, stOpt);
                m_vRPN.AsciiDump();
            }
        } // while (true)

        if (ParserBase::g_DbgDumpCmdCode)
            m_vRPN.AsciiDump();

        if (m_nIfElseCounter > 0)
            Error(ecMISSING_ELSE_CLAUSE);

        // get the last value (= final result) from the stack
        MUP_ASSERT(stArgCount.size() == 1);
        m_nFinalResultIdx = stArgCount.top();
        if (m_nFinalResultIdx == 0)
            Error(ecINTERNAL_ERROR, 9);

        if (stVal.size() == 0)
            Error(ecEMPTY_EXPRESSION);

        if (stVal.top().GetType() != tpDBL)
            Error(ecSTR_RESULT);

        m_vStackBuffer.resize(m_vRPN.GetMaxStackSize() * s_MaxNumOpenMPThreads);
    }

    //---------------------------------------------------------------------------
    /** \brief One of the two main parse functions.
        \sa ParseCmdCode(...)

      Parse expression from input string. Perform syntax checking and create
      bytecode. After parsing the string and creating the bytecode the function
      pointer #m_pParseFormula will be changed to the second parse routine the
      uses bytecode instead of string parsing.
    */
    value_type ParserBase::ParseString()
    {
        CreateRPN();
        if (bMakeLoopByteCode && !bPauseLoopByteCode && vValidByteCode[nthLoopElement])
        {
            vLoopByteCode[nthLoopElement] = m_vRPN;
            //vLoopByteCode[nthLoopElement].AsciiDump();
            vLoopString[nthLoopElement] = m_pTokenReader->GetExpr();
            //std::cerr << vLoopString[nthLoopElement] << endl;
            vNumResultsIDX[nthLoopElement] = m_nFinalResultIdx;
            vLoopStackBuf[nthLoopElement].resize(m_vStackBuffer.size());
            vLoopStackBuf[nthLoopElement] = m_vStackBuffer;
            vUsedVar[nthLoopElement] = m_pTokenReader->GetUsedVar();
        }
        m_pParseFormula = &ParserBase::ParseCmdCode;
        return (this->*m_pParseFormula)();
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
        throw exception_type(a_iErrc, a_sTok, m_pTokenReader->GetExpr(), a_iPos);
    }

    //------------------------------------------------------------------------------
    /** \brief Clear all user defined variables.
        \throw nothrow

        Resets the parser to string parsing mode by calling #ReInit.
    */
    void ParserBase::ClearVar()
    {
        m_VarDef.clear();
        ReInit();
    }

    //------------------------------------------------------------------------------
    /** \brief Remove a variable from internal storage.
        \throw nothrow

        Removes a variable if it exists. If the Variable does not exist nothing will be done.
    */
    void ParserBase::RemoveVar(const string_type& a_strVarName)
    {
        varmap_type::iterator item = m_VarDef.find(a_strVarName);
        if (item != m_VarDef.end())
        {
            m_VarDef.erase(item);
            ReInit();
        }
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
        m_StrVarDef.clear();
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
        m_vRPN.EnableOptimizer(a_bIsOn);
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

        mu::console() << _nrT("\nValue stack:\n");
        while ( !stVal.empty() )
        {
            token_type val = stVal.pop();
            if (val.GetType() == tpSTR)
                mu::console() << _nrT(" \"") << val.GetAsString() << _nrT("\" ");
            else
                mu::console() << _nrT(" ") << val.GetVal() << _nrT(" ");
        }
        mu::console() << "\nOperator stack:\n";

        while ( !stOprt.empty() )
        {
            if (stOprt.top().GetCode() <= cmASSIGN)
            {
                mu::console() << _nrT("OPRT_INTRNL \"")
                              << ParserBase::c_DefaultOprt[stOprt.top().GetCode()]
                              << _nrT("\" \n");
            }
            else
            {
                switch (stOprt.top().GetCode())
                {
                    case cmVAR:
                        mu::console() << _nrT("VAR\n");
                        break;
                    case cmVAL:
                        mu::console() << _nrT("VAL\n");
                        break;
                    case cmFUNC:
                        mu::console() << _nrT("FUNC \"")
                                      << stOprt.top().GetAsString()
                                      << _nrT("\"\n");
                        break;
                    case cmFUNC_BULK:
                        mu::console() << _nrT("FUNC_BULK \"")
                                      << stOprt.top().GetAsString()
                                      << _nrT("\"\n");
                        break;
                    case cmOPRT_INFIX:
                        mu::console() << _nrT("OPRT_INFIX \"")
                                      << stOprt.top().GetAsString()
                                      << _nrT("\"\n");
                        break;
                    case cmOPRT_BIN:
                        mu::console() << _nrT("OPRT_BIN \"")
                                      << stOprt.top().GetAsString()
                                      << _nrT("\"\n");
                        break;
                    case cmFUNC_STR:
                        mu::console() << _nrT("FUNC_STR\n");
                        break;
                    case cmEND:
                        mu::console() << _nrT("END\n");
                        break;
                    case cmUNKNOWN:
                        mu::console() << _nrT("UNKNOWN\n");
                        break;
                    case cmBO:
                        mu::console() << _nrT("BRACKET \"(\"\n");
                        break;
                    case cmBC:
                        mu::console() << _nrT("BRACKET \")\"\n");
                        break;
                    case cmIF:
                        mu::console() << _nrT("IF\n");
                        break;
                    case cmELSE:
                        mu::console() << _nrT("ELSE\n");
                        break;
                    case cmENDIF:
                        mu::console() << _nrT("ENDIF\n");
                        break;
                    default:
                        mu::console() << stOprt.top().GetCode() << _nrT(" ");
                        break;
                }
            }
            stOprt.pop();
        }

        mu::console() << dec << endl;
    }

    string_type ParserBase::getNextVarObject(std::string& sArgList, bool bCut)
    {
        int nParenthesis = 0;
        int nVektorbrace = 0;
        unsigned int nPos = 0;
        for (unsigned int i = 0; i < sArgList.length(); i++)
        {
            //cerr << nParenthesis << " ";
            if (sArgList[i] == '(')
                nParenthesis++;
            if (sArgList[i] == ')')
                nParenthesis--;
            if (sArgList[i] == '{')
            {
                nVektorbrace++;
                i++;
            }
            if (sArgList[i] == '}')
            {
                nVektorbrace--;
                i++;
            }
            if (sArgList[i] == ',' && !nParenthesis && !nVektorbrace)
            {
                nPos = i;
                break;
            }
        }
        if (!nPos && sArgList[0] != ',')
            nPos = sArgList.length();
        if (!nPos)
        {
            if (bCut && sArgList[0] == ',')
                sArgList.erase(0, 1);
            return "";
        }
        std::string sArg = sArgList.substr(0, nPos);
        while (sArg[0] == ' ' || sArg[0] == '\t')
            sArg.erase(0, 1);
        while (sArg[sArg.length() - 1] == ' ' || sArg[sArg.length() - 1] == '\t')
            sArg.erase(sArg.length() - 1);
        if (bCut && sArgList.length() > nPos + 1)
            sArgList = sArgList.substr(nPos + 1);
        else if (bCut)
            sArgList = "";
        return sArg;
    }


    //------------------------------------------------------------------------------
    /** \brief Evaluate an expression containing comma seperated subexpressions
        \param [out] nStackSize The total number of results available
        \return Pointer to the array containing all expression results

        This member function can be used to retriev all results of an expression
        made up of multiple comma seperated subexpressions (i.e. "x+y,sin(x),cos(y)")
    */
    value_type* ParserBase::Eval(int& nStackSize)
    {
        (this->*m_pParseFormula)();
        if (bMakeLoopByteCode && !bPauseLoopByteCode && vValidByteCode[nthLoopElement])
        {
            nStackSize = vNumResultsIDX[nthLoopElement];
            m_vStackBuffer = vLoopStackBuf[nthLoopElement];
            //m_nFinalResultIdx = nStackSize;
            if (mVectorVars.size() && !(mTargets.size() && mVectorVars.size() == 1 && vUsedVar[nthLoopElement].find("~TRGTVCT[~]") != vUsedVar[nthLoopElement].end()))
            {
                unsigned int nVectorlength = 0;
                varmap_type vars = vUsedVar[nthLoopElement];
                std::map<double*, double> mFirstVals;
                valbuf_type buffer;
                buffer.push_back(0.0); // erster Wert wird nicht mitgezaehlt
                for (auto iter = mVectorVars.begin(); iter != mVectorVars.end(); ++iter)
                {
                    // Laengsten Vektor suchen
                    if ((iter->second).size() > nVectorlength && iter->first != "~TRGTVCT[~]" && vars.find(iter->first) != vars.end())
                        nVectorlength = (iter->second).size();
                }
                if (nVectorlength)
                {
                    //std::cerr << "Evaluating " << nVectorlength << " Expressions ..." << endl;
                    // erste Elemente kopieren
                    for (int j = 1; j < nStackSize + 1; j++)
                        buffer.push_back(vLoopStackBuf[nthLoopElement][j]);

                    for (unsigned int i = 0; i < nVectorlength; i++)
                    {
                        for (auto iter = mVectorVars.begin(); iter != mVectorVars.end(); ++iter)
                        {
                            // Werte der Variablen aendern
                            if (vars.find(iter->first) != vars.end())
                            {
                                if (i == 0)
                                {
                                    // Zwischenspeichern der ersten Elemente
                                    mFirstVals[vars[iter->first]] = *(vars[iter->first]);
                                    //std::cerr << iter->first << " = " << *(vars[iter->first]) << endl;
                                    continue;
                                }
                                if ((iter->second).size() > i)
                                    *vars[iter->first] = (iter->second)[i];
                                else if ((iter->second).size() == 1)
                                    *vars[iter->first] = (iter->second)[0];
                                else
                                    *vars[iter->first] = 0.0;
                            }
                        }
                        // Parser erneut ausfuehren und Werte kopieren
                        if (i)
                        {
                            (this->*m_pParseFormula)();
                            for (int j = 1; j < nStackSize + 1; j++)
                                buffer.push_back(vLoopStackBuf[nthLoopElement][j]);
                        }
                    }

                    m_vStackBuffer = buffer;
                    for (auto iter = mFirstVals.begin(); iter != mFirstVals.end(); ++iter)
                        *(iter->first) = iter->second;
                    /*for (unsigned int i = 0; i < m_vStackBuffer.size(); i++)
                        std::cerr << m_vStackBuffer[i] << endl;*/

                    nStackSize *= nVectorlength;
                }
            }
            if (mTargets.size() && vUsedVar[nthLoopElement].find("~TRGTVCT[~]") != vUsedVar[nthLoopElement].end())
            {
                std::string sTemp = sTargets;
                for (unsigned int i = 1; i <  m_vStackBuffer.size(); i++)
                {
                    if (!sTemp.length())
                        break;
                    if (mTargets.find(getNextVarObject(sTemp, false)) != mTargets.end())
                    {
                        *(mTargets[getNextVarObject(sTemp, false)]) = m_vStackBuffer[i];
                    }
                    getNextVarObject(sTemp, true);
                }
            }
            return &m_vStackBuffer[1];
        }
        else
        {
            nStackSize = m_nFinalResultIdx;
            if (mVectorVars.size() && !(mVectorVars.size() == 1 && mTargets.size() && GetUsedVar().find("~TRGTVCT[~]") != GetUsedVar().end()))
            {
                unsigned int nVectorlength = 0;
                varmap_type vars = GetUsedVar();
                valbuf_type buffer;
                std::map<double*, double> mFirstVals;
                buffer.push_back(0.0);
                for (auto iter = mVectorVars.begin(); iter != mVectorVars.end(); ++iter)
                {
                    //std::cerr << iter->first << endl;
                    if ((iter->second).size() > nVectorlength && iter->first != "~TRGTVCT[~]" && vars.find(iter->first) != vars.end())
                    {
                        nVectorlength = (iter->second).size();
                        /*for (unsigned int nsize = 0; nsize < (iter->second).size(); nsize++)
                            std::cerr << (iter->second)[nsize] << endl;*/
                        //std::cerr << "found" << endl;
                    }
                }
                //std::cerr << "Evaluating " << nVectorlength << " Expressions ..." << endl;
                for (int j = 1; j < m_nFinalResultIdx + 1; j++)
                    buffer.push_back(m_vStackBuffer[j]);

                if (nVectorlength)
                {
                    for (unsigned int i = 0; i < nVectorlength; i++)
                    {
                        for (auto iter = mVectorVars.begin(); iter != mVectorVars.end(); ++iter)
                        {
                            if (vars.find(iter->first) != vars.end())
                            {
                                if (i == 0)
                                {
                                    mFirstVals[vars[iter->first]] = *(vars[iter->first]);
                                    continue;
                                }
                                if ((iter->second).size() > i)
                                    *vars[iter->first] = (iter->second)[i];
                                else if ((iter->second).size() == 1)
                                    *vars[iter->first] = (iter->second)[0];
                                else
                                    *vars[iter->first] = 0.0;
                            }
                        }
                        if (i)
                        {
                            (this->*m_pParseFormula)();
                            for (int j = 1; j < m_nFinalResultIdx + 1; j++)
                                buffer.push_back(m_vStackBuffer[j]);
                        }
                    }

                    m_vStackBuffer = buffer;
                    for (auto iter = mFirstVals.begin(); iter != mFirstVals.end(); ++iter)
                        *(iter->first) = iter->second;
                    /*for (unsigned int i = 0; i < m_vStackBuffer.size(); i++)
                        std::cerr << m_vStackBuffer[i] << endl;*/
                    nStackSize *= nVectorlength;
                }
            }
            if (mTargets.size() && GetUsedVar().find("~TRGTVCT[~]") != GetUsedVar().end())
            {
                std::string sTemp = sTargets;
                for (unsigned int i = 1; i <  m_vStackBuffer.size(); i++)
                {
                    if (!sTemp.length())
                        break;
                    if (mTargets.find(getNextVarObject(sTemp, false)) != mTargets.end())
                    {
                        *(mTargets[getNextVarObject(sTemp, false)]) = m_vStackBuffer[i];
                    }
                    getNextVarObject(sTemp, true);
                }
            }
            // (for historic reasons the stack starts at position 1)
            return &m_vStackBuffer[1];
        }
    }

    //---------------------------------------------------------------------------
    /** \brief Return the number of results on the calculation stack.

      If the expression contains comma seperated subexpressions (i.e. "sin(y), x+y").
      There mey be more than one return value. This function returns the number of
      available results.
    */
    int ParserBase::GetNumResults() const
    {
        if (bMakeLoopByteCode && !bPauseLoopByteCode && vValidByteCode[nthLoopElement])
            return vNumResultsIDX[nthLoopElement];
        return m_nFinalResultIdx;
    }

    //---------------------------------------------------------------------------
    /** \brief Calculate the result.

      A note on const correctness:
      I consider it important that Calc is a const function.
      Due to caching operations Calc changes only the state of internal variables with one exception
      m_UsedVar this is reset during string parsing and accessible from the outside. Instead of making
      Calc non const GetUsedVar is non const because it explicitely calls Eval() forcing this update.

      \pre A formula must be set.
      \pre Variables must have been set (if needed)

      \sa #m_pParseFormula
      \return The evaluation result
      \throw ParseException if no Formula is set or in case of any other error related to the formula.
    */
    value_type ParserBase::Eval()
    {
        if (mVectorVars.size())
        {
            (this->*m_pParseFormula)();

            unsigned int nVectorlength = 0;
            varmap_type vars = GetUsedVar();
            valbuf_type buffer;
            buffer.push_back(0.0);
            for (auto iter = mVectorVars.begin(); iter != mVectorVars.end(); ++iter)
            {
                if ((iter->second).size() > nVectorlength && vars.find(iter->first) != vars.end())
                    nVectorlength = (iter->second).size();
            }
            //std::cerr << "Evaluating " << nVectorlength << " Expressions ..." << endl;

            for (int j = 1; j < m_nFinalResultIdx + 1; j++)
                buffer.push_back(m_vStackBuffer[j]);

            if (nVectorlength)
            {
                std::map<double*, double> mFirstVals;
                for (unsigned int i = 0; i < nVectorlength; i++)
                {
                    for (auto iter = mVectorVars.begin(); iter != mVectorVars.end(); ++iter)
                    {
                        if (vars.find(iter->first) != vars.end())
                        {
                            if (i == 0)
                            {
                                mFirstVals[vars[iter->first]] = *(vars[iter->first]);
                                continue;
                            }
                            if ((iter->second).size() > i)
                                *vars[iter->first] = (iter->second)[i];
                            else if ((iter->second).size() == 1)
                                *vars[iter->first] = (iter->second)[0];
                            else
                                *vars[iter->first] = 0.0;
                        }
                    }
                    if (i)
                    {
                        (this->*m_pParseFormula)();
                        for (int j = 1; j < m_nFinalResultIdx + 1; j++)
                            buffer.push_back(m_vStackBuffer[j]);
                    }
                }
                for (auto iter = mFirstVals.begin(); iter != mFirstVals.end(); ++iter)
                    *(iter->first) = iter->second;
                /*for (unsigned int i = 0; i < m_vStackBuffer.size(); i++)
                    std::cerr << m_vStackBuffer[i] << endl;*/
                //nStackSize *= nVectorlength;
            }

            if (mTargets.size() && vars.find("~TRGTVCT[~]") != vars.end())
            {
                std::string sTemp = sTargets;
                for (unsigned int i = 1; i <  buffer.size(); i++)
                {
                    if (!sTemp.length())
                        break;
                    if (mTargets.find(getNextVarObject(sTemp, false)) != mTargets.end())
                    {
                        *(mTargets[getNextVarObject(sTemp, false)]) = buffer[i];
                    }
                    getNextVarObject(sTemp, true);
                }
            }

            return buffer[1];
        }
        return (this->*m_pParseFormula)();
    }

    ParserByteCode ParserBase::GetByteCode() const
    {
        return m_vRPN;
    }

    void ParserBase::SetByteCode(const ParserByteCode& _bytecode)
    {
        m_vRPN.Assign(_bytecode);
        m_pParseFormula = &ParserBase::ParseCmdCode;
        return;
    }

    //---------------------------------------------------------------------------
    void ParserBase::Eval(value_type* results, int nBulkSize)
    {
        CreateRPN();

        int i = 0;

#ifdef MUP_USE_OPENMP
//#define DEBUG_OMP_STUFF
#ifdef DEBUG_OMP_STUFF
        int* pThread = new int[nBulkSize];
        int* pIdx = new int[nBulkSize];
#endif

        int nMaxThreads = std::min(omp_get_max_threads(), s_MaxNumOpenMPThreads);
        int nThreadID, ct = 0;
        omp_set_num_threads(nMaxThreads);

        #pragma omp parallel for schedule(static, nBulkSize/nMaxThreads) private(nThreadID)
        for (i = 0; i < nBulkSize; ++i)
        {
            nThreadID = omp_get_thread_num();
            results[i] = ParseCmdCodeBulk(i, nThreadID);

#ifdef DEBUG_OMP_STUFF
            #pragma omp critical
            {
                pThread[ct] = nThreadID;
                pIdx[ct] = i;
                ct++;
            }
#endif
        }

#ifdef DEBUG_OMP_STUFF
        FILE* pFile = fopen("bulk_dbg.txt", "w");
        for (i = 0; i < nBulkSize; ++i)
        {
            fprintf(pFile, "idx: %d  thread: %d \n", pIdx[i], pThread[i]);
        }

        delete [] pIdx;
        delete [] pThread;

        fclose(pFile);
#endif

#else
        for (i = 0; i < nBulkSize; ++i)
        {
            results[i] = ParseCmdCodeBulk(i, 0);
        }
#endif

    }

    void ParserBase::SetVectorVar(const std::string& sVarName, const std::vector<double>& vVar, bool bAddVectorType)
    {
        if (!dVectorVars)
            dVectorVars = new double[nVectorVarsSize];
        if (!bAddVectorType && mVectorVars.find(sVarName) == mVectorVars.end())
        {
            //cerr << "mVectorVars.find() == mVectorVars.end() [" << vVar[0] << "/" << nVectorIndex << "]" << endl;
            if (nVectorIndex < nVectorVarsSize)
            {
                DefineVar(sVarName, &dVectorVars[nVectorIndex]);
                dVectorVars[nVectorIndex] = vVar[0];
                nVectorIndex++;
            }
            else
            {
                delete[] dVectorVars;
                dVectorVars = 0;
                nVectorIndex = 0;
                nVectorVarsSize *= 2;
                dVectorVars = new double[nVectorVarsSize];
                if (bMakeLoopByteCode)
                {
                    vLoopString.assign(nLoopLength, "");
                }

                for (auto iter = mVectorVars.begin(); iter != mVectorVars.end(); ++iter)
                {
                    DefineVar(iter->first, &dVectorVars[nVectorIndex]);
                    nVectorIndex++;
                }
                DefineVar(sVarName, &dVectorVars[nVectorIndex]);
                dVectorVars[nVectorIndex] = vVar[0];
                nVectorIndex++;
            }
        }
        else if (!bAddVectorType && GetVar().find(sVarName) != GetVar().end())
        {
            *(GetVar().find(sVarName)->second) = vVar[0];
            //std::cerr << "Defining " << sVarName << " with " << vVar[0] << endl;
        }
        //std::cerr << "'" << sVarName << "'" << endl;
        //std::cerr << vVar.size() << endl;

        /*for (unsigned int i = 0; i < vVar.size(); i++)
            std::cerr << vVar[i] << endl;*/
        mVectorVars[sVarName] = vVar;
        //std::cerr << mVectorVars[sVarName].size() << endl;
        return;
    }

    void ParserBase::ClearVectorVars(bool bIgnoreProcedureVects)
    {
        if (!mVectorVars.size())
            return;
        //cerr << "clearing vect vars; size = " << mVectorVars.size() << endl;
        auto iter = mVectorVars.begin();
        while (iter != mVectorVars.end())
        {
            //cerr << iter->first << endl;
            if ((iter->first).find('[') != string::npos && (iter->first).find(']') != string::npos)
            {
                if (bIgnoreProcedureVects && (iter->first).substr(0, 6) == "PROC~[")
                {
                    iter++;
                    continue;
                }
                RemoveVar(iter->first);
                iter = mVectorVars.erase(iter);
            }
            else
                iter = mVectorVars.erase(iter); //iter++;
        }
        if (!bIgnoreProcedureVects || !mVectorVars.size())
        {
            mVectorVars.clear();
            mTargets.clear();
            sTargets.clear();
            nVectorIndex = 0;
            if (dVectorVars && nVectorVarsSize > 200)
            {
                delete[] dVectorVars;
                dVectorVars = 0;
            }
            nVectorVarsSize = 200;
        }
        //cerr << "finished" << endl;
        return;
    }

    bool ParserBase::checkDelimiter(const std::string& sLine)
    {
        bool isDelimitedLeft = false;
        bool isDelimitedRight = false;
        std::string sDelimiter = "+-*/ ()={}^&|!<>,\\%#~[]:";

        // --> Versuche jeden Delimiter, der dir bekannt ist und setze bei einem Treffer den entsprechenden BOOL auf TRUE <--
        for (unsigned int i = 0; i < sDelimiter.length(); i++)
        {
            if (sDelimiter[i] == sLine[0])
                isDelimitedLeft = true;
            if (sDelimiter[i] == sLine[sLine.length() - 1])
                isDelimitedRight = true;
        }

        // --> Gib die Auswertung dieses logischen Ausdrucks zurueck <--
        return (isDelimitedLeft && isDelimitedRight);
    }


    void ParserBase::replaceLocalVars(std::string& sLine)
    {
        if (!mVarMapPntr || !mVarMapPntr->size())
            return;
        for (auto iter = mVarMapPntr->begin(); iter != mVarMapPntr->end(); ++iter)
        {
            for (unsigned int i = 0; i < sLine.length(); i++)
            {
                if (sLine.substr(i, (iter->first).length()) == iter->first)
                {
                    if ((i && checkDelimiter(sLine.substr(i - 1, (iter->first).length() + 2)))
                            || (!i && checkDelimiter(" " + sLine.substr(i, (iter->first).length() + 1))))
                    {
                        sLine.replace(i, (iter->first).length(), iter->second);
                    }
                }
            }
        }
        return;
    }

} // namespace mu
