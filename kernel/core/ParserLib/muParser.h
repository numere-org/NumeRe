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
#ifndef MU_PARSER_H
#define MU_PARSER_H

//--- Standard includes ------------------------------------------------------------------------
#include <vector>

//--- Parser includes --------------------------------------------------------------------------
#include "muParserBase.h"

/** \file
    \brief Definition of the standard floating point parser.
*/

namespace mu
{
  /** \brief Mathematical expressions parser.

    Standard implementation of the mathematical expressions parser.
    Can be used as a reference implementation for subclassing the parser.

    <small>
    (C) 2011 Ingo Berg<br>
    muparser(at)gmx.de
    </small>
  */
  /* final */ class Parser : public ParserBase
  {
  public:

    Parser();

    virtual void InitCharSets() override;
    virtual void InitFun() override;
    virtual void InitConst() override;
    virtual void InitOprt() override;
    virtual void OnDetectVar(string_type *pExpr, int &nStart, int &nEnd) override;

    value_type Diff(value_type *a_Var,
                    value_type a_fPos,
                    value_type a_fEpsilon = 0,
                    size_t order = 1);

  protected:

    // Trigonometric functions
    static value_type  Sin(const value_type&);
    static value_type  Cos(const value_type&);
    static value_type  Tan(const value_type&);
    static value_type  Tan2(const value_type&, const value_type&);
    // arcus functions
    static value_type  ASin(const value_type&);
    static value_type  ACos(const value_type&);
    static value_type  ATan(const value_type&);
    static value_type  ATan2(const value_type&, const value_type&);

    // hyperbolic functions
    static value_type  Sinh(const value_type&);
    static value_type  Cosh(const value_type&);
    static value_type  Tanh(const value_type&);
    // arcus hyperbolic functions
    static value_type  ASinh(const value_type&);
    static value_type  ACosh(const value_type&);
    static value_type  ATanh(const value_type&);
    // Logarithm functions
    static value_type  Log2(const value_type&);  // Logarithm Base 2
    static value_type  Log10(const value_type&); // Logarithm Base 10
    static value_type  Ln(const value_type&);    // Logarithm Base e (natural logarithm)
    // misc
    static value_type  Exp(const value_type&);
    static value_type  Abs(const value_type&);
    static value_type  Sqrt(const value_type&);
    static value_type  Rint(const value_type&);
    static value_type  Sign(const value_type&);

    // Prefix operators
    // !!! Unary Minus is a MUST if you want to use negative signs !!!
    static value_type  UnaryMinus(const value_type&);

    // Functions with variable number of arguments
    static value_type Sum(const value_type*, int);  // sum
    static value_type Avg(const value_type*, int);  // mean value
    static value_type Min(const value_type*, int);  // minimum
    static value_type Max(const value_type*, int);  // maximum

    static int IsVal(const char_type* a_szExpr, int *a_iPos, value_type *a_fVal);
  };
} // namespace mu

#endif


