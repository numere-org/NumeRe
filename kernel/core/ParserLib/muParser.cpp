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
#include "muParser.h"
#include "muParserTemplateMagic.h"

//--- Standard includes ------------------------------------------------------------------------
#include <cmath>
#include <algorithm>
#include <numeric>

/** \brief Pi (what else?). */
#define PARSER_CONST_PI  3.141592653589793238462643

/** \brief The eulerian number. */
#define PARSER_CONST_E   2.718281828459045235360287

using namespace std;

/** \file
    \brief Implementation of the standard floating point parser.
*/

mu::Array numfnc_Sum(const mu::Array*, int);
mu::Array numfnc_Avg(const mu::Array*, int);
mu::Array numfnc_Min(const mu::Array*, int);
mu::Array numfnc_Max(const mu::Array*, int);
mu::Array numfnc_abs(const mu::Array& a);

/** \brief Namespace for mathematical applications. */
namespace mu
{
    //---------------------------------------------------------------------------
    /** \brief Callback for the unary minus operator.
        \param v The value to negate
        \return -v
    */
    Array Parser::UnaryMinus(const Array& v)
    {
        return -v;
    }

    Array Parser::UnaryPlus(const Array& v)
    {
        return v;
    }

    Array Parser::LogicalNot(const Array& v)
    {
        return !v;
    }

    //---------------------------------------------------------------------------
    /** \brief Callback for adding multiple values.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    Array Parser::Sum(const Array* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function sum."));
        return numfnc_Sum(a_afArg, a_iArgc);
    }

    //---------------------------------------------------------------------------
    /** \brief Callback for averaging multiple values.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    Array Parser::Avg(const Array* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function avg."));
        return numfnc_Avg(a_afArg, a_iArgc);
    }


    //---------------------------------------------------------------------------
    /** \brief Callback for determining the minimum value out of a vector.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    Array Parser::Min(const Array* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function min."));
        return numfnc_Min(a_afArg, a_iArgc);
    }


    //---------------------------------------------------------------------------
    /** \brief Callback for determining the maximum value out of a vector.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    Array Parser::Max(const Array* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function max."));
        return numfnc_Max(a_afArg, a_iArgc);
    }


    //---------------------------------------------------------------------------
    /** \brief Default value recognition callback.
        \param [in] a_szExpr Pointer to the expression
        \param [in, out] a_iPos Pointer to an index storing the current position within the expression
        \param [out] a_fVal Pointer where the value should be stored in case one is found.
        \return 1 if a value was found 0 otherwise.
    */
    int Parser::IsVal(StringView a_szExpr, int* a_iPos, Value* a_fVal)
    {
        std::complex<double> fVal(0);

        stringstream_type stream(a_szExpr.get_viewed_string());
        stream.seekg(a_szExpr.get_offset());        // todo:  check if this really is necessary
        stream.imbue(Parser::s_locale);
        stream >> fVal;
        stringstream_type::pos_type iEnd = stream.tellg(); // Position after reading

        if (!stream.eof() && iEnd == (stringstream_type::pos_type)-1)
            return 0;

        if (stream.eof())
            *a_iPos = a_szExpr.get_viewed_string().length();
        else
            *a_iPos += (int)(iEnd-(long long int)a_szExpr.get_offset());

        *a_fVal = Numerical::autoType(fVal);
        return 1;
    }


    //---------------------------------------------------------------------------
    /** \brief Constructor.

      Call ParserBase class constructor and trigger Function, Operator and Constant initialization.
    */
    Parser::Parser()
        : ParserBase()
    {
        AddValIdent(IsVal);

        InitCharSets();
        InitFun();
        InitConst();
        InitOprt();
    }

    //---------------------------------------------------------------------------
    /** \brief Define the character sets.
        \sa DefineNameChars, DefineOprtChars, DefineInfixOprtChars

      This function is used for initializing the default character sets that define
      the characters to be useable in function and variable names and operators.
    */
    void Parser::InitCharSets()
    {
        DefineNameChars("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]~\\");
        DefineOprtChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-*^/?<>=#!$%&|~'_{}");
        DefineInfixOprtChars("/+-*^?<>=#!$%&|~'_");
    }

