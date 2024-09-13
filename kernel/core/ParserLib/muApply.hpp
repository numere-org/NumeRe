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

#ifndef MUAPPLY_HPP
#define MUAPPLY_HPP

#include <complex>
#include <string>

namespace mu
{
    class Array;
    class Value;

    Array apply(std::complex<double>(*)(const std::complex<double>&),
                const Array& a);
    Array apply(Value(*)(const Value&),
                const Array& a);
    Array apply(std::string(*)(const std::string&),
                const Array& a);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2);
    Array apply(Value(*)(const Value&, const Value&),
                const Array& a1, const Array& a2);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2, const Array& a3);
    Array apply(Value(*)(const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4);
    Array apply(Value(*)(const Value&, const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4);

    Array apply(std::complex<double>(*)(const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&, const std::complex<double>&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5);
    Array apply(Value(*)(const Value&, const Value&, const Value&, const Value&, const Value&),
                const Array& a1, const Array& a2, const Array& a3, const Array& a4, const Array& a5);

    Array apply(std::complex<double>(*)(const std::complex<double>*, int),
                const Array* arrs, int elems);
    Array apply(Value(*)(const Value*, int),
                const Array* arrs, int elems);
}

#endif // MUAPPLY_HPP

