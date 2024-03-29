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

#include "muParserTest.h"

#include <cstdio>
#include <cmath>
#include <iostream>
#include <limits>

#define PARSER_CONST_PI  3.141592653589793238462643
#define PARSER_CONST_E   2.718281828459045235360287

using namespace std;

/** \file
    \brief This file contains the implementation of parser test cases.
*/

namespace mu
{
  namespace Test
  {
    int ParserTester::c_iCount = 0;

    //---------------------------------------------------------------------------------------------
    ParserTester::ParserTester()
      :m_vTestFun()
    {
      AddTest(&ParserTester::TestNames);
      AddTest(&ParserTester::TestSyntax);
      AddTest(&ParserTester::TestPostFix);
      AddTest(&ParserTester::TestInfixOprt);
      AddTest(&ParserTester::TestVarConst);
      AddTest(&ParserTester::TestMultiArg);
      AddTest(&ParserTester::TestExpression);
      AddTest(&ParserTester::TestIfThenElse);
      AddTest(&ParserTester::TestInterface);
      AddTest(&ParserTester::TestBinOprt);
      AddTest(&ParserTester::TestException);
      AddTest(&ParserTester::TestStrArg);

      ParserTester::c_iCount = 0;
    }

    //---------------------------------------------------------------------------------------------
    int ParserTester::IsHexVal(const char_type *a_szExpr, int *a_iPos, value_type *a_fVal)
    {
      if (a_szExpr[1]==0 || (a_szExpr[0]!='0' || a_szExpr[1]!='x') )
        return 0;

      unsigned iVal(0);

      // New code based on streams for UNICODE compliance:
      stringstream_type::pos_type nPos(0);
      stringstream_type ss(a_szExpr + 2);
      ss >> std::hex >> iVal;
      nPos = ss.tellg();

      if (nPos==(stringstream_type::pos_type)0)
        return 1;

      *a_iPos += (int)(2 + nPos);
      *a_fVal = (value_type)iVal;
      return 1;
    }

    //---------------------------------------------------------------------------------------------
    int ParserTester::TestInterface()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Parserfunktionsfaehigkeit ... ");

      // Test RemoveVar
      value_type afVal[3] = {1,2,3};
      Parser p;

      try
      {
        p.DefineVar( _nrT("a"), &afVal[0]);
        p.DefineVar( _nrT("b"), &afVal[1]);
        p.DefineVar( _nrT("c"), &afVal[2]);
        p.SetExpr( string("a+b+c") );
        p.Eval();
      }
      catch(...)
      {
        iStat += 1;  // this is not supposed to happen
      }

      try
      {
        p.RemoveVar( _nrT("c") );
        p.Eval();
        iStat += 1;  // not supposed to reach this, nonexisting variable "c" deleted...
      }
      catch(...)
      {
        // failure is expected...
      }

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Funktionsfaehigkeit fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------------------------
    int ParserTester::TestStrArg()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Zeichenkettenargumente ... ");

      iStat += EqnTest(_nrT("valueof(\"\")"), 123, true);   // empty string arguments caused a crash
      iStat += EqnTest(_nrT("valueof(\"aaa\")+valueof(\"bbb\")  "), 246, true);
      iStat += EqnTest(_nrT("2*(valueof(\"aaa\")-23)+valueof(\"bbb\")"), 323, true);
      // use in expressions with variables
      iStat += EqnTest(_nrT("a*(atof(\"10\")-b)"), 8, true);
      iStat += EqnTest(_nrT("a-(atof(\"10\")*b)"), -19, true);
      // string + numeric arguments
      iStat += EqnTest(_nrT("strfun1(\"100\")"), 100, true);
      iStat += EqnTest(_nrT("strfun2(\"100\",1)"), 101, true);
      iStat += EqnTest(_nrT("strfun3(\"99\",1,2)"), 102, true);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Zeichenkettenargumente fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------------------------
    int ParserTester::TestBinOprt()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste binaere Operatoren ... ");

      // built in operators
      // xor operator
      //iStat += EqnTest(_nrT("1 xor 2"), 3, true);
      //iStat += EqnTest(_nrT("a xor b"), 3, true);            // with a=1 and b=2
      //iStat += EqnTest(_nrT("1 xor 2 xor 3"), 0, true);
      //iStat += EqnTest(_nrT("a xor b xor 3"), 0, true);      // with a=1 and b=2
      //iStat += EqnTest(_nrT("a xor b xor c"), 0, true);      // with a=1 and b=2
      //iStat += EqnTest(_nrT("(1 xor 2) xor 3"), 0, true);
      //iStat += EqnTest(_nrT("(a xor b) xor c"), 0, true);    // with a=1 and b=2
      //iStat += EqnTest(_nrT("(a) xor (b) xor c"), 0, true);  // with a=1 and b=2
      //iStat += EqnTest(_nrT("1 or 2"), 3, true);
      //iStat += EqnTest(_nrT("a or b"), 3, true);             // with a=1 and b=2
      iStat += EqnTest(_nrT("a++b"), 3, true);
      iStat += EqnTest(_nrT("a ++ b"), 3, true);
      iStat += EqnTest(_nrT("1++2"), 3, true);
      iStat += EqnTest(_nrT("1 ++ 2"), 3, true);
      iStat += EqnTest(_nrT("a add b"), 3, true);
      iStat += EqnTest(_nrT("1 add 2"), 3, true);
      iStat += EqnTest(_nrT("a<b"), 1, true);
      iStat += EqnTest(_nrT("b>a"), 1, true);
      iStat += EqnTest(_nrT("a>a"), 0, true);
      iStat += EqnTest(_nrT("a<a"), 0, true);
      iStat += EqnTest(_nrT("a>a"), 0, true);
      iStat += EqnTest(_nrT("a<=a"), 1, true);
      iStat += EqnTest(_nrT("a<=b"), 1, true);
      iStat += EqnTest(_nrT("b<=a"), 0, true);
      iStat += EqnTest(_nrT("a>=a"), 1, true);
      iStat += EqnTest(_nrT("b>=a"), 1, true);
      iStat += EqnTest(_nrT("a>=b"), 0, true);

      // Test logical operators, expecially if user defined "&" and the internal "&&" collide
      iStat += EqnTest(_nrT("1 && 1"), 1, true);
      iStat += EqnTest(_nrT("1 && 0"), 0, true);
      iStat += EqnTest(_nrT("(a<b) && (b>a)"), 1, true);
      iStat += EqnTest(_nrT("(a<b) && (a>b)"), 0, true);
      //iStat += EqnTest(_nrT("12 and 255"), 12, true);
      //iStat += EqnTest(_nrT("12 and 0"), 0, true);
      iStat += EqnTest(_nrT("12 & 255"), 12, true);
      iStat += EqnTest(_nrT("12 & 0"), 0, true);
      iStat += EqnTest(_nrT("12&255"), 12, true);
      iStat += EqnTest(_nrT("12&0"), 0, true);


      // Assignement operator
      iStat += EqnTest(_nrT("a = b"), 2, true);
      iStat += EqnTest(_nrT("a = sin(b)"), 0.909297, true);
      iStat += EqnTest(_nrT("a = 1+sin(b)"), 1.909297, true);
      iStat += EqnTest(_nrT("(a=b)*2"), 4, true);
      iStat += EqnTest(_nrT("2*(a=b)"), 4, true);
      iStat += EqnTest(_nrT("2*(a=b+1)"), 6, true);
      iStat += EqnTest(_nrT("(a=b+1)*2"), 6, true);
      iStat += EqnTest(_nrT("2^2^3"), 256, true);
      iStat += EqnTest(_nrT("1/2/3"), 1.0/6.0, true);

