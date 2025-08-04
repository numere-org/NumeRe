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


namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Print a string on the terminal.
    ///
    /// \param msg const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void print(const std::string& msg)
    {
#ifdef PARSERSTANDALONE
        std::cout << "|-> " << msg << std::endl;
#else
        NumeReKernel::print(msg);
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Print a formatted string on the
    /// terminal.
    ///
    /// \param msg const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void printFormatted(const std::string& msg)
    {
#ifdef PARSERSTANDALONE
        std::cout << msg;
#else
        NumeReKernel::printPreFmt(msg);
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Toggle the table printing mode (i.e.
    /// delay/refresh after) to avoid flickering.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void toggleTableMode()
    {
#ifdef PARSERSTANDALONE
        //
#else
        NumeReKernel::toggleTableStatus();
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Implements the val2str operator.
    ///
    /// \param arr const Array&
    /// \param nLen size_t
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array val2Str(const Array& arr, size_t nLen)
    {
        Array res;
        size_t elems = arr.size();
        res.reserve(elems);

        for (size_t i = 0; i < elems; i++)
        {
#ifndef PARSERSTANDALONE
            std::string sStr = arr.get(i).printVal(NumeReKernel::getInstance()->getSettings().getPrecision());
#else
            std::string sStr = arr.get(i).printVal();
#endif // PARSERSTANDALONE

            if (sStr.length() < nLen && sStr.length() > 0)
                sStr.insert(0, std::string(nLen-sStr.length(), '0'));

            res.push_back(sStr);
        }

        return res;
    }


    /////////////////////////////////////////////////
    /// \brief Implements the path tokens within the
    /// parser.
    ///
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array getPathToken(const Array& arr)
    {
#ifndef PARSERSTANDALONE
        return mu::Value(NumeReKernel::getInstance()->getFileSystem().getTokenValue(arr.front().getStr()));
#else
        return arr;
#endif // PARSERSTANDALONE
    }
}

