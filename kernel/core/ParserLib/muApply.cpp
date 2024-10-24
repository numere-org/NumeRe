/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "muApply.hpp"
#include "muStructures.hpp"

namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Apply a function to an Array.
    ///
    /// \param func std::complex<double>(*func)(const std::complex<double>&)
    /// \param a const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(std::complex<double>(*func)(const std::complex<double>&), const Array& a)
    {
        Array ret;
        ret.reserve(a.size());

        for (const auto& val : a)
        {
            ret.push_back(Numerical(func(val.getNum().asCF64())));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Apply a function to an Array.
    ///
    /// \param func Value(*func)(const Value&)
    /// \param a const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(Value(*func)(const Value&), const Array& a)
    {
        Array ret;
        ret.reserve(a.size());

        for (const auto& val : a)
        {
            ret.push_back(func(val));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Apply a function to an Array
    ///
    /// \param func std::string(*func)(const std::string&)
    /// \param a const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(std::string(*func)(const std::string&), const Array& a)
    {
        Array ret;
        ret.reserve(a.size());

        for (const auto& val : a)
        {
            ret.push_back(func(val.getStr()));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to two Arrays.
    ///
    /// \param func Value(*func)(const Value&, const Value&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(Value(*func)(const Value&, const Value&), const Array& a1, const Array& a2)
    {
        Array ret;
        ret.reserve(std::max(a1.size(), a2.size()));

        for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i)));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Apply a function to two Arrays.
    ///
    /// \param func std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2)
    {
        Array ret;
        ret.reserve(std::max(a1.size(), a2.size()));

        for (size_t i = 0; i < std::max(a1.size(), a2.size()); i++)
        {
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64())));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to three Arrays.
    ///
    /// \param func Value(*func)(const Value&, const Value&, const Value&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(Value(*func)(const Value&, const Value&, const Value&), const Array& a1, const Array& a2, const Array& a3)
    {
        Array ret;
        ret.reserve(std::max({a1.size(), a2.size(), a3.size()}));

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size()}); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i), a3.get(i)));
        }

        return ret;
    }

    /////////////////////////////////////////////////
    /// \brief Apply a function to three Arrays.
    ///
    /// \param func std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3)
    {
        Array ret;
        ret.reserve(std::max({a1.size(), a2.size(), a3.size()}));

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size()}); i++)
        {
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64(),
                                         a3.get(i).getNum().asCF64())));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to four Arrays.
    ///
    /// \param func std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \param a4 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3, const Array& a4)
    {
        Array ret;
        ret.reserve(std::max({a1.size(), a2.size(), a3.size(), a4.size()}));

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size()}); i++)
        {
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64(),
                                         a3.get(i).getNum().asCF64(),
                                         a4.get(i).getNum().asCF64())));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Apply a function to four Arrays.
    ///
    /// \param func Value(*func)(const Value&, const Value&, const Value&, const Value&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \param a4 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(Value(*func)(const Value&, const Value&, const Value&, const Value&), const Array& a1, const Array& a2, const Array& a3, const Array& a4)
    {
        Array ret;
        ret.reserve(std::max({a1.size(), a2.size(), a3.size(), a4.size()}));

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size()}); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i), a3.get(i), a4.get(i)));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to five Arrays.
    ///
    /// \param func std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \param a4 const Array&
    /// \param a5 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(std::complex<double>(*func)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&), const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5)
    {
        Array ret;
        ret.reserve(std::max({a1.size(), a2.size(), a3.size(), a4.size(), a5.size()}));

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size(), a5.size()}); i++)
        {
            ret.push_back(Numerical(func(a1.get(i).getNum().asCF64(),
                                         a2.get(i).getNum().asCF64(),
                                         a3.get(i).getNum().asCF64(),
                                         a4.get(i).getNum().asCF64(),
                                         a5.get(i).getNum().asCF64())));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Apply a function to five Arrays.
    ///
    /// \param func Value(*func)(const Value&, const Value&, const Value&, const Value&, const Value&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \param a4 const Array&
    /// \param a5 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(Value(*func)(const Value&, const Value&, const Value&, const Value&, const Value&), const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5)
    {
        Array ret;
        ret.reserve(std::max({a1.size(), a2.size(), a3.size(), a4.size(), a5.size()}));

        for (size_t i = 0; i < std::max({a1.size(), a2.size(), a3.size(), a4.size(), a5.size()}); i++)
        {
            ret.push_back(func(a1.get(i), a2.get(i), a3.get(i), a4.get(i), a5.get(i)));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to an array of Arrays.
    ///
    /// \param std::complex<double>(*func)(const std::complex<double>*, int)
    /// \param arr const Array*
    /// \param elems int
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(std::complex<double>(*func)(const std::complex<double>*, int), const Array* arrs, int elems)
    {
        size_t nCount = 0;

        // Find the maximum along all arrays
        for (int i = 0; i < elems; i++)
        {
            nCount = std::max(nCount, arrs[i].size());
        }

        mu::Array res;
        res.reserve(nCount);

        for (size_t i = 0; i < nCount; i++)
        {
            std::vector<std::complex<double>> vVals;

            for (int e = 0; e < elems; e++)
            {
                vVals.push_back(arrs[e].get(i).getNum().asCF64());
            }

            res.push_back(Numerical(func(&vVals[0], elems)));
        }

        return res;
    }

    /////////////////////////////////////////////////
    /// \brief Apply a function to an array of Arrays.
    ///
    /// \param func Value(*func)(const Value*, int)
    /// \param arr const Array*
    /// \param elems int
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(Value(*func)(const Value*, int), const Array* arrs, int elems)
    {
        size_t nCount = 0;

        // Find the maximum along all arrays
        for (int i = 0; i < elems; i++)
        {
            nCount = std::max(nCount, arrs[i].size());
        }

        mu::Array res;
        res.reserve(nCount);

        for (size_t i = 0; i < nCount; i++)
        {
            std::vector<Value> vVals;

            for (int e = 0; e < elems; e++)
            {
                vVals.push_back(arrs[e].get(i));
            }

            res.push_back(func(&vVals[0], elems));
        }

        return res;
    }
}

