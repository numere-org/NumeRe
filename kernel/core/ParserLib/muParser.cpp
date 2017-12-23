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

mu::value_type parser_Sum(const mu::value_type*, int);
mu::value_type parser_Avg(const mu::value_type*, int);
mu::value_type parser_Min(const mu::value_type*, int);
mu::value_type parser_Max(const mu::value_type*, int);

/** \brief Namespace for mathematical applications. */
namespace mu
{


    //---------------------------------------------------------------------------
    // Trigonometric function
    value_type Parser::Sin(value_type v)
    {
        return MathImpl<value_type>::Sin(v);
    }
    value_type Parser::Cos(value_type v)
    {
        return MathImpl<value_type>::Cos(v);
    }
    value_type Parser::Tan(value_type v)
    {
        return MathImpl<value_type>::Tan(v);
    }
    value_type Parser::ASin(value_type v)
    {
        return MathImpl<value_type>::ASin(v);
    }
    value_type Parser::ACos(value_type v)
    {
        return MathImpl<value_type>::ACos(v);
    }
    value_type Parser::ATan(value_type v)
    {
        return MathImpl<value_type>::ATan(v);
    }
    value_type Parser::ATan2(value_type v1, value_type v2)
    {
        return MathImpl<value_type>::ATan2(v1, v2);
    }
    value_type Parser::Sinh(value_type v)
    {
        return MathImpl<value_type>::Sinh(v);
    }
    value_type Parser::Cosh(value_type v)
    {
        return MathImpl<value_type>::Cosh(v);
    }
    value_type Parser::Tanh(value_type v)
    {
        return MathImpl<value_type>::Tanh(v);
    }
    value_type Parser::ASinh(value_type v)
    {
        return MathImpl<value_type>::ASinh(v);
    }
    value_type Parser::ACosh(value_type v)
    {
        return MathImpl<value_type>::ACosh(v);
    }
    value_type Parser::ATanh(value_type v)
    {
        return MathImpl<value_type>::ATanh(v);
    }

    //---------------------------------------------------------------------------
    // Logarithm functions
    value_type Parser::Log2(value_type v)
    {
        return MathImpl<value_type>::Log2(v);     // Logarithm base 2
    }
    value_type Parser::Log10(value_type v)
    {
        return MathImpl<value_type>::Log10(v);    // Logarithm base 10
    }
    value_type Parser::Ln(value_type v)
    {
        return MathImpl<value_type>::Log(v);      // Logarithm base e (natural logarithm)
    }

    //---------------------------------------------------------------------------
    //  misc
    value_type Parser::Exp(value_type v)
    {
        return MathImpl<value_type>::Exp(v);
    }
    value_type Parser::Abs(value_type v)
    {
        return MathImpl<value_type>::Abs(v);
    }
    value_type Parser::Sqrt(value_type v)
    {
        return MathImpl<value_type>::Sqrt(v);
    }
    value_type Parser::Rint(value_type v)
    {
        return MathImpl<value_type>::Rint(v);
    }
    value_type Parser::Sign(value_type v)
    {
        return MathImpl<value_type>::Sign(v);
    }

    //---------------------------------------------------------------------------
    /** \brief Callback for the unary minus operator.
        \param v The value to negate
        \return -v
    */
    value_type Parser::UnaryMinus(value_type v)
    {
        return -v;
    }

    //---------------------------------------------------------------------------
    /** \brief Callback for adding multiple values.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    value_type Parser::Sum(const value_type* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function sum."));
        return parser_Sum(a_afArg, a_iArgc);
    }

    //---------------------------------------------------------------------------
    /** \brief Callback for averaging multiple values.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    value_type Parser::Avg(const value_type* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function avg."));
        return parser_Avg(a_afArg, a_iArgc);
    }


    //---------------------------------------------------------------------------
    /** \brief Callback for determining the minimum value out of a vector.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    value_type Parser::Min(const value_type* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function min."));
        return parser_Min(a_afArg, a_iArgc);
    }


    //---------------------------------------------------------------------------
    /** \brief Callback for determining the maximum value out of a vector.
        \param [in] a_afArg Vector with the function arguments
        \param [in] a_iArgc The size of a_afArg
    */
    value_type Parser::Max(const value_type* a_afArg, int a_iArgc)
    {
        if (!a_iArgc)
            throw exception_type(_nrT("too few arguments for function max."));
        return parser_Max(a_afArg, a_iArgc);
    }


