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
#ifndef MUP_DEF_H
#define MUP_DEF_H

#include <map>
#include <complex>
#include <vector>

#include "muStringTypeDefs.hpp"
#include "muParserFixes.h"
#include "muStructures.hpp"

/** \file
    \brief This file contains standard definitions used by the parser.
*/

#define MUP_VERSION _nrT("2.2.2")
#define MUP_VERSION_DATE _nrT("20120218; SF")

#define MUP_CHARS _nrT("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")

#define MU_VECTOR_CREATE "_~vect~create"
#define MU_VECTOR_EXP2 "_~vect~exp2"
#define MU_VECTOR_EXP3 "_~vect~exp3"
#define MU_IF_ELSE "_~ifelse"

/** \brief If this macro is defined mathematical exceptions (div by zero) will be thrown as exceptions. */
//#define MUP_MATH_EXCEPTIONS

/** \brief Define the base datatype for values.

  This datatype must be a built in value type. You can not use custom classes.
  It should be working with all types except "int"!
*/
#define MUP_BASETYPE mu::Array

/** \brief Activate this option in order to compile with OpenMP support.

  OpenMP is used only in the bulk mode it may increase the performance a bit.
*/
//#define MUP_USE_OPENMP

#define _nrT(x) x
#if defined(_UNICODE)
/** \brief Definition of the basic parser string type. */
#define MUP_STRING_TYPE std::string //std::wstring

#if !defined(_T)
#define _T(x) x//L##x
#endif // not defined _T
#else
#ifndef _T
#define _T(x) x
#endif

/** \brief Definition of the basic parser string type. */
#define MUP_STRING_TYPE std::string
#endif

#if defined(_DEBUG)
/** \brief Debug macro to force an abortion of the programm with a certain message.
*/
#define MUP_FAIL(MSG)     \
    {                 \
        bool MSG=false; \
        assert(MSG);    \
    }