      // reference: http://www.wolframalpha.com/input/?i=3%2B4*2%2F%281-5%29^2^3
      iStat += EqnTest(_nrT("3+4*2/(1-5)^2^3"), 3.0001220703125, true);
      // Test user defined binary operators
      iStat += EqnTestInt(_nrT("1 | 2"), 3, true);
      iStat += EqnTestInt(_nrT("1 || 2"), 1, true);
      iStat += EqnTestInt(_nrT("123 & 456"), 72, true);
      iStat += EqnTestInt(_nrT("(123 & 456) % 10"), 2, true);
      iStat += EqnTestInt(_nrT("1 && 0"), 0, true);
      iStat += EqnTestInt(_nrT("123 && 456"), 1, true);
      iStat += EqnTestInt(_nrT("1 << 3"), 8, true);
      iStat += EqnTestInt(_nrT("8 >> 3"), 1, true);
      iStat += EqnTestInt(_nrT("9 / 4"), 2, true);
      iStat += EqnTestInt(_nrT("9 % 4"), 1, true);
      iStat += EqnTestInt(_nrT("if(5%2,1,0)"), 1, true);
      iStat += EqnTestInt(_nrT("if(4%2,1,0)"), 0, true);
      iStat += EqnTestInt(_nrT("-10+1"), -9, true);
      iStat += EqnTestInt(_nrT("1+2*3"), 7, true);
      iStat += EqnTestInt(_nrT("const1 != const2"), 1, true);
      iStat += EqnTestInt(_nrT("const1 != const2"), 0, false);
      iStat += EqnTestInt(_nrT("const1 == const2"), 0, true);
      iStat += EqnTestInt(_nrT("const1 == 1"), 1, true);
      iStat += EqnTestInt(_nrT("10*(const1 == 1)"), 10, true);
      iStat += EqnTestInt(_nrT("2*(const1 | const2)"), 6, true);
      iStat += EqnTestInt(_nrT("2*(const1 | const2)"), 7, false);
      iStat += EqnTestInt(_nrT("const1 < const2"), 1, true);
      iStat += EqnTestInt(_nrT("const2 > const1"), 1, true);
      iStat += EqnTestInt(_nrT("const1 <= 1"), 1, true);
      iStat += EqnTestInt(_nrT("const2 >= 2"), 1, true);
      iStat += EqnTestInt(_nrT("2*(const1 + const2)"), 6, true);
      iStat += EqnTestInt(_nrT("2*(const1 - const2)"), -2, true);
      iStat += EqnTestInt(_nrT("a != b"), 1, true);
      iStat += EqnTestInt(_nrT("a != b"), 0, false);
      iStat += EqnTestInt(_nrT("a == b"), 0, true);
      iStat += EqnTestInt(_nrT("a == 1"), 1, true);
      iStat += EqnTestInt(_nrT("10*(a == 1)"), 10, true);
      iStat += EqnTestInt(_nrT("2*(a | b)"), 6, true);
      iStat += EqnTestInt(_nrT("2*(a | b)"), 7, false);
      iStat += EqnTestInt(_nrT("a < b"), 1, true);
      iStat += EqnTestInt(_nrT("b > a"), 1, true);
      iStat += EqnTestInt(_nrT("a <= 1"), 1, true);
      iStat += EqnTestInt(_nrT("b >= 2"), 1, true);
      iStat += EqnTestInt(_nrT("2*(a + b)"), 6, true);
      iStat += EqnTestInt(_nrT("2*(a - b)"), -2, true);
      iStat += EqnTestInt(_nrT("a + (a << b)"), 5, true);
      iStat += EqnTestInt(_nrT("-2^2"), -4, true);
      iStat += EqnTestInt(_nrT("3--a"), 4, true);
      iStat += EqnTestInt(_nrT("3+-3^2"), -6, true);
      // Test reading of hex values:
      iStat += EqnTestInt(_nrT("0xff"), 255, true);
      iStat += EqnTestInt(_nrT("10+0xff"), 265, true);
      iStat += EqnTestInt(_nrT("0xff+10"), 265, true);
      iStat += EqnTestInt(_nrT("10*0xff"), 2550, true);
      iStat += EqnTestInt(_nrT("0xff*10"), 2550, true);
      iStat += EqnTestInt(_nrT("10+0xff+1"), 266, true);
      iStat += EqnTestInt(_nrT("1+0xff+10"), 266, true);
// incorrect: '^' is yor here, not power
//    iStat += EqnTestInt("-(1+2)^2", -9, true);
//    iStat += EqnTestInt("-1^3", -1, true);

      // Test precedence
      // a=1, b=2, c=3
      iStat += EqnTestInt(_nrT("a + b * c"), 7, true);
      iStat += EqnTestInt(_nrT("a * b + c"), 5, true);
      iStat += EqnTestInt(_nrT("a<b && b>10"), 0, true);
      iStat += EqnTestInt(_nrT("a<b && b<10"), 1, true);

