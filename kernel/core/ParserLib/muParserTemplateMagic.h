#ifndef MU_PARSER_TEMPLATE_MAGIC_H
#define MU_PARSER_TEMPLATE_MAGIC_H

#include <cmath>
#include "muParserError.h"

std::complex<double> intPower(const std::complex<double>&, int);

namespace mu
{
  //-----------------------------------------------------------------------------------------------
  //
  // Compile time type detection
  //
  //-----------------------------------------------------------------------------------------------

  /** \brief A class singling out integer types at compile time using
             template meta programming.
  */
  template<typename T>
  struct TypeInfo
  {
    static bool IsInteger() { return false; }
  };

  template<>
  struct TypeInfo<char>
  {
    static bool IsInteger() { return true;  }
  };

  template<>
  struct TypeInfo<short>
  {
    static bool IsInteger() { return true;  }
  };

  template<>
  struct TypeInfo<int>
  {
    static bool IsInteger() { return true;  }
  };

  template<>
  struct TypeInfo<long>
  {
    static bool IsInteger() { return true;  }
  };

  template<>
  struct TypeInfo<unsigned char>
  {
    static bool IsInteger() { return true;  }
  };

  template<>
  struct TypeInfo<unsigned short>
  {
    static bool IsInteger() { return true;  }
  };

  template<>
  struct TypeInfo<unsigned int>
  {
    static bool IsInteger() { return true;  }
  };

  template<>
  struct TypeInfo<unsigned long>
  {
    static bool IsInteger() { return true;  }
  };


  //-----------------------------------------------------------------------------------------------
  //
  // Standard math functions with dummy overload for integer types
  //
  //-----------------------------------------------------------------------------------------------

  /** \brief A template class for providing wrappers for essential math functions.

    This template is spezialized for several types in order to provide a unified interface
    for parser internal math function calls regardless of the data type.
  */
  template<typename T>
  struct MathImpl
  {
    static T Sin(const T& v)   { return sin(v);  }
    static T Cos(const T& v)   { return cos(v);  }
    static T Tan(const T& v)   { return tan(v);  }
    static T ASin(const T& v)  { return asin(v); }
    static T ACos(const T& v)  { return acos(v); }
    static T ATan(const T& v)  { return isnan(v.real()) || isnan(v.imag()) ? NAN : atan(v); }
    static T ATan2(const T& v1, const T& v2) { return atan2(v1, v2); }
    static T Sinh(const T& v)  { return sinh(v); }
    static T Cosh(const T& v)  { return cosh(v); }
    static T Tanh(const T& v)  { return tanh(v); }
    static T Sqrt(const T& v)  { return v.imag() == 0.0 ? std::sqrt(v.real()) : std::sqrt(v);  }
    static T ASinh(const T& v) { return log(v + Sqrt(v * v + 1.0)); }
    static T ACosh(const T& v) { return log(v + Sqrt(v * v - 1.0)); }
    static T ATanh(const T& v) { return ((T)0.5 * log((1.0 + v) / (1.0 - v))); }
    static T Log(const T& v)   { return log(v); }
    static T Log2(const T& v)  { return log(v)/log((T)2.0); } // Logarithm base 2
    static T Log10(const T& v) { return log10(v); }         // Logarithm base 10
    static T Exp(const T& v)   { return exp(v);}
    static T Abs(const T& v)   { return (v>=0.0) ? v : -v; }
    static T Rint(const T& v)  { return floor(v + (T)0.5); }
    static T Sign(const T& v)  { return (T)((v<0.0) ? -1.0 : (v>0.0) ? 1.0 : 0.0); }
    static T Pow(const T& v1, const T& v2) { return v2.imag() == 0.0 && v2.real() == (int)v2.real() ? intPower(v1, v2.real()) : std::pow(v1, v2); }
  };

  // Create (mostly) dummy math function definitions for integer types. They are mostly
  // empty since they are not applicable for integer values.
#define MAKE_MATH_DUMMY(TYPE)                    \
  template<>                                     \
  struct MathImpl<TYPE>                          \
  {                                              \
    static TYPE Sin(TYPE)          { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Cos(TYPE)          { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Tan(TYPE)          { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE ASin(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE ACos(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE ATan(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE ATan2(TYPE, TYPE)  { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Sinh(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Cosh(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Tanh(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE ASinh(TYPE)        { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE ACosh(TYPE)        { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE ATanh(TYPE)        { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Log(TYPE)          { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Log2(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Log10(TYPE)        { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Exp(TYPE)          { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Abs(TYPE)          { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Sqrt(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Rint(TYPE)         { throw ParserError(_nrT("unimplemented function.")); } \
    static TYPE Sign(TYPE v)          { return (TYPE)((v<0) ? -1 : (v>0) ? 1 : 0);     } \
    static TYPE Pow(TYPE v1, TYPE v2) { return (TYPE)std::pow((double)v1, (double)v2); } \
  };

  MAKE_MATH_DUMMY(char)
  MAKE_MATH_DUMMY(short)
  MAKE_MATH_DUMMY(int)
  MAKE_MATH_DUMMY(long)

#undef MAKE_MATH_DUMMY
}

#endif