/** \brief An assertion that does not kill the program.

    This macro is neutralised in UNICODE builds. It's
    too difficult to translate.
*/
#define MUP_ASSERT(COND)                         \
    if (!(COND))                             \
    {                                        \
        stringstream_type ss;                  \
        ss << _T("Assertion \"") _T(#COND) _T("\" failed: ") \
           << __FILE__ << _T(" line ")         \
           << __LINE__ << _T(".");             \
        throw ParserError( ss.str() );         \
    }
#else
#define MUP_FAIL(MSG)
#define MUP_ASSERT(COND)
#endif

class StringView;

namespace mu
{
    //------------------------------------------------------------------------------
    /** \brief Bytecode values.

        \attention The order of the operator entries must match the order in ParserBase::c_DefaultOprt!
    */
    enum ECmdCode
    {
        // The following are codes for built in binary operators
        // apart from built in operators the user has the opportunity to
        // add user defined operators.
        cmLE,                  ///< Operator item:  less or equal
        cmGE,                  ///< Operator item:  greater or equal
        cmNEQ,                 ///< Operator item:  not equal
        cmEQ,                  ///< Operator item:  equals
        cmLT,                  ///< Operator item:  less than
        cmGT,                  ///< Operator item:  greater than
        cmASSIGN,              ///< Operator item:  Assignment operator
        cmADDASGN,             ///< Operator item:  Addition-assignment operator
        cmSUBASGN,             ///< Operator item:  Subtraction-assignment operator
        cmMULASGN,             ///< Operator item:  Multiplication-assignment operator
        cmDIVASGN,             ///< Operator item:  Division-assignment operator
        cmPOWASGN,             ///< Operator item:  Power-assignment operator
        cmINCR,                ///< Operator item:  Increment operator
        cmDECR,                ///< Operator item:  Decrement operator
        cmADD,                 ///< Operator item:  add
        cmSUB,                 ///< Operator item:  subtract
        cmMUL,                 ///< Operator item:  multiply
        cmDIV,                 ///< Operator item:  division
        cmPOW,                 ///< Operator item:  y to the power of ...
        cmLAND,                ///< Operator item:  logical and
        cmLOR,                 ///< Operator item:  logical or
        cmBO,                  ///< Operator item:  opening bracket
        cmBC,                  ///< Operator item:  closing bracket
        cmVO,                  ///< Operator item:  opening vector brace
        cmVC,                  ///< Operator item:  closing vector brace
        cmIF,                  ///< For use in the ternary if-then-else operator
        cmELSE,                ///< For use in the ternary if-then-else operator
        cmENDIF,               ///< For use in the ternary if-then-else operator
        cmEXP2,                ///< The two-value expansion operator
        cmEXP3,                ///< The three-value expansion operator
        cmARG_SEP,             ///< function argument separator
        cmVAL,                 ///< value item

        // Variable and optimization block
        cmVAR,                 ///< variable item
        cmVARARRAY,
        cmVARPOW2,
        cmVARPOW3,
        cmVARPOW4,
        cmVARPOWN,
        cmVARMUL,
        cmREVVARMUL,
        cmVAR_END,             ///< Only for identifying the end of the variable block

        // operators and functions
        cmFUNC,                ///< Code for a generic function item
        cmMETHOD,              ///< Code for a generic method item
        cmSTRING,              ///< Code for a string token
        cmOPRT_BIN,            ///< user defined binary operator
        cmOPRT_POSTFIX,        ///< code for postfix operators
        cmOPRT_INFIX,          ///< code for infix operators
        cmVAL2STR,             ///< code for special var2str operator
        cmPATHPLACEHOLDER,     ///< code for path placeholder-operator
        cmEND,                 ///< end of formula
        cmUNKNOWN              ///< uninitialized item
    };

    //------------------------------------------------------------------------------
    /** \brief Types internally used by the parser.
    */
    enum ETypeCode
    {
        tpSTR    = 0,     ///< String type (Function arguments and constants only, no string variables)
        tpDBL    = 1,     ///< Floating point variables
        tpNOARGS = 2,     ///< Method with no args (no braces to be expected)
        tpVOID   = -1     ///< Undefined type.
    };

    //------------------------------------------------------------------------------
    enum EParserVersionInfo
    {
        pviBRIEF,
        pviFULL
    };

    //------------------------------------------------------------------------------
    /** \brief Parser operator precedence values. */
    enum EOprtAssociativity
    {
        oaLEFT  = 0,
        oaRIGHT = 1,
        oaNONE  = 2
    };

    //------------------------------------------------------------------------------
    /** \brief Parser operator precedence values. */
    enum EOprtPrecedence
    {
        // binary operators
        prLOR     = 1,
        prLAND    = 2,
        prLOGIC   = 3,  ///< logic operators
        prCMP     = 4,  ///< comparsion operators
        prADD_SUB = 5,  ///< addition
        prMUL_DIV = 6,  ///< multiplication/division
        prPOW     = 7,  ///< power operator priority (highest)

        // infix operators
        prINFIX   = 6, ///< Signs have a higher priority than ADD_SUB, but lower than power operator
        prPOSTFIX = 6  ///< Postfix operator priority (currently unused)
    };

    //------------------------------------------------------------------------------
    // basic types

    /** \brief The numeric datatype used by the parser.

      Normally this is a floating point type either single or double precision.
    */
    //typedef MUP_BASETYPE value_type;

    // Data container types

    class Variable;
    /** \brief Type used for storing variables. */
    typedef std::map<string_type, Variable*> varmap_type;

    /** \brief Type used for storing constants. */
    typedef std::map<string_type, Value> valmap_type;

    /** \brief Type for assigning a string name to an index in the internal string table. */
    typedef std::map<string_type, std::size_t> strmap_type;

    /** \brief Type used for storing an array of values. */
    typedef std::vector<Array> valbuf_type;

    // Parser callbacks

    /** \brief Callback type used for functions without arguments. */
    typedef Array (*generic_fun_type)();

    /** \brief Callback type used for functions without arguments. */
    typedef Array (*fun_type0)();

    /** \brief Callback type used for functions with a single arguments. */
    typedef Array (*fun_type1)(const Array&);

    /** \brief Callback type used for functions with two arguments. */
    typedef Array (*fun_type2)(const Array&, const Array&);

    /** \brief Callback type used for functions with three arguments. */
    typedef Array (*fun_type3)(const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with four arguments. */
    typedef Array (*fun_type4)(const Array&, const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with five arguments. */
    typedef Array (*fun_type5)(const Array&, const Array&, const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with five arguments. */
    typedef Array (*fun_type6)(const Array&, const Array&, const Array&, const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with five arguments. */
    typedef Array (*fun_type7)(const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with five arguments. */
    typedef Array (*fun_type8)(const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with five arguments. */
    typedef Array (*fun_type9)(const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with five arguments. */
    typedef Array (*fun_type10)(const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&, const Array&);

    /** \brief Callback type used for functions with a variable argument list. */
    typedef Array (*multfun_type)(const Array*, int);

    /** \brief Callback used for functions that identify values in a string. */
    typedef int (*identfun_type)(StringView sExpr, int* nPos, Value* fVal);

    // Forward declarations
    inline bool isinf(const std::complex<double>& v)
	{
	    return std::isinf(v.real()) || std::isinf(v.imag());
	}

	inline bool isnan(const Value& v)
	{
	    return (bool)(v != v) || v.isString();
	}

    std::vector<double> real(const std::vector<std::complex<double>>& vVec);

} // end of namespace

#endif



