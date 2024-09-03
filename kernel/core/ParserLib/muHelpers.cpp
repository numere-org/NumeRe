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

#include "muHelpers.hpp"
#ifdef PARSERSTANDALONE
#include <iostream>
#else
#include "../../kernel.hpp"
#endif

#warning TODO (numere#2#08/14/24): Handle new method types within autocompletion -> NEW ISSUE
#warning TODO (numere#1#08/13/24): Update function language strings

namespace mu
{
    void print(const std::string& msg)
    {
#ifdef PARSERSTANDALONE
        std::cout << "|-> " << msg << std::endl;
#else
        NumeReKernel::print(msg);
#endif
    }

    void printFormatted(const std::string& msg)
    {
#ifdef PARSERSTANDALONE
        std::cout << msg;
#else
        NumeReKernel::printPreFmt(msg);
#endif
    }

    void toggleTableMode()
    {
#ifdef PARSERSTANDALONE
        //
#else
        NumeReKernel::toggleTableStatus();
#endif
    }

    Array val2Str(const Array& arr, size_t nLen)
    {
        Array res;

        for (const auto& val : arr)
        {
#ifndef PARSERSTANDALONE
            std::string sStr = val.printVal(NumeReKernel::getInstance()->getSettings().getPrecision());
#else
            std::string sStr = val.printVal();
#endif // PARSERSTANDALONE

            if (sStr.length() < nLen && sStr.length() > 0)
                sStr.insert(0, std::string(nLen-sStr.length(), '0'));

            res.push_back(sStr);
        }

        return res;
    }

    Array getPathToken(const Array& arr)
    {
        #ifndef PARSERSTANDALONE
        return mu::Value(NumeReKernel::getInstance()->getFileSystem().getTokenValue(arr.front().getStr()));
        #else
        return arr;
        #endif // PARSERSTANDALONE
    }
}

