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
    /// \param func cmplx64(*func)(const cmplx64&)
    /// \param a const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(cmplx64(*func)(const cmplx64&),
                const Array& a)
    {
        Array ret;
        size_t elements = a.size();
        ret.copyDims(a);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a.get(i).getNum().asCF64()));
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
    Array apply(Value(*func)(const Value&),
                const Array& a)
    {
        Array ret;
        size_t elements = a.size();
        ret.copyDims(a);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a.get(i)));
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
    Array apply(std::string(*func)(const std::string&),
                const Array& a)
    {
        Array ret;
        size_t elements = a.size();
        ret.copyDims(a);

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a.get(i).getStr()));
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
    Array apply(Value(*func)(const Value&, const Value&),
                const Array& a1, const Array& a2)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        Array ret = a1View.prepare(a2View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i), a2View.get(i)));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Apply a function to two Arrays.
    ///
    /// \param func cmplx64(*func)(const cmplx64&, const cmplx64&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(cmplx64(*func)(const cmplx64&, const cmplx64&),
                const Array& a1, const Array& a2)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        Array ret = a1View.prepare(a2View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i).getNum().asCF64(),
                                  a2View.get(i).getNum().asCF64()));
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
    Array apply(Value(*func)(const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        MatrixView a3View(a3);
        Array ret = a1View.prepare(a2View, a3View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i), a2View.get(i), a3View.get(i)));
        }

        return ret;
    }


    /////////////////////////////////////////////////
    /// \brief Apply a function to three Arrays.
    ///
    /// \param func cmplx64(*func)(const cmplx64&, const cmplx64&, const cmplx64&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(cmplx64(*func)(const cmplx64&, const cmplx64&, const cmplx64&),
                const Array& a1, const Array& a2, const Array& a3)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        MatrixView a3View(a3);
        Array ret = a1View.prepare(a2View, a3View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i).getNum().asCF64(),
                                  a2View.get(i).getNum().asCF64(),
                                  a3View.get(i).getNum().asCF64()));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to four Arrays.
    ///
    /// \param func cmplx64(*func)(const cmplx64&, const cmplx64&, const cmplx64&, const cmplx64&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \param a4 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(cmplx64(*func)(const cmplx64&, const cmplx64&, const cmplx64&, const cmplx64&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        MatrixView a3View(a3);
        MatrixView a4View(a4);
        Array ret = a1View.prepare(a2View, a3View, a4View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i).getNum().asCF64(),
                                  a2View.get(i).getNum().asCF64(),
                                  a3View.get(i).getNum().asCF64(),
                                  a4View.get(i).getNum().asCF64()));
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
    Array apply(Value(*func)(const Value&, const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        MatrixView a3View(a3);
        MatrixView a4View(a4);
        Array ret = a1View.prepare(a2View, a3View, a4View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i), a2View.get(i), a3View.get(i), a4View.get(i)));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to five Arrays.
    ///
    /// \param func cmplx64(*func)(const cmplx64&, const cmplx64&, const cmplx64&, const cmplx64&, const cmplx64&)
    /// \param a1 const Array&
    /// \param a2 const Array&
    /// \param a3 const Array&
    /// \param a4 const Array&
    /// \param a5 const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(cmplx64(*func)(const cmplx64&, const cmplx64&, const cmplx64&, const cmplx64&, const cmplx64&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        MatrixView a3View(a3);
        MatrixView a4View(a4);
        MatrixView a5View(a5);
        Array ret = a1View.prepare(a2View, a3View, a4View, a5View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i).getNum().asCF64(),
                                  a2View.get(i).getNum().asCF64(),
                                  a3View.get(i).getNum().asCF64(),
                                  a4View.get(i).getNum().asCF64(),
                                  a5View.get(i).getNum().asCF64()));
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
    Array apply(Value(*func)(const Value&, const Value&, const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5)
    {
        MatrixView a1View(a1);
        MatrixView a2View(a2);
        MatrixView a3View(a3);
        MatrixView a4View(a4);
        MatrixView a5View(a5);
        Array ret = a1View.prepare(a2View, a3View, a4View, a5View);

        size_t elements = a1View.size();

        for (size_t i = 0; i < elements; i++)
        {
            ret.emplace_back(func(a1View.get(i), a2View.get(i), a3View.get(i), a4View.get(i), a5View.get(i)));
        }

        return ret;
    }



    /////////////////////////////////////////////////
    /// \brief Apply a function to an array of Arrays.
    ///
    /// \param cmplx64(*func)(const cmplx64*, int)
    /// \param arr const Array*
    /// \param elems int
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array apply(cmplx64(*func)(const cmplx64*, int),
                const Array* arrs, int elems)
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
            std::vector<cmplx64> vVals;

            for (int e = 0; e < elems; e++)
            {
                vVals.push_back(arrs[e].get(i).getNum().asCF64());
            }

            res.emplace_back(func(&vVals[0], elems));
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
    Array apply(Value(*func)(const Value*, int),
                const Array* arrs, int elems)
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

            res.emplace_back(func(&vVals[0], elems));
        }

        return res;
    }
}

