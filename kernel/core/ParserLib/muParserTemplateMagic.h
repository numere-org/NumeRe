#ifndef MU_PARSER_TEMPLATE_MAGIC_H
#define MU_PARSER_TEMPLATE_MAGIC_H

#include <cmath>
#include "muParserError.h"

std::complex<double> intPower(const std::complex<double>&, int64_t);

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
    inline static T Sin(const T& v)
    {
        return v.imag() == 0.0 ? std::sin(v.real()) : std::sin(v);
    }


    inline static T Cos(const T& v)
    {
        return v.imag() == 0.0 ? std::cos(v.real()) : std::cos(v);
    }


    inline static T Tan(const T& v)
    {
        return v.imag() == 0.0 ? std::tan(v.real()) : std::tan(v);
    }

    inline static T ASin(const T& v)
    {
        return v.imag() == 0.0 ? std::asin(v.real()) : std::asin(v);
    }


    inline static T ACos(const T& v)
    {
        return v.imag() == 0.0 ? std::acos(v.real()) : std::acos(v);
    }


    inline static T ATan(const T& v)
    {
        return std::isnan(v.real()) || std::isnan(v.imag())
            ? NAN
            : (v.imag() == 0.0 ? std::atan(v.real()) : std::atan(v));
    }


    inline static T ATan2(const T& v1, const T& v2)
    {
        return std::atan2(v1, v2); // only available for doubles
    }


    inline static T Sinh(const T& v)
    {
        return v.imag() == 0.0 ? std::sinh(v.real()) : std::sinh(v);
    }


    inline static T Cosh(const T& v)
    {
        return v.imag() == 0.0 ? std::cosh(v.real()) : std::cosh(v);
    }


    inline static T Tanh(const T& v)
    {
        return v.imag() == 0.0 ? std::tanh(v.real()) : std::tanh(v);
    }


    inline static T Sqrt(const T& v)
    {
        return v.imag() == 0.0 ? std::sqrt(v.real()) : std::sqrt(v);
    }


    inline static T Log(const T& v)
    {
        return v.imag() == 0.0 ? std::log(v.real()) : std::log(v);
    }


    inline static T Log2(const T& v)
    {
        return (v.imag() == 0.0 ? std::log(v.real()) : std::log(v))/std::log(2.0); // Logarithm base 2
    }


    inline static T Log10(const T& v)
    {
        return v.imag() == 0.0 ? std::log10(v.real()) : std::log10(v); // Logarithm base 10
    }


    inline static T ASinh(const T& v)
    {
        return Log(v + Sqrt(v * v + 1.0));
    }


    inline static T ACosh(const T& v)
    {
        return Log(v + Sqrt(v * v - 1.0));
    }


    inline static T ATanh(const T& v)
    {
        return (0.5 * Log((1.0 + v) / (1.0 - v)));
    }


    inline static T Exp(const T& v)
    {
        return v.imag() == 0 ? std::exp(v.real()) : std::exp(v);
    }


    inline static T Rint(const T& v)
    {
        return std::floor(v + (T)0.5);
    }


    inline static T Sign(const T& v)
    {
        return (T)((v<0.0) ? -1.0 : (v>0.0) ? 1.0 : 0.0);
    }


    inline static T Pow(const T& v1, const T& v2)
    {
        return v2.imag() == 0.0 && v2.real() == (int)v2.real()
            ? intPower(v1, v2.real())
            : (v1.imag() == 0.0 && v2.imag() == 0.0 ? std::pow(v1.real(), v2.real()) : std::pow(v1, v2));
    }
  };
}

#endif