    //---------------------------------------------------------------------------
    /** \brief Initialize the default functions. */
    void Parser::InitFun()
    {
        // Functions with variable number of arguments
        DefineFun("sum", Sum);
        DefineFun("avg", Avg);
        DefineFun("min", Min);
        DefineFun("max", Max);

        DefineFun(MU_VECTOR_CREATE, ParserBase::VectorCreate);
        DefineFun(MU_VECTOR_EXP2, ParserBase::expandVector2);
        DefineFun(MU_VECTOR_EXP3, ParserBase::expandVector3);
    }

    //---------------------------------------------------------------------------
    /** \brief Initialize constants.

      By default the parser recognizes two constants. Pi ("pi") and the eulerian
      number ("_e").
    */
    void Parser::InitConst()
    {
        DefineConst("_pi", Numerical(PARSER_CONST_PI));
    }

    //---------------------------------------------------------------------------
    /** \brief Initialize operators.

      By default only the unary minus operator is added.
    */
    void Parser::InitOprt()
    {
        DefineInfixOprt("-", UnaryMinus);
        DefineInfixOprt("+", UnaryPlus);
        DefineInfixOprt("!", LogicalNot);
    }

    //---------------------------------------------------------------------------
    void Parser::OnDetectVar(string_type* /*pExpr*/, int& /*nStart*/, int& /*nEnd*/)
    {
    }
    // this is just sample code to illustrate modifying variable names on the fly.
    // I'm not sure anyone really needs such a feature...
    /*


    string sVar(pExpr->begin()+nStart, pExpr->begin()+nEnd);
    string sRepl = std::string("_") + sVar + "_";

    int nOrigVarEnd = nEnd;
    cout << "variable detected!\n";
    cout << "  Expr: " << *pExpr << "\n";
    cout << "  Start: " << nStart << "\n";
    cout << "  End: " << nEnd << "\n";
    cout << "  Var: \"" << sVar << "\"\n";
    cout << "  Repl: \"" << sRepl << "\"\n";
    nEnd = nStart + sRepl.length();
    cout << "  End: " << nEnd << "\n";
    pExpr->replace(pExpr->begin()+nStart, pExpr->begin()+nOrigVarEnd, sRepl);
    cout << "  New expr: " << *pExpr << "\n";
    */
//}

    //---------------------------------------------------------------------------
    /** \brief Numerically differentiate with regard to a variable.
        \param [in] a_Var Pointer to the differentiation variable.
        \param [in] a_fPos Position at which the differentiation should take place.
        \param [in] a_fEpsilon Epsilon used for the numerical differentiation.
        \param [in] order Gives the order of differentiation

      Numerical differentiation uses a 5 point operator yielding a 4th order
      formula. The default value for epsilon is 0.00074 which is
      numeric_limits<double>::epsilon() ^ (1/5) as suggested in the muparser
      forum:

      http://sourceforge.net/forum/forum.php?thread_id=1994611&forum_id=462843
    */
    Array Parser::Diff(Variable* a_Var,
                       const Array& a_fPos,
                       Value fEpsilon,
                       size_t order)
    {
        Variable fBuf(*a_Var);
        Array fRes;
        std::array<Array, 5> f;
        std::array<double, 5> factors = {-2, -1, 0, 1, 2};

        // Backwards compatible calculation of epsilon inc case the user doesnt provide
        // his own epsilon
        if (fEpsilon == Value(0.0))
        {
            Array absVal = numfnc_abs(a_fPos);
            fEpsilon = all(a_fPos == Value(0.0)) ? Value(1e-10) : Value(Value(1e-7)*Max(&absVal, 1).front()*intPower(10, 2*(order-1)));
        }

        for (size_t i = 0; i < f.size(); i++)
        {
            *a_Var = a_fPos + Value(factors[i]) * fEpsilon;
            f[i] = Eval().front();
        }

        // Reference: https://web.media.mit.edu/~crtaylor/calculator.html
        if (order == 1)
            fRes = (f[0] - Value(8.0) * f[1] + Value(8.0) * f[3] - f[4]) / (Value(12.0) * fEpsilon);
        else if (order == 2)
            fRes = (-f[0] + Value(16.0) * f[1] - Value(30.0)*f[2] + Value(16.0) * f[3] - f[4]) / (Value(12.0) * fEpsilon * fEpsilon);
        else if (order == 3)
            fRes = (-f[0] + Value(2.0) * f[1] - Value(2.0) * f[3] + f[4]) / (Value(2.0) * fEpsilon * fEpsilon * fEpsilon);
        else
            fRes = Value(NAN);

        *a_Var = fBuf; // restore variable
        return fRes;
    }
} // namespace mu
