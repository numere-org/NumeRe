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
    Array Parser::Sin(const Array& v)
    {
        return apply(std::sin, v);
    }
    Array Parser::Cos(const Array& v)
    {
        return apply(std::cos, v);
    }
    Array Parser::Tan(const Array& v)
    {
        return apply(std::tan, v);
    }
    Array Parser::ASin(const Array& v)
    {
        return apply(std::asin, v);
    }
    Array Parser::ACos(const Array& v)
    {
        return apply(std::acos, v);
    }
    Array Parser::ATan(const Array& v)
    {
        return apply(std::atan, v);
    }
    value_type Parser::ATan2(const value_type& v1, const value_type& v2)
    {
        return v1;// MathImpl<double>::ATan2(v1.real(), v2.real());
    }
    Array Parser::Sinh(const Array& v)
    {
        return apply(std::sinh, v);
    }
    Array Parser::Cosh(const Array& v)
    {
        return apply(std::cosh, v);
    }
    Array Parser::Tanh(const Array& v)
    {
        return apply(std::tanh, v);
    }
    Array Parser::ASinh(const Array& v)
    {
        return apply(std::asinh, v);
    }
    Array Parser::ACosh(const Array& v)
    {
        return apply(std::acosh, v);
    }
    Array Parser::ATanh(const Array& v)
    {
        return apply(std::atanh, v);
    }

    //---------------------------------------------------------------------------
    // Logarithm functions
    Array Parser::Log2(const Array& v)
    {
        return apply(MathImpl<std::complex<double>>::Log2, v);     // Logarithm base 2
    }
    Array Parser::Log10(const Array& v)
    {
        return apply(MathImpl<std::complex<double>>::Log10, v);    // Logarithm base 10
    }
    Array Parser::Ln(const Array& v)
    {
        return apply(std::log, v);     // Logarithm base e (natural logarithm)
    }

    //---------------------------------------------------------------------------
    //  misc
    Array Parser::Exp(const Array& v)
    {
        return apply(MathImpl<std::complex<double>>::Exp, v);
    }
    Array Parser::Abs(const Array& v)
    {
        return v; //apply(MathImpl<std::complex<double>>::Abs, v); //(v.real() == 0.0 || v.imag() == 0.0) ? (std::abs(v.real()) + std::abs(v.imag())) : std::abs(v); //MathImpl<value_type>::Abs(v);
    }
    Array Parser::Sqrt(const Array& v)
    {
        return apply(MathImpl<std::complex<double>>::Sqrt, v);
    }
    Array Parser::Rint(const Array& v)
    {
        return v;// value_type(MathImpl<double>::Rint(v.real()), MathImpl<double>::Rint(v.imag()));
    }
    Array Parser::Sign(const Array& v)
    {
        return v;//value_type(MathImpl<double>::Sign(v.real()), MathImpl<double>::Sign(v.imag()));
    }

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
    int Parser::IsVal(const char_type* a_szExpr, int* a_iPos, Value* a_fVal)
    {
        std::complex<double> fVal(0);

        stringstream_type stream(a_szExpr);
        stream.seekg(0);        // todo:  check if this really is necessary
        stream.imbue(Parser::s_locale);
        stream >> fVal;
        stringstream_type::pos_type iEnd = stream.tellg(); // Position after reading

        if (iEnd == (stringstream_type::pos_type) - 1)
            return 0;

        *a_iPos += (int)iEnd;
        *a_fVal = Numerical(fVal);
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
            DefineFun("sin", Sin);
            DefineFun("cos", Cos);
            DefineFun("tan", Tan);
            // arcus functions
            DefineFun("asin", ASin);
            DefineFun("arcsin", ASin);
            DefineFun("acos", ACos);
            DefineFun("arccos", ACos);
            DefineFun("atan", ATan);
            DefineFun("arctan", ATan);
            DefineFun("atan2", ATan2);
            // hyperbolic functions
            DefineFun("sinh", Sinh);
            DefineFun("cosh", Cosh);
            DefineFun("tanh", Tanh);
            // arcus hyperbolic functions
            DefineFun("asinh", ASinh);
            DefineFun("arsinh", ASinh);
            DefineFun("acosh", ACosh);
            DefineFun("arcosh", ACosh);
            DefineFun("atanh", ATanh);
            DefineFun("artanh", ATanh);
            // Logarithm functions
            DefineFun("log2", Log2);
            DefineFun("log10", Log10);
            DefineFun("log", Log10);
            DefineFun("ln", Ln);
            // misc
            DefineFun("exp", Exp);
            DefineFun("sqrt", Sqrt);
            DefineFun("sign", Sign);
            DefineFun("rint", Rint);
            DefineFun("abs", Abs);
            // Functions with variable number of arguments
            DefineFun("sum", Sum);
            DefineFun("avg", Avg);
            DefineFun("min", Min);
            DefineFun("max", Max);
        }
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
    std::vector<Array> Parser::Diff(Value* a_Var,
                       const Array& a_fPos,
                       Value fEpsilon,
                       size_t order)
    {
        Value fBuf(*a_Var);
        std::vector<Array> fRes;
        std::array<Array, 5> f;
        std::array<double, 5> factors = {-2, -1, 0, 1, 2};

        // Backwards compatible calculation of epsilon inc case the user doesnt provide
        // his own epsilon
        if (fEpsilon == Value(0.0))
        {
            Array absVal = Abs(a_fPos);
            fEpsilon = (a_fPos == Value(0.0)) ? Value(1e-10) : Value(1e-7*Max(&absVal, 1).front()*intPower(10, 2*(order-1)));
        }

        for (size_t n = 0; n < a_fPos.size(); n++)
        {
            for (size_t i = 0; i < f.size(); i++)
            {
                *a_Var = a_fPos[n] + Value(factors[i]) * fEpsilon;
                f[i] = Eval().front();
            }

            // Reference: https://web.media.mit.edu/~crtaylor/calculator.html
            if (order == 1)
                fRes.push_back(( f[0]  - Value(8.0) * f[1]              + Value(8.0) * f[3] - f[4]) / (Value(12.0) * fEpsilon));
            else if (order == 2)
                fRes.push_back((-f[0] + Value(16.0) * f[1] - Value(30.0)*f[2] + Value(16.0) * f[3] - f[4]) / (Value(12.0) * fEpsilon * fEpsilon));
            else if (order == 3)
                fRes.push_back((-f[0]  + Value(2.0) * f[1]              - Value(2.0) * f[3] + f[4]) / (Value(2.0) * fEpsilon * fEpsilon * fEpsilon));
            else
                fRes.push_back(Value(NAN));
        }

        *a_Var = fBuf; // restore variable
        return fRes;
    }
} // namespace mu