      iStat += EqnTestInt(_nrT("a + b << c"), 17, true);
      iStat += EqnTestInt(_nrT("a << b + c"), 7, true);
      iStat += EqnTestInt(_nrT("c * b < a"), 0, true);
      iStat += EqnTestInt(_nrT("c * b == 6 * a"), 1, true);
      iStat += EqnTestInt(_nrT("2^2^3"), 256, true);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Binaere Operatoren fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------------------------
    /** \brief Check muParser name restriction enforcement. */
    int ParserTester::TestNames()
    {
      int  iStat= 0,
           iErr = 0;

      cerr << "\r                                                                              \r";
      mu::console() << " -> Teste Namensbeschraenkungen ... ";

      Parser p;

  #define PARSER_THROWCHECK(DOMAIN, FAIL, EXPR, ARG) \
      iErr = 0;                                      \
      ParserTester::c_iCount++;                      \
      try                                            \
      {                                              \
        p.Define##DOMAIN(EXPR, ARG);                 \
      }                                              \
      catch(Parser::exception_type&)                 \
      {                                              \
        iErr = (FAIL==false) ? 0 : 1;                \
      }                                              \
      iStat += iErr;

      // constant names
      PARSER_THROWCHECK(Const, false, _nrT("0a"), 1)
      PARSER_THROWCHECK(Const, false, _nrT("9a"), 1)
      PARSER_THROWCHECK(Const, false, _nrT("+a"), 1)
      PARSER_THROWCHECK(Const, false, _nrT("-a"), 1)
      PARSER_THROWCHECK(Const, false, _nrT("a-"), 1)
      PARSER_THROWCHECK(Const, false, _nrT("a*"), 1)
      PARSER_THROWCHECK(Const, false, _nrT("a?"), 1)
      PARSER_THROWCHECK(Const, true, _nrT("a"), 1)
      PARSER_THROWCHECK(Const, true, _nrT("a_min"), 1)
      PARSER_THROWCHECK(Const, true, _nrT("a_min0"), 1)
      PARSER_THROWCHECK(Const, true, _nrT("a_min9"), 1)
      // variable names
      value_type a;
      p.ClearConst();
      PARSER_THROWCHECK(Var, false, _nrT("123abc"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("9a"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("0a"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("+a"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("-a"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("?a"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("!a"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("a+"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("a-"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("a*"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("a?"), &a)
      PARSER_THROWCHECK(Var, true, _nrT("a"), &a)
      PARSER_THROWCHECK(Var, true, _nrT("a_min"), &a)
      PARSER_THROWCHECK(Var, true, _nrT("a_min0"), &a)
      PARSER_THROWCHECK(Var, true, _nrT("a_min9"), &a)
      PARSER_THROWCHECK(Var, false, _nrT("a_min9"), 0)
      // Postfix operators
      // fail
      PARSER_THROWCHECK(PostfixOprt, false, _nrT("(k"), f1of1)
      PARSER_THROWCHECK(PostfixOprt, false, _nrT("9+"), f1of1)
      PARSER_THROWCHECK(PostfixOprt, false, _nrT("+"), 0)
      // pass
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("-a"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("?a"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("_"),   f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("#"),   f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("&&"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("||"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("&"),   f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("|"),   f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("++"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("--"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("?>"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("?<"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("**"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("xor"), f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("and"), f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("or"),  f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("not"), f1of1)
      PARSER_THROWCHECK(PostfixOprt, true, _nrT("!"),   f1of1)
      // Binary operator
      // The following must fail with builtin operators activated
      // p.EnableBuiltInOp(true); -> this is the default
      p.ClearPostfixOprt();
      PARSER_THROWCHECK(Oprt, false, _nrT("+"),  f1of2)
      PARSER_THROWCHECK(Oprt, false, _nrT("-"),  f1of2)
      PARSER_THROWCHECK(Oprt, false, _nrT("*"),  f1of2)
      PARSER_THROWCHECK(Oprt, false, _nrT("/"),  f1of2)
      PARSER_THROWCHECK(Oprt, false, _nrT("^"),  f1of2)
      PARSER_THROWCHECK(Oprt, false, _nrT("&&"),  f1of2)
      PARSER_THROWCHECK(Oprt, false, _nrT("||"),  f1of2)
      // without activated built in operators it should work
      p.EnableBuiltInOprt(false);
      PARSER_THROWCHECK(Oprt, true, _nrT("+"),  f1of2)
      PARSER_THROWCHECK(Oprt, true, _nrT("-"),  f1of2)
      PARSER_THROWCHECK(Oprt, true, _nrT("*"),  f1of2)
      PARSER_THROWCHECK(Oprt, true, _nrT("/"),  f1of2)
      PARSER_THROWCHECK(Oprt, true, _nrT("^"),  f1of2)
      PARSER_THROWCHECK(Oprt, true, _nrT("&&"),  f1of2)
      PARSER_THROWCHECK(Oprt, true, _nrT("||"),  f1of2)
  #undef PARSER_THROWCHECK

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Namensbeschraenkung fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------
    int ParserTester::TestSyntax()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Syntaxerkennung ... ");

      iStat += ThrowTest(_nrT("1,"), ecUNEXPECTED_EOF);  // incomplete hex definition
      iStat += ThrowTest(_nrT("a,"), ecUNEXPECTED_EOF);  // incomplete hex definition
      iStat += ThrowTest(_nrT("sin(8),"), ecUNEXPECTED_EOF);  // incomplete hex definition
      iStat += ThrowTest(_nrT("(sin(8)),"), ecUNEXPECTED_EOF);  // incomplete hex definition
      iStat += ThrowTest(_nrT("a m,"), ecUNEXPECTED_EOF);  // incomplete hex definition


      iStat += EqnTest(_nrT("(1+ 2*a)"), 3, true);   // Spaces within formula
      iStat += EqnTest(_nrT("sqrt((4))"), 2, true);  // Multiple brackets
      iStat += EqnTest(_nrT("sqrt((2)+2)"), 2, true);// Multiple brackets
      iStat += EqnTest(_nrT("sqrt(2+(2))"), 2, true);// Multiple brackets
      iStat += EqnTest(_nrT("sqrt(a+(3))"), 2, true);// Multiple brackets
      iStat += EqnTest(_nrT("sqrt((3)+a)"), 2, true);// Multiple brackets
      iStat += EqnTest(_nrT("order(1,2)"), 1, true); // May not cause name collision with operator "or"
      iStat += EqnTest(_nrT("(2+"), 0, false);       // missing closing bracket
      iStat += EqnTest(_nrT("2++4"), 0, false);      // unexpected operator
      iStat += EqnTest(_nrT("2+-4"), 0, false);      // unexpected operator
      iStat += EqnTest(_nrT("(2+)"), 0, false);      // unexpected closing bracket
      iStat += EqnTest(_nrT("--2"), 0, false);       // double sign
      iStat += EqnTest(_nrT("ksdfj"), 0, false);     // unknown token
      iStat += EqnTest(_nrT("()"), 0, false);        // empty bracket without a function
      iStat += EqnTest(_nrT("5+()"), 0, false);      // empty bracket without a function
      iStat += EqnTest(_nrT("sin(cos)"), 0, false);  // unexpected function
      iStat += EqnTest(_nrT("5t6"), 0, false);       // unknown token
      iStat += EqnTest(_nrT("5 t 6"), 0, false);     // unknown token
      iStat += EqnTest(_nrT("8*"), 0, false);        // unexpected end of formula
      iStat += EqnTest(_nrT(",3"), 0, false);        // unexpected comma
      iStat += EqnTest(_nrT("3,5"), 0, false);       // unexpected comma
      iStat += EqnTest(_nrT("sin(8,8)"), 0, false);  // too many function args
      iStat += EqnTest(_nrT("(7,8)"), 0, false);     // too many function args
      iStat += EqnTest(_nrT("sin)"), 0, false);      // unexpected closing bracket
      iStat += EqnTest(_nrT("a)"), 0, false);        // unexpected closing bracket
      iStat += EqnTest(_nrT("pi)"), 0, false);       // unexpected closing bracket
      iStat += EqnTest(_nrT("sin(())"), 0, false);   // unexpected closing bracket
      iStat += EqnTest(_nrT("sin()"), 0, false);     // unexpected closing bracket


      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Syntaxerkennung fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------
    int ParserTester::TestVarConst()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Variablen-/Konstantenerkennung ... ");

      // Test if the result changes when a variable changes
      iStat += EqnTestWithVarChange( _nrT("a"), 1, 1, 2, 2 );
      iStat += EqnTestWithVarChange( _nrT("2*a"), 2, 4, 3, 6 );

      // distinguish constants with same basename
      iStat += EqnTest( _nrT("const"), 1, true);
      iStat += EqnTest( _nrT("const1"), 2, true);
      iStat += EqnTest( _nrT("const2"), 3, true);
      iStat += EqnTest( _nrT("2*const"), 2, true);
      iStat += EqnTest( _nrT("2*const1"), 4, true);
      iStat += EqnTest( _nrT("2*const2"), 6, true);
      iStat += EqnTest( _nrT("2*const+1"), 3, true);
      iStat += EqnTest( _nrT("2*const1+1"), 5, true);
      iStat += EqnTest( _nrT("2*const2+1"), 7, true);
      iStat += EqnTest( _nrT("const"), 0, false);
      iStat += EqnTest( _nrT("const1"), 0, false);
      iStat += EqnTest( _nrT("const2"), 0, false);

      // distinguish variables with same basename
      iStat += EqnTest( _nrT("a"), 1, true);
      iStat += EqnTest( _nrT("aa"), 2, true);
      iStat += EqnTest( _nrT("2*a"), 2, true);
      iStat += EqnTest( _nrT("2*aa"), 4, true);
      iStat += EqnTest( _nrT("2*a-1"), 1, true);
      iStat += EqnTest( _nrT("2*aa-1"), 3, true);

      // custom value recognition
      iStat += EqnTest( _nrT("0xff"), 255, true);
      iStat += EqnTest( _nrT("0x97 + 0xff"), 406, true);

      // Finally test querying of used variables
      try
      {
        int idx;
        mu::Parser p;
        mu::value_type vVarVal[] = { 1, 2, 3, 4, 5};
        p.DefineVar( _nrT("a"), &vVarVal[0]);
        p.DefineVar( _nrT("b"), &vVarVal[1]);
        p.DefineVar( _nrT("c"), &vVarVal[2]);
        p.DefineVar( _nrT("d"), &vVarVal[3]);
        p.DefineVar( _nrT("e"), &vVarVal[4]);

        // Test lookup of defined variables
        // 4 used variables
        p.SetExpr( _nrT("a+b+c+d") );
        mu::varmap_type UsedVar = p.GetUsedVar();
        int iCount = (int)UsedVar.size();
        if (iCount!=4)
          throw false;

        // the next check will fail if the parser
        // erroneousely creates new variables internally
        if (p.GetVar().size()!=5)
          throw false;

        mu::varmap_type::const_iterator item = UsedVar.begin();
        for (idx=0; item!=UsedVar.end(); ++item)
        {
          if (&vVarVal[idx++]!=item->second)
            throw false;
        }

        // Test lookup of undefined variables
        p.SetExpr( _nrT("undef1+undef2+undef3") );
        UsedVar = p.GetUsedVar();
        iCount = (int)UsedVar.size();
        if (iCount!=3)
          throw false;

        // the next check will fail if the parser
        // erroneousely creates new variables internally
        if (p.GetVar().size()!=5)
          throw false;

        for (item = UsedVar.begin(); item!=UsedVar.end(); ++item)
        {
          if (item->second!=0)
            throw false; // all pointers to undefined variables must be null
        }

        // 1 used variables
        p.SetExpr( _nrT("a+b") );
        UsedVar = p.GetUsedVar();
        iCount = (int)UsedVar.size();
        if (iCount!=2) throw false;
        item = UsedVar.begin();
        for (idx=0; item!=UsedVar.end(); ++item)
          if (&vVarVal[idx++]!=item->second) throw false;

      }
      catch(...)
      {
        iStat += 1;
      }

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Variablen-/Konstantenerkennung fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------
    int ParserTester::TestMultiArg()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Mehrfachargumentausdruecke ... ");

      // Compound expressions
      iStat += EqnTest( _nrT("1,2,3"), 3, true);
      iStat += EqnTest( _nrT("a,b,c"), 3, true);
      iStat += EqnTest( _nrT("a=10,b=20,c=a*b"), 200, true);
      iStat += EqnTest( _nrT("1,\n2,\n3"), 3, true);
      iStat += EqnTest( _nrT("a,\nb,\nc"), 3, true);
      iStat += EqnTest( _nrT("a=10,\nb=20,\nc=a*b"), 200, true);
      iStat += EqnTest( _nrT("1,\r\n2,\r\n3"), 3, true);
      iStat += EqnTest( _nrT("a,\r\nb,\r\nc"), 3, true);
      iStat += EqnTest( _nrT("a=10,\r\nb=20,\r\nc=a*b"), 200, true);

      // picking the right argument
      iStat += EqnTest( _nrT("f1of1(1)"), 1, true);
      iStat += EqnTest( _nrT("f1of2(1, 2)"), 1, true);
      iStat += EqnTest( _nrT("f2of2(1, 2)"), 2, true);
      iStat += EqnTest( _nrT("f1of3(1, 2, 3)"), 1, true);
      iStat += EqnTest( _nrT("f2of3(1, 2, 3)"), 2, true);
      iStat += EqnTest( _nrT("f3of3(1, 2, 3)"), 3, true);
      iStat += EqnTest( _nrT("f1of4(1, 2, 3, 4)"), 1, true);
      iStat += EqnTest( _nrT("f2of4(1, 2, 3, 4)"), 2, true);
      iStat += EqnTest( _nrT("f3of4(1, 2, 3, 4)"), 3, true);
      iStat += EqnTest( _nrT("f4of4(1, 2, 3, 4)"), 4, true);
      iStat += EqnTest( _nrT("f1of5(1, 2, 3, 4, 5)"), 1, true);
      iStat += EqnTest( _nrT("f2of5(1, 2, 3, 4, 5)"), 2, true);
      iStat += EqnTest( _nrT("f3of5(1, 2, 3, 4, 5)"), 3, true);
      iStat += EqnTest( _nrT("f4of5(1, 2, 3, 4, 5)"), 4, true);
      iStat += EqnTest( _nrT("f5of5(1, 2, 3, 4, 5)"), 5, true);

      // Too few arguments / Too many arguments
      iStat += EqnTest( _nrT("1+ping()"), 11, true);
      iStat += EqnTest( _nrT("ping()+1"), 11, true);
      iStat += EqnTest( _nrT("2*ping()"), 20, true);
      iStat += EqnTest( _nrT("ping()*2"), 20, true);
      iStat += EqnTest( _nrT("ping(1,2)"), 0, false);
      iStat += EqnTest( _nrT("1+ping(1,2)"), 0, false);
      iStat += EqnTest( _nrT("f1of1(1,2)"), 0, false);
      iStat += EqnTest( _nrT("f1of1()"), 0, false);
      iStat += EqnTest( _nrT("f1of2(1, 2, 3)"), 0, false);
      iStat += EqnTest( _nrT("f1of2(1)"), 0, false);
      iStat += EqnTest( _nrT("f1of3(1, 2, 3, 4)"), 0, false);
      iStat += EqnTest( _nrT("f1of3(1)"), 0, false);
      iStat += EqnTest( _nrT("f1of4(1, 2, 3, 4, 5)"), 0, false);
      iStat += EqnTest( _nrT("f1of4(1)"), 0, false);
      iStat += EqnTest( _nrT("(1,2,3)"), 0, false);
      iStat += EqnTest( _nrT("1,2,3"), 0, false);
      iStat += EqnTest( _nrT("(1*a,2,3)"), 0, false);
      iStat += EqnTest( _nrT("1,2*a,3"), 0, false);

      // correct calculation of arguments
      iStat += EqnTest( _nrT("min(a, 1)"),  1, true);
      iStat += EqnTest( _nrT("min(3*2, 1)"),  1, true);
      iStat += EqnTest( _nrT("min(3*2, 1)"),  6, false);
      iStat += EqnTest( _nrT("firstArg(2,3,4)"), 2, true);
      iStat += EqnTest( _nrT("lastArg(2,3,4)"), 4, true);
      iStat += EqnTest( _nrT("min(3*a+1, 1)"),  1, true);
      iStat += EqnTest( _nrT("max(3*a+1, 1)"),  4, true);
      iStat += EqnTest( _nrT("max(3*a+1, 1)*2"),  8, true);
      iStat += EqnTest( _nrT("2*max(3*a+1, 1)+2"),  10, true);

      // functions with Variable argument count
      iStat += EqnTest( _nrT("sum(a)"), 1, true);
      iStat += EqnTest( _nrT("sum(1,2,3)"),  6, true);
      iStat += EqnTest( _nrT("sum(a,b,c)"),  6, true);
      iStat += EqnTest( _nrT("sum(1,-max(1,2),3)*2"),  4, true);
      iStat += EqnTest( _nrT("2*sum(1,2,3)"),  12, true);
      iStat += EqnTest( _nrT("2*sum(1,2,3)+2"),  14, true);
      iStat += EqnTest( _nrT("2*sum(-1,2,3)+2"),  10, true);
      iStat += EqnTest( _nrT("2*sum(-1,2,-(-a))+2"),  6, true);
      iStat += EqnTest( _nrT("2*sum(-1,10,-a)+2"),  18, true);
      iStat += EqnTest( _nrT("2*sum(1,2,3)*2"),  24, true);
      iStat += EqnTest( _nrT("sum(1,-max(1,2),3)*2"),  4, true);
      iStat += EqnTest( _nrT("sum(1*3, 4, a+2)"),  10, true);
      iStat += EqnTest( _nrT("sum(1*3, 2*sum(1,2,2), a+2)"),  16, true);
      iStat += EqnTest( _nrT("sum(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2)"), 24, true);

      // some failures
      iStat += EqnTest( _nrT("sum()"),  0, false);
      iStat += EqnTest( _nrT("sum(,)"),  0, false);
      iStat += EqnTest( _nrT("sum(1,2,)"),  0, false);
      iStat += EqnTest( _nrT("sum(,1,2)"),  0, false);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Mehrfachausdruecke fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }


    //---------------------------------------------------------------------------
    int ParserTester::TestInfixOprt()
    {
      int iStat(0);
      cerr << "\r                                                                              \r";
      mu::console() << " -> Teste Infix Operatoren ... ";

      iStat += EqnTest( _nrT("-1"),    -1, true);
      iStat += EqnTest( _nrT("-(-1)"),  1, true);
      iStat += EqnTest( _nrT("-(-1)*2"),  2, true);
      iStat += EqnTest( _nrT("-(-2)*sqrt(4)"),  4, true);
      iStat += EqnTest( _nrT("-_pi"), -PARSER_CONST_PI, true);
      iStat += EqnTest( _nrT("-a"),  -1, true);
      iStat += EqnTest( _nrT("-(a)"),  -1, true);
      iStat += EqnTest( _nrT("-(-a)"),  1, true);
      iStat += EqnTest( _nrT("-(-a)*2"),  2, true);
      iStat += EqnTest( _nrT("-(8)"), -8, true);
      iStat += EqnTest( _nrT("-8"), -8, true);
      iStat += EqnTest( _nrT("-(2+1)"), -3, true);
      iStat += EqnTest( _nrT("-(f1of1(1+2*3)+1*2)"), -9, true);
      iStat += EqnTest( _nrT("-(-f1of1(1+2*3)+1*2)"), 5, true);
      iStat += EqnTest( _nrT("-sin(8)"), -0.989358, true);
      iStat += EqnTest( _nrT("3-(-a)"), 4, true);
      iStat += EqnTest( _nrT("3--a"), 4, true);
      iStat += EqnTest( _nrT("-1*3"),  -3, true);

      // Postfix / infix priorities
      iStat += EqnTest( _nrT("~2#"), 8, true);
      iStat += EqnTest( _nrT("~f1of1(2)#"), 8, true);
      iStat += EqnTest( _nrT("~(b)#"), 8, true);
      iStat += EqnTest( _nrT("(~b)#"), 12, true);
      iStat += EqnTest( _nrT("~(2#)"), 8, true);
      iStat += EqnTest( _nrT("~(f1of1(2)#)"), 8, true);
      //
      iStat += EqnTest( _nrT("-2^2"),-4, true);
      iStat += EqnTest( _nrT("-(a+b)^2"),-9, true);
      iStat += EqnTest( _nrT("(-3)^2"),9, true);
      iStat += EqnTest( _nrT("-(-2^2)"),4, true);
      iStat += EqnTest( _nrT("3+-3^2"),-6, true);
      // The following assumes use of sqr as postfix operator ("�") together
      // with a sign operator of low priority:
      iStat += EqnTest( _nrT("-2'"), -4, true);
      iStat += EqnTest( _nrT("-(1+1)'"),-4, true);
      iStat += EqnTest( _nrT("2+-(1+1)'"),-2, true);
      iStat += EqnTest( _nrT("2+-2'"), -2, true);
      // This is the classic behaviour of the infix sign operator (here: "$") which is
      // now deprecated:
      iStat += EqnTest( _nrT("$2^2"),4, true);
      iStat += EqnTest( _nrT("$(a+b)^2"),9, true);
      iStat += EqnTest( _nrT("($3)^2"),9, true);
      iStat += EqnTest( _nrT("$($2^2)"),-4, true);
      iStat += EqnTest( _nrT("3+$3^2"),12, true);

      // infix operators sharing the first few characters
      iStat += EqnTest( _nrT("~ 123"),  123+2, true);
      iStat += EqnTest( _nrT("~~ 123"),  123+2, true);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Infix-Operatoren fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }


    //---------------------------------------------------------------------------
    int ParserTester::TestPostFix()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Postfix Operatoren ... ");

      // application
      iStat += EqnTest( _nrT("3m+5"), 5.003, true);
      iStat += EqnTest( _nrT("1000m"), 1, true);
      iStat += EqnTest( _nrT("1000 m"), 1, true);
      iStat += EqnTest( _nrT("(a)m"), 1e-3, true);
      iStat += EqnTest( _nrT("a m"), 1e-3, true);
      //iStat += EqnTest( _nrT("a m"), 1e-3, true);
      iStat += EqnTest( _nrT("-(a)m"), -1e-3, true);
      iStat += EqnTest( _nrT("-2m"), -2e-3, true);
      iStat += EqnTest( _nrT("-2 m"), -2e-3, true);
      iStat += EqnTest( _nrT("f1of1(1000)m"), 1, true);
      iStat += EqnTest( _nrT("-f1of1(1000)m"), -1, true);
      iStat += EqnTest( _nrT("-f1of1(-1000)m"), 1, true);
      iStat += EqnTest( _nrT("f4of4(0,0,0,1000)m"), 1, true);
      iStat += EqnTest( _nrT("2+(a*1000)m"), 3, true);

      // can postfix operators "m" und "meg" be told apart properly?
      iStat += EqnTest( _nrT("2*3000meg+2"), 2*3e9+2, true);

      // some incorrect results
      iStat += EqnTest( _nrT("1000m"), 0.1, false);
      iStat += EqnTest( _nrT("(a)m"), 2, false);
      // failure due to syntax checking
      iStat += ThrowTest(_nrT("0x"), ecUNASSIGNABLE_TOKEN);  // incomplete hex definition
      iStat += ThrowTest(_nrT("3+"), ecUNEXPECTED_EOF);
      iStat += ThrowTest( _nrT("4 + m"), ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest( _nrT("m4"), ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest( _nrT("sin(m)"), ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest( _nrT("m m"), ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest( _nrT("m(8)"), ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest( _nrT("4,m"), ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest( _nrT("-m"), ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest( _nrT("2(-m)"), ecUNEXPECTED_PARENS);
      iStat += ThrowTest( _nrT("2(m)"), ecUNEXPECTED_PARENS);

      iStat += ThrowTest( _nrT("multi*1.0"), ecUNASSIGNABLE_TOKEN);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Postfix-Operatoren fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------
    int ParserTester::TestExpression()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Beispielausdruecke ... ");

      value_type b = 2;

      // Optimization
      iStat += EqnTest( _nrT("2*b*5"), 20, true);
      iStat += EqnTest( _nrT("2*b*5 + 4*b"), 28, true);
      iStat += EqnTest( _nrT("2*a/3"), 2.0/3.0, true);

      // Addition auf cmVARMUL
      iStat += EqnTest( _nrT("3+b"), b+3, true);
      iStat += EqnTest( _nrT("b+3"), b+3, true);
      iStat += EqnTest( _nrT("b*3+2"), b*3+2, true);
      iStat += EqnTest( _nrT("3*b+2"), b*3+2, true);
      iStat += EqnTest( _nrT("2+b*3"), b*3+2, true);
      iStat += EqnTest( _nrT("2+3*b"), b*3+2, true);
      iStat += EqnTest( _nrT("b+3*b"), b+3*b, true);
      iStat += EqnTest( _nrT("3*b+b"), b+3*b, true);

      iStat += EqnTest( _nrT("2+b*3+b"), 2+b*3+b, true);
      iStat += EqnTest( _nrT("b+2+b*3"), b+2+b*3, true);

      iStat += EqnTest( _nrT("(2*b+1)*4"), (2*b+1)*4, true);
      iStat += EqnTest( _nrT("4*(2*b+1)"), (2*b+1)*4, true);

      // operator precedencs
      iStat += EqnTest( _nrT("1+2-3*4/5^6"), 2.99923, true);
      iStat += EqnTest( _nrT("1^2/3*4-5+6"), 2.33333333, true);
      iStat += EqnTest( _nrT("1+2*3"), 7, true);
      iStat += EqnTest( _nrT("1+2*3"), 7, true);
      iStat += EqnTest( _nrT("(1+2)*3"), 9, true);
      iStat += EqnTest( _nrT("(1+2)*(-3)"), -9, true);
      iStat += EqnTest( _nrT("2/4"), 0.5, true);

      iStat += EqnTest( _nrT("exp(ln(7))"), 7, true);
      iStat += EqnTest( _nrT("e^ln(7)"), 7, true);
      iStat += EqnTest( _nrT("e^(ln(7))"), 7, true);
      iStat += EqnTest( _nrT("(e^(ln(7)))"), 7, true);
      iStat += EqnTest( _nrT("1-(e^(ln(7)))"), -6, true);
      iStat += EqnTest( _nrT("2*(e^(ln(7)))"), 14, true);
      iStat += EqnTest( _nrT("10^log(5)"), 5, true);
      iStat += EqnTest( _nrT("10^log10(5)"), 5, true);
      iStat += EqnTest( _nrT("2^log2(4)"), 4, true);
      iStat += EqnTest( _nrT("-(sin(0)+1)"), -1, true);
      iStat += EqnTest( _nrT("-(2^1.1)"), -2.14354692, true);

      iStat += EqnTest( _nrT("(cos(2.41)/b)"), -0.372056, true);
      iStat += EqnTest( _nrT("(1*(2*(3*(4*(5*(6*(a+b)))))))"), 2160, true);
      iStat += EqnTest( _nrT("(1*(2*(3*(4*(5*(6*(7*(a+b))))))))"), 15120, true);
      iStat += EqnTest( _nrT("(a/((((b+(((e*(((((pi*((((3.45*((pi+a)+pi))+b)+b)*a))+0.68)+e)+a)/a))+a)+b))+b)*a)-pi))"), 0.00377999, true);

      // long formula (Reference: Matlab)
      iStat += EqnTest(
        _nrT("(((-9))-e/(((((((pi-(((-7)+(-3)/4/e))))/(((-5))-2)-((pi+(-0))*(sqrt((e+e))*(-8))*(((-pi)+(-pi)-(-9)*(6*5))")
        _nrT("/(-e)-e))/2)/((((sqrt(2/(-e)+6)-(4-2))+((5/(-2))/(1*(-pi)+3))/8)*pi*((pi/((-2)/(-6)*1*(-1))*(-6)+(-e)))))/")
        _nrT("((e+(-2)+(-e)*((((-3)*9+(-e)))+(-9)))))))-((((e-7+(((5/pi-(3/1+pi)))))/e)/(-5))/(sqrt((((((1+(-7))))+((((-")
        _nrT("e)*(-e)))-8))*(-5)/((-e)))*(-6)-((((((-2)-(-9)-(-e)-1)/3))))/(sqrt((8+(e-((-6))+(9*(-9))))*(((3+2-8))*(7+6")
        _nrT("+(-5))+((0/(-e)*(-pi))+7)))+(((((-e)/e/e)+((-6)*5)*e+(3+(-5)/pi))))+pi))/sqrt((((9))+((((pi))-8+2))+pi))/e")
        _nrT("*4)*((-5)/(((-pi))*(sqrt(e)))))-(((((((-e)*(e)-pi))/4+(pi)*(-9)))))))+(-pi)"), -12.23016549, true);

      // long formula (Reference: Matlab)
      iStat += EqnTest(
          _nrT("(atan(sin((((((((((((((((pi/cos((a/((((0.53-b)-pi)*e)/b))))+2.51)+a)-0.54)/0.98)+b)*b)+e)/a)+b)+a)+b)+pi)/e")
          _nrT(")+a)))*2.77)"), -2.16995656, true);

      // long formula (Reference: Matlab)
      iStat += EqnTest( _nrT("1+2-3*4/5^6*(2*(1-5+(3*7^9)*(4+6*7-3)))+12"), -7995810.09926, true);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n Beispielausdruecke fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }



    //---------------------------------------------------------------------------
    int ParserTester::TestIfThenElse()
    {
      int iStat = 0;
      cerr << "\r                                                                              \r";
      mu::console() << _nrT(" -> Teste Wenn-Dann-Sonst Operator ... ");

      // Test error detection
      iStat += ThrowTest(_nrT(":3"), ecUNEXPECTED_CONDITIONAL);
      iStat += ThrowTest(_nrT("? 1 : 2"), ecUNEXPECTED_CONDITIONAL);
      iStat += ThrowTest(_nrT("(a<b) ? (b<c) ? 1 : 2"), ecMISSING_ELSE_CLAUSE);
      iStat += ThrowTest(_nrT("(a<b) ? 1"), ecMISSING_ELSE_CLAUSE);
      iStat += ThrowTest(_nrT("(a<b) ? a"), ecMISSING_ELSE_CLAUSE);
      iStat += ThrowTest(_nrT("(a<b) ? a+b"), ecMISSING_ELSE_CLAUSE);
      iStat += ThrowTest(_nrT("a : b"), ecMISPLACED_COLON);
      iStat += ThrowTest(_nrT("1 : 2"), ecMISPLACED_COLON);
      iStat += ThrowTest(_nrT("(1) ? 1 : 2 : 3"), ecMISPLACED_COLON);
      iStat += ThrowTest(_nrT("(true) ? 1 : 2 : 3"), ecUNASSIGNABLE_TOKEN);

      iStat += EqnTest(_nrT("1 ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("1<2 ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("a<b ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("(a<b) ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("(1) ? 10 : 11"), 10, true);
      iStat += EqnTest(_nrT("(0) ? 10 : 11"), 11, true);
      iStat += EqnTest(_nrT("(1) ? a+b : c+d"), 3, true);
      iStat += EqnTest(_nrT("(0) ? a+b : c+d"), 1, true);
      iStat += EqnTest(_nrT("(1) ? 0 : 1"), 0, true);
      iStat += EqnTest(_nrT("(0) ? 0 : 1"), 1, true);
      iStat += EqnTest(_nrT("(a<b) ? 10 : 11"), 10, true);
      iStat += EqnTest(_nrT("(a>b) ? 10 : 11"), 11, true);
      iStat += EqnTest(_nrT("(a<b) ? c : d"), 3, true);
      iStat += EqnTest(_nrT("(a>b) ? c : d"), -2, true);

      iStat += EqnTest(_nrT("(a>b) ? 1 : 0"), 0, true);
      iStat += EqnTest(_nrT("((a>b) ? 1 : 0) ? 1 : 2"), 2, true);
      iStat += EqnTest(_nrT("((a>b) ? 1 : 0) ? 1 : sum((a>b) ? 1 : 2)"), 2, true);
      iStat += EqnTest(_nrT("((a>b) ? 0 : 1) ? 1 : sum((a>b) ? 1 : 2)"), 1, true);

      iStat += EqnTest(_nrT("sum((a>b) ? 1 : 2)"), 2, true);
      iStat += EqnTest(_nrT("sum((1) ? 1 : 2)"), 1, true);
      iStat += EqnTest(_nrT("sum((a>b) ? 1 : 2, 100)"), 102, true);
      iStat += EqnTest(_nrT("sum((1) ? 1 : 2, 100)"), 101, true);
      iStat += EqnTest(_nrT("sum(3, (a>b) ? 3 : 10)"), 13, true);
      iStat += EqnTest(_nrT("sum(3, (a<b) ? 3 : 10)"), 6, true);
      iStat += EqnTest(_nrT("10*sum(3, (a>b) ? 3 : 10)"), 130, true);
      iStat += EqnTest(_nrT("10*sum(3, (a<b) ? 3 : 10)"), 60, true);
      iStat += EqnTest(_nrT("sum(3, (a>b) ? 3 : 10)*10"), 130, true);
      iStat += EqnTest(_nrT("sum(3, (a<b) ? 3 : 10)*10"), 60, true);
      iStat += EqnTest(_nrT("(a<b) ? sum(3, (a<b) ? 3 : 10)*10 : 99"), 60, true);
      iStat += EqnTest(_nrT("(a>b) ? sum(3, (a<b) ? 3 : 10)*10 : 99"), 99, true);
      iStat += EqnTest(_nrT("(a<b) ? sum(3, (a<b) ? 3 : 10,10,20)*10 : 99"), 360, true);
      iStat += EqnTest(_nrT("(a>b) ? sum(3, (a<b) ? 3 : 10,10,20)*10 : 99"), 99, true);
      iStat += EqnTest(_nrT("(a>b) ? sum(3, (a<b) ? 3 : 10,10,20)*10 : sum(3, (a<b) ? 3 : 10)*10"), 60, true);

      // todo: auch f�r muParserX hinzuf�gen!
      iStat += EqnTest(_nrT("(a<b)&&(a<b) ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("(a>b)&&(a<b) ? 128 : 255"), 255, true);
      iStat += EqnTest(_nrT("(1<2)&&(1<2) ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("(1>2)&&(1<2) ? 128 : 255"), 255, true);
      iStat += EqnTest(_nrT("((1<2)&&(1<2)) ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("((1>2)&&(1<2)) ? 128 : 255"), 255, true);
      iStat += EqnTest(_nrT("((a<b)&&(a<b)) ? 128 : 255"), 128, true);
      iStat += EqnTest(_nrT("((a>b)&&(a<b)) ? 128 : 255"), 255, true);

      iStat += EqnTest(_nrT("1>0 ? 1>2 ? 128 : 255 : 1>0 ? 32 : 64"), 255, true);
      iStat += EqnTest(_nrT("1>0 ? 1>2 ? 128 : 255 :(1>0 ? 32 : 64)"), 255, true);
      iStat += EqnTest(_nrT("1>0 ? 1>0 ? 128 : 255 : 1>2 ? 32 : 64"), 128, true);
      iStat += EqnTest(_nrT("1>0 ? 1>0 ? 128 : 255 :(1>2 ? 32 : 64)"), 128, true);
      iStat += EqnTest(_nrT("1>2 ? 1>2 ? 128 : 255 : 1>0 ? 32 : 64"), 32, true);
      iStat += EqnTest(_nrT("1>2 ? 1>0 ? 128 : 255 : 1>2 ? 32 : 64"), 64, true);
      iStat += EqnTest(_nrT("1>0 ? 50 :  1>0 ? 128 : 255"), 50, true);
      iStat += EqnTest(_nrT("1>0 ? 50 : (1>0 ? 128 : 255)"), 50, true);
      iStat += EqnTest(_nrT("1>0 ? 1>0 ? 128 : 255 : 50"), 128, true);
      iStat += EqnTest(_nrT("1>2 ? 1>2 ? 128 : 255 : 1>0 ? 32 : 1>2 ? 64 : 16"), 32, true);
      iStat += EqnTest(_nrT("1>2 ? 1>2 ? 128 : 255 : 1>0 ? 32 :(1>2 ? 64 : 16)"), 32, true);
      iStat += EqnTest(_nrT("1>0 ? 1>2 ? 128 : 255 :  1>0 ? 32 :1>2 ? 64 : 16"), 255, true);
      iStat += EqnTest(_nrT("1>0 ? 1>2 ? 128 : 255 : (1>0 ? 32 :1>2 ? 64 : 16)"), 255, true);
      iStat += EqnTest(_nrT("1 ? 0 ? 128 : 255 : 1 ? 32 : 64"), 255, true);

      // assignment operators
      iStat += EqnTest(_nrT("a= 0 ? 128 : 255, a"), 255, true);
      iStat += EqnTest(_nrT("a=((a>b)&&(a<b)) ? 128 : 255, a"), 255, true);
      iStat += EqnTest(_nrT("c=(a<b)&&(a<b) ? 128 : 255, c"), 128, true);
      iStat += EqnTest(_nrT("0 ? a=a+1 : 666, a"), 1, true);
      iStat += EqnTest(_nrT("1?a=10:a=20, a"), 10, true);
      iStat += EqnTest(_nrT("0?a=10:a=20, a"), 20, true);
      iStat += EqnTest(_nrT("0?a=sum(3,4):10, a"), 1, true);  // a should not change its value due to lazy calculation

      iStat += EqnTest(_nrT("a=1?b=1?3:4:5, a"), 3, true);
      iStat += EqnTest(_nrT("a=1?b=1?3:4:5, b"), 3, true);
      iStat += EqnTest(_nrT("a=0?b=1?3:4:5, a"), 5, true);
      iStat += EqnTest(_nrT("a=0?b=1?3:4:5, b"), 2, true);

      iStat += EqnTest(_nrT("a=1?5:b=1?3:4, a"), 5, true);
      iStat += EqnTest(_nrT("a=1?5:b=1?3:4, b"), 2, true);
      iStat += EqnTest(_nrT("a=0?5:b=1?3:4, a"), 3, true);
      iStat += EqnTest(_nrT("a=0?5:b=1?3:4, b"), 3, true);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Wenn-Dann-Sonst fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }

    //---------------------------------------------------------------------------
    int ParserTester::TestException()
    {
      int  iStat = 0;
        cerr << "\r                                                                              \r";
        mu::console() << _nrT(" -> Teste Fehlercodes ... ");

      iStat += ThrowTest(_nrT("3+"),           ecUNEXPECTED_EOF);
      iStat += ThrowTest(_nrT("3+)"),          ecUNEXPECTED_PARENS);
      iStat += ThrowTest(_nrT("()"),           ecUNEXPECTED_PARENS);
      iStat += ThrowTest(_nrT("3+()"),         ecUNEXPECTED_PARENS);
      iStat += ThrowTest(_nrT("sin(3,4)"),     ecTOO_MANY_PARAMS);
      iStat += ThrowTest(_nrT("sin()"),        ecTOO_FEW_PARAMS);
      iStat += ThrowTest(_nrT("(1+2"),         ecMISSING_PARENS);
      iStat += ThrowTest(_nrT("sin(3)3"),      ecUNEXPECTED_VAL);
      iStat += ThrowTest(_nrT("sin(3)xyz"),    ecUNASSIGNABLE_TOKEN);
      iStat += ThrowTest(_nrT("sin(3)cos(3)"), ecUNEXPECTED_FUN);
      iStat += ThrowTest(_nrT("a+b+c=10"),     ecUNEXPECTED_OPERATOR);
      iStat += ThrowTest(_nrT("a=b=3"),        ecUNEXPECTED_OPERATOR);

      // functions without parameter
      iStat += ThrowTest( _nrT("3+ping(2)"),    ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("3+ping(a+2)"),  ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("3+ping(sin(a)+2)"),  ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("3+ping(1+sin(a))"),  ecTOO_MANY_PARAMS);

      // String function related
      iStat += ThrowTest( _nrT("valueof(\"xxx\")"),  999, false);
      iStat += ThrowTest( _nrT("valueof()"),          ecUNEXPECTED_PARENS);
      iStat += ThrowTest( _nrT("1+valueof(\"abc\""),  ecMISSING_PARENS);
      iStat += ThrowTest( _nrT("valueof(\"abc\""),    ecMISSING_PARENS);
      iStat += ThrowTest( _nrT("valueof(\"abc"),      ecUNTERMINATED_STRING);
      iStat += ThrowTest( _nrT("valueof(\"abc\",3)"), ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("valueof(3)"),         ecSTRING_EXPECTED);
      iStat += ThrowTest( _nrT("sin(\"abc\")"),       ecVAL_EXPECTED);
      iStat += ThrowTest( _nrT("valueof(\"\\\"abc\\\"\")"),  999, false);
      iStat += ThrowTest( _nrT("\"hello world\""),    ecSTR_RESULT);
      iStat += ThrowTest( _nrT("(\"hello world\")"),  ecSTR_RESULT);
      iStat += ThrowTest( _nrT("\"abcd\"+100"),       ecOPRT_TYPE_CONFLICT);
      iStat += ThrowTest( _nrT("\"a\"+\"b\""),        ecOPRT_TYPE_CONFLICT);
      iStat += ThrowTest( _nrT("strfun1(\"100\",3)"),     ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("strfun2(\"100\",3,5)"),   ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("strfun3(\"100\",3,5,6)"), ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("strfun2(\"100\")"),       ecTOO_FEW_PARAMS);
      iStat += ThrowTest( _nrT("strfun3(\"100\",6)"),   ecTOO_FEW_PARAMS);
      iStat += ThrowTest( _nrT("strfun2(1,1)"),         ecSTRING_EXPECTED);
      iStat += ThrowTest( _nrT("strfun2(a,1)"),         ecSTRING_EXPECTED);
      iStat += ThrowTest( _nrT("strfun2(1,1,1)"),       ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("strfun2(a,1,1)"),       ecTOO_MANY_PARAMS);
      iStat += ThrowTest( _nrT("strfun3(1,2,3)"),         ecSTRING_EXPECTED);
      iStat += ThrowTest( _nrT("strfun3(1, \"100\",3)"),  ecSTRING_EXPECTED);
      iStat += ThrowTest( _nrT("strfun3(\"1\", \"100\",3)"),  ecVAL_EXPECTED);
      iStat += ThrowTest( _nrT("strfun3(\"1\", 3, \"100\")"),  ecVAL_EXPECTED);
      iStat += ThrowTest( _nrT("strfun3(\"1\", \"100\", \"100\", \"100\")"),  ecTOO_MANY_PARAMS);

      // assignement operator
      iStat += ThrowTest( _nrT("3=4"), ecUNEXPECTED_OPERATOR);
      iStat += ThrowTest( _nrT("sin(8)=4"), ecUNEXPECTED_OPERATOR);
      iStat += ThrowTest( _nrT("\"test\"=a"), ecUNEXPECTED_OPERATOR);

      // <ibg 20090529>
      // this is now legal, for reference see:
      // https://sourceforge.net/forum/message.php?msg_id=7411373
      //      iStat += ThrowTest( _nrT("sin=9"), ecUNEXPECTED_OPERATOR);
      // </ibg>

      iStat += ThrowTest( _nrT("(8)=5"), ecUNEXPECTED_OPERATOR);
      iStat += ThrowTest( _nrT("(a)=5"), ecUNEXPECTED_OPERATOR);
      iStat += ThrowTest( _nrT("a=\"tttt\""), ecOPRT_TYPE_CONFLICT);

      if (iStat==0)
        mu::console() << _nrT("Abgeschlossen.");
      else
        mu::console() << _nrT("\n -> Fehlercodes fehlgeschlagen mit ") << iStat << _nrT(" Fehlern.") << endl;

      return iStat;
    }


    //---------------------------------------------------------------------------
    void ParserTester::AddTest(testfun_type a_pFun)
    {
      m_vTestFun.push_back(a_pFun);
    }

    //---------------------------------------------------------------------------
    void ParserTester::Run()
    {
      int iStat = 0;
      try
      {
        for (int i=0; i<(int)m_vTestFun.size(); ++i)
          iStat += (this->*m_vTestFun[i])();
      }
      catch(Parser::exception_type &e)
      {
        mu::console() << "\n" << e.GetMsg() << endl;
        mu::console() << e.GetToken() << endl;
        Abort();
      }
      catch(std::exception &e)
      {
        mu::console() << e.what() << endl;
        Abort();
      }
      catch(...)
      {
        mu::console() << "Internal error";
        Abort();
      }

      if (iStat==0)
      {
        cerr << "\r                                                                              \r";
        mu::console() << " -> Erfolgreich abgeschlossen (" <<  ParserTester::c_iCount << " Ausdruecke)";
      }
      else
      {
        mu::console() << " -> FEHLSCHLAG: " << iStat
                  << " Fehler (" <<  ParserTester::c_iCount
                  << " Ausdruecke)" << endl;
      }
      ParserTester::c_iCount = 0;
    }


    //---------------------------------------------------------------------------
    int ParserTester::ThrowTest(const string_type &a_str, int a_iErrc, bool a_bFail)
    {
      ParserTester::c_iCount++;

      try
      {
        value_type fVal[] = {1,1,1};
        Parser p;

        p.DefineVar( _nrT("a"), &fVal[0]);
        p.DefineVar( _nrT("b"), &fVal[1]);
        p.DefineVar( _nrT("c"), &fVal[2]);
        //p.DefinePostfixOprt( _nrT("{m}"), Milli);
        p.DefinePostfixOprt( _nrT("m"), Milli);
        p.DefineFun( _nrT("ping"), Ping);
        p.DefineFun( _nrT("valueof"), ValueOf);
        p.DefineFun( _nrT("strfun1"), StrFun1);
        p.DefineFun( _nrT("strfun2"), StrFun2);
        p.DefineFun( _nrT("strfun3"), StrFun3);
        p.SetExpr(a_str);
        p.Eval();
      }
      catch(ParserError &e)
      {
        // output the formula in case of an failed test
        if (a_bFail==false || (a_bFail==true && a_iErrc!=e.GetCode()) )
        {
          mu::console() << _nrT("\n  ")
                        << _nrT("Expression: ") << a_str
                        << _nrT("  Code:") << e.GetCode() << _nrT("(") << e.GetMsg() << _nrT(")")
                        << _nrT("  Expected:") << a_iErrc;
        }

        return (a_iErrc==e.GetCode()) ? 0 : 1;
      }

      // if a_bFail==false no exception is expected
      bool bRet((a_bFail==false) ? 0 : 1);
      if (bRet==1)
      {
        mu::console() << _nrT("\n  ")
                      << _nrT("Expression: ") << a_str
                      << _nrT("  did evaluate; Expected error:") << a_iErrc;
      }

      return bRet;
    }

    //---------------------------------------------------------------------------
    /** \brief Evaluate a tet expression.

        \return 1 in case of a failure, 0 otherwise.
    */
    int ParserTester::EqnTestWithVarChange(const string_type &a_str,
                                           double a_fVar1,
                                           double a_fRes1,
                                           double a_fVar2,
                                           double a_fRes2)
    {
      ParserTester::c_iCount++;
      value_type fVal[2] = {-999, -999 }; // should be equalinitially

      try
      {
        Parser  p;

        // variable
        value_type var = 0;
        p.DefineVar( _nrT("a"), &var);
        p.SetExpr(a_str);

        var = a_fVar1;
        fVal[0] = p.Eval();

        var = a_fVar2;
        fVal[1] = p.Eval();

        if ( fabs(a_fRes1-fVal[0]) > 0.0000000001)
          throw std::runtime_error("incorrect result (first pass)");

        if ( fabs(a_fRes2-fVal[1]) > 0.0000000001)
          throw std::runtime_error("incorrect result (second pass)");
      }
      catch(Parser::exception_type &e)
      {
        mu::console() << _nrT("\n  fail: ") << a_str.c_str() << _nrT(" (") << e.GetMsg() << _nrT(")");
        return 1;
      }
      catch(std::exception &e)
      {
        mu::console() << _nrT("\n  fail: ") << a_str.c_str() << _nrT(" (") << e.what() << _nrT(")");
        return 1;  // always return a failure since this exception is not expected
      }
      catch(...)
      {
        mu::console() << _nrT("\n  fail: ") << a_str.c_str() <<  _nrT(" (unexpected exception)");
        return 1;  // exceptions other than ParserException are not allowed
      }

      return 0;
    }

    //---------------------------------------------------------------------------
    /** \brief Evaluate a tet expression.

        \return 1 in case of a failure, 0 otherwise.
    */
    int ParserTester::EqnTest(const string_type &a_str, double a_fRes, bool a_fPass)
    {
      ParserTester::c_iCount++;
      int iRet(0);
      value_type fVal[5] = {-999, -998, -997, -996, -995}; // initially should be different

      try
      {
        std::unique_ptr<Parser> p1;
        Parser  p2, p3;   // three parser objects
                          // they will be used for testing copy and assihnment operators
        // p1 is a pointer since i'm going to delete it in order to test if
        // parsers after copy construction still refer to members of it.
        // !! If this is the case this function will crash !!
        //cerr << 0 << endl;
        p1.reset(new mu::Parser());
        //cerr << 1 << endl;
        // Add constants
        p1->DefineConst( _nrT("pi"), (value_type)PARSER_CONST_PI);
        p1->DefineConst( _nrT("e"), (value_type)PARSER_CONST_E);
        p1->DefineConst( _nrT("const"), 1);
        p1->DefineConst( _nrT("const1"), 2);
        p1->DefineConst( _nrT("const2"), 3);
        // variables
        value_type vVarVal[] = { 1, 2, 3, -2};
        p1->DefineVar( _nrT("a"), &vVarVal[0]);
        p1->DefineVar( _nrT("aa"), &vVarVal[1]);
        p1->DefineVar( _nrT("b"), &vVarVal[1]);
        p1->DefineVar( _nrT("c"), &vVarVal[2]);
        p1->DefineVar( _nrT("d"), &vVarVal[3]);

        // custom value ident functions
        p1->AddValIdent(&ParserTester::IsHexVal);
        //cerr << 2 << endl;
        // functions
        p1->DefineFun( _nrT("ping"), Ping);
        p1->DefineFun( _nrT("f1of1"), f1of1);  // one parameter
        p1->DefineFun( _nrT("f1of2"), f1of2);  // two parameter
        p1->DefineFun( _nrT("f2of2"), f2of2);
        p1->DefineFun( _nrT("f1of3"), f1of3);  // three parameter
        p1->DefineFun( _nrT("f2of3"), f2of3);
        p1->DefineFun( _nrT("f3of3"), f3of3);
        p1->DefineFun( _nrT("f1of4"), f1of4);  // four parameter
        p1->DefineFun( _nrT("f2of4"), f2of4);
        p1->DefineFun( _nrT("f3of4"), f3of4);
        p1->DefineFun( _nrT("f4of4"), f4of4);
        p1->DefineFun( _nrT("f1of5"), f1of5);  // five parameter
        p1->DefineFun( _nrT("f2of5"), f2of5);
        p1->DefineFun( _nrT("f3of5"), f3of5);
        p1->DefineFun( _nrT("f4of5"), f4of5);
        p1->DefineFun( _nrT("f5of5"), f5of5);

        // binary operators
        p1->DefineOprt( _nrT("add"), add, 0);
        p1->DefineOprt( _nrT("++"), add, 0);
        p1->DefineOprt( _nrT("&"), land, prLAND);

        // sample functions
        p1->DefineFun( _nrT("min"), Min);
        p1->DefineFun( _nrT("max"), Max);
        p1->DefineFun( _nrT("sum"), Sum);
        p1->DefineFun( _nrT("valueof"), ValueOf);
        p1->DefineFun( _nrT("atof"), StrToFloat);
        p1->DefineFun( _nrT("strfun1"), StrFun1);
        p1->DefineFun( _nrT("strfun2"), StrFun2);
        p1->DefineFun( _nrT("strfun3"), StrFun3);
        p1->DefineFun( _nrT("lastArg"), LastArg);
        p1->DefineFun( _nrT("firstArg"), FirstArg);
        p1->DefineFun( _nrT("order"), FirstArg);
        //cerr << 3 << endl;
        // infix / postfix operator
        // Note: Identifiers used here do not have any meaning
        //       they are mere placeholders to test certain features.
        p1->DefineInfixOprt( _nrT("$"), sign, prPOW+1);  // sign with high priority
        p1->DefineInfixOprt( _nrT("~"), plus2);          // high priority
        p1->DefineInfixOprt( _nrT("~~"), plus2);
        //p1->DefinePostfixOprt( _nrT("{m}"), Milli);
        //p1->DefinePostfixOprt( _nrT("{M}"), Mega);
        p1->DefinePostfixOprt( _nrT("m"), Milli);
        p1->DefinePostfixOprt( _nrT("meg"), Mega);
        p1->DefinePostfixOprt( _nrT("#"), times3);
        p1->DefinePostfixOprt( _nrT("'"), sqr);
        //cerr << 4 << endl;
        p1->SetExpr(a_str);
        // Test bytecode integrity
        // String parsing and bytecode parsing must yield the same result
        fVal[0] = p1->Eval(); // result from stringparsing
        fVal[1] = p1->Eval(); // result from bytecode
        if (fVal[0]!=fVal[1])
          throw Parser::exception_type( _nrT("Bytecode / string parsing mismatch.") );

        //std::cerr << 1 << endl;
        // Test copy and assignement operators
        try
        {
          // Test copy constructor
          std::vector<mu::Parser> vParser;
          vParser.push_back(*(p1.get()));
          mu::Parser p2 = vParser[0];   // take parser from vector

          // destroy the originals from p2
          vParser.clear();              // delete the vector
          p1.reset(0);

          fVal[2] = p2.Eval();

          // Test assignement operator
          // additionally  disable Optimizer this time
          mu::Parser p3;
          p3 = p2;
          p3.EnableOptimizer(false);
          fVal[3] = p3.Eval();

          // Test Eval function for multiple return values
          // use p2 since it has the optimizer enabled!
          int nNum;
          value_type *v = p2.Eval(nNum);
          fVal[4] = v[nNum-1];
          //std::cerr << 2 << endl;
        }
        catch(std::exception &e)
        {
          mu::console() << _nrT("\n  ") << e.what() << _nrT("\n");
        }

        // limited floating point accuracy requires the following test
        bool bCloseEnough(true);
        for (unsigned i=0; i<sizeof(fVal)/sizeof(value_type); ++i)
        {
          bCloseEnough &= (fabs(a_fRes-fVal[i]) <= fabs(fVal[i]*0.00001));

          // The tests equations never result in infinity, if they do thats a bug.
          // reference:
          // http://sourceforge.net/projects/muparser/forums/forum/462843/topic/5037825
          if (numeric_limits<value_type>::has_infinity)
            bCloseEnough &= (fabs(fVal[i]) != numeric_limits<value_type>::infinity());
        }
        //std::cerr << 3 << endl;
        iRet = ((bCloseEnough && a_fPass) || (!bCloseEnough && !a_fPass)) ? 0 : 1;


        if (iRet==1)
        {
          mu::console() << _nrT("\n  fail: ") << a_str.c_str()
                        << _nrT(" (incorrect result; expected: ") << a_fRes
                        << _nrT(" ;calculated: ") << fVal[0] << _nrT(",")
                                                << fVal[1] << _nrT(",")
                                                << fVal[2] << _nrT(",")
                                                << fVal[3] << _nrT(",")
                                                << fVal[4] << _nrT(").");
        }
      }
      catch(Parser::exception_type &e)
      {
        if (a_fPass)
        {
          if (fVal[0]!=fVal[2] && fVal[0]!=-999 && fVal[1]!=-998)
            mu::console() << _nrT("\n  fail: ") << a_str.c_str() << _nrT(" (copy construction)");
          else
            mu::console() << _nrT("\n  fail: ") << a_str.c_str() << _nrT(" (") << e.GetMsg() << _nrT(")");
          return 1;
        }
      }
      catch(std::exception &e)
      {
        mu::console() << _nrT("\n  fail: ") << a_str.c_str() << _nrT(" (") << e.what() << _nrT(")");
        return 1;  // always return a failure since this exception is not expected
      }
      catch(...)
      {
        mu::console() << _nrT("\n  fail: ") << a_str.c_str() <<  _nrT(" (unexpected exception)");
        return 1;  // exceptions other than ParserException are not allowed
      }

      return iRet;
    }

    //---------------------------------------------------------------------------
    int ParserTester::EqnTestInt(const string_type &a_str, double a_fRes, bool a_fPass)
    {
      ParserTester::c_iCount++;

      value_type vVarVal[] = {1, 2, 3};    // variable values
      value_type fVal[2] = {-99, -999}; // results: initially should be different
      int iRet(0);

      try
      {
        ParserInt p;
        p.DefineConst( _nrT("const1"), 1);
        p.DefineConst( _nrT("const2"), 2);
        p.DefineVar( _nrT("a"), &vVarVal[0]);
        p.DefineVar( _nrT("b"), &vVarVal[1]);
        p.DefineVar( _nrT("c"), &vVarVal[2]);

        p.SetExpr(a_str);
        fVal[0] = p.Eval(); // result from stringparsing
        fVal[1] = p.Eval(); // result from bytecode

        if (fVal[0]!=fVal[1])
          throw Parser::exception_type( _nrT("Bytecode corrupt.") );

        iRet =  ( (a_fRes==fVal[0] &&  a_fPass) ||
                  (a_fRes!=fVal[0] && !a_fPass) ) ? 0 : 1;
        if (iRet==1)
        {
          mu::console() << _nrT("\n  fail: ") << a_str.c_str()
                        << _nrT(" (incorrect result; expected: ") << a_fRes
                        << _nrT(" ;calculated: ") << fVal[0]<< _nrT(").");
        }
      }
      catch(Parser::exception_type &e)
      {
        if (a_fPass)
        {
          mu::console() << _nrT("\n  fail: ") << e.GetExpr() << _nrT(" : ") << e.GetMsg();
          iRet = 1;
        }
      }
      catch(...)
      {
        mu::console() << _nrT("\n  fail: ") << a_str.c_str() <<  _nrT(" (unexpected exception)");
        iRet = 1;  // exceptions other than ParserException are not allowed
      }

      return iRet;
    }

    //---------------------------------------------------------------------------
    /** \brief Internal error in test class Test is going to be aborted. */
    void ParserTester::Abort() const
    {
      mu::console() << _nrT("Test failed (internal error in test class)") << endl;
      while (!getchar());
      exit(-1);
    }
  } // namespace test
} // namespace mu