    //---------------------------------------------------------------------------
    /** \brief Default value recognition callback.
        \param [in] a_szExpr Pointer to the expression
        \param [in, out] a_iPos Pointer to an index storing the current position within the expression
        \param [out] a_fVal Pointer where the value should be stored in case one is found.
        \return 1 if a value was found 0 otherwise.
    */
    int Parser::IsVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
    {
        value_type fVal(0);

        stringstream_type stream(a_szExpr);
        stream.seekg(0);        // todo:  check if this really is necessary
        stream.imbue(Parser::s_locale);
        stream >> fVal;
        stringstream_type::pos_type iEnd = stream.tellg(); // Position after reading

        if (iEnd == (stringstream_type::pos_type) - 1)
            return 0;

        *a_iPos += (int)iEnd;
        *a_fVal = fVal;
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
        DefineNameChars( _nrT("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]~\\") );
        DefineOprtChars( _nrT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-*^/?<>=#!$%&|~'_{}") );
        DefineInfixOprtChars( _nrT("/+-*^?<>=#!$%&|~'_") );
    }

    //---------------------------------------------------------------------------
    /** \brief Initialize the default functions. */
    void Parser::InitFun()
    {
        if (mu::TypeInfo<mu::value_type>::IsInteger())
        {
            // When setting MUP_BASETYPE to an integer type
            // Place functions for dealing with integer values here
            // ...
            // ...
            // ...
        }
        else
        {
            // trigonometric functions
            DefineFun(_nrT("sin"), Sin);
            DefineFun(_nrT("cos"), Cos);
            DefineFun(_nrT("tan"), Tan);
            // arcus functions
            DefineFun(_nrT("asin"), ASin);
            DefineFun(_nrT("arcsin"), ASin);
            DefineFun(_nrT("acos"), ACos);
            DefineFun(_nrT("arccos"), ACos);
            DefineFun(_nrT("atan"), ATan);
            DefineFun(_nrT("arctan"), ATan);
            DefineFun(_nrT("atan2"), ATan2);
            // hyperbolic functions
            DefineFun(_nrT("sinh"), Sinh);
            DefineFun(_nrT("cosh"), Cosh);
            DefineFun(_nrT("tanh"), Tanh);
            // arcus hyperbolic functions
            DefineFun(_nrT("asinh"), ASinh);
            DefineFun(_nrT("arsinh"), ASinh);
            DefineFun(_nrT("acosh"), ACosh);
            DefineFun(_nrT("arcosh"), ACosh);
            DefineFun(_nrT("atanh"), ATanh);
            DefineFun(_nrT("artanh"), ATanh);
            // Logarithm functions
            DefineFun(_nrT("log2"), Log2);
            DefineFun(_nrT("log10"), Log10);
            DefineFun(_nrT("log"), Log10);
            DefineFun(_nrT("ln"), Ln);
            // misc
            DefineFun(_nrT("exp"), Exp);
            DefineFun(_nrT("sqrt"), Sqrt);
            DefineFun(_nrT("sign"), Sign);
            DefineFun(_nrT("rint"), Rint);
            DefineFun(_nrT("abs"), Abs);
            // Functions with variable number of arguments
            DefineFun(_nrT("sum"), Sum);
            DefineFun(_nrT("avg"), Avg);
            DefineFun(_nrT("min"), Min);
            DefineFun(_nrT("max"), Max);
        }
    }

    //---------------------------------------------------------------------------
    /** \brief Initialize constants.

      By default the parser recognizes two constants. Pi ("pi") and the eulerian
      number ("_e").
    */
    void Parser::InitConst()
    {
        DefineConst(_nrT("_pi"), (value_type)PARSER_CONST_PI);
        //DefineConst(_nrT("_e"), (value_type)PARSER_CONST_E);
    }

    //---------------------------------------------------------------------------
    /** \brief Initialize operators.

      By default only the unary minus operator is added.
    */
    void Parser::InitOprt()
    {
        DefineInfixOprt(_nrT("-"), UnaryMinus);
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

      Numerical differentiation uses a 5 point operator yielding a 4th order
      formula. The default value for epsilon is 0.00074 which is
      numeric_limits<double>::epsilon() ^ (1/5) as suggested in the muparser
      forum:

      http://sourceforge.net/forum/forum.php?thread_id=1994611&forum_id=462843
    */
    value_type Parser::Diff(value_type* a_Var,
                            value_type  a_fPos,
                            value_type  a_fEpsilon)
    {
        value_type fRes(0),
                   fBuf(*a_Var),
                   f[4] = {0, 0, 0, 0},
                          fEpsilon(a_fEpsilon);

        // Backwards compatible calculation of epsilon inc case the user doesnt provide
        // his own epsilon
        if (fEpsilon == 0)
            fEpsilon = (a_fPos == 0) ? (value_type)1e-10 : (value_type)1e-7 * a_fPos;

        *a_Var = a_fPos + 2 * fEpsilon;
        f[0] = Eval();
        *a_Var = a_fPos + 1 * fEpsilon;
        f[1] = Eval();
        *a_Var = a_fPos - 1 * fEpsilon;
        f[2] = Eval();
        *a_Var = a_fPos - 2 * fEpsilon;
        f[3] = Eval();
        *a_Var = fBuf; // restore variable

        fRes = (-f[0] + 8 * f[1] - 8 * f[2] + f[3]) / (12 * fEpsilon);
        return fRes;
    }
} // namespace mu
